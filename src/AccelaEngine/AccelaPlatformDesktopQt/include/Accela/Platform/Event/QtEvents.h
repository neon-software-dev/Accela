/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_EVENT_QTEVENTS_H
#define LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_EVENT_QTEVENTS_H

#include <Accela/Platform/Event/IEvents.h>

#include <Accela/Common/SharedLib.h>
#include <Accela/Common/Log/ILogger.h>

#include <QKeyCombination>
#include <QEvent>
#include <QPointF>

#include <mutex>

namespace Accela::Platform
{
    class ACCELA_PUBLIC QtEvents : public IEvents
    {
        public:

            explicit QtEvents(Common::ILogger::Ptr logger);

            [[nodiscard]] std::queue<SystemEvent> PopLocalEvents() override;

            [[nodiscard]] std::shared_ptr<const IKeyboardState> GetKeyboardState() override;
            [[nodiscard]] std::shared_ptr<const IMouseState> GetMouseState() override;

            /**
             * Should be called when a Qt event has arrived to an accela-powered QWidget/QWindow.
             */
            void OnLocalEvent(QEvent* pEvent);

            /**
             * Should be called when a Qt event has been delivered to a Qt window in general, irregardless
             * of the widget that ultimately handles the event.
             */
            void OnGlobalEvent(QEvent* pEvent);

        private:

            Common::ILogger::Ptr m_logger;

            std::mutex m_localEventsMutex;
            std::queue<SystemEvent> m_localEvents;

            std::shared_ptr<IKeyboardState> m_keyboardState;
            std::shared_ptr<IMouseState> m_mouseState;

            std::optional<QPointF> m_lastMousePoint;
    };
}

#endif //LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_EVENT_QTEVENTS_H
