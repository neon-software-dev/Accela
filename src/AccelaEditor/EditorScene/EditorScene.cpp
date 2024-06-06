/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "EditorScene.h"

#include <Accela/Engine/Component/Components.h>

#include <Accela/Engine/Package/CTransformComponent.h>
#include <Accela/Engine/Package/CModelRenderableComponent.h>

namespace Accela
{

void EditorScene::OnSceneStart(const Engine::IEngineRuntime::Ptr& _engine)
{
    Scene::OnSceneStart(_engine);

    engine->GetWorldState()->SetWorldCamera(Engine::DEFAULT_SCENE, std::make_shared<Engine::Camera3D>(glm::vec3{0,0,2}));
    engine->GetWorldState()->SetAmbientLighting(Engine::DEFAULT_SCENE, 0.5f, glm::vec3(1));
}

void EditorScene::OnSimulationStep(unsigned int timeStep)
{
    Scene::OnSimulationStep(timeStep);

    ProcessMessages();
    m_messageFulfiller.FulfillFinished();
}

void EditorScene::OnSceneStop()
{
    m_messageFulfiller.BlockingWaitForAll();

    Scene::OnSceneStop();
}

void EditorScene::EnqueueMessage(const Common::Message::Ptr& message)
{
    std::lock_guard<std::mutex> commandsLock(m_messagesMutex);
    m_messages.push(message);
}

void EditorScene::ProcessMessages()
{
    std::lock_guard<std::mutex> lock(m_messagesMutex);

    while (!m_messages.empty())
    {
        ProcessMessage(m_messages.front());
        m_messages.pop();
    }
}

void EditorScene::ProcessMessage(const Common::Message::Ptr& message)
{
    if (message->GetTypeIdentifier() == SceneQuitCommand::TYPE) {
        ProcessSceneQuitCommand(std::dynamic_pointer_cast<SceneQuitCommand>(message));
    } else if (message->GetTypeIdentifier() == LoadPackageResourcesCommand::TYPE) {
        ProcessLoadPackageResourcesCommand(std::dynamic_pointer_cast<LoadPackageResourcesCommand>(message));
    } else if (message->GetTypeIdentifier() == DestroySceneResourcesCommand::TYPE) {
        ProcessDestroySceneResourcesCommand(std::dynamic_pointer_cast<DestroySceneResourcesCommand>(message));
    } else if (message->GetTypeIdentifier() == DestroyAllEntitiesCommand::TYPE) {
        ProcessDestroyAllEntitiesCommand(std::dynamic_pointer_cast<DestroyAllEntitiesCommand>(message));
    } else if (message->GetTypeIdentifier() == CreateEntityCommand::TYPE) {
        ProcessCreateEntityCommand(std::dynamic_pointer_cast<CreateEntityCommand>(message));
    } else if (message->GetTypeIdentifier() == SetEntityComponentCommand::TYPE) {
        ProcessSetEntityComponentCommand(std::dynamic_pointer_cast<SetEntityComponentCommand>(message));
    }
}

void EditorScene::ProcessSceneQuitCommand(const SceneQuitCommand::Ptr&)
{
    engine->StopEngine();
}

void EditorScene::ProcessLoadPackageResourcesCommand(const LoadPackageResourcesCommand::Ptr& cmd)
{
    m_messageFulfiller.FulfillWhenFinished(
        cmd,
        engine->GetWorldResources()->EnsurePackageResources(cmd->packageName, Engine::ResultWhen::FullyLoaded)
    );
}

void EditorScene::ProcessDestroySceneResourcesCommand(const DestroySceneResourcesCommand::Ptr& cmd)
{
    engine->GetWorldResources()->DestroyAll();
    cmd->SetResult(true);
}

void EditorScene::ProcessDestroyAllEntitiesCommand(const DestroyAllEntitiesCommand::Ptr& cmd)
{
    engine->GetWorldState()->DestroyAllEntities();

    // TODO! Remove when lights supported
    const auto eid = engine->GetWorldState()->CreateEntity();

    auto lightProperties = Render::LightProperties{};
    lightProperties.attenuationMode = Render::AttenuationMode::Linear;
    lightProperties.diffuseColor = glm::vec3(1,1,1);
    lightProperties.diffuseIntensity = glm::vec3(1,1,1);
    lightProperties.specularColor = glm::vec3(1,1,1);
    lightProperties.specularIntensity = glm::vec3(1,1,1);
    lightProperties.directionUnit = glm::vec3(0,0,-1);
    lightProperties.coneFovDegrees = 360.0f;

    auto lightComponent = Engine::LightComponent(lightProperties);
    lightComponent.castsShadows = true;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, lightComponent);

    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition({0,0,2});
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);

    cmd->SetResult(true);
}

void EditorScene::ProcessCreateEntityCommand(const CreateEntityCommand::Ptr& cmd)
{
    const auto entityId = engine->GetWorldState()->CreateEntity();

    cmd->SetResult(entityId);
}

void EditorScene::ProcessSetEntityComponentCommand(const SetEntityComponentCommand::Ptr& cmd)
{
    switch (cmd->component->GetType())
    {
        case Engine::Component::Type::Transform:
            Engine::AddOrUpdateComponent(engine->GetWorldState(), cmd->eid, std::dynamic_pointer_cast<Engine::CTransformComponent>(cmd->component)->component);
        break;
        case Engine::Component::Type::ModelRenderable:
            Engine::AddOrUpdateComponent(engine->GetWorldState(), cmd->eid, std::dynamic_pointer_cast<Engine::CModelRenderableComponent>(cmd->component)->component);
        break;
    }
}

}
