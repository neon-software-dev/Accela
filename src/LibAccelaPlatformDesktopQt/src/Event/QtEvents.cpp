/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Platform/Event/QtEvents.h>

namespace Accela::Platform
{

QtEvents::QtEvents(Common::ILogger::Ptr logger)
    : m_logger(std::move(logger))
{

}

std::queue<SystemEvent> QtEvents::PopSystemEvents()
{
    std::lock_guard<std::mutex> lock(m_systemEventsMutex);
    const auto systemEvents = m_systemEvents;
    m_systemEvents = {};
    return systemEvents;
}

void QtEvents::EnqueueSystemEvent(const SystemEvent& systemEvent)
{
    std::lock_guard<std::mutex> lock(m_systemEventsMutex);
    m_systemEvents.push(systemEvent);
}

Platform::Key QtEvents::QtKeyComboToKey(QKeyCombination keyCombo)
{
    switch (keyCombo.key())
    {
        case Qt::Key_Escape: return Key::Escape;
        case Qt::Key_Control: return Key::Control;
        case Qt::Key_Shift: return Key::Shift;
        case Qt::Key_Backspace: return Key::Backspace;
        case Qt::Key_Enter: return Key::Keypad_Enter;
        case Qt::Key_Return: return Key::Return;

        case Qt::Key_A: return Key::A;
        case Qt::Key_B: return Key::B;
        case Qt::Key_C: return Key::C;
        case Qt::Key_D: return Key::D;
        case Qt::Key_E: return Key::E;
        case Qt::Key_F: return Key::F;
        case Qt::Key_G: return Key::G;
        case Qt::Key_H: return Key::H;
        case Qt::Key_I: return Key::I;
        case Qt::Key_J: return Key::J;
        case Qt::Key_K: return Key::K;
        case Qt::Key_L: return Key::L;
        case Qt::Key_M: return Key::M;
        case Qt::Key_N: return Key::N;
        case Qt::Key_O: return Key::O;
        case Qt::Key_P: return Key::P;
        case Qt::Key_Q: return Key::Q;
        case Qt::Key_R: return Key::R;
        case Qt::Key_S: return Key::S;
        case Qt::Key_T: return Key::T;
        case Qt::Key_U: return Key::U;
        case Qt::Key_V: return Key::V;
        case Qt::Key_W: return Key::W;
        case Qt::Key_X: return Key::X;
        case Qt::Key_Y: return Key::Y;
        case Qt::Key_Z: return Key::Z;
        case Qt::Key_0: return Key::Zero;
        case Qt::Key_1: return Key::One;
        case Qt::Key_2: return Key::Two;
        case Qt::Key_3: return Key::Three;
        case Qt::Key_4: return Key::Four;
        case Qt::Key_5: return Key::Five;
        case Qt::Key_6: return Key::Six;
        case Qt::Key_7: return Key::Seven;
        case Qt::Key_8: return Key::Eight;
        case Qt::Key_9: return Key::Nine;
        case Qt::Key_Space: return Key::Space;
        case Qt::Key_Period: return Key::Period;
        case Qt::Key_Question: return Key::Question;
        case Qt::Key_Comma: return Key::Comma;
        case Qt::Key_QuoteLeft: return Key::BackQuote;
        case Qt::Key_Minus: return keyCombo.keyboardModifiers() & Qt::KeyboardModifier::ShiftModifier ? Key::Underscore :Key::Minus;

        default: return Key::Unknown;
    }
}

}
