/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_WINDOW_ACCELAWINDOW_H
#define ACCELAEDITOR_WINDOW_ACCELAWINDOW_H

#include "../EditorScene/EditorScene.h"

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

            AccelaWindow();
            ~AccelaWindow() override;

            void EnqueueCommand(const SceneCommand::Ptr& command) const;

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

#endif //ACCELAEDITOR_WINDOW_ACCELAWINDOW_H
