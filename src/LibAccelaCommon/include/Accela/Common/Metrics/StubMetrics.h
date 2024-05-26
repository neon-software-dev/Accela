#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_METRICS_STUBMETRICS_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_METRICS_STUBMETRICS_H

#include "IMetrics.h"

namespace Accela::Common
{
    class StubMetrics : public IMetrics
    {
        public:

            void SetCounterValue(const std::string&, uintmax_t) override {};
            [[nodiscard]] std::optional<uintmax_t> GetCounterValue(const std::string&) const override { return std::nullopt; };

            void SetDoubleValue(const std::string&, double) override {};
            [[nodiscard]] std::optional<double> GetDoubleValue(const std::string&) const override { return std::nullopt; };
    };
}

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_METRICS_STUBMETRICS_H
