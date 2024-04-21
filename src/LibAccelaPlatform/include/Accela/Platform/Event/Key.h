/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_KEY_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_KEY_H

#include <Accela/Common/Build.h>

namespace Accela::Platform
{
    /**
     * A key on a keyboard.
     *
     * WARNING: engine logic depends on the order of items in this enum
     */
    enum class Key
    {
        Escape,
        LeftControl,
        Backspace,
        Keypad_Enter,
        Return,

        //////////////
        // START Typed Keys
            A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,
            Zero,One,Two,Three,Four,Five,Six,Seven,Eight,Nine,
            Space,
            Period,
            Question,
            Comma,
            BackQuote,
            Minus, Underscore,
        // END Typed Keys
        //////////////

        Unknown
    };

    SUPPRESS_IS_NOT_USED
    static bool IsTypedKey(const Key& key)
    {
        return key >= Key::A && key < Key::Unknown;
    }

    SUPPRESS_IS_NOT_USED
    static char ToTypedChar(const Key& key)
    {
        switch (key)
        {
            case Key::A: return 'a';
            case Key::B: return 'b';
            case Key::C: return 'c';
            case Key::D: return 'd';
            case Key::E: return 'e';
            case Key::F: return 'f';
            case Key::G: return 'g';
            case Key::H: return 'h';
            case Key::I: return 'i';
            case Key::J: return 'j';
            case Key::K: return 'k';
            case Key::L: return 'l';
            case Key::M: return 'm';
            case Key::N: return 'n';
            case Key::O: return 'o';
            case Key::P: return 'p';
            case Key::Q: return 'q';
            case Key::R: return 'r';
            case Key::S: return 's';
            case Key::T: return 't';
            case Key::U: return 'u';
            case Key::V: return 'v';
            case Key::W: return 'w';
            case Key::X: return 'x';
            case Key::Y: return 'y';
            case Key::Z: return 'z';
            case Key::Zero: return '0';
            case Key::One: return '1';
            case Key::Two: return '2';
            case Key::Three: return '3';
            case Key::Four: return '4';
            case Key::Five: return '5';
            case Key::Six: return '6';
            case Key::Seven: return '7';
            case Key::Eight: return '8';
            case Key::Nine: return '9';
            case Key::Space: return ' ';
            case Key::Period: return '.';
            case Key::Question: return '?';
            case Key::Comma: return ',';
            case Key::BackQuote: return '`';
            case Key::Minus: return '-';
            case Key::Underscore: return '_';
            default: return '?';
        }
    }
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_KEY_H
