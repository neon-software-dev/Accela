/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "EditorScene.h"

#include <Accela/Engine/Component/Components.h>

#include <Accela/Engine/Package/CTransformComponent.h>
#include <Accela/Engine/Package/CModelRenderableComponent.h>

#include <Accela/Render/Util/Vector.h>

namespace Accela
{

void EditorScene::OnSceneStart(const Engine::IEngineRuntime::Ptr& _engine)
{
    Scene::OnSceneStart(_engine);

    engine->GetWorldState()->SetAmbientLighting(Engine::DEFAULT_SCENE, 0.5f, glm::vec3(1));

    InitCamera();
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
    } else if (message->GetTypeIdentifier() == DestroyEntityCommand::TYPE) {
        ProcessDestroyEntityCommand(std::dynamic_pointer_cast<DestroyEntityCommand>(message));
    } else if (message->GetTypeIdentifier() == DestroyAllEntitiesCommand::TYPE) {
        ProcessDestroyAllEntitiesCommand(std::dynamic_pointer_cast<DestroyAllEntitiesCommand>(message));
    } else if (message->GetTypeIdentifier() == CreateEntityCommand::TYPE) {
        ProcessCreateEntityCommand(std::dynamic_pointer_cast<CreateEntityCommand>(message));
    } else if (message->GetTypeIdentifier() == SetEntityComponentCommand::TYPE) {
        ProcessSetEntityComponentCommand(std::dynamic_pointer_cast<SetEntityComponentCommand>(message));
    } else if (message->GetTypeIdentifier() == RemoveEntityComponentCommand::TYPE) {
        ProcessRemoveEntityComponentCommand(std::dynamic_pointer_cast<RemoveEntityComponentCommand>(message));
    } else if (message->GetTypeIdentifier() == RotateCameraCommand::TYPE) {
        ProcessRotateCameraCommand(std::dynamic_pointer_cast<RotateCameraCommand>(message));
    } else if (message->GetTypeIdentifier() == PanCameraCommand::TYPE) {
        ProcessPanCameraCommand(std::dynamic_pointer_cast<PanCameraCommand>(message));
    } else if (message->GetTypeIdentifier() == ScaleCommand::TYPE) {
        ProcessScaleCommand(std::dynamic_pointer_cast<ScaleCommand>(message));
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

void EditorScene::ProcessDestroyEntityCommand(const DestroyEntityCommand::Ptr& cmd)
{
    engine->GetWorldState()->DestroyEntity(cmd->eid);
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
        {
            const auto cTransformComponent = std::dynamic_pointer_cast<Engine::CTransformComponent>(cmd->component);

            Engine::AddOrUpdateComponent(engine->GetWorldState(), cmd->eid, cTransformComponent->ToEngineComponent());
        }
        break;
        case Engine::Component::Type::ModelRenderable:
        {
            auto engineComponent = std::dynamic_pointer_cast<Engine::CModelRenderableComponent>(cmd->component)->component;

            Engine::AddOrUpdateComponent(engine->GetWorldState(), cmd->eid, engineComponent);
        }
        break;
    }

    cmd->SetResult(true);
}

void EditorScene::ProcessRemoveEntityComponentCommand(const RemoveEntityComponentCommand::Ptr& cmd)
{
    switch (cmd->type)
    {
        case Engine::Component::Type::Transform:
            Engine::RemoveComponent<Engine::TransformComponent>(engine->GetWorldState(), cmd->eid);
        break;
        case Engine::Component::Type::ModelRenderable:
            Engine::RemoveComponent<Engine::ModelRenderableComponent>(engine->GetWorldState(), cmd->eid);
        break;
    }

    cmd->SetResult(true);
}

void EditorScene::ProcessRotateCameraCommand(const RotateCameraCommand::Ptr& cmd)
{
    const float rotateSensitivityFactor = 0.2f;

    RotateCamera(
        (float)cmd->xRot * rotateSensitivityFactor,
        (float)cmd->yRot * rotateSensitivityFactor
    );
}

void EditorScene::ProcessPanCameraCommand(const PanCameraCommand::Ptr& cmd)
{
    const auto camera = engine->GetWorldState()->GetWorldCamera(Engine::DEFAULT_SCENE);
    const auto renderSettings = engine->GetRenderSettings();

    const float panSensitivityFactor = 0.002f;

    const float xPanAmount = (float)cmd->xPan * (1.0f / renderSettings.globalViewScale) * panSensitivityFactor;
    const float yPanAmount = (float)cmd->yPan * (1.0f / renderSettings.globalViewScale) * panSensitivityFactor;

    const auto xPan = xPanAmount * -camera->GetRightUnit();
    const auto yPan = yPanAmount * camera->GetUpUnit();
    const auto pan = xPan + yPan;

    m_focusPoint += pan;
    camera->SetPosition(camera->GetPosition() + pan);

    RotateCamera(0.0f, 0.0f);
}

void EditorScene::ProcessScaleCommand(const ScaleCommand::Ptr& cmd)
{
    const float scaleSensitivityFactor = 0.01f;

    auto renderSettings = engine->GetRenderSettings();
    renderSettings.globalViewScale *= (1.0f + (cmd->scaleDeltaDegrees * scaleSensitivityFactor));

    engine->SetRenderSettings(renderSettings);
}

void EditorScene::InitCamera()
{
    const auto camera = engine->GetWorldState()->GetWorldCamera(Engine::DEFAULT_SCENE);

    camera->SetPosition({0,0,1});
    RotateCamera(0.0f, 0.0f);
}

void EditorScene::RotateCamera(float yRotDegrees, float rightRotDegrees)
{
    const auto camera = engine->GetWorldState()->GetWorldCamera(Engine::DEFAULT_SCENE);

    // Current camera rotation
    auto currentCameraRot = glm::identity<glm::quat>();

    {
        // Note: Two-step rotation: Determine rotation around y-axis, and then that rotation gives you a rotated right
        // vector which is then itself rotated over
        const auto yRot = glm::angleAxis(glm::radians(m_yRot), glm::vec3(0, -1, 0));
        const auto yRotRight = yRot * glm::vec3(-1, 0, 0);
        const auto rightRot = glm::angleAxis(glm::radians(m_rightRot), yRotRight);

        currentCameraRot = rightRot * yRot;
    }

    // Update rotation angles
    m_yRot += yRotDegrees;
    if (m_yRot >= 360.0f) { m_yRot -= 360.0f; }
    if (m_yRot <= -360.0f) { m_yRot += 360.0f; }

    m_rightRot += rightRotDegrees;
    if (m_rightRot >= 360.0f) { m_rightRot -= 360.0f; }
    if (m_rightRot <= -360.0f) { m_rightRot += 360.0f; }

    // New camera rotation
    auto newCameraRot = glm::identity<glm::quat>();

    {
        const auto yRot = glm::angleAxis(glm::radians(m_yRot), glm::vec3(0, -1, 0));
        const auto newCameraRight = yRot * glm::vec3(-1, 0, 0);
        const auto rightRot = glm::angleAxis(glm::radians(m_rightRot), newCameraRight);

        newCameraRot = rightRot * yRot;
    }

    // Update camera parameters given the new camera position

    // Position
    const auto initialCameraPos = glm::inverse(currentCameraRot) * (camera->GetPosition() - m_focusPoint);
    const auto newCameraPos = (newCameraRot * initialCameraPos) + m_focusPoint;
    camera->SetPosition(newCameraPos);

    // Look
    const auto newCameraLookUnit = m_focusPoint - newCameraPos;
    camera->SetLookUnit(newCameraLookUnit);

    // Up
    const auto newCameraUpUnit = newCameraRot * glm::vec3(0,1,0);
    camera->SetUpUnit(newCameraUpUnit);
}

}
