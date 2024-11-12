/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_LOG_ILOGGER_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_LOG_ILOGGER_H

#include <Accela/Common/SharedLib.h>

#include <string_view>
#include <memory>
#include <format>

#define LogFatal(...) \
    m_logger->Log(Common::LogLevel::Fatal, __VA_ARGS__) \

#define LogError(...) \
    m_logger->Log(Common::LogLevel::Error, __VA_ARGS__) \

#define LogWarning(...) \
    m_logger->Log(Common::LogLevel::Warning, __VA_ARGS__) \

#define LogInfo(...) \
    m_logger->Log(Common::LogLevel::Info, __VA_ARGS__) \

#define LogDebug(...) \
    m_logger->Log(Common::LogLevel::Debug, __VA_ARGS__) \

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

    class ACCELA_PUBLIC ILogger
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
