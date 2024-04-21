/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_THREAD_MESSAGE_H
#define LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_THREAD_MESSAGE_H

#include <memory>
#include <string>

namespace Accela::Common
{
    /**
     * Generic message object which can be used as a cross-thread communication primitive.
     */
    class Message
    {
        public:

            using Ptr = std::shared_ptr<Message>;

        public:

            /**
             * @param typeIdentifier A string which uniquely identifies the type/subclass of the message
             */
            explicit Message(std::string typeIdentifier)
                : m_typeIdentifier(std::move(typeIdentifier))
            {}

            virtual ~Message() = default;

            /**
             * @return A string which uniquely identifies the type/subclass of the message
             */
            [[nodiscard]] virtual std::string GetTypeIdentifier() const noexcept
            {
                return m_typeIdentifier;
            }

        private:

            std::string m_typeIdentifier;
    };
}

#endif //LIBACCELACOMMON_INCLUDE_ACCELA_COMMON_THREAD_MESSAGE_H
