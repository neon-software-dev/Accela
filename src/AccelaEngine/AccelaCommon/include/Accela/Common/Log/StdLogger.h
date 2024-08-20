/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_LOG_STDLOGGER_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_LOG_STDLOGGER_H

#include "ILogger.h"

#include <mutex>

namespace Accela::Common
{
    /**
     * Concrete ILogger which sends logs to std::cout
     */
    class ACCELA_PUBLIC StdLogger : public ILogger
    {
        public:

            explicit StdLogger(const LogLevel& minLogLevel = LogLevel::Debug);

            void Log(LogLevel loglevel, std::string_view str) override;

        private:

            std::mutex m_logMutex;

            LogLevel m_minLogLevel;
    };
}

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_LOG_STDLOGGER_H
