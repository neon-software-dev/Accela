/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_IKEYBOARDSTATE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_IKEYBOARDSTATE_H

#include <Accela/Platform/Event/Key.h>

#include <memory>

namespace Accela::Engine
{
    /**
     * User-facing interface to answering keyboard related queries
     */
    class IKeyboardState
    {
        public:

            using CPtr = std::shared_ptr<const IKeyboardState>;

        public:

            virtual ~IKeyboardState() = default;

            /**
             * @return Whether the specified key is actively pressed
             */
            [[nodiscard]] virtual bool IsKeyPressed(const Platform::Key& key) const = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_IKEYBOARDSTATE_H
