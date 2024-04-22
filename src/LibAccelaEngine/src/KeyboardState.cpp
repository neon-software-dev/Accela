/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "KeyboardState.h"

namespace Accela::Engine
{

void KeyboardState::ProcessKeyEvent(const Platform::KeyEvent& event)
{
    if (event.action == Platform::KeyEvent::Action::KeyPress)
    {
        m_pressedKeys.insert(event.key);
    }
    else
    {
        m_pressedKeys.erase(event.key);
    }
}

void KeyboardState::ClearState()
{
    m_pressedKeys.clear();
}

bool KeyboardState::IsKeyPressed(const Platform::Key& key) const
{
    return m_pressedKeys.contains(key);
}

}
