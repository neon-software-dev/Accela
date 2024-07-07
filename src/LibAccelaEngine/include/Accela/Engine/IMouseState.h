/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_IMOUSESTATE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_IMOUSESTATE_H

#include <Accela/Platform/Event/Mouse.h>

#include <memory>

namespace Accela::Engine
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

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_IMOUSESTATE_H
