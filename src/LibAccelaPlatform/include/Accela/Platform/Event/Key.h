/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_KEY_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_KEY_H

#include <utility>
#include <cstdint>

namespace Accela::Platform
{
    enum class PhysicalKey
    {
        Unknown,
        A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,
        _1,_2,_3,_4,_5,_6,_7,_8,_9,_0,
        KeypadEnter,
        Return,
        Escape,
        Backspace,
        Tab,
        Space,
        Minus,
        Grave,
        Comma,
        Period,
        Slash,
        LControl, RControl,
        LShift, RShift
    };

    using ScanCode = uint32_t;

    struct PhysicalKeyPair
    {
        PhysicalKeyPair(PhysicalKey _key, ScanCode _scanCode)
            : key(_key)
            , scanCode(_scanCode)
        { }

        [[nodiscard]] bool operator==(const PhysicalKey& _key) const
        {
            return key == _key;
        }

        [[nodiscard]] bool operator==(const ScanCode& _scanCode) const
        {
            return scanCode == _scanCode;
        }

        PhysicalKey key;
        ScanCode scanCode;
    };

    enum class LogicalKey
    {
        Unknown,
        A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,
        _1,_2,_3,_4,_5,_6,_7,_8,_9,_0,
        Enter,
        Return,
        Escape,
        Backspace,
        Tab,
        Space,
        Minus,
        Grave,
        Comma,
        Period,
        Slash,
        Control,
        Shift,
    };

    using VirtualCode = uint32_t;

    struct LogicalKeyPair
    {
        LogicalKeyPair(LogicalKey _key, VirtualCode _virtualCode)
            : key(_key)
            , virtualCode(_virtualCode)
        { }

        [[nodiscard]] bool operator==(const LogicalKey& _key) const
        {
            return key == _key;
        }

        [[nodiscard]] bool operator==(const VirtualCode& _virtualCode) const
        {
            return virtualCode == _virtualCode;
        }

        LogicalKey key;
        VirtualCode virtualCode;
    };

    enum class KeyMod
    {
        Control,
        Shift
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_KEY_H
