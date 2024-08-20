/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "QtKeyboardState.h"

#include <QApplication>

#include <cassert>

namespace Accela::Platform
{

bool QtKeyboardState::IsPhysicalKeyPressed(const PhysicalKey&) const
{
    // Qt does not provide the capability to answer this question, this should
    // never be called by a Qt-backed client.
    assert(false);
    return false;
}

bool QtKeyboardState::IsPhysicalKeyPressed(const ScanCode& scanCode) const
{
    std::lock_guard<std::mutex> lock(m_pressedScanCodesMutex);
    return m_pressedScanCodes.contains(scanCode);
}

bool QtKeyboardState::IsModifierPressed(const KeyMod& keyMod) const
{
    const auto keyModifiers = QApplication::keyboardModifiers();

    switch (keyMod)
    {
        case KeyMod::Control: return keyModifiers & Qt::KeyboardModifier::ControlModifier;
        case KeyMod::Shift: return keyModifiers & Qt::KeyboardModifier::ShiftModifier;
    }

    return false;
}

void QtKeyboardState::OnGlobalEvent(const SystemEvent& systemEvent)
{
    if (std::holds_alternative<KeyEvent>(systemEvent))
    {
        std::lock_guard<std::mutex> lock(m_pressedScanCodesMutex);

        const auto keyEvent = std::get<KeyEvent>(systemEvent);

        switch (keyEvent.action)
        {
            case KeyEvent::Action::KeyPress:
            {
                m_pressedScanCodes.insert(keyEvent.physicalKey.scanCode);
            }
            break;
            case KeyEvent::Action::KeyRelease:
            {
                m_pressedScanCodes.erase(keyEvent.physicalKey.scanCode);
            }
            break;
        }
    }
}

void QtKeyboardState::ForceResetState()
{
    std::lock_guard<std::mutex> lock(m_pressedScanCodesMutex);
    m_pressedScanCodes.clear();
}

}
