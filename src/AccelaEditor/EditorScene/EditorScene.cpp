/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "EditorScene.h"

namespace Accela
{

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
    }
    else if (message->GetTypeIdentifier() == LoadPackageResourcesCommand::TYPE) {
        ProcessLoadPackageResourcesCommand(std::dynamic_pointer_cast<LoadPackageResourcesCommand>(message));
    }
    else if (message->GetTypeIdentifier() == DestroySceneResourcesCommand::TYPE) {
        ProcessDestroySceneResourcesCommand(std::dynamic_pointer_cast<DestroySceneResourcesCommand>(message));
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

}
