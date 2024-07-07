/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_THREAD_ACCELATHREAD_H
#define ACCELAEDITOR_THREAD_ACCELATHREAD_H

#include "../MessageBasedScene.h"

#include <Accela/Engine/IEngine.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <QThread>

#include <mutex>
#include <condition_variable>
#include <optional>

class QWindow;

namespace Accela
{
    namespace Platform
    {
        class PlatformQt;
    }

    /**
     * Runs an accela engine instance in a QThread
     */
    class AccelaThread : public QThread
    {
        Q_OBJECT

        public:

            /**
             * Create an AccelaThread which renders into the provided (vulkan-capable)
             * QWindow, and which runs the provided EditorScene.
             */
            AccelaThread(QWindow *pWindow,
                         Common::ILogger::Ptr logger,
                         Common::IMetrics::Ptr metrics,
                         std::shared_ptr<Platform::PlatformQt> platform,
                         std::shared_ptr<MessageBasedScene> scene);

            /**
             * Start the engine within this thread, if it's not already
             * running, and wait for it to initialize. This thread object
             * must be started and running.
             */
            void RunEngine();

            /**
             * Quit the engine, if it's running, and wait for the engine
             * to give up thread control and finish
             */
            void QuitEngine();

        protected:

            // QThread entry point
            void run() override;

        private:

            // Commands that can be sent to the QThread to control its operation
            enum class ThreadCommand
            {
                RunEngine, // Tells the thread to start the engine
                QuitEngine // Tells the thread to quit the engine
            };

        private:

            void WaitForCommand();
            void WaitForEngineInitFinished();
            void WaitForEngineQuitFinished();

            void SignalThreadCommand(ThreadCommand command);
            void SignalEngineInitFinished();
            void SignalEngineQuitFinished();

            void EngineRunLoop();

        private:

            enum class State
            {
                WaitingForCommand,
                RunningEngine
            };

        private:

            QWindow* m_pWindow;
            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            std::shared_ptr<Platform::PlatformQt> m_platform;
            std::shared_ptr<MessageBasedScene> m_scene;

            State m_state{State::WaitingForCommand};

            std::mutex m_commandMutex;
            std::condition_variable m_commandCv;
            std::optional<ThreadCommand> m_command;

            std::mutex m_isInitFinishedMutex;
            std::condition_variable m_isInitFinishedCv;
            bool isInitFinished{false};

            std::mutex m_isQuitFinishedMutex;
            std::condition_variable m_isQuitFinishedCv;
            bool isQuitFinished{false};

            std::unique_ptr<Engine::IEngine> m_engine;
    };
}

#endif //ACCELAEDITOR_THREAD_ACCELATHREAD_H
