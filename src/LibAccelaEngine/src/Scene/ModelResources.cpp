/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ModelResources.h"

#include <Accela/Engine/IEngineAssets.h>

#include <Accela/Platform/File/IFiles.h>

#include <Accela/Render/IRenderer.h>
#include <Accela/Render/Mesh/StaticMesh.h>
#include <Accela/Render/Mesh/BoneMesh.h>

#include <Accela/Common/Thread/ResultMessage.h>

namespace Accela::Engine
{

struct BoolResultMessage : public Common::ResultMessage<bool>
{
    BoolResultMessage()
        : Common::ResultMessage<bool>("BoolResultMessage")
    { }
};

ModelResources::ModelResources(Common::ILogger::Ptr logger,
                               std::shared_ptr<Render::IRenderer> renderer,
                               std::shared_ptr<IEngineAssets> assets,
                               std::shared_ptr<Platform::IFiles> files,
                               std::shared_ptr<Common::MessageDrivenThreadPool> threadPool)
   : m_logger(std::move(logger))
   , m_renderer(std::move(renderer))
   , m_assets(std::move(assets))
   , m_files(std::move(files))
   , m_threadPool(std::move(threadPool))
{

}

std::future<bool> ModelResources::LoadAssetsModel(const std::string& modelName, const std::string& fileExtension, ResultWhen resultWhen)
{
    auto message = std::make_shared<BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& message){
        std::dynamic_pointer_cast<BoolResultMessage>(message)->SetResult(
            OnLoadAssetsModel(modelName, fileExtension, resultWhen)
        );
    });

    return messageFuture;
}

std::future<bool> ModelResources::LoadModel(const std::string& modelName, const Model::Ptr& model, ResultWhen resultWhen)
{
    auto message = std::make_shared<BoolResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& message){
        std::dynamic_pointer_cast<BoolResultMessage>(message)->SetResult(
            OnLoadModel(modelName, model, resultWhen)
        );
    });

    return messageFuture;
}

bool ModelResources::OnLoadAssetsModel(const std::string& modelName, const std::string& fileExtension, ResultWhen resultWhen)
{
    const auto modelExpect = m_assets->ReadModelBlocking(modelName, fileExtension);
    if (!modelExpect)
    {
        m_logger->Log(Common::LogLevel::Error,
          "ModelResources::OnLoadAssetsModel: Failed to load model from assets, name: {}", modelName);
        return false;
    }

    return OnLoadModel(modelName, *modelExpect, resultWhen);
}

bool ModelResources::OnLoadModel(const std::string& modelName, const Model::Ptr& model, ResultWhen resultWhen)
{
    m_logger->Log(Common::LogLevel::Info, "ModelResources: Loading model: {}", modelName);

    if (m_models.contains(modelName))
    {
        m_logger->Log(Common::LogLevel::Error,
          "ModelResources::OnLoadModel: Model already existed, name: {}", modelName);
        return false;
    }

    RegisteredModel registeredModel{};
    registeredModel.model = model;

    //
    // Load the model's materials/textures into the renderer
    //

    // Material index -> Material id
    std::unordered_map<unsigned int, Render::MaterialId> registeredMaterials;

    for (const auto& materialIt : model->materials)
    {
        const auto materialIdExpected = LoadModelMeshMaterial(registeredModel, modelName, materialIt.second, resultWhen);
        if (!materialIdExpected)
        {
            m_logger->Log(Common::LogLevel::Error,
              "ModelResources::OnLoadModel: Failed to load a mesh material: {}", materialIt.second.name);
            return false;
        }

        registeredMaterials[materialIt.first] = *materialIdExpected;
    }

    //
    // Load the model's meshes into the renderer
    //
    for (const auto& modelMeshIt : model->meshes)
    {
        const auto meshIdExpected = LoadModelMesh(registeredModel, registeredMaterials, modelMeshIt.second, resultWhen);
        if (!meshIdExpected)
        {
            m_logger->Log(Common::LogLevel::Error,
              "ModelResources::OnLoadModel: Failed to load a mesh: {}", modelMeshIt.second.name);
            return false;
        }
    }

    std::lock_guard<std::recursive_mutex> modelsLock(m_modelsMutex);
    m_models.insert({modelName, registeredModel});

    return true;
}

std::expected<Render::MaterialId, bool> ModelResources::LoadModelMeshMaterial(RegisteredModel& registeredModel,
                                                                              const std::string& modelName,
                                                                              const ModelMaterial& material,
                                                                              ResultWhen resultWhen) const
{
    //
    // Sanity check that the material the mesh uses only specifies one texture file for each
    // of its texture binding points, as that's all our shaders/engine currently supports.
    //
    if (material.ambientTextures.size() > 1)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadModelMeshMaterial: Only one ambient texture per mesh is supported: {}", material.name);
        return std::unexpected(false);
    }
    if (material.diffuseTextures.size() > 1)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadModelMeshMaterial: Only one diffuse texture per mesh is supported: {}", material.name);
        return std::unexpected(false);
    }
    if (material.specularTextures.size() > 1)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadModelMeshMaterial: Only one specular texture per mesh is supported: {}", material.name);
        return std::unexpected(false);
    }
    if (material.normalTextures.size() > 1)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadModelMeshMaterial: Only one normal texture per mesh is supported: {}", material.name);
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
        const auto textureIdExpected = LoadModelMaterialTexture(registeredModel, modelName, texture, resultWhen);
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
        const auto textureIdExpected = LoadModelMaterialTexture(registeredModel, modelName, texture, resultWhen);
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
        const auto textureIdExpected = LoadModelMaterialTexture(registeredModel, modelName, texture, resultWhen);
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
        const auto textureIdExpected = LoadModelMaterialTexture(registeredModel, modelName, texture, resultWhen);
        if (!textureIdExpected)
        {
            return std::unexpected(false);
        }

        objectMaterialProperties.normalTextureBind = *textureIdExpected;
    }

    //
    // Create the material
    //
    const auto objectMaterial = std::make_shared<Render::ObjectMaterial>(
        m_renderer->GetIds()->materialIds.GetId(),
        objectMaterialProperties,
        material.name
    );

    auto opFuture = m_renderer->CreateMaterial(objectMaterial);

    if (resultWhen == ResultWhen::FullyLoaded && !opFuture.get())
    {
        // The material creation failed
        return std::unexpected(false);
    }

    registeredModel.loadedMaterials.insert(objectMaterial->materialId);

    return objectMaterial->materialId;
}

std::expected<Render::TextureId, bool> ModelResources::LoadModelMaterialTexture(RegisteredModel& registeredModel,
                                                                                const std::string& modelName,
                                                                                const ModelTexture& modelTexture,
                                                                                ResultWhen resultWhen) const
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

    auto opFuture = m_renderer->CreateTexture(texture, textureView, textureSampler, true);

    if (resultWhen == ResultWhen::FullyLoaded && !opFuture.get())
    {
        // The texture creation failed
        return std::unexpected(false);
    }

    registeredModel.loadedTextures.insert(std::make_pair(modelTexture.fileName, textureId));

    return textureId;
}

std::expected<Render::MeshId, bool> ModelResources::LoadModelMesh(RegisteredModel& registeredModel,
                                                                  const std::unordered_map<unsigned int, Render::MaterialId>& registeredMaterials,
                                                                  const ModelMesh& modelMesh,
                                                                  ResultWhen resultWhen) const
{
    const auto materialId = registeredMaterials.find(modelMesh.materialIndex);
    if (materialId == registeredMaterials.cend())
    {
        m_logger->Log(Common::LogLevel::Error, "LoadModelMesh: Can't load mesh as its material doesn't exist");
        return std::unexpected(false);
    }

    const auto meshId = m_renderer->GetIds()->meshIds.GetId();

    Render::Mesh::Ptr mesh = nullptr;

    switch (modelMesh.meshType)
    {
        case Render::MeshType::Static:
        {
            mesh = std::make_shared<Render::StaticMesh>(
                meshId,
                modelMesh.staticVertices.value(),
                modelMesh.indices,
                modelMesh.name
            );
        }
        break;
        case Render::MeshType::Bone:
        {
            mesh = std::make_shared<Render::BoneMesh>(
                meshId,
                modelMesh.boneVertices.value(),
                modelMesh.indices,
                modelMesh.boneMap.size(),
                modelMesh.name
            );
        }
        break;
    }

    auto opFuture = m_renderer->CreateMesh(mesh, Render::MeshUsage::Immutable);

    if (resultWhen == ResultWhen::FullyLoaded && !opFuture.get())
    {
        // The mesh creation failed
        return std::unexpected(false);
    }

    //
    // Record this mesh's loaded data
    //
    LoadedModelMesh loadedModelMesh;
    loadedModelMesh.meshId = meshId;
    loadedModelMesh.meshMaterialId = materialId->second;

    registeredModel.loadedMeshes.insert(std::make_pair(modelMesh.meshIndex, loadedModelMesh));

    return meshId;
}

std::optional<RegisteredModel> ModelResources::GetLoadedModel(const std::string& modelName) const
{
    std::lock_guard<std::recursive_mutex> modelsLock(m_modelsMutex);

    const auto it = m_models.find(modelName);
    if (it == m_models.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

void ModelResources::DestroyModel(const std::string& modelName)
{
    m_logger->Log(Common::LogLevel::Info, "ModelResources::DestroyModel: Destroying model, name: {}", modelName);

    std::lock_guard<std::recursive_mutex> modelsLock(m_modelsMutex);

    const auto model = GetLoadedModel(modelName);
    if (!model)
    {
        m_logger->Log(Common::LogLevel::Error, "ModelResources::DestroyModel: Model doesn't exist, name: {}", modelName);
        return;
    }

    // Destroy the model's material's textures
    for (const auto& textureIt : model->loadedTextures)
    {
        m_renderer->DestroyTexture(textureIt.second);
    }

    // Destroy the model's materials
    for (const auto& materialId : model->loadedMaterials)
    {
        m_renderer->DestroyMaterial(materialId);
    }

    // Destroy the model's meshes
    for (const auto& meshIt : model->loadedMeshes)
    {
        m_renderer->DestroyMesh(meshIt.second.meshId);
    }

    // Erase our knowledge of the model
    m_models.erase(modelName);
}

void ModelResources::DestroyAll()
{
    m_logger->Log(Common::LogLevel::Info, "ModelResources::DestroyAll: Destroying all models");

    while (!m_models.empty())
    {
        DestroyModel(m_models.cbegin()->first);
    }
}

}
