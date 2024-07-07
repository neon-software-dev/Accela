/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_MESSAGEBASEDSCENE_H
#define ACCELAEDITOR_MESSAGEBASEDSCENE_H

#include "SceneMessageListener.h"

#include "Util/PollingMessageFulfiller.h"

#include <Accela/Engine/Scene/Scene.h>

#include <mutex>
#include <queue>

namespace Accela
{
    class MessageBasedScene : public Engine::Scene
    {
        public:

            ~MessageBasedScene() override
            {
                m_pListener = nullptr;
            }

            void SetListener(SceneMessageListener* pListener)
            {
                m_pListener = pListener;
            }

            void EnqueueMessage(const Common::Message::Ptr& message)
            {
                std::lock_guard<std::mutex> commandsLock(m_messagesMutex);
                m_messages.push(message);
            }

            void OnSimulationStep(unsigned int timeStep) override
            {
                Scene::OnSimulationStep(timeStep);

                ProcessMessages();
                m_messageFulfiller.FulfillFinished();
            }

            void OnSceneStop() override
            {
                m_messageFulfiller.BlockingWaitForAll();

                Scene::OnSceneStop();
            }

        protected:

            virtual void ProcessMessage(const Common::Message::Ptr& message) = 0;

            void SendMessageToListener(const Common::Message::Ptr& message) const
            {
                if (m_pListener != nullptr)
                {
                    m_pListener->OnSceneMessage(message);
                }
            }

        protected:

            PollingMessageFulfiller m_messageFulfiller;

        private:

            void ProcessMessages()
            {
                std::lock_guard<std::mutex> commandsLock(m_messagesMutex);

                while (!m_messages.empty())
                {
                    auto message = m_messages.front();
                    m_messages.pop();
                    ProcessMessage(message);
                }
            }

        private:

            std::mutex m_messagesMutex;
            std::queue<Common::Message::Ptr> m_messages;

            SceneMessageListener* m_pListener{nullptr};
    };
}

#endif //ACCELAEDITOR_MESSAGEBASEDSCENE_H
