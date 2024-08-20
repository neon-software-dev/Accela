/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_METRICS_IMETRICS_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_METRICS_IMETRICS_H

#include <Accela/Common/SharedLib.h>

#include <memory>
#include <string>
#include <optional>

namespace Accela::Common
{
    enum class MetricType
    {
        Counter,
        Double
    };

    class ACCELA_PUBLIC IMetrics
    {
        public:

            using Ptr = std::shared_ptr<IMetrics>;

        public:

            virtual ~IMetrics() = default;

            virtual void SetCounterValue(const std::string& name, uintmax_t value) = 0;
            [[nodiscard]] virtual std::optional<uintmax_t> GetCounterValue(const std::string& name) const = 0;

            virtual void SetDoubleValue(const std::string& name, double value) = 0;
            [[nodiscard]] virtual std::optional<double> GetDoubleValue(const std::string& name) const = 0;
    };
}

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_METRICS_IMETRICS_H
