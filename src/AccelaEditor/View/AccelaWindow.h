/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_ACCELAWINDOW_H
#define ACCELAEDITOR_VIEW_ACCELAWINDOW_H

#include "../GlobalEventFilter.h"
#include "../MessageBasedScene.h"

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>
#include <Accela/Common/Thread/Message.h>

#include <QWindow>

#include <memory>
#include <optional>

namespace Accela
{
    class AccelaThread;

    namespace Platform
    {
        class PlatformQt;
    }

    /**
     * A Vulkan-capable QWindow which runs an Accela Engine instance
     * in a separate thread to render to the window.
     */
    class AccelaWindow : public QWindow, public SceneMessageListener
    {
        Q_OBJECT

        public:

            AccelaWindow(Common::ILogger::Ptr logger,
                         Common::IMetrics::Ptr metrics,
                         std::shared_ptr<MessageBasedScene> scene);
            ~AccelaWindow() override;

            void EnqueueSceneMessage(const Common::Message::Ptr& message) const;

            void OnSceneMessage(const Common::Message::Ptr& message) override;

            /**
             * Stops the accela engine, if it's running, stops the engine
             * thread, and detaches the Vulkan instance from this window
             */
            void Destroy();

        signals:

            void OnSceneMessageReceived(Common::Message::Ptr);

        protected:

            bool eventFilter(QObject *obj, QEvent *ev) override;
            void showEvent(QShowEvent* e) override;

        private:

            Common::ILogger::Ptr m_logger;
            std::shared_ptr<MessageBasedScene> m_scene;
            std::shared_ptr<Platform::PlatformQt> m_platform;
            std::unique_ptr<GlobalEventFilter> m_globalEventFilter;

            std::unique_ptr<AccelaThread> m_accelaThread;
    };
}

#endif //ACCELAEDITOR_VIEW_ACCELAWINDOW_H
