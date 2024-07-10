/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_IKEYBOARDSTATE_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_IKEYBOARDSTATE_H

#include <Accela/Platform/Event/Key.h>

#include <memory>

namespace Accela::Platform
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

            [[nodiscard]] virtual bool IsPhysicalKeyPressed(const Platform::PhysicalKey& physicalKey) const = 0;
            [[nodiscard]] virtual bool IsPhysicalKeyPressed(const Platform::ScanCode& scanCode) const = 0;
            [[nodiscard]] virtual bool IsModifierPressed(const Platform::KeyMod& keyMod) const = 0;

            virtual void ForceResetState() = 0;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_IKEYBOARDSTATE_H
