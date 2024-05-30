/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_VIEW_ACCELAWINDOW_H
#define ACCELAEDITOR_VIEW_ACCELAWINDOW_H

#include "../EditorScene/EditorScene.h"

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>
#include <Accela/Common/Thread/Message.h>

#include <QWindow>

#include <memory>

namespace Accela
{
    class AccelaThread;

    /**
     * A Vulkan-capable QWindow which runs an Accela Engine instance
     * in a separate thread to render to the window.
     */
    class AccelaWindow : public QWindow
    {
        Q_OBJECT

        public:

            AccelaWindow(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics);
            ~AccelaWindow() override;

            void EnqueueSceneMessage(const Common::Message::Ptr& message) const;

            /**
             * Stops the accela engine, if it's running, stops the engine
             * thread, and detaches the Vulkan instance from this window
             */
            void Destroy();

        protected:

            void showEvent(QShowEvent* e) override;

        private:

            std::unique_ptr<AccelaThread> m_accelaThread;
            EditorScene::Ptr m_scene;
    };
}

#endif //ACCELAEDITOR_VIEW_ACCELAWINDOW_H
