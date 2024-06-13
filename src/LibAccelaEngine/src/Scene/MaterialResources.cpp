/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "MaterialResources.h"

#include <Accela/Engine/Scene/ITextureResources.h>

#include <Accela/Render/IRenderer.h>

#include <Accela/Common/Thread/ResultMessage.h>

namespace Accela::Engine
{

struct MaterialResultMessage : public Common::ResultMessage<Render::MaterialId>
{
    MaterialResultMessage()
        : Common::ResultMessage<Render::MaterialId>("MaterialResultMessage")
    { }
};

MaterialResources::MaterialResources(Common::ILogger::Ptr logger,
                                     ITextureResourcesPtr textures,
                                     std::shared_ptr<Render::IRenderer> renderer,
                                     std::shared_ptr<Common::MessageDrivenThreadPool> threadPool)
    : m_logger(std::move(logger))
    , m_textures(std::move(textures))
    , m_renderer(std::move(renderer))
    , m_threadPool(std::move(threadPool))
{

}

std::future<Render::MaterialId> MaterialResources::LoadObjectMaterial(
    const CustomResourceIdentifier& resource,
    const ObjectMaterialProperties& properties,
    ResultWhen resultWhen)
{
    auto message = std::make_shared<MaterialResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<MaterialResultMessage>(_message)->SetResult(
            OnLoadObjectMaterial(resource, properties, resultWhen)
        );
    });

    return messageFuture;
}

Render::MaterialId MaterialResources::OnLoadObjectMaterial(const CustomResourceIdentifier& resource,
                                                           const ObjectMaterialProperties& properties,
                                                           ResultWhen resultWhen)
{
    m_logger->Log(Common::LogLevel::Info, "MaterialResources: Loading object material resource: {}", resource.GetUniqueName());

    const auto materialId = GetMaterialId(resource);
    if (materialId)
    {
        m_logger->Log(Common::LogLevel::Warning,
          "MaterialResources::OnLoadObjectMaterial: Material was already loaded, ignoring: {}", resource.GetUniqueName());
        return *materialId;
    }

    const auto renderMaterial = ToRenderMaterial(resource, properties, resultWhen);
    if (!renderMaterial)
    {
        m_logger->Log(Common::LogLevel::Error,
          "MaterialResources::OnLoadObjectMaterial: Failed to create render material: {}", resource.GetUniqueName());
        return Render::INVALID_ID;
    }

    //
    // Tell the renderer to create the material
    //
    std::future<bool> opFuture = m_renderer->CreateMaterial(*renderMaterial);

    if (resultWhen == ResultWhen::FullyLoaded && !opFuture.get())
    {
        // The material creation failed
        std::lock_guard<std::mutex> materialsLock(m_materialsMutex);

        m_renderer->GetIds()->materialIds.ReturnId((*renderMaterial)->materialId);
        m_materials.erase(resource);

        return {Render::INVALID_ID};
    }

    //
    // Record a record of the created material
    //
    std::lock_guard<std::mutex> materialsLock(m_materialsMutex);

    m_materials.insert({resource, (*renderMaterial)->materialId});

    return (*renderMaterial)->materialId;
}

std::expected<Render::Material::Ptr, bool> MaterialResources::ToRenderMaterial(const CustomResourceIdentifier& resource,
                                                                               const ObjectMaterialProperties& properties,
                                                                               ResultWhen resultWhen) const
{
    auto renderMaterialProperties = Render::ObjectMaterialProperties{};
    renderMaterialProperties.isAffectedByLighting = properties.isAffectedByLighting;
    renderMaterialProperties.ambientColor = properties.ambientColor;
    renderMaterialProperties.diffuseColor = properties.diffuseColor;
    renderMaterialProperties.specularColor = properties.specularColor;
    renderMaterialProperties.opacity = properties.opacity;
    renderMaterialProperties.alphaMode = properties.alphaMode;
    renderMaterialProperties.alphaCutoff = properties.alphaCutoff;
    renderMaterialProperties.shininess = properties.shininess;
    renderMaterialProperties.twoSided = properties.twoSided;
    renderMaterialProperties.ambientTextureBlendFactor = properties.ambientTextureBlendFactor;
    renderMaterialProperties.ambientTextureOp = properties.ambientTextureOp;
    renderMaterialProperties.diffuseTextureBlendFactor = properties.diffuseTextureBlendFactor;
    renderMaterialProperties.diffuseTextureOp = properties.diffuseTextureOp;
    renderMaterialProperties.specularTextureBlendFactor = properties.specularTextureBlendFactor;
    renderMaterialProperties.specularTextureOp = properties.specularTextureOp;

    if (!ResolveMaterialTexture(properties.ambientTexture, renderMaterialProperties.ambientTextureBind, resultWhen)) { return std::unexpected(false); }
    if (!ResolveMaterialTexture(properties.diffuseTexture, renderMaterialProperties.diffuseTextureBind, resultWhen)) { return std::unexpected(false); }
    if (!ResolveMaterialTexture(properties.specularTexture, renderMaterialProperties.specularTextureBind, resultWhen)) { return std::unexpected(false); }
    if (!ResolveMaterialTexture(properties.normalTexture, renderMaterialProperties.normalTextureBind, resultWhen)) { return std::unexpected(false); }

    return std::make_shared<Render::ObjectMaterial>(
        m_renderer->GetIds()->materialIds.GetId(),
        renderMaterialProperties,
        resource.GetResourceName()
    );
}

bool MaterialResources::ResolveMaterialTexture(const std::optional<ResourceIdentifier>& resource,
                                               Render::TextureId& out,
                                               ResultWhen resultWhen) const
{
    // If there's no resource to resolve, nothing to do
    if (!resource) { return true; }

    // Look up the texture resource and reuse it if it was already previously loaded
    auto textureId = m_textures->GetTextureId(*resource);

    // If the texture isn't loaded, and the texture resource is a package resource, try to load it
    if (!textureId && resource->IsPackageResource())
    {
        textureId = m_textures->LoadTexture(PackageResourceIdentifier(*resource), resultWhen).get();
    }

    // If we couldn't either find or load the texture, error out
    if (!textureId)
    {
        m_logger->Log(Common::LogLevel::Error,
          "MaterialResources::ToRenderMaterial: Failed to fetch or load texture {}", resource->GetUniqueName());
        return false;
    }

    out = *textureId;

    return true;
}

std::optional<Render::MaterialId> MaterialResources::GetMaterialId(const ResourceIdentifier& resource) const
{
    std::lock_guard<std::mutex> materialsLock(m_materialsMutex);

    const auto it = m_materials.find(resource);
    if (it == m_materials.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

void MaterialResources::DestroyMaterial(const ResourceIdentifier& resource)
{
    m_logger->Log(Common::LogLevel::Info,
        "MaterialResources: Destroying material resource: {}", resource.GetUniqueName());

    std::lock_guard<std::mutex> materialsLock(m_materialsMutex);

    const auto it = m_materials.find(resource);
    if (it == m_materials.cend())
    {
        return;
    }

    m_renderer->DestroyMaterial(it->second);

    m_materials.erase(it);
}

void MaterialResources::DestroyAll()
{
    m_logger->Log(Common::LogLevel::Info, "MaterialResources: Destroying all material resources");

    while (!m_materials.empty())
    {
        DestroyMaterial(m_materials.cbegin()->first);
    }
}

}
