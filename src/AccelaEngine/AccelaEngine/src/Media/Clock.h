/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_CLOCK_H
#define ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_CLOCK_H

#include "MediaCommon.h"

#include <chrono>
#include <optional>

namespace Accela::Engine
{
    struct Clock
    {
        [[nodiscard]] std::optional<MediaPoint> InterpolatedTime(const std::chrono::steady_clock::time_point& now) const
        {
            if (!IsValid()) { return std::nullopt; }

            return *syncPoint + (now - *syncTime);
        }

        void Invalidate()
        {
            syncPoint = std::nullopt;
            syncTime = std::nullopt;
        }

        [[nodiscard]] bool IsValid() const { return syncPoint && syncTime; }

        void SetExplicit(const MediaPoint& _syncPoint, const std::chrono::steady_clock::time_point& _syncTime)
        {
            syncPoint = _syncPoint;
            syncTime = _syncTime;
        }

        std::optional<MediaPoint> syncPoint;
        std::optional<std::chrono::steady_clock::time_point> syncTime;
    };
}

#endif //ACCELAENGINE_ACCELAENGINE_SRC_MEDIA_CLOCK_H
