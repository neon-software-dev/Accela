/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_SRC_MODEL_ASSIMPLOGSTREAM_H
#define LIBACCELAENGINE_SRC_MODEL_ASSIMPLOGSTREAM_H

#include <assimp/LogStream.hpp>
#include <assimp/DefaultLogger.hpp>

#include <Accela/Common/Log/ILogger.h>

namespace Accela::Engine
{
    class AssimpLogStream : public Assimp::LogStream
    {
        private:

            static AssimpLogStream* pAssimpLogStream;

            explicit AssimpLogStream(Common::ILogger::Ptr logger)
                : m_logger(std::move(logger))
            { }

        public:

            static void Install(const Common::ILogger::Ptr& logger);
            static void Uninstall();

            void write(const char *message) override
            {
                std::string messageStr(message);
                if (messageStr.empty())
                {
                    return;
                }

                // Strip extra newlines that assimp appends to their logs since our logger already adds them
                if (messageStr.find('\n') == messageStr.size() - 1)
                {
                    messageStr = messageStr.substr(0, messageStr.size() - 1);
                }

                m_logger->Log(Common::LogLevel::Debug, "[AssimpMessage] {}", messageStr);
            }

        private:

            Common::ILogger::Ptr m_logger;
    };
}

#endif //LIBACCELAENGINE_SRC_MODEL_ASSIMPLOGSTREAM_H
