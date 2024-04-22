/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_KEYBOARDSTATE_H
#define LIBACCELAENGINE_SRC_KEYBOARDSTATE_H

#include <Accela/Engine/IKeyboardState.h>

#include <Accela/Platform/Event/KeyEvent.h>

#include <unordered_set>

namespace Accela::Engine
{
    class KeyboardState : public IKeyboardState
    {
        public:

            void ProcessKeyEvent(const Platform::KeyEvent& event);
            void ClearState();

            [[nodiscard]] bool IsKeyPressed(const Platform::Key& key) const override;

        private:

            std::unordered_set<Platform::Key> m_pressedKeys;
    };
}

#endif //LIBACCELAENGINE_SRC_KEYBOARDSTATE_H
