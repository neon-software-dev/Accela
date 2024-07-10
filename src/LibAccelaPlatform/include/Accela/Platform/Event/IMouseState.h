/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_IMOUSESTATE_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_IMOUSESTATE_H

#include <Accela/Platform/Event/Mouse.h>

#include <memory>

namespace Accela::Platform
{
    /**
     * User-facing interface to answering mouse related queries
     */
    class IMouseState
    {
        public:

            using CPtr = std::shared_ptr<const IMouseState>;

        public:

            virtual ~IMouseState() = default;

            /**
             * @return Whether the specified mouse button is actively pressed
             */
            [[nodiscard]] virtual bool IsMouseButtonPressed(const Platform::MouseButton& button) const = 0;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_IMOUSESTATE_H
