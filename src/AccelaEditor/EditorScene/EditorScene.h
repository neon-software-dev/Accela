/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_EDITORSCENE_EDITORSCENE_H
#define ACCELAEDITOR_EDITORSCENE_EDITORSCENE_H

#include "Messages.h"

#include "../Util/PollingMessageFulfiller.h"

#include <Accela/Engine/Scene/Scene.h>

#include <Accela/Common/Thread/Message.h>

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

            void OnSceneStart(const Engine::IEngineRuntime::Ptr& engine) override;
            void OnSimulationStep(unsigned int) override;
            void OnSceneStop() override;

            //
            // Internal
            //

            /**
             * Enqueues a message for processing. Thread safe. The messages are popped
             * and consumed during OnSimulationStep callbacks.
             */
            void EnqueueMessage(const Common::Message::Ptr& command);

        private:

            void ProcessMessages();
            void ProcessMessage(const Common::Message::Ptr& message);
            void ProcessSceneQuitCommand(const SceneQuitCommand::Ptr& cmd);
            void ProcessLoadPackageResourcesCommand(const LoadPackageResourcesCommand::Ptr& cmd);
            void ProcessDestroySceneResourcesCommand(const DestroySceneResourcesCommand::Ptr& cmd);
            void ProcessDestroyEntityCommand(const DestroyEntityCommand::Ptr& cmd);
            void ProcessDestroyAllEntitiesCommand(const DestroyAllEntitiesCommand::Ptr& cmd);
            void ProcessCreateEntityCommand(const CreateEntityCommand::Ptr& cmd);
            void ProcessSetEntityComponentCommand(const SetEntityComponentCommand::Ptr& cmd);
            void ProcessRemoveEntityComponentCommand(const RemoveEntityComponentCommand::Ptr& cmd);

        private:

            std::mutex m_messagesMutex;
            std::queue<Common::Message::Ptr> m_messages;

            PollingMessageFulfiller m_messageFulfiller;
    };
}

#endif //ACCELAEDITOR_EDITORSCENE_EDITORSCENE_H
