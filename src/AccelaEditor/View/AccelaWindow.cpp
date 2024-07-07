/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AccelaWindow.h"

#include "../Thread/AccelaThread.h"

#include <Accela/Platform/PlatformQt.h>
#include <Accela/Platform/Event/QtEvents.h>

#include <QMouseEvent>

namespace Accela
{

AccelaWindow::AccelaWindow(Common::ILogger::Ptr logger,
                           Common::IMetrics::Ptr metrics,
                           std::shared_ptr<MessageBasedScene> scene)
    : m_logger(std::move(logger))
    , m_scene(std::move(scene))
{
    setSurfaceType(QSurface::VulkanSurface);

    m_scene->SetListener(this);

    m_platform = std::make_shared<Platform::PlatformQt>(m_logger);

    // Start the engine thread when the window is constructed, but doesn't run
    // the engine yet
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
    m_scene->SetListener(nullptr);

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

void AccelaWindow::showEvent(QShowEvent *e)
{
    QWindow::showEvent(e);
    e->accept();

    // When the window is shown, start the engine. Note that showEvent may
    // be called numerous times; RunEngine() internally does nothing if
    // it's already running.
    m_accelaThread->RunEngine();
}

void AccelaWindow::keyPressEvent(QKeyEvent* pKeyEvent)
{
    QWindow::keyPressEvent(pKeyEvent);
    pKeyEvent->accept();

    OnKeyEvent(Platform::KeyEvent::Action::KeyPress, pKeyEvent);
}

void AccelaWindow::keyReleaseEvent(QKeyEvent* pKeyEvent)
{
    QWindow::keyReleaseEvent(pKeyEvent);
    pKeyEvent->accept();

    OnKeyEvent(Platform::KeyEvent::Action::KeyRelease, pKeyEvent);
}

void AccelaWindow::OnKeyEvent(Platform::KeyEvent::Action action, QKeyEvent* pKeyEvent)
{
    const Platform::KeyEvent keyEvent(
        action,
        Platform::QtEvents::QtKeyComboToKey(pKeyEvent->keyCombination())
    );

    std::dynamic_pointer_cast<Platform::QtEvents>(m_platform->GetEvents())
        ->EnqueueSystemEvent(keyEvent);
}

void AccelaWindow::mousePressEvent(QMouseEvent* pMouseEvent)
{
    QWindow::mousePressEvent(pMouseEvent);
    pMouseEvent->accept();

    OnMouseButtonEvent(pMouseEvent);
}

void AccelaWindow::mouseReleaseEvent(QMouseEvent *pMouseEvent)
{
    QWindow::mouseReleaseEvent(pMouseEvent);
    pMouseEvent->accept();

    OnMouseButtonEvent(pMouseEvent);
}

void AccelaWindow::OnMouseButtonEvent(QMouseEvent* pMouseEvent)
{
    Platform::MouseButton button{Platform::MouseButton::Left};

    switch (pMouseEvent->button())
    {
        case Qt::LeftButton:
            button = Platform::MouseButton::Left;
            break;
        case Qt::RightButton:
            button = Platform::MouseButton::Right;
            break;
        case Qt::MiddleButton:
            button = Platform::MouseButton::Middle;
            break;
        case Qt::ExtraButton1:
            button = Platform::MouseButton::X1;
            break;
        case Qt::ExtraButton2:
            button = Platform::MouseButton::X2;
            break;
        default:
            // Note! Ignoring unsupported buttons
            return;
    }

    const Platform::MouseButtonEvent mouseButtonEvent(
        pMouseEvent->pointingDevice()->uniqueId().numericId(),
        button,
        pMouseEvent->type() == QMouseEvent::MouseButtonPress ? Platform::ClickType::Press : Platform::ClickType::Release,
        pMouseEvent->type() == QMouseEvent::Type::MouseButtonDblClick ? 2 : 1,
        (uint32_t)pMouseEvent->scenePosition().x(),
        (uint32_t)pMouseEvent->scenePosition().y()
    );

    std::dynamic_pointer_cast<Platform::QtEvents>(m_platform->GetEvents())
        ->EnqueueSystemEvent(mouseButtonEvent);
}

void AccelaWindow::mouseMoveEvent(QMouseEvent *pMouseEvent)
{
    QWindow::mouseMoveEvent(pMouseEvent);
    pMouseEvent->accept();

    double xRel = 0;
    double yRel = 0;

    if (m_lastMousePoint)
    {
        xRel = pMouseEvent->scenePosition().x() - m_lastMousePoint->x();
        yRel = pMouseEvent->scenePosition().y() - m_lastMousePoint->y();
    }

    const Platform::MouseMoveEvent mouseMoveEvent(
        pMouseEvent->pointingDevice()->uniqueId().numericId(),
        (float)pMouseEvent->scenePosition().x(),
        (float)pMouseEvent->scenePosition().y(),
        (float)xRel,
        (float)yRel
    );

    std::dynamic_pointer_cast<Platform::QtEvents>(m_platform->GetEvents())
        ->EnqueueSystemEvent(mouseMoveEvent);

    m_lastMousePoint = pMouseEvent->scenePosition();
}

void AccelaWindow::wheelEvent(QWheelEvent *pWheelEvent)
{
    QWindow::wheelEvent(pWheelEvent);
    pWheelEvent->accept();

    const Platform::MouseWheelEvent mouseWheelEvent(
        pWheelEvent->pointingDevice()->uniqueId().numericId(),
        (float)pWheelEvent->angleDelta().x(),
        (float)pWheelEvent->angleDelta().y()
    );

    std::dynamic_pointer_cast<Platform::QtEvents>(m_platform->GetEvents())
        ->EnqueueSystemEvent(mouseWheelEvent);
}

}
