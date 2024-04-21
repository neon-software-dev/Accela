/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_TIMER_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_TIMER_H

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <string>
#include <chrono>

namespace Accela::Common
{
    /**
     * Functionality for timing events. The timer is started
     * at object construction time.
     */
    class Timer
    {
        public:

            /**
             * @param identifier A textual identifier for this timer
             * @param logger A logger to receive timer logs
             */
            explicit Timer(std::string identifier);

            /**
             * Stops the timer and returns the elapsed time.
             */
            std::chrono::duration<double, std::milli> StopTimer();

            /**
             * Stops the timer and returns the elapsed time. Also outputs the result
             * of the timer to the specified logger
             *
             * @param logger The logger to receive the timer output
             */
            std::chrono::duration<double, std::milli> StopTimer(const Common::ILogger::Ptr& logger);

            /**
             * Stops the timer and returns the elapsed time. Also outputs the result
             * of the timer as a metric
             *
             * @param logger The metric to receive the timer output
             */
            std::chrono::duration<double, std::milli> StopTimer(const Common::IMetrics::Ptr& metrics);

        private:

            std::string m_identifier;

            std::chrono::high_resolution_clock::time_point m_startTime;
    };
}

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_TIMER_H
