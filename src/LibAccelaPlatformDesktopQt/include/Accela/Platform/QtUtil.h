/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_QTUTIL_H
#define LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_QTUTIL_H

#include <Accela/Platform/Event/SystemEvent.h>

#include <QPointF>

#include <optional>

class QEvent;

namespace Accela::Platform
{
    struct QtUtil
    {
        [[nodiscard]] static std::optional<SystemEvent> QtEventToSystemEvent(QEvent* qEvent, const std::optional<QPointF>& lastMousePoint);

        private:

            [[nodiscard]] static std::optional<SystemEvent> QtKeyEventToSystemEvent(QEvent* qEvent);
            [[nodiscard]] static std::optional<SystemEvent> QtMouseButtonEventToSystemEvent(QEvent* qEvent);
            [[nodiscard]] static std::optional<SystemEvent> QtMouseMoveEventToSystemEvent(QEvent* qEvent, const std::optional<QPointF>& lastMousePoint);
            [[nodiscard]] static std::optional<SystemEvent> QtWheelEventToSystemEvent(QEvent* qEvent);
    };
}

#endif //LIBACCELAPLATFORMDESKTOPQT_INCLUDE_ACCELA_PLATFORM_QTUTIL_H
