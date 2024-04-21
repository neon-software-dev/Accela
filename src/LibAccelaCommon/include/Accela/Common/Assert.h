/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_ASSERT_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_ASSERT_H

#include "Log/ILogger.h"

#include <cassert>

namespace Accela::Common
{
    template <typename... Args>
    bool Assert(bool condition, const Common::ILogger::Ptr& logger, std::string_view rt_fmt_str, Args&&... args)
    {
        if (!condition)
        {
            logger->Log(Common::LogLevel::Fatal, rt_fmt_str, args...);
        }

        assert(condition);

        return condition;
    }
}

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_ASSERT_H
