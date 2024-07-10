/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_GLOBALEVENTFILTER_H
#define ACCELAEDITOR_GLOBALEVENTFILTER_H

#include <Accela/Platform/Event/QtEvents.h>

#include <QObject>

namespace Accela
{
    /**
     * Standalone event filter which passes application global events to PlatformQt
     */
    class GlobalEventFilter : public QObject
    {
        Q_OBJECT

        public:

            explicit GlobalEventFilter(std::shared_ptr<Platform::QtEvents> events)
                : m_events(std::move(events))
            { }

        protected:

            bool eventFilter(QObject*, QEvent* pEvent) override
            {
                m_events->OnGlobalEvent(pEvent);
                return false;
            }

        private:

            std::shared_ptr<Platform::QtEvents> m_events;
    };
}

#endif //ACCELAEDITOR_GLOBALEVENTFILTER_H
