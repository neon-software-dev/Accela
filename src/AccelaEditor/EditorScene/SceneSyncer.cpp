/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SceneSyncer.h"

#include "../View/AccelaWindow.h"

#include <Accela/Common/Thread/ThreadUtil.h>

namespace Accela
{

SceneSyncer::SceneSyncer(Common::ILogger::Ptr logger)
    : m_logger(std::move(logger))
{

}

void SceneSyncer::AttachToAccelaWindow(AccelaWindow *pAccelaWindow)
{
    m_accelaWindow = pAccelaWindow;
}

std::future<bool> SceneSyncer::LoadPackageResources(const Engine::PackageName& packageName) const
{
    assert(m_accelaWindow.has_value());
    if (!m_accelaWindow.has_value()) { return Common::ImmediateFuture(false); }

    auto msg = std::make_shared<LoadPackageResourcesCommand>(packageName);
    auto fut = msg->CreateFuture();
    (*m_accelaWindow)->EnqueueSceneMessage(msg);
    return fut;
}

std::future<bool> SceneSyncer::DestroyAllResources() const
{
    assert(m_accelaWindow.has_value());
    if (!m_accelaWindow.has_value()) { return Common::ImmediateFuture(false); }

    const auto cmd = std::make_shared<DestroySceneResourcesCommand>();
    auto fut = cmd->CreateFuture();
    (*m_accelaWindow)->EnqueueSceneMessage(cmd);
    return fut;
}

void SceneSyncer::BlockingFullSyncConstruct(const std::optional<Engine::Construct::Ptr>& construct)
{
    assert(m_accelaWindow.has_value());
    if (!m_accelaWindow.has_value()) { return; }

    const auto constructStr = construct.has_value() ? (*construct)->GetName() : "None";
    m_logger->Log(Common::LogLevel::Info, "SceneSyncer: Full syncing construct: {}", constructStr);

    //
    // Destroy all entities
    //
    (*m_accelaWindow)->EnqueueSceneMessage(std::make_shared<DestroyAllEntitiesCommand>());
    m_entities.clear();

    //
    // Create all entities
    //
    if (!construct)
    {
        // No active construct, no entities to create
        return;
    }

    m_logger->Log(Common::LogLevel::Info, "SceneSyncer: Full syncing {} entities", (*construct)->GetEntities().size());

    //
    // Register entities / assign EntityIds to each
    //

    // Entity name -> Entity creation future
    std::vector<std::pair<std::string, std::future<Engine::EntityId>>> createFutures;

    for (const auto& entity : (*construct)->GetEntities())
    {
        const auto createEntityCommand = std::make_shared<CreateEntityCommand>();
        auto createEntityFuture = createEntityCommand->CreateFuture();
        createFutures.emplace_back(entity->name, std::move(createEntityFuture));

        (*m_accelaWindow)->EnqueueSceneMessage(createEntityCommand);
    }

    for (auto& createFuture : createFutures)
    {
        const auto entityId = createFuture.second.get();
        m_entities.insert({createFuture.first, entityId});
    }

    //
    // Sync each entity to its component data
    //
    for (const auto& entity : (*construct)->GetEntities())
    {
        for (const auto& component : entity->components)
        {
            UpdateEntityComponent(entity->name, component).get();
        }
    }
}

void SceneSyncer::BlockingCreateEntity(const Engine::CEntity::Ptr& entity)
{
    assert(m_accelaWindow.has_value());
    if (!m_accelaWindow.has_value()) { return; }

    m_logger->Log(Common::LogLevel::Info, "SceneSyncer: Creating entity: {}", entity->name);

    //
    // Create a new entity
    //
    const auto createEntityCommand = std::make_shared<CreateEntityCommand>();
    auto createEntityFuture = createEntityCommand->CreateFuture();

    (*m_accelaWindow)->EnqueueSceneMessage(createEntityCommand);

    const auto entityId = createEntityFuture.get();

    m_entities.insert({entity->name, entityId});

    //
    // Sync entity component data
    //
    for (const auto& component : entity->components)
    {
        UpdateEntityComponent(entity->name, component).get();
    }
}

std::future<bool> SceneSyncer::DestroyEntity(const std::string& entityName)
{
    assert(m_accelaWindow.has_value());
    if (!m_accelaWindow.has_value()) { return Common::ImmediateFuture(false); }

    const auto it = m_entities.find(entityName);
    if (it == m_entities.cend())
    {
        return Common::ImmediateFuture(false);
    }

    const auto entityId = it->second;

    m_entities.erase(it);

    const auto cmd = std::make_shared<DestroyEntityCommand>(entityId);
    auto fut = cmd->CreateFuture();
    (*m_accelaWindow)->EnqueueSceneMessage(cmd);
    return fut;
}

std::future<bool> SceneSyncer::DestroyAllEntities()
{
    assert(m_accelaWindow.has_value());
    if (!m_accelaWindow.has_value()) { return Common::ImmediateFuture(false); }

    m_entities.clear();

    const auto cmd = std::make_shared<DestroySceneResourcesCommand>();
    auto fut = cmd->CreateFuture();
    (*m_accelaWindow)->EnqueueSceneMessage(cmd);
    return fut;
}

std::future<bool> SceneSyncer::UpdateEntityComponent(const std::string& entityName, const Engine::Component::Ptr& component) const
{
    assert(m_accelaWindow.has_value());
    if (!m_accelaWindow.has_value()) { return Common::ImmediateFuture(false); }

    // Don't send component data to Accela if the component data isn't complete
    if (!component->IsComplete())
    {
        return Common::ImmediateFuture(false);
    }

    const auto it = m_entities.find(entityName);
    if (it == m_entities.cend())
    {
        m_logger->Log(Common::LogLevel::Error, "SceneSyncer:UpdateEntityComponent: No such entity: {}", entityName);
        return Common::ImmediateFuture(false);
    }

    const auto entityId = it->second;

    const auto cmd = std::make_shared<SetEntityComponentCommand>(entityId, component);
    auto fut = cmd->CreateFuture();
    (*m_accelaWindow)->EnqueueSceneMessage(cmd);
    return fut;
}

}
