/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "QtKeyboardState.h"
#include "QtMouseState.h"

#include <Accela/Platform/Event/QtEvents.h>
#include <Accela/Platform/QtUtil.h>

#include <QEvent>
#include <QKeyEvent>

namespace Accela::Platform
{

QtEvents::QtEvents(Common::ILogger::Ptr logger)
    : m_logger(std::move(logger))
    , m_keyboardState(std::make_shared<QtKeyboardState>())
    , m_mouseState(std::make_shared<QtMouseState>())
{

}

std::queue<SystemEvent> QtEvents::PopLocalEvents()
{
    std::lock_guard<std::mutex> lock(m_localEventsMutex);
    const auto systemEvents = m_localEvents;
    m_localEvents = {};
    return systemEvents;
}

std::shared_ptr<const IKeyboardState> QtEvents::GetKeyboardState()
{
    return m_keyboardState;
}

std::shared_ptr<const IMouseState> QtEvents::GetMouseState()
{
    return m_mouseState;
}

void QtEvents::OnLocalEvent(QEvent* pEvent)
{
    std::vector<SystemEvent> localEvents;

    // Map Qt event to an Accela system event
    const auto systemEvent = QtUtil::QtEventToSystemEvent(pEvent, m_lastMousePoint);
    if (!systemEvent)
    {
        return;
    }

    // Special-handling: Qt doesn't give us relative mouse movement data like SDL does, so manually keep track of the
    // last seen mouse point, so we can calculate it ourselves as we go
    if (std::holds_alternative<MouseMoveEvent>((*systemEvent)))
    {
        const auto mouseMoveEvent = std::get<MouseMoveEvent>(*systemEvent);
        m_lastMousePoint = QPointF(mouseMoveEvent.xPos, mouseMoveEvent.yPos);
    }

    localEvents.push_back(*systemEvent);

    // Special-handling: Qt combines key press and text input into one "key event" event, unlike SDL which has separate
    // events, so if we're processing  a key press event, also create a fake text input event which contains the text
    // portion of that event
    if (pEvent->type() == QEvent::Type::KeyPress)
    {
        const auto text = ((QKeyEvent*)(pEvent))->text().toStdString();
        if (!text.empty())
        {
            localEvents.emplace_back(TextInputEvent(text));
        }
    }

    std::lock_guard<std::mutex> lock(m_localEventsMutex);

    for (const auto& event : localEvents)
    {
        m_localEvents.push(event);
    }
}

void QtEvents::OnGlobalEvent(QEvent* pEvent)
{
    // WARNING! All local events get passed to both OnLocalEvent and OnGlobalEvent, so need to be careful not to
    // do anything in this method which would cause duplicate processing of the same event on top of OnLocalEvent

    const auto systemEvent = QtUtil::QtEventToSystemEvent(pEvent, m_lastMousePoint);
    if (!systemEvent)
    {
        return;
    }

    // All we do with global events is pass them to QtKeyboardState, so it can update its mapping of what keys
    // are actively pressed
    std::dynamic_pointer_cast<QtKeyboardState>(m_keyboardState)->OnGlobalEvent(*systemEvent);
}

}
