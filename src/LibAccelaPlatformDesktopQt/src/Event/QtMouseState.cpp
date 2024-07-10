/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "QtMouseState.h"

#include <QApplication>

namespace Accela::Platform
{

bool QtMouseState::IsMouseButtonPressed(const MouseButton& button) const
{
    const auto mouseButtons = QApplication::mouseButtons();

    switch (button)
    {
        case MouseButton::Left: return mouseButtons & Qt::MouseButton::LeftButton;
        case MouseButton::Middle: return mouseButtons & Qt::MouseButton::MiddleButton;
        case MouseButton::Right: return mouseButtons & Qt::MouseButton::RightButton;
        case MouseButton::X1: return mouseButtons & Qt::MouseButton::XButton1;
        case MouseButton::X2: return mouseButtons & Qt::MouseButton::XButton2;
    }

    return false;
}

}
