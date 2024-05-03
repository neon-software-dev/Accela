/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "MaterialResources.h"

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
                                     std::shared_ptr<Render::IRenderer> renderer,
                                     std::shared_ptr<Common::MessageDrivenThreadPool> threadPool)
    : m_logger(std::move(logger))
    , m_renderer(std::move(renderer))
    , m_threadPool(std::move(threadPool))
{

}

std::future<Render::MaterialId> MaterialResources::LoadObjectMaterial(
    const Render::ObjectMaterialProperties& properties,
    const std::string& tag,
    ResultWhen resultWhen)
{
    auto message = std::make_shared<MaterialResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<MaterialResultMessage>(_message)->SetResult(
            OnLoadObjectMaterial(properties, tag, resultWhen)
        );
    });

    return messageFuture;
}

Render::MaterialId MaterialResources::OnLoadObjectMaterial(
    const Render::ObjectMaterialProperties& properties,
    const std::string& tag,
    ResultWhen resultWhen)
{
    m_logger->Log(Common::LogLevel::Info, "MaterialResources: Loading object material: {}", tag);

    auto material = std::make_shared<Render::ObjectMaterial>(
        m_renderer->GetIds()->materialIds.GetId(),
        properties,
        tag
    );

    //
    // Tell the renderer to create the material
    //
    std::future<bool> opFuture = m_renderer->CreateMaterial(material);

    if (resultWhen == ResultWhen::FullyLoaded && !opFuture.get())
    {
        // The material creation failed
        std::lock_guard<std::mutex> materialsLock(m_materialsMutex);

        m_renderer->GetIds()->materialIds.ReturnId(material->materialId);
        m_materials.erase(material->materialId);

        return {Render::INVALID_ID};
    }

    //
    // Record a record of the created material
    //
    std::lock_guard<std::mutex> materialsLock(m_materialsMutex);

    m_materials.insert(material->materialId);

    return material->materialId;
}

void MaterialResources::DestroyMaterial(Render::MaterialId materialId)
{
    if (!materialId.IsValid()) { return; }

    m_logger->Log(Common::LogLevel::Info, "MaterialResources::DestroyMaterial: Destroying material, id: {}", materialId.id);

    std::lock_guard<std::mutex> materialsLock(m_materialsMutex);

    if (!m_materials.contains(materialId))
    {
        return;
    }

    m_renderer->DestroyMaterial(materialId);

    m_materials.erase(materialId);
}

void MaterialResources::DestroyAll()
{
    m_logger->Log(Common::LogLevel::Info, "MaterialResources::DestroyAll: Destroying all materials");

    while (!m_materials.empty())
    {
        DestroyMaterial(*m_materials.cbegin());
    }
}

}
