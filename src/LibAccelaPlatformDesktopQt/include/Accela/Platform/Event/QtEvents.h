/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_EVENT_QTEVENTS_H
#define LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_EVENT_QTEVENTS_H

#include <Accela/Platform/Event/IEvents.h>

#include "Accela/Common/Log/ILogger.h"

#include <QKeyCombination>
#include <QEvent>
#include <QPointF>

#include <mutex>

namespace Accela::Platform
{
    class QtEvents : public IEvents
    {
        public:

            explicit QtEvents(Common::ILogger::Ptr logger);

            [[nodiscard]] std::queue<SystemEvent> PopLocalEvents() override;

            [[nodiscard]] std::shared_ptr<const IKeyboardState> GetKeyboardState() override;
            [[nodiscard]] std::shared_ptr<const IMouseState> GetMouseState() override;

            void OnLocalEvent(QEvent* pEvent);
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
