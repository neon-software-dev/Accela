/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMDESKTOPSDL_SRC_EVENT_SDLEVENTSTATE_H
#define LIBACCELAPLATFORMDESKTOPSDL_SRC_EVENT_SDLEVENTSTATE_H

#include <Accela/Platform/Event/IKeyboardState.h>
#include <Accela/Platform/Event/IMouseState.h>

namespace Accela::Platform
{
    class SDLKeyboardState : public IKeyboardState
    {
        public:

            [[nodiscard]] bool IsPhysicalKeyPressed(const Platform::PhysicalKey& physicalKey) const override;
            [[nodiscard]] bool IsPhysicalKeyPressed(const Platform::ScanCode& scanCode) const override;
            [[nodiscard]] bool IsModifierPressed(const Platform::KeyMod& keyMod) const override;
            void ForceResetState() override { /* no-op */ }
    };

    class SDLMouseState : public IMouseState
    {
        public:

            [[nodiscard]] bool IsMouseButtonPressed(const Platform::MouseButton& button) const override;
    };
}

#endif //LIBACCELAPLATFORMDESKTOPSDL_SRC_EVENT_SDLEVENTSTATE_H
