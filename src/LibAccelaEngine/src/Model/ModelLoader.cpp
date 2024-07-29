/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ModelLoader.h"
#include "AssimpUtil.h"

#include <Accela/Render/Mesh/MeshVertex.h>
#include <Accela/Render/Mesh/BoneMeshVertex.h>

#include <Accela/Common/Timer.h>

#include <assimp/postprocess.h>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/Importer.hpp>
#include <assimp/MemoryIOWrapper.h>

#include <queue>
#include <filesystem>

namespace Accela::Engine
{

static const unsigned int MAX_BONES_PER_VERTEX = 4;

#define AI_MATKEY_GLTF_ALPHAMODE "$mat.gltf.alphaMode", 0, 0
#define AI_MATKEY_GLTF_ALPHACUTOFF "$mat.gltf.alphaCutoff", 0, 0

/**
 * Assimp IOSystem which calls into a PackageSource to fetch model files. Needed
 * because there could be supplementary files associated with a model (like a .mtl
 * file) and we need to provide the ability to load them from a package on demand;
 * we can't just only give Assimp the model data from the main model file.
 */
class PackageIOSystem : public Assimp::IOSystem
{
    public:

        explicit PackageIOSystem(Platform::PackageSource::Ptr source)
            : m_source(std::move(source))
        {

        }

        bool Exists(const char* pFile) const override
        {
            return std::ranges::contains(m_source->GetModelResourceNames(), std::string{pFile});
        }

        Assimp::IOStream* Open(const char* pFile, const char*) override
        {
            const auto fileStr = std::string(pFile);

            if (!m_fileContents.contains(fileStr))
            {
                std::expected<std::vector<std::byte>, unsigned int> modelData = m_source->GetModelData(pFile);
                if (!modelData)
                {
                    return nullptr;
                }

                m_fileContents.insert({fileStr, *modelData});
            }

            const auto& fileBytes = m_fileContents.at(fileStr);

            return new Assimp::MemoryIOStream((const uint8_t*)fileBytes.data(), fileBytes.size(), false);
        }

        [[nodiscard]] char getOsSeparator() const override
        {
            return std::filesystem::path::preferred_separator;
        }

        void Close(Assimp::IOStream* pFile) override
        {
            delete pFile;
        }

    private:

        Platform::PackageSource::Ptr m_source;

        // Cache of file contents as assimp often calls Open/Close flows a lot of times for the same file
        std::unordered_map<std::string, std::vector<std::byte>> m_fileContents;
};

ModelLoader::ModelLoader(Common::ILogger::Ptr logger)
    : m_logger(std::move(logger))
{

}

Model::Ptr ModelLoader::LoadModel(const ResourceIdentifier& resource,
                                  const Platform::PackageSource::Ptr& source,
                                  const std::string& fileHint,
                                  const std::string& tag) const
{
    m_logger->Log(Common::LogLevel::Info, "--[Disk Model Load] {}, {} --", tag, fileHint);

    Common::Timer loadTimer("LoadModelTime");

    Assimp::Importer importer;
    importer.SetIOHandler(new PackageIOSystem(source));

    const aiScene *pScene = importer.ReadFile(
        resource.GetResourceName(),
        aiProcess_Triangulate |           // Always output triangles instead of arbitrarily sized faces
        //aiProcess_FlipWindingOrder |
        //aiProcess_MakeLeftHanded |
        aiProcess_JoinIdenticalVertices |       // Combine identical vertices to save memory
        aiProcess_GenUVCoords |                 // Convert non-UV texture mappings to UV mappings
        aiProcess_FlipUVs |                     // Vulkan uses a flipped UV coordinate system
        aiProcess_GenSmoothNormals |            // Generate normals for models that don't have them
        aiProcess_ValidateDataStructure |       // Validate the model
        aiProcess_CalcTangentSpace              // Calculate tangent and bitangent vectors for models with normal maps
    );

    if (pScene == nullptr || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || pScene->mRootNode == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "Failed to load model from disk: {}", tag);
        return nullptr;
    }

    auto model = std::make_shared<Model>();

    ProcessMaterials(model, pScene);
    ProcessEmbeddedTextures(model, pScene);
    ProcessMeshes(model, pScene);
    ProcessNodes(model, pScene);
    ProcessSkeletons(model);
    ProcessAnimations(model, pScene);

    const auto loadTime = loadTimer.StopTimer();

    m_logger->Log(Common::LogLevel::Debug, "{}: Num Meshes: {}", tag, model->meshes.size());
    m_logger->Log(Common::LogLevel::Debug, "{}: Num Materials: {}", tag, model->materials.size());
    m_logger->Log(Common::LogLevel::Debug, "{}: Num Nodes: {}", tag, model->nodeMap.size());
    m_logger->Log(Common::LogLevel::Debug, "{}: Num Nodes With Meshes: {}", tag, model->nodesWithMeshes.size());
    m_logger->Log(Common::LogLevel::Debug, "{}: Num Animations: {}", tag, model->animations.size());
    m_logger->Log(Common::LogLevel::Debug, "{}: loaded in {}ms", tag, loadTime.count());

    return model;
}

void ModelLoader::ProcessMaterials(const Model::Ptr& model, const aiScene* pScene) const
{
    for (unsigned int materialIndex = 0; materialIndex < pScene->mNumMaterials; ++materialIndex)
    {
        const aiMaterial* pMaterial = pScene->mMaterials[materialIndex];
        const ModelMaterial material = ProcessMaterial(pMaterial);
        model->materials[materialIndex] = material;
    }
}

std::optional<Render::AlphaMode> ToAlphaMode(const aiString& value)
{
    if (value == aiString("OPAQUE")) { return Render::AlphaMode::Opaque; }
    if (value == aiString("MASK")) { return Render::AlphaMode::Mask; }
    if (value == aiString("BLEND")) { return Render::AlphaMode::Blend; }
    return std::nullopt;
}

ModelMaterial ModelLoader::ProcessMaterial(const aiMaterial* pMaterial) const
{
    ModelMaterial rawMaterial{};

    aiString materialName;
    aiGetMaterialString(pMaterial, AI_MATKEY_NAME, &materialName);
    rawMaterial.name = std::string(materialName.C_Str());

    //
    // Texture material properties
    //
    rawMaterial.ambientTextures = GetMaterialTextures(pMaterial, aiTextureType_AMBIENT);
    rawMaterial.diffuseTextures = GetMaterialTextures(pMaterial, aiTextureType_DIFFUSE);
    rawMaterial.specularTextures = GetMaterialTextures(pMaterial, aiTextureType_SPECULAR);

    // Note: Assimp loads normal maps as aiTextureType_HEIGHT for some reason; verify this happens for all models. (Only for OBJs?)
    rawMaterial.normalTextures = GetMaterialTextures(pMaterial, aiTextureType_HEIGHT);

    //
    // Color material properties
    //
    ai_real opacity{1.0f};
    pMaterial->Get(AI_MATKEY_OPACITY, opacity);
    rawMaterial.opacity = opacity;

    ai_int blendFunc{0};
    pMaterial->Get(AI_MATKEY_BLEND_FUNC, blendFunc);

    ai_int twoSided{0};
    pMaterial->Get(AI_MATKEY_TWOSIDED, twoSided);
    rawMaterial.twoSided = twoSided == 1;

    ai_int shadingModel{1};
    pMaterial->Get(AI_MATKEY_SHADING_MODEL, shadingModel);

    ai_real transparencyFactor{1.0f};
    pMaterial->Get(AI_MATKEY_TRANSPARENCYFACTOR, transparencyFactor);

    aiColor4D ambientColor(0.0f, 0.f, 0.f, 0.0f);
    pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, ambientColor);
    rawMaterial.ambientColor = glm::vec4(ambientColor.r, ambientColor.g, ambientColor.b, ambientColor.a);

    aiColor4D diffuseColor(0.f, 0.f, 0.f, 0.0f);
    pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
    rawMaterial.diffuseColor = glm::vec4(diffuseColor.r, diffuseColor.g, diffuseColor.b, diffuseColor.a);

    aiColor4D specularColor(0.f, 0.f, 0.f, 0.0f);
    pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
    rawMaterial.specularColor = glm::vec4(specularColor.r, specularColor.g, specularColor.b, specularColor.a);

    ai_real shininess{0.0f};
    pMaterial->Get(AI_MATKEY_SHININESS, shininess);
    rawMaterial.shininess = shininess;

    //
    // GLTF specific
    //
    aiString gltfAlphaModeStr;
    pMaterial->Get(AI_MATKEY_GLTF_ALPHAMODE, gltfAlphaModeStr);
    const auto gltfAlphaMode = ToAlphaMode(gltfAlphaModeStr);

    ai_real gltfAlphaCutoff{1.0f};
    pMaterial->Get(AI_MATKEY_GLTF_ALPHACUTOFF, gltfAlphaCutoff);

    if (gltfAlphaMode)
    {
        rawMaterial.alphaMode = *gltfAlphaMode;
        rawMaterial.alphaCutoff = gltfAlphaCutoff;
    }

    //
    // Material Fixes/Adjustments
    //

    // For some reason it's common for people to define materials to have no ambient texture and a black
    // ambient color, which means the material will never be affected by ambient lighting, which doesn't
    // make any sense. If we see a material in this situation, we force its ambient color/texture to match
    // its diffuse.
    if (rawMaterial.ambientTextures.empty() &&
        glm::all(glm::epsilonEqual(rawMaterial.ambientColor, glm::vec4{0,0,0,0}, std::numeric_limits<float>::epsilon())))
    {
        rawMaterial.ambientColor = rawMaterial.diffuseColor;
        rawMaterial.ambientTextures = rawMaterial.diffuseTextures;

        m_logger->Log(Common::LogLevel::Warning,
          "Fixed material which had no ambient color or texture defined: {}", rawMaterial.name);
    }

    /*
    // Debug logging for material properties
    m_logger->Log(Common::LogLevel::Error, "------ MATERIAL: {} -------", rawMaterial.name);

    for (unsigned int x = 0; x < pMaterial->mNumProperties; ++x)
    {
        auto* pProperty = pMaterial->mProperties[x];
        const std::string key = std::string(pProperty->mKey.C_Str());

        m_logger->Log(Common::LogLevel::Error, "[Key: {} ]", key);

        switch (pProperty->mType)
        {
            case aiPTI_Float:
            case aiPTI_Double:
            {
                double val{0.0};
                pMaterial->Get(key.c_str(), 0, 0, val);
                m_logger->Log(Common::LogLevel::Error, "{}", val);
            }
            break;
            case aiPTI_String:
            {
                aiString val{};
                pMaterial->Get(key.c_str(), 0, 0, val);
                m_logger->Log(Common::LogLevel::Error, "{}", val.C_Str());
            }
            break;
            case aiPTI_Integer:
            {
                unsigned int val{0};
                pMaterial->Get(key.c_str(), 0, 0, val);
                m_logger->Log(Common::LogLevel::Error, "{}", val);
            }
            break;
            case aiPTI_Buffer:
            case _aiPTI_Force32Bit:
                m_logger->Log(Common::LogLevel::Error, "Unhandled");
            break;
        }
    }*/

    return rawMaterial;
}

std::vector<ModelTexture> ModelLoader::GetMaterialTextures(const aiMaterial* pMaterial, aiTextureType type) const
{
    std::vector<ModelTexture> results;

    for (unsigned int textureIndex = 0; textureIndex < pMaterial->GetTextureCount(type); ++textureIndex)
    {
        aiString fileName;
        pMaterial->GetTexture(type, textureIndex, &fileName);
        const std::string textureFileName(fileName.C_Str());

        //
        // Sampler U Address Mode
        //
        auto uAddressMode = Render::SamplerAddressMode::Wrap;

        int aiUMapMode{aiTextureMapMode_Wrap};
        pMaterial->Get(AI_MATKEY_MAPPINGMODE_U(type, textureIndex), aiUMapMode);
        if (aiUMapMode == aiTextureMapMode_Wrap) { uAddressMode = Render::SamplerAddressMode::Wrap; }
        else if (aiUMapMode == aiTextureMapMode_Clamp) { uAddressMode = Render::SamplerAddressMode::Clamp; }
        else if (aiUMapMode == aiTextureMapMode_Mirror) { uAddressMode = Render::SamplerAddressMode::Mirror; }

        //
        // Sampler V Address Mode
        //
        auto vAddressMode = Render::SamplerAddressMode::Wrap;

        int aiVMapMode{aiTextureMapMode_Wrap};
        pMaterial->Get(AI_MATKEY_MAPPINGMODE_V(type, textureIndex), aiVMapMode);
        if (aiVMapMode == aiTextureMapMode_Wrap) { vAddressMode = Render::SamplerAddressMode::Wrap; }
        else if (aiVMapMode == aiTextureMapMode_Clamp) { vAddressMode = Render::SamplerAddressMode::Clamp; }
        else if (aiVMapMode == aiTextureMapMode_Mirror) { vAddressMode = Render::SamplerAddressMode::Mirror; }

        //
        // Texture blend factor
        //
        ai_real aiTextureBlend{1.0f};
        pMaterial->Get(AI_MATKEY_TEXBLEND(type, textureIndex), aiTextureBlend);

        //
        // Texture combine operation
        //
        ai_int aiTextureOp{0}; // Note that we're defaulting to multiply if none is specified
        pMaterial->Get(AI_MATKEY_TEXOP(type, textureIndex), aiTextureOp);

        Render::TextureOp textureOp{Render::TextureOp::Multiply};

        switch (aiTextureOp)
        {
            case 0: { textureOp = Render::TextureOp::Multiply; } break;
            case 1: { textureOp = Render::TextureOp::Add; } break;
            case 2: { textureOp = Render::TextureOp::Subtract; } break;
            case 3: { textureOp = Render::TextureOp::Divide; } break;
            case 4: { textureOp = Render::TextureOp::SmoothAdd; } break;
            case 5: { textureOp = Render::TextureOp::SignedAdd; } break;
            default:
            {
                m_logger->Log(Common::LogLevel::Error,
                  "GetMaterialTextures: Unsupported texture op: {} for texture: {}", aiTextureOp, textureFileName);
                continue;
            }
        }

        results.emplace_back(
            textureFileName,
            std::make_pair(uAddressMode, vAddressMode),
            aiTextureBlend,
            textureOp
        );
    }

    return results;
}

void ModelLoader::ProcessMeshes(const Model::Ptr& model, const aiScene* pScene) const
{
    for (unsigned int meshIndex = 0; meshIndex < pScene->mNumMeshes; ++meshIndex)
    {
        const aiMesh* pMesh = pScene->mMeshes[meshIndex];
        const ModelMesh mesh = ProcessMesh(pMesh, meshIndex);
        model->meshes[meshIndex] = mesh;
    }
}

ModelMesh ModelLoader::ProcessMesh(const aiMesh *pMesh, const unsigned int& meshIndex) const
{
    if (pMesh->HasBones())
    {
        return ProcessBoneMesh(pMesh, meshIndex);
    }
    else
    {
        return ProcessStaticMesh(pMesh, meshIndex);
    }
}

ModelMesh ModelLoader::ProcessStaticMesh(const aiMesh *pMesh, const unsigned int& meshIndex)
{
    std::vector<Render::MeshVertex> vertices;
    std::vector<uint32_t> indices;

    //
    // Record mesh vertex data
    //
    for (unsigned int x = 0; x < pMesh->mNumVertices; ++x)
    {
        const glm::vec3 pos = ConvertToGLM(pMesh->mVertices[x]);
        const glm::vec3 normal = glm::normalize(ConvertToGLM(pMesh->mNormals[x]));

        glm::vec2 texCoord{0};

        if (pMesh->HasTextureCoords(0))
        {
            texCoord = ConvertToGLM(pMesh->mTextureCoords[0][x]);
        }

        glm::vec3 tangent{0};

        if (pMesh->HasTangentsAndBitangents())
        {
            tangent = glm::normalize(ConvertToGLM(pMesh->mTangents[x]));
        }

        vertices.emplace_back(
            pos,
            normal,
            texCoord,
            tangent
        );
    }

    //
    // Record mesh face data
    //
    for (unsigned int x = 0; x < pMesh->mNumFaces; ++x)
    {
        const aiFace& face = pMesh->mFaces[x];

        for (unsigned int f = 0; f < face.mNumIndices; ++f)
        {
            indices.emplace_back(face.mIndices[f]);
        }
    }

    //
    // Record the mesh data
    //
    ModelMesh mesh{};
    mesh.meshIndex = meshIndex;
    mesh.name = pMesh->mName.C_Str();
    mesh.meshType = Render::MeshType::Static;
    mesh.staticVertices = vertices;
    mesh.indices = indices;
    mesh.materialIndex = pMesh->mMaterialIndex;

    return mesh;
}

ModelMesh ModelLoader::ProcessBoneMesh(const aiMesh *pMesh, const unsigned int& meshIndex) const
{
    std::vector<Render::BoneMeshVertex> vertices;
    std::vector<uint32_t> indices;

    //
    // Record mesh vertex data
    //
    for (unsigned int x = 0; x < pMesh->mNumVertices; ++x)
    {
        const glm::vec3 pos = ConvertToGLM(pMesh->mVertices[x]);
        const glm::vec3 normal = glm::normalize(ConvertToGLM(pMesh->mNormals[x]));

        glm::vec2 texCoord{0};

        if (pMesh->HasTextureCoords(0))
        {
            texCoord = ConvertToGLM(pMesh->mTextureCoords[0][x]);
        }

        glm::vec3 tangent{0};

        if (pMesh->HasTangentsAndBitangents())
        {
            tangent = glm::normalize(ConvertToGLM(pMesh->mTangents[x]));
        }

        vertices.emplace_back(
            pos,
            normal,
            texCoord,
            tangent
        );
    }

    //
    // Record mesh face data
    //
    for (unsigned int x = 0; x < pMesh->mNumFaces; ++x)
    {
        const aiFace& face = pMesh->mFaces[x];

        for (unsigned int f = 0; f < face.mNumIndices; ++f)
        {
            indices.emplace_back(face.mIndices[f]);
        }
    }

    ModelMesh mesh{};

    //
    // Record mesh bone data
    //
    for (unsigned int x = 0; x < pMesh->mNumBones; ++x)
    {
        aiBone* pBone = pMesh->mBones[x];

        // Record the bone's info
        ModelBone boneInfo(pBone->mName.C_Str(), x, ConvertToGLM(pBone->mOffsetMatrix));
        mesh.boneMap.insert(std::make_pair(boneInfo.boneName, boneInfo));

        // Update applicable mesh vertex data to include references to this bone
        for (unsigned int y = 0; y < pBone->mNumWeights; ++y)
        {
            const aiVertexWeight& vertexWeight = pBone->mWeights[y];

            Render::BoneMeshVertex& affectedVertex = vertices[vertexWeight.mVertexId];

            bool updatedVertex = false;

            for (int z = 0; z < (int)MAX_BONES_PER_VERTEX; ++z)
            {
                if (affectedVertex.bones[z] == -1)
                {
                    affectedVertex.bones[z] = (int)boneInfo.boneIndex;
                    affectedVertex.boneWeights[z] = vertexWeight.mWeight;

                    updatedVertex = true;
                    break;
                }
            }

            if (!updatedVertex)
            {
                m_logger->Log(Common::LogLevel::Error,
                  "Too many bone attachments for vertex in mesh: {}", std::string(pMesh->mName.C_Str()));
            }
        }
    }

    //
    // Mesh data
    //
    mesh.meshIndex = meshIndex;
    mesh.name = pMesh->mName.C_Str();
    mesh.meshType = Render::MeshType::Bone;
    mesh.boneVertices = vertices;
    mesh.indices = indices;
    mesh.materialIndex = pMesh->mMaterialIndex;

    return mesh;
}

struct NodeToProcess
{
    NodeToProcess(const aiNode* _pNode, std::optional<ModelNode::Ptr> _parentNode)
        : pNode(_pNode)
        , parentNode(std::move(_parentNode))
    { }

    const aiNode* pNode;
    std::optional<ModelNode::Ptr> parentNode;
};

void ModelLoader::ProcessNodes(const Model::Ptr& model, const aiScene *pScene) const
{
    ModelNode::Ptr rootNode;

    std::queue<NodeToProcess> toProcess;
    toProcess.emplace(pScene->mRootNode, std::nullopt);

    while (!toProcess.empty())
    {
        const auto nodeToProcess = toProcess.front();

        const auto node = ProcessNode(model, nodeToProcess.pNode);

        node->bindGlobalTransform = node->localTransform;

        if (nodeToProcess.parentNode.has_value())
        {
            node->parent = *nodeToProcess.parentNode;
            (*nodeToProcess.parentNode)->children.push_back(node);
            node->bindGlobalTransform = (*nodeToProcess.parentNode)->bindGlobalTransform * node->localTransform;
        }

        if (rootNode == nullptr)
        {
            rootNode = node;
        }

        model->nodeMap.insert(std::make_pair(node->id, node));

        for (unsigned int x = 0; x < nodeToProcess.pNode->mNumChildren; ++x)
        {
            toProcess.emplace(nodeToProcess.pNode->mChildren[x], node);
        }

        toProcess.pop();
    }

    model->rootNode = rootNode;
}

ModelNode::Ptr ModelLoader::ProcessNode(const Model::Ptr& model, const aiNode *pNode)
{
    //
    // Process scene graph data
    //
    ModelNode::Ptr node = std::make_shared<ModelNode>();
    node->id = model->nodeMap.size();
    node->name = pNode->mName.C_Str();
    node->localTransform = ConvertToGLM(pNode->mTransformation);

    //
    // Process node mesh data
    //
    for (unsigned int x = 0; x < pNode->mNumMeshes; ++x)
    {
        const unsigned int meshIndex = pNode->mMeshes[x];
        node->meshIndices.push_back(meshIndex);
        model->nodesWithMeshes.insert(node->id);
    }

    return node;
}

void ModelLoader::ProcessSkeletons(const Model::Ptr& model)
{
    for (const auto& nodeId : model->nodesWithMeshes)
    {
        const ModelNode::Ptr node = model->nodeMap[nodeId];
        const ModelNode::Ptr nodeParent = node->parent.lock();

        for (const auto& meshIndex : node->meshIndices)
        {
            const ModelMesh& modelMesh = model->meshes[meshIndex];

            if (modelMesh.boneMap.empty())
            {
                continue;
            }

            //
            // We've found a node with a mesh with a skeleton - traverse up the node hierarchy
            // until either the mesh's node or the parent of the mesh's node is found one level
            // above us
            //
            const ModelBone sampleBoneInfo = modelMesh.boneMap.begin()->second;
            const ModelNode::Ptr boneNode = FindNodeByName(model, sampleBoneInfo.boneName);

            bool skeletonRootFound = false;

            ModelNode::Ptr curNode = boneNode;

            while (curNode != nullptr)
            {
                const auto curNodeParent = curNode->parent.lock();

                if (curNodeParent)
                {
                    if (curNodeParent->id == node->id)
                    {
                        skeletonRootFound = true;
                        break;
                    }

                    if (nodeParent && curNodeParent->id == nodeParent->id)
                    {
                        skeletonRootFound = true;
                        break;
                    }
                }

                curNode = curNodeParent;
            }

            if (skeletonRootFound)
            {
                node->meshSkeletonRoots.insert(std::make_pair(meshIndex, curNode));
            }
        }
    }
}

void ModelLoader::ProcessAnimations(const Model::Ptr& model, const aiScene *pScene)
{
    for (unsigned int x = 0; x < pScene->mNumAnimations; ++x)
    {
        const ModelAnimation animation = ProcessAnimation(pScene->mAnimations[x]);
        model->animations.insert(std::make_pair(animation.animationName, animation));
    }
}

ModelAnimation ModelLoader::ProcessAnimation(const aiAnimation* pAnimation)
{
    ModelAnimation modelAnimation{};
    modelAnimation.animationName = pAnimation->mName.C_Str();
    modelAnimation.animationDurationTicks = pAnimation->mDuration;
    modelAnimation.animationTicksPerSecond = pAnimation->mTicksPerSecond;

    for (unsigned int x = 0; x < pAnimation->mNumChannels; ++x)
    {
        aiNodeAnim* pChannel = pAnimation->mChannels[x];

        NodeKeyFrames nodeKeyFrames{};

        for (unsigned int y = 0; y < pChannel->mNumPositionKeys; ++y)
        {
            const aiVectorKey& positionKey = pChannel->mPositionKeys[y];
            nodeKeyFrames.positionKeyFrames.emplace_back(ConvertToGLM(positionKey.mValue), positionKey.mTime);
        }

        for (unsigned int y = 0; y < pChannel->mNumRotationKeys; ++y)
        {
            const aiQuatKey& rotationKey = pChannel->mRotationKeys[y];
            nodeKeyFrames.rotationKeyFrames.emplace_back(ConvertToGLM(rotationKey.mValue), rotationKey.mTime);
        }

        for (unsigned int y = 0; y < pChannel->mNumScalingKeys; ++y)
        {
            const aiVectorKey& scaleKey = pChannel->mScalingKeys[y];
            nodeKeyFrames.scaleKeyFrames.emplace_back(ConvertToGLM(scaleKey.mValue), scaleKey.mTime);
        }

        modelAnimation.nodeKeyFrameMap.insert(std::make_pair(pChannel->mNodeName.C_Str(), nodeKeyFrames));
    }

    return modelAnimation;
}

void ModelLoader::ProcessEmbeddedTextures(const Model::Ptr& model, const aiScene* pScene)
{
    // Go over all textures in all materials, and attempt to load the texture from the model itself
    for (auto& materialIt : model->materials)
    {
        for (auto& texture : materialIt.second.ambientTextures)
        {
            ProcessEmbeddedTexture(materialIt.second, pScene->GetEmbeddedTexture(texture.fileName.c_str()), texture);
        }
        for (auto& texture : materialIt.second.diffuseTextures)
        {
            ProcessEmbeddedTexture(materialIt.second, pScene->GetEmbeddedTexture(texture.fileName.c_str()), texture);
        }
        for (auto& texture : materialIt.second.specularTextures)
        {
            ProcessEmbeddedTexture(materialIt.second, pScene->GetEmbeddedTexture(texture.fileName.c_str()), texture);
        }
    }
}

void ModelLoader::ProcessEmbeddedTexture(const ModelMaterial& material, const aiTexture* pAiTexture, ModelTexture& modelTexture)
{
    // If pAiTexture is null then the model has no embedded texture, nothing to do
    if (pAiTexture == nullptr)
    {
        return;
    }

    // Since embedded textures don't use real file names (.e.g. "*1"), rewrite the texture's file name to at least be
    // unique, so there aren't file name collisions across textures/materials
    modelTexture.fileName = material.name + modelTexture.fileName;

    ModelEmbeddedData embeddedData{};

    std::size_t numDataBytes = pAiTexture->mWidth * pAiTexture->mHeight * 4;

    // If the texture's height is set to zero, then the embedded data is compressed raw data with a byte size
    // equal to the width's value
    if (pAiTexture->mHeight == 0)
    {
        numDataBytes = pAiTexture->mWidth;
    }

    // Load the embedded texture data from the model
    embeddedData.data = std::vector<std::byte>(
        (std::byte*)pAiTexture->pcData,
        (std::byte*)pAiTexture->pcData + numDataBytes
    );

    embeddedData.dataWidth = pAiTexture->mWidth;
    embeddedData.dataHeight = pAiTexture->mHeight;

    // If we have uncompressed data, swizzle the data from BGRA to RGBA
    // TODO Perf: Better way to handle this?
    if (pAiTexture->mHeight != 0)
    {
        for (std::size_t x = 0; x < embeddedData.data.size(); x = x + 4)
        {
            std::swap(embeddedData.data[x], embeddedData.data[x+2]);
        }
    }

    const bool formatHintZeroed = std::ranges::all_of(pAiTexture->achFormatHint, [](char c){
        return c == 0;
    });
    if (!formatHintZeroed)
    {
        embeddedData.dataFormat = pAiTexture->achFormatHint;
    }

    modelTexture.embeddedData = embeddedData;
}

ModelNode::Ptr ModelLoader::FindNodeByName(const Model::Ptr& model, const std::string& name)
{
    std::queue<ModelNode::Ptr> toProcess;
    toProcess.push(model->rootNode);

    while (!toProcess.empty())
    {
        ModelNode::Ptr node = toProcess.front();
        toProcess.pop();

        if (node->name == name)
        {
            return node;
        }

        for (const auto& child : node->children)
        {
            toProcess.push(child);
        }
    }

    return nullptr;
}

}
