/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_LOG_STUBLOGGER_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_LOG_STUBLOGGER_H

#include "ILogger.h"

namespace Accela::Common
{
    /**
     * Concrete ILogger which swallows logs
     */
    class ACCELA_PUBLIC StubLogger : public ILogger
    {
        public:

            void Log(LogLevel, std::string_view) override {}
    };
}

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_LOG_STUBLOGGER_H
