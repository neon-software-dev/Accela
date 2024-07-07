/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_MOUSESTATE_H
#define LIBACCELAENGINE_SRC_MOUSESTATE_H

#include <Accela/Engine/IMouseState.h>

#include <Accela/Platform/Event/MouseButtonEvent.h>

#include <unordered_set>

namespace Accela::Engine
{
    class MouseState : public IMouseState
    {
        public:

            void ProcessMouseButtonEvent(const Platform::MouseButtonEvent& event);
            void ClearState();

            [[nodiscard]] bool IsMouseButtonPressed(const Platform::MouseButton& button) const override;

        private:

            std::unordered_set<Platform::MouseButton> m_pressedMouseButtons;
    };
}

#endif //LIBACCELAENGINE_SRC_MOUSESTATE_H
