/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "MouseState.h"

namespace Accela::Engine
{

void MouseState::ProcessMouseButtonEvent(const Platform::MouseButtonEvent& event)
{
    if (event.clickType == Platform::ClickType::Press)
    {
        m_pressedMouseButtons.insert(event.button);
    }
    else
    {
        m_pressedMouseButtons.erase(event.button);
    }
}

void MouseState::ClearState()
{
    m_pressedMouseButtons.clear();
}

bool MouseState::IsMouseButtonPressed(const Platform::MouseButton& button) const
{
    return m_pressedMouseButtons.contains(button);
}

}
