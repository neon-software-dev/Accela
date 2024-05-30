/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AccelaWindow.h"

#include "../Thread/AccelaThread.h"

namespace Accela
{

AccelaWindow::AccelaWindow(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics)
    : m_scene(std::make_shared<EditorScene>())
{
    setSurfaceType(QSurface::VulkanSurface);

    // Start the engine thread when the window is constructed, but doesn't run
    // the engine yet
    m_accelaThread = std::make_unique<AccelaThread>(
        this,
        std::move(logger),
        std::move(metrics),
        m_scene
    );
    m_accelaThread->start(); // TODO Perf: Configure priority
}

AccelaWindow::~AccelaWindow()
{
    Destroy();
}

void AccelaWindow::EnqueueSceneMessage(const Common::Message::Ptr& message) const
{
    m_scene->EnqueueMessage(message);
}

void AccelaWindow::showEvent(QShowEvent *e)
{
    QWindow::showEvent(e);

    // When the window is shown, start the engine. Note that showEvent may
    // be called numerous times; RunEngine() internally does nothing if
    // it's already running.
    m_accelaThread->RunEngine();
}

void AccelaWindow::Destroy()
{
    if (m_accelaThread != nullptr)
    {
        // Tell the engine to quit and block until it has
        m_accelaThread->QuitEngine();

        // Stop and wait for the engine thread to finish
        m_accelaThread->wait();
        m_accelaThread->quit();
        m_accelaThread = nullptr;
    }

    // Null out the window's vulkan instance since we've already
    // destroyed all Vulkan and Qt instance objects during engine
    // shutdown. If we don't then the window will crash when trying
    // to destroy its already destroyed instance in destroy()
    setVulkanInstance(nullptr);
}

}
