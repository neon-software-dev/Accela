/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "WorldResources.h"
#include "TextureResources.h"
#include "MeshResources.h"
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
    , m_meshes(std::make_shared<MeshResources>(m_logger, m_textures, m_renderer, m_assets, m_files, m_threadPool))
{

}

ITextureResources::Ptr WorldResources::Textures() const { return m_textures; }
IMeshResources::Ptr WorldResources::Meshes() const { return m_meshes; }

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

}
