/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "WorldResources.h"
#include "TextureResources.h"
#include "EngineAssets.h"

#include "../Audio/AudioManager.h"

#include <Accela/Engine/IEngineAssets.h>

#include <Accela/Render/Mesh/BoneMesh.h>
#include <Accela/Render/Mesh/StaticMesh.h>

#include <Accela/Platform/File/IFiles.h>
#include <Accela/Platform/Text/IText.h>

#include <Accela/Render/IRenderer.h>

#include <algorithm>

namespace Accela::Engine
{

WorldResources::WorldResources(Common::ILogger::Ptr logger,
                               std::shared_ptr<Render::IRenderer> renderer,
                               std::shared_ptr<Platform::IFiles> files,
                               std::shared_ptr<IEngineAssets> assets,
                               std::shared_ptr<Platform::IText> text,
                               AudioManagerPtr audioManager)
    : m_logger(std::move(logger))
    , m_threadPool(std::make_shared<Common::MessageDrivenThreadPool>("Resources", 4))
    , m_renderer(std::move(renderer))
    , m_files(std::move(files))
    , m_assets(std::move(assets))
    , m_text(std::move(text))
    , m_audioManager(std::move(audioManager))
    , m_textures(std::make_shared<TextureResources>(m_logger, m_renderer, m_assets, m_files, m_text, m_threadPool))
{

}

ITextureResources::Ptr WorldResources::Textures() const { return m_textures; }

Render::MeshId WorldResources::RegisterMesh(const Render::Mesh::Ptr& mesh, Render::MeshUsage usage)
{
    if (!m_renderer->CreateMesh(mesh, usage).get())
    {
        m_renderer->GetIds()->meshIds.ReturnId(mesh->id);
        return {Render::INVALID_ID};
    }

    return mesh->id;
}

Render::MeshId WorldResources::RegisterStaticMesh(std::vector<Render::MeshVertex> vertices,
                                                  std::vector<uint32_t> indices,
                                                  Render::MeshUsage usage,
                                                  const std::string& tag)
{
    const auto mesh = std::make_shared<Render::StaticMesh>(
        m_renderer->GetIds()->meshIds.GetId(),
        vertices,
        indices,
        tag
    );

    return RegisterMesh(mesh, usage);
}

Render::MeshId WorldResources::GenerateHeightMapMesh(const Render::TextureId& heightMapTextureId,
                                                     const Render::USize& heightMapDataSize,
                                                     const Render::USize& meshSize_worldSpace,
                                                     const float& displacementFactor,
                                                     Render::MeshUsage usage,
                                                     const std::string& tag)
{
    const auto heightMapTextureOpt = m_textures->GetLoadedTextureData(heightMapTextureId);
    if (!heightMapTextureOpt)
    {
        m_logger->Log(Common::LogLevel::Error,
          "GenerateHeightMapMesh: No such texture is registered, id: {}", heightMapTextureId.id);
        return {Render::INVALID_ID};
    }

    if (!heightMapTextureOpt->data.has_value())
    {
        m_logger->Log(Common::LogLevel::Error,
          "GenerateHeightMapMesh: Texture has no image data, not a height map, id: {}", heightMapTextureId.id);
        return {Render::INVALID_ID};
    }

    //
    // Parse the texture's data to generate height map data
    //
    const auto heightMapData = GenerateHeightMapData(heightMapTextureOpt->data.value(), heightMapDataSize, meshSize_worldSpace, displacementFactor);

    //
    // Transform the height map data points into a mesh
    //
    const auto heightMapMesh = GenerateHeightMapMesh(*heightMapData, meshSize_worldSpace, tag);

    //
    // Register the mesh
    //
    const auto meshId = RegisterMesh(heightMapMesh, usage);
    if (!meshId.IsValid())
    {
        m_logger->Log(Common::LogLevel::Error, "RegisterHeightMapMesh: Failed to register mesh");
        return {Render::INVALID_ID};
    }

    //
    // Record local state about the height map mesh
    //
    m_registeredHeightMaps.insert({meshId, heightMapData});

    return meshId;
}

std::optional<HeightMapData::Ptr> WorldResources::GetHeightMapData(const Render::MeshId& heightMapMeshId) const
{
    const auto it = m_registeredHeightMaps.find(heightMapMeshId);
    if (it == m_registeredHeightMaps.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

void WorldResources::DestroyMesh(Render::MeshId meshId)
{
    //
    // Tell the renderer to destroy the mesh
    //
    m_renderer->DestroyMesh(meshId);

    //
    // Clear out any local state for the mesh
    //
    const auto heightMapIt = m_registeredHeightMaps.find(meshId);
    if (heightMapIt != m_registeredHeightMaps.cend())
    {
        m_registeredHeightMaps.erase(heightMapIt);
    }
}

Render::MaterialId WorldResources::RegisterObjectMaterial(const Render::ObjectMaterialProperties& properties,
                                                          const std::string& tag)
{
    const auto materialId = m_renderer->GetIds()->materialIds.GetId();

    auto material = std::make_shared<Render::ObjectMaterial>(materialId, properties, tag);

    if (!m_renderer->CreateMaterial(material).get())
    {
        m_renderer->GetIds()->materialIds.ReturnId(materialId);
        return {Render::INVALID_ID};
    }

    return materialId;
}

void WorldResources::DestroyMaterial(Render::MaterialId materialId)
{
    m_renderer->DestroyMaterial(materialId);
}

bool WorldResources::RegisterAudio(const std::string& name, const Common::AudioData::Ptr& audioData)
{
    return m_audioManager->RegisterAudio(name, audioData);
}

void WorldResources::DestroyAudio(const std::string& name)
{
    m_audioManager->DestroyAudio(name);
}

bool WorldResources::LoadFontBlocking(const std::string& fontFileName, uint8_t fontSize)
{
    return m_text->LoadFontBlocking(fontFileName, fontSize);
}

bool WorldResources::LoadFontBlocking(const std::string& fontFileName, uint8_t startFontSize, uint8_t endFontSize)
{
    bool allSuccessful = true;

    for (uint8_t fontSize = startFontSize; fontSize <= endFontSize; ++fontSize)
    {
        if (!LoadFontBlocking(fontFileName, fontSize)) { allSuccessful = false; }
    }

    return allSuccessful;
}

bool WorldResources::IsFontLoaded(const std::string& fontFileName, uint8_t fontSize)
{
    return m_text->IsFontLoaded(fontFileName, fontSize);
}

bool WorldResources::RegisterModel(const std::string& modelName, const Model::Ptr& model)
{
    RegisteredModel registeredModel{};
    registeredModel.model = model;

    //
    // Load Materials
    //

    // Material index -> Material id
    std::unordered_map<unsigned int, Render::MaterialId> registeredMaterials;

    for (const auto& materialIt : model->materials)
    {
        const auto materialIdExpected = LoadModelMeshMaterial(registeredModel, modelName, materialIt.second);
        if (!materialIdExpected)
        {
            m_logger->Log(Common::LogLevel::Error, "RegisterModel: Failed to load a mesh material: {}", materialIt.second.name);
            return false;
        }

        registeredMaterials[materialIt.first] = *materialIdExpected;
    }

    //
    // Load Meshes
    //

    for (const auto& modelMeshIt : model->meshes)
    {
        //
        // Create the mesh
        //
        const auto meshId = m_renderer->GetIds()->meshIds.GetId();

        Render::Mesh::Ptr mesh = nullptr;

        switch (modelMeshIt.second.meshType)
        {
            case Render::MeshType::Static:
            {
                mesh = std::make_shared<Render::StaticMesh>(
                    meshId,
                    modelMeshIt.second.staticVertices.value(),
                    modelMeshIt.second.indices,
                    modelMeshIt.second.name
                );
            }
            break;
            case Render::MeshType::Bone:
            {
                mesh = std::make_shared<Render::BoneMesh>(
                    meshId,
                    modelMeshIt.second.boneVertices.value(),
                    modelMeshIt.second.indices,
                    modelMeshIt.second.boneMap.size(),
                    modelMeshIt.second.name
                );
            }
            break;
        }

        const auto materialId = registeredMaterials.find(modelMeshIt.second.materialIndex);
        if (materialId == registeredMaterials.cend())
        {
            m_logger->Log(Common::LogLevel::Error, "RegisterModel: Can't register mesh as its material doesn't exist ");
            return false;
        }

        if (!m_renderer->CreateMesh(mesh, Render::MeshUsage::Immutable).get())
        {
            m_renderer->GetIds()->meshIds.ReturnId(meshId);
            return false;
        }

        //
        // Record this mesh's loaded data
        //
        LoadedModelMesh loadedModelMesh;
        loadedModelMesh.meshId = meshId;
        loadedModelMesh.meshMaterialId = materialId->second;

        registeredModel.loadedMeshes.insert(std::make_pair(modelMeshIt.second.meshIndex, loadedModelMesh));
    }

    m_registeredModels.insert({modelName, registeredModel});

    return true;
}

std::expected<Render::MaterialId, bool>
WorldResources::LoadModelMeshMaterial(RegisteredModel& registeredModel, const std::string& modelName, const ModelMaterial& material)
{
    //
    // Sanity check that the material the mesh uses only specifies one texture file for each
    // of its texture binding points, as that's all our shaders/engine currently supports.
    //
    if (material.ambientTextures.size() > 1)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadMeshMaterial: Only one ambient texture per mesh is supported: {}", material.name);
        return std::unexpected(false);
    }
    if (material.diffuseTextures.size() > 1)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadMeshMaterial: Only one diffuse texture per mesh is supported: {}", material.name);
        return std::unexpected(false);
    }
    if (material.specularTextures.size() > 1)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadMeshMaterial: Only one specular texture per mesh is supported: {}", material.name);
        return std::unexpected(false);
    }
    if (material.normalTextures.size() > 1)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadMeshMaterial: Only one normal texture per mesh is supported: {}", material.name);
        return std::unexpected(false);
    }

    Render::ObjectMaterialProperties objectMaterialProperties{};
    objectMaterialProperties.ambientColor = material.ambientColor;
    objectMaterialProperties.diffuseColor = material.diffuseColor;
    objectMaterialProperties.specularColor = material.specularColor;
    objectMaterialProperties.opacity = material.opacity;

    // If the material supplied alpha mode (gltf models) then use its values directly
    if (material.alphaMode)
    {
        objectMaterialProperties.alphaMode = *material.alphaMode;
        objectMaterialProperties.alphaCutoff = *material.alphaCutoff;
    }
    // Otherwise, use the material's opacity field to determine alpha mode
    else
    {
        objectMaterialProperties.alphaMode = material.opacity == 1.0f ? Render::AlphaMode::Opaque : Render::AlphaMode::Blend;
        objectMaterialProperties.alphaCutoff = 0.01f;
    }

    objectMaterialProperties.shininess = material.shininess;

    // Textures

    for (const auto& texture : material.ambientTextures)
    {
        const auto textureIdExpected = LoadModelMaterialTexture(registeredModel, modelName, texture);
        if (!textureIdExpected)
        {
            return std::unexpected(false);
        }

        objectMaterialProperties.ambientTextureBind = *textureIdExpected;
        objectMaterialProperties.ambientTextureBlendFactor = texture.texBlendFactor;
        objectMaterialProperties.ambientTextureOp = texture.texOp;
    }
    for (const auto& texture : material.diffuseTextures)
    {
        const auto textureIdExpected = LoadModelMaterialTexture(registeredModel, modelName, texture);
        if (!textureIdExpected)
        {
            return std::unexpected(false);
        }

        objectMaterialProperties.diffuseTextureBind = *textureIdExpected;
        objectMaterialProperties.diffuseTextureBlendFactor = texture.texBlendFactor;
        objectMaterialProperties.diffuseTextureOp = texture.texOp;
    }
    for (const auto& texture : material.specularTextures)
    {
        const auto textureIdExpected = LoadModelMaterialTexture(registeredModel, modelName, texture);
        if (!textureIdExpected)
        {
            return std::unexpected(false);
        }

        objectMaterialProperties.specularTextureBind = *textureIdExpected;
        objectMaterialProperties.specularTextureBlendFactor = texture.texBlendFactor;
        objectMaterialProperties.specularTextureOp = texture.texOp;
    }
    for (const auto& texture : material.normalTextures)
    {
        const auto textureIdExpected = LoadModelMaterialTexture(registeredModel, modelName, texture);
        if (!textureIdExpected)
        {
            return std::unexpected(false);
        }

        objectMaterialProperties.normalTextureBind = *textureIdExpected;
    }

    //
    // Create the material
    //
    const auto materialId = m_renderer->GetIds()->materialIds.GetId();

    const Render::Material::Ptr objectMaterial = std::make_shared<Render::ObjectMaterial>(materialId, objectMaterialProperties, material.name);

    if (!m_renderer->CreateMaterial(objectMaterial).get())
    {
        m_renderer->GetIds()->materialIds.ReturnId(materialId);
        return std::unexpected(false);
    }

    return materialId;
}

std::expected<Render::TextureId, bool> WorldResources::LoadModelMaterialTexture(
    RegisteredModel& registeredModel,
    const std::string& modelName,
    const ModelTexture& modelTexture)
{
    const auto loadedTextureIt = registeredModel.loadedTextures.find(modelTexture.fileName);

    // If the texture is already loaded from a previous material, don't load it again
    if (loadedTextureIt != registeredModel.loadedTextures.cend())
    {
        return loadedTextureIt->second;
    }

    const auto textureId = m_renderer->GetIds()->textureIds.GetId();

    std::optional<Common::ImageData::Ptr> textureData;

    //
    // If the model's texture had embedded data, process it into an ImageData
    //
    if (modelTexture.embeddedData)
    {
        const bool embeddedDataIsCompressed = modelTexture.embeddedData->dataHeight == 0;

        // If the embedded data is compressed, rely on platform to uncompress it into an image
        if (embeddedDataIsCompressed)
        {
            const auto textureLoadExpect = m_files->LoadCompressedTexture(
                modelTexture.embeddedData->data,
                modelTexture.embeddedData->dataWidth,
                modelTexture.embeddedData->dataFormat);
            if (!textureLoadExpect)
            {
                m_logger->Log(Common::LogLevel::Error,
                  "LoadModelMaterialTexture: Failed to interpret compressed texture data: {}", modelTexture.fileName);
                return std::unexpected(false);
            }

            textureData = *textureLoadExpect;
        }
        // Otherwise, if the embedded data is uncompressed, we can interpret it directly
        else
        {
            textureData = std::make_shared<Common::ImageData>(
                modelTexture.embeddedData->data,
                1,
                modelTexture.embeddedData->dataWidth,
                modelTexture.embeddedData->dataHeight,
                Common::ImageData::PixelFormat::RGBA32
            );
        }
    }

    //
    // If the model texture has no embedded data, we need to load its data from disk
    //
    if (!textureData)
    {
        const auto textureLoadExpect = m_files->LoadAssetModelTexture(modelName, modelTexture.fileName);
        if (!textureLoadExpect)
        {
            m_logger->Log(Common::LogLevel::Error, "LoadModelMaterialTexture: Failed to load texture file: {}", modelTexture.fileName);
            return std::unexpected(false);
        }

        textureData = *textureLoadExpect;
    }

    //
    // Register the texture and its data as a Texture in the renderer
    //
    const auto texture = Render::Texture::FromImageData(textureId, Render::TextureUsage::ImageMaterial, 1, *textureData, modelTexture.fileName);
    const auto textureView = Render::TextureView::ViewAs2D(Render::TextureView::DEFAULT, Render::TextureView::Aspect::ASPECT_COLOR_BIT);
    const auto textureSampler = Render::TextureSampler(modelTexture.uvAddressMode);

    m_renderer->CreateTexture(texture, textureView, textureSampler, true);
    //if (!) TODO
    //{
        //m_renderer->GetIds()->textureIds.ReturnId(textureId);
        //return std::unexpected(false);
    //}

    registeredModel.loadedTextures.insert(std::make_pair(modelTexture.fileName, textureId));

    return textureId;
}

std::optional<RegisteredModel> WorldResources::GetRegisteredModel(const std::string& modelName) const
{
    const auto it = m_registeredModels.find(modelName);
    if (it != m_registeredModels.cend())
    {
        return it->second;
    }

    return std::nullopt;
}

// Maps a value X in the range of [A1...A2] into the range [B1...B2]
template<typename T>
inline T MapValue(const std::pair<T,T>& a, const std::pair<T, T>& b, const T& val)
{
    return b.first + (((float)(val - a.first) / (float)(a.second - a.first)) * (b.second - b.first));
}

std::vector<double> GenerateHeightMapDataValues(const Common::ImageData::Ptr& heightMapImage,
                                                const Render::USize& heightMapDataSize,
                                                const float& displacementFactor)
{
    std::vector<double> data;
    data.reserve(heightMapDataSize.w * heightMapDataSize.h);

    for (std::size_t y = 0; y < heightMapDataSize.h; ++y)
    {
        for (std::size_t x = 0; x < heightMapDataSize.w; ++x)
        {
            // Map from data/grid position within the height map to pixel position within the image
            const auto imageXPixel = MapValue({0, heightMapDataSize.w - 1}, {0, heightMapImage->GetPixelWidth() - 1}, x);
            const auto imageYPixel = MapValue({0, heightMapDataSize.h - 1}, {0, heightMapImage->GetPixelHeight() - 1}, y);

            const auto imagePixelIndex = (heightMapImage->GetPixelWidth() * imageYPixel) + imageXPixel;
            const auto imagePixelBytes = heightMapImage->GetPixelBytes(0, imagePixelIndex);

            const std::byte& pixelValue = imagePixelBytes[0]; // Noteworthy, assuming grayscale heightmap, only looking at first byte

            data.push_back(
                (((double)std::to_integer<unsigned char>(pixelValue)) / 255.0f)
                * displacementFactor
            );
        }
    }

    return data;
}

HeightMapData::Ptr WorldResources::GenerateHeightMapData(const Common::ImageData::Ptr& heightMapImage,
                                                         const Render::USize& heightMapDataSize,
                                                         const Render::USize& meshSize_worldSpace,
                                                         const float& displacementFactor)
{
    //
    // Create height map data values from sampling the image's data
    //
    const auto heightMapDataValues = GenerateHeightMapDataValues(heightMapImage, heightMapDataSize, displacementFactor);

    //
    // Determine min/max height map values
    //
    double minValue = std::numeric_limits<double>::max();
    double maxValue = std::numeric_limits<double>::min();

    for (const auto& val : heightMapDataValues)
    {
        minValue = std::fmin(minValue, val);
        maxValue = std::fmax(maxValue, val);
    }

    //
    // Result
    //
    HeightMapData::Ptr heightMapData = std::make_shared<HeightMapData>();
    heightMapData->data = heightMapDataValues;
    heightMapData->dataSize = heightMapDataSize;
    heightMapData->minValue = minValue;
    heightMapData->maxValue = maxValue;
    heightMapData->meshSize_worldSpace = meshSize_worldSpace;

    return heightMapData;
}

Render::Mesh::Ptr WorldResources::GenerateHeightMapMesh(const HeightMapData& heightMapData,
                                                        const Render::USize& meshSize_worldSpace,
                                                        const std::string& tag) const
{
    std::vector<Render::MeshVertex> vertices;
    vertices.reserve(heightMapData.dataSize.w * heightMapData.dataSize.h);

    std::vector<uint32_t> indices;
    indices.reserve(heightMapData.dataSize.w * heightMapData.dataSize.h);

    // World distance between adjacent vertices in x and z directions
    const float vertexXDelta = (float)meshSize_worldSpace.w / (float)(heightMapData.dataSize.w - 1);
    const float vertexZDelta = (float)meshSize_worldSpace.h / (float)(heightMapData.dataSize.h - 1);

    // Current world position of the vertex we're processing. Start at the front left corner of the mesh.
    float xPos = -1.0f * (float)meshSize_worldSpace.w / 2.0f;
    float zPos = 1.0f * (float)meshSize_worldSpace.h / 2.0f;

    // Loop over data points in the height map and create a vertex for each
    for (std::size_t y = 0; y < heightMapData.dataSize.h; ++y)
    {
        for (std::size_t x = 0; x < heightMapData.dataSize.w; ++x)
        {
            // The height map data is stored with the "top" row of the height map image in the start of the
            // vector. As we're building our vertices starting from the bottom left, flip the Y coordinate so
            // the bottom left vertex is getting its data from the end of the vector, where the bottom height map
            // row is.
            const auto flippedY = (heightMapData.dataSize.h - 1) - y;

            // Index of this vertex's height map data entry
            const auto dataIndex = x + (flippedY * heightMapData.dataSize.w);

            const auto position = glm::vec3(xPos, heightMapData.data[dataIndex], zPos);

            // Normals are determined in a separate loop below, once we have all the positions of each vertex figured out
            const auto normal = glm::vec3(0,1,0);

            const float uvX = (float)x / ((float)heightMapData.dataSize.w - 1);
            const float uvY = 1.0f - (float)flippedY / ((float)heightMapData.dataSize.h - 1); // Flipped for Vulkan flipped y-axis
            const auto uv = glm::vec2(uvX, uvY);

            const auto tangent = glm::vec3(0,1,0); // TODO: Can/should we calculate this manually

            vertices.emplace_back(position, normal, uv, tangent);

            xPos += vertexXDelta;
        }

        xPos = -1.0f * (float)meshSize_worldSpace.w / 2.0f;
        zPos -= vertexZDelta;
    }

    // Loop over vertices and calculate normals from looking at neighboring vertices
    for (std::size_t y = 0; y < heightMapData.dataSize.h; ++y)
    {
        for (std::size_t x = 0; x < heightMapData.dataSize.w; ++x)
        {
            // Index of this vertex's height map data entry
            const auto dataIndex = x + (y * heightMapData.dataSize.w);

            // model-space position of the vertex to compute a normal for
            const auto centerPosition = vertices[dataIndex].position;

            const bool isLeftEdgeVertex = x == 0;
            const bool isRightEdgeVertex = x == (heightMapData.dataSize.w - 1);
            const bool isBottomEdgeVertex = y == 0;
            const bool isTopEdgeVertex = y == (heightMapData.dataSize.h - 1);

            // Get the positions of the vertices to all four sides of this vertex. If some don't exist
            // because the vertex is on an edge, just default them to the center vertex's position.
            glm::vec3 leftVertexPosition = centerPosition;
            glm::vec3 rightVertexPosition = centerPosition;
            glm::vec3 upVertexPosition = centerPosition;
            glm::vec3 bottomVertexPosition = centerPosition;

            if (!isLeftEdgeVertex)
            {
                leftVertexPosition = vertices[dataIndex - 1].position;
            }
            if (!isBottomEdgeVertex)
            {
                bottomVertexPosition = vertices[dataIndex - heightMapData.dataSize.w].position;
            }
            if (!isRightEdgeVertex)
            {
                rightVertexPosition = vertices[dataIndex + 1].position;
            }
            if (!isTopEdgeVertex)
            {
                upVertexPosition = vertices[dataIndex + heightMapData.dataSize.w].position;
            }

            // Calculate vectors that point left to right and back to front across the center vertex
            const glm::vec3 dx = rightVertexPosition - leftVertexPosition;
            const glm::vec3 dz = bottomVertexPosition - upVertexPosition;

            // Center vertex normal is the normalized cross product of these vectors
            vertices[dataIndex].normal = glm::normalize(glm::cross(dz, dx));
        }
    }

    // Loop over data points in the height map (minus one row/column) and create indices
    for (std::size_t y = 0; y < heightMapData.dataSize.h - 1; ++y)
    {
        for (std::size_t x = 0; x < heightMapData.dataSize.w - 1; ++x)
        {
            const auto dataIndex = x + (y * heightMapData.dataSize.w);

            // triangle 1
            indices.push_back(dataIndex);
            indices.push_back(dataIndex + 1);
            indices.push_back(dataIndex + heightMapData.dataSize.w);

            // triangle 2
            indices.push_back(dataIndex + 1);
            indices.push_back(dataIndex + heightMapData.dataSize.w + 1);
            indices.push_back(dataIndex + heightMapData.dataSize.w);
        }
    }

    return std::make_shared<Render::StaticMesh>(
        m_renderer->GetIds()->meshIds.GetId(),
        vertices,
        indices,
        tag
    );
}

}
