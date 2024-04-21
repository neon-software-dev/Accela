/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_LOG_ILOGGER_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_LOG_ILOGGER_H

#include <string_view>
#include <memory>
#include <format>

namespace Accela::Common
{
    enum class LogLevel
    {
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

    class ILogger
    {
        public:

            using Ptr = std::shared_ptr<ILogger>;

        public:

            virtual ~ILogger() = default;

            virtual void Log(LogLevel loglevel, std::string_view str) = 0;

            template<typename... Args>
            void Log(LogLevel loglevel, std::string_view rt_fmt_str, Args&&... args)
            {
                Log(loglevel, std::vformat(rt_fmt_str, std::make_format_args(args...)));
            }
    };
}

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_LOG_ILOGGER_H
