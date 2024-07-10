/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Platform/QtUtil.h>

#include <QEvent>
#include <QKeyEvent>

namespace Accela::Platform
{

std::optional<SystemEvent> QtUtil::QtEventToSystemEvent(QEvent* qEvent, const std::optional<QPointF>& lastMousePoint)
{
    switch (qEvent->type())
    {
        case QEvent::Type::KeyPress:
        case QEvent::Type::KeyRelease:
            return QtKeyEventToSystemEvent(qEvent);

        case QEvent::Type::MouseButtonPress:
            return QtMouseButtonEventToSystemEvent(qEvent);

        case QEvent::Type::MouseMove:
            return QtMouseMoveEventToSystemEvent(qEvent, lastMousePoint);

        case QEvent::Type::Wheel:
            return QtWheelEventToSystemEvent(qEvent);

        default:
            return std::nullopt;
    }
}

std::optional<SystemEvent> QtUtil::QtKeyEventToSystemEvent(QEvent* qEvent)
{
    auto pKeyEvent = (QKeyEvent*)qEvent;

    //
    // Action
    //
    KeyEvent::Action action{KeyEvent::Action::KeyPress};

    switch (pKeyEvent->type())
    {
        case QEvent::Type::KeyPress: action = KeyEvent::Action::KeyPress; break;
        case QEvent::Type::KeyRelease: action = KeyEvent::Action::KeyRelease; break;
        default: return std::nullopt;
    }

    //
    // Physical Key
    //
    PhysicalKeyPair physicalKeyPair{PhysicalKey::Unknown, pKeyEvent->nativeScanCode()};

    //
    // Logical Key
    //
    LogicalKeyPair logicalKeyPair{LogicalKey::Unknown, pKeyEvent->nativeVirtualKey()};

    switch (pKeyEvent->key())
    {
        case Qt::Key_Escape: logicalKeyPair.key = LogicalKey::Escape; break;
        case Qt::Key_Control: logicalKeyPair.key = LogicalKey::Control; break;
        case Qt::Key_Shift: logicalKeyPair.key = LogicalKey::Shift; break;
        case Qt::Key_Backspace: logicalKeyPair.key = LogicalKey::Backspace; break;
        case Qt::Key_Enter: logicalKeyPair.key = LogicalKey::Enter; break;
        case Qt::Key_Return: logicalKeyPair.key = LogicalKey::Return; break;
        case Qt::Key_A: logicalKeyPair.key = LogicalKey::A; break;
        case Qt::Key_B: logicalKeyPair.key = LogicalKey::B; break;
        case Qt::Key_C: logicalKeyPair.key = LogicalKey::C; break;
        case Qt::Key_D: logicalKeyPair.key = LogicalKey::D; break;
        case Qt::Key_E: logicalKeyPair.key = LogicalKey::E; break;
        case Qt::Key_F: logicalKeyPair.key = LogicalKey::F; break;
        case Qt::Key_G: logicalKeyPair.key = LogicalKey::G; break;
        case Qt::Key_H: logicalKeyPair.key = LogicalKey::H; break;
        case Qt::Key_I: logicalKeyPair.key = LogicalKey::I; break;
        case Qt::Key_J: logicalKeyPair.key = LogicalKey::J; break;
        case Qt::Key_K: logicalKeyPair.key = LogicalKey::K; break;
        case Qt::Key_L: logicalKeyPair.key = LogicalKey::L; break;
        case Qt::Key_M: logicalKeyPair.key = LogicalKey::M; break;
        case Qt::Key_N: logicalKeyPair.key = LogicalKey::N; break;
        case Qt::Key_O: logicalKeyPair.key = LogicalKey::O; break;
        case Qt::Key_P: logicalKeyPair.key = LogicalKey::P; break;
        case Qt::Key_Q: logicalKeyPair.key = LogicalKey::Q; break;
        case Qt::Key_R: logicalKeyPair.key = LogicalKey::R; break;
        case Qt::Key_S: logicalKeyPair.key = LogicalKey::S; break;
        case Qt::Key_T: logicalKeyPair.key = LogicalKey::T; break;
        case Qt::Key_U: logicalKeyPair.key = LogicalKey::U; break;
        case Qt::Key_V: logicalKeyPair.key = LogicalKey::V; break;
        case Qt::Key_W: logicalKeyPair.key = LogicalKey::W; break;
        case Qt::Key_X: logicalKeyPair.key = LogicalKey::X; break;
        case Qt::Key_Y: logicalKeyPair.key = LogicalKey::Y; break;
        case Qt::Key_Z: logicalKeyPair.key = LogicalKey::Z; break;
        case Qt::Key_1: logicalKeyPair.key = LogicalKey::_1; break;
        case Qt::Key_2: logicalKeyPair.key = LogicalKey::_2; break;
        case Qt::Key_3: logicalKeyPair.key = LogicalKey::_3; break;
        case Qt::Key_4: logicalKeyPair.key = LogicalKey::_4; break;
        case Qt::Key_5: logicalKeyPair.key = LogicalKey::_5; break;
        case Qt::Key_6: logicalKeyPair.key = LogicalKey::_6; break;
        case Qt::Key_7: logicalKeyPair.key = LogicalKey::_7; break;
        case Qt::Key_8: logicalKeyPair.key = LogicalKey::_8; break;
        case Qt::Key_9: logicalKeyPair.key = LogicalKey::_9; break;
        case Qt::Key_0: logicalKeyPair.key = LogicalKey::_0; break;
        case Qt::Key_Space: logicalKeyPair.key = LogicalKey::Space; break;
        case Qt::Key_Period: logicalKeyPair.key = LogicalKey::Period; break;
        case Qt::Key_Slash:
        case Qt::Key_Question: logicalKeyPair.key = LogicalKey::Slash; break;
        case Qt::Key_Comma: logicalKeyPair.key = LogicalKey::Comma; break;
        case Qt::Key_QuoteLeft: logicalKeyPair.key = LogicalKey::Grave; break;
        case Qt::Key_Minus:
        case Qt::Key_Underscore: logicalKeyPair.key = LogicalKey::Minus; break;
        default: break;
    }

    //
    // Modifiers
    //
    std::vector<KeyMod> keyMod;

    if (pKeyEvent->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier)) { keyMod.push_back(KeyMod::Shift); }
    if (pKeyEvent->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) { keyMod.push_back(KeyMod::Control); }

    return KeyEvent(action, physicalKeyPair, logicalKeyPair, keyMod);
}

std::optional<SystemEvent> QtUtil::QtMouseButtonEventToSystemEvent(QEvent *qEvent)
{
    auto pMouseEvent = (QMouseEvent*)qEvent;

    Platform::MouseButton button{Platform::MouseButton::Left};

    switch (pMouseEvent->button())
    {
        case Qt::LeftButton:
            button = Platform::MouseButton::Left;
            break;
        case Qt::RightButton:
            button = Platform::MouseButton::Right;
            break;
        case Qt::MiddleButton:
            button = Platform::MouseButton::Middle;
            break;
        case Qt::ExtraButton1:
            button = Platform::MouseButton::X1;
            break;
        case Qt::ExtraButton2:
            button = Platform::MouseButton::X2;
            break;
        default:
            // Note! Ignoring unsupported buttons
            return std::nullopt;
    }

    return Platform::MouseButtonEvent(
        pMouseEvent->pointingDevice()->uniqueId().numericId(),
        button,
        pMouseEvent->type() == QMouseEvent::MouseButtonPress ? Platform::ClickType::Press : Platform::ClickType::Release,
        pMouseEvent->type() == QMouseEvent::Type::MouseButtonDblClick ? 2 : 1,
        (uint32_t)pMouseEvent->scenePosition().x(),
        (uint32_t)pMouseEvent->scenePosition().y()
    );
}

std::optional<SystemEvent> QtUtil::QtMouseMoveEventToSystemEvent(QEvent *qEvent, const std::optional<QPointF>& lastMousePoint)
{
    auto pMouseEvent = (QMouseEvent*)qEvent;

    double xRel = 0;
    double yRel = 0;

    if (lastMousePoint)
    {
        xRel = pMouseEvent->scenePosition().x() - lastMousePoint->x();
        yRel = pMouseEvent->scenePosition().y() - lastMousePoint->y();
    }

    return Platform::MouseMoveEvent(
        pMouseEvent->pointingDevice()->uniqueId().numericId(),
        (float)pMouseEvent->scenePosition().x(),
        (float)pMouseEvent->scenePosition().y(),
        (float)xRel,
        (float)yRel
    );
}

std::optional<SystemEvent> QtUtil::QtWheelEventToSystemEvent(QEvent *qEvent)
{
    auto pWheelEvent = (QWheelEvent*)qEvent;

    return Platform::MouseWheelEvent(
        pWheelEvent->pointingDevice()->uniqueId().numericId(),
        (float)pWheelEvent->angleDelta().x(),
        (float)pWheelEvent->angleDelta().y()
    );
}

}
