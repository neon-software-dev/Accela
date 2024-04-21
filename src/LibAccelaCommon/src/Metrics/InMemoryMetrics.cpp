/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#include <Accela/Common/Metrics/InMemoryMetrics.h>

namespace Accela::Common
{

void InMemoryMetrics::SetCounterValue(const std::string& name, uintmax_t value)
{
    m_counters.insert_or_assign(name, value);
}

std::optional<uintmax_t> InMemoryMetrics::GetCounterValue(const std::string& name) const
{
    const auto it = m_counters.find(name);
    if (it == m_counters.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

void InMemoryMetrics::SetDoubleValue(const std::string& name, double value)
{
    m_doubles.insert_or_assign(name, value);
}

std::optional<double> InMemoryMetrics::GetDoubleValue(const std::string& name) const
{
    const auto it = m_doubles.find(name);
    if (it == m_doubles.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

}
