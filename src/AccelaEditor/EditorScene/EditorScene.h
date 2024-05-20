/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_EDITORSCENE_EDITORSCENE_H
#define ACCELAEDITOR_EDITORSCENE_EDITORSCENE_H

#include "SceneQuitCommand.h"

#include <Accela/Engine/Scene/Scene.h>

#include <memory>
#include <queue>
#include <mutex>

namespace Accela
{
    /**
     * Scene run by the editor accela instance
     */
    class EditorScene : public Engine::Scene
    {
        public:

          using Ptr = std::shared_ptr<EditorScene>;

        public:

            //
            // Scene
            //
            [[nodiscard]] std::string GetName() const override { return "EditorScene"; };

            void OnSimulationStep(unsigned int) override;

            //
            // Internal
            //

            /**
             * Enqueues a SceneCommand for processing. Thread safe. The commands are popped
             * and consumed during OnSimulationStep callbacks.
             */
            void EnqueueCommand(const SceneCommand::Ptr& command);

        private:

            void ProcessCommands();
            void ProcessCommand(const SceneCommand::Ptr& command);
            void ProcessQuitCommand(const SceneQuitCommand::Ptr&);

        private:

            std::mutex m_commandsMutex;
            std::queue<SceneCommand::Ptr> m_commands;
    };
}

#endif //ACCELAEDITOR_EDITORSCENE_EDITORSCENE_H
