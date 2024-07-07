/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMDESKTOPQT_SRC_EVENT_QTEVENTS_H
#define LIBACCELAPLATFORMDESKTOPQT_SRC_EVENT_QTEVENTS_H

#include "Accela/Platform/Event/IEvents.h"

#include "Accela/Common/Log/ILogger.h"

#include <QKeyCombination>

#include <mutex>

namespace Accela::Platform
{
    class QtEvents : public IEvents
    {
        public:

            explicit QtEvents(Common::ILogger::Ptr logger);

            [[nodiscard]] std::queue<SystemEvent> PopSystemEvents() override;

            void EnqueueSystemEvent(const SystemEvent& systemEvent);

            [[nodiscard]] static Platform::Key QtKeyComboToKey(QKeyCombination keyCombo);

        private:

            Common::ILogger::Ptr m_logger;

            std::mutex m_systemEventsMutex;
            std::queue<SystemEvent> m_systemEvents;
    };
}

#endif //LIBACCELAPLATFORMDESKTOPQT_SRC_EVENT_QTEVENTS_H
