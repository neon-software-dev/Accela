/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_THREAD_MESSAGEDRIVENTHREADPOOL_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_THREAD_MESSAGEDRIVENTHREADPOOL_H

#include <Accela/Common/SharedLib.h>
#include <Accela/Common/Thread/Message.h>
#include <Accela/Common/Container/ConcurrentQueue.h>

#include <thread>
#include <memory>
#include <functional>
#include <optional>
#include <vector>

namespace Accela::Common
{
    /**
     * Manages a thread pool which messages can be posted to for asynchronous processing on thread pool threads.
     *
     * Use the PostMessage() method to submit messages to be processed by the thread pool.
     *
     * When posting a message, a specific message handler can be provided. If so, that handler will be invoked by
     * the thread pool. If not, the global message handler provided to the constructor will be invoked.
     *
     * An optional idle handler can be provided to the constructor. If so, *every* thread in the thread pool which
     * has not received a message within the last 50ms will invoke that handler.
     */
    class ACCELA_PUBLIC MessageDrivenThreadPool
    {
        public:

            using MessageHandler = std::function<void(const Message::Ptr&)>;
            using IdleHandler = std::function<void()>;

        public:

            /**
             * @param tag Tag to associate with the thread pool
             * @param poolSize Number of threads to spawn for handling messages
             * @param msgHandler An optional MessageHandler that should be executed when messages arrive
             * @param idleHandler An optional handler, that if provided, will be executed every 50ms that no message has arrived
             */
            explicit MessageDrivenThreadPool(
                std::string tag,
                unsigned int poolSize = 1,
                std::optional<MessageHandler> msgHandler = std::nullopt,
                std::optional<IdleHandler> idleHandler = std::nullopt);
            ~MessageDrivenThreadPool();

            /**
             * Send a message from the current thread to the message handling thread pool.
             *
             * Full thread safety to be called from any thread.
             *
             * @param message The message to be sent.
             * @param messageHandler An optional handler to be invoked, overriding the global message handler
             */
            void PostMessage(const Message::Ptr& message, const std::optional<MessageHandler>& messageHandler = std::nullopt);

        private:

            struct EnqueuedMessage
            {
                EnqueuedMessage() = default;

                EnqueuedMessage(Message::Ptr _message, std::optional<MessageHandler> _handler)
                    : message(std::move(_message))
                    , handler(std::move(_handler))
                { }

                Message::Ptr message;
                std::optional<MessageHandler> handler;
            };

        private:

            [[nodiscard]] std::string GetThreadIdentifier(unsigned int threadIndex);

            void MessageReceiverThreadFunc(const std::string& threadIdentifier);

        private:

            std::string m_tag;
            unsigned int m_poolSize;
            std::optional<MessageHandler> m_msgHandler;
            std::optional<IdleHandler> m_idleHandler;
            std::unique_ptr<ConcurrentQueue<EnqueuedMessage>> m_msgQueue;

            bool m_run = true;
            std::vector<std::thread> m_threads;
    };
}


#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_THREAD_MESSAGEDRIVENTHREADPOOL_H
