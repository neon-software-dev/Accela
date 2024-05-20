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

    ProcessCommands();
}

void EditorScene::EnqueueCommand(const SceneCommand::Ptr& command)
{
    std::lock_guard<std::mutex> commandsLock(m_commandsMutex);
    m_commands.push(command);
}

void EditorScene::ProcessCommands()
{
    std::lock_guard<std::mutex> commandsLock(m_commandsMutex);

    while (!m_commands.empty())
    {
        ProcessCommand(m_commands.front());
        m_commands.pop();
    }
}

void EditorScene::ProcessCommand(const SceneCommand::Ptr& command)
{
    if (command->GetType() == SceneQuitCommand::TYPE) {
        ProcessQuitCommand(std::dynamic_pointer_cast<SceneQuitCommand>(command));
    }
}

void EditorScene::ProcessQuitCommand(const SceneCommand::Ptr&)
{
    engine->StopEngine();
}

}
