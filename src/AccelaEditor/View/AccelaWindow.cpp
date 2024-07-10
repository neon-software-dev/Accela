/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AccelaWindow.h"

#include "../Thread/AccelaThread.h"

#include <Accela/Platform/PlatformQt.h>
#include <Accela/Platform/QtUtil.h>
#include <Accela/Platform/Event/QtEvents.h>

#include <QMouseEvent>
#include <QApplication>

namespace Accela
{

AccelaWindow::AccelaWindow(Common::ILogger::Ptr logger,
                           Common::IMetrics::Ptr metrics,
                           std::shared_ptr<MessageBasedScene> scene)
    : m_logger(std::move(logger))
    , m_scene(std::move(scene))
    , m_platform(std::make_shared<Platform::PlatformQt>(m_logger))
    , m_globalEventFilter(std::make_unique<GlobalEventFilter>(std::dynamic_pointer_cast<Platform::QtEvents>(m_platform->GetEvents())))
{
    setSurfaceType(QSurface::VulkanSurface);

    // Register this object as a listener of outbound/upwards scene messages
    m_scene->SetListener(this);

    // Register a global event filter to pass application events to our platform implementation
    QApplication::instance()->installEventFilter(m_globalEventFilter.get());

    // Install a local event filter for this window to pass local events to our platform implementation
    installEventFilter(this);

    // Starts the engine thread when the window is constructed, but doesn't actually run the engine yet
    m_accelaThread = std::make_unique<AccelaThread>(
        this,
        m_logger,
        std::move(metrics),
        m_platform,
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

void AccelaWindow::OnSceneMessage(const Common::Message::Ptr& message)
{
    emit OnSceneMessageReceived(message);
}

void AccelaWindow::Destroy()
{
    // Unregister local event filter
    removeEventFilter(this);

    // Unregister global event filter
    QApplication::instance()->removeEventFilter(m_globalEventFilter.get());

    // Unregister scene message listener
    m_scene->SetListener(nullptr);

    // Stop the engine and join its thread
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

bool AccelaWindow::eventFilter(QObject*, QEvent* pEvent)
{
    std::dynamic_pointer_cast<Platform::QtEvents>(m_platform->GetEvents())->OnLocalEvent(pEvent);
    return false;
}

void AccelaWindow::showEvent(QShowEvent *e)
{
    QWindow::showEvent(e);
    e->accept();

    // When the window is shown, start the engine. Note that showEvent may
    // be called numerous times; RunEngine() internally does nothing if
    // it's already running.
    m_accelaThread->RunEngine();
}

}
