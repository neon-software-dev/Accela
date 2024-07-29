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
    //
    // See: PlatformQt.h and PlatformSDL.h for platform-specific documentation
    // around what values various key fields are set to.
    //

    /**
     * Accela concept - identifies a physical key on a standard US keyboard.
     */
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

    /**
     * Identifies a specific physical key by OS/hardware scancode
     */
    using ScanCode = uint32_t;

    /**
     * Data structure which combines all available information about a physical key
     */
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

        /**
         * The physical key, if we could determine it, Unknown otherwise. Always
         * set to Unknown for Qt subsystem.
         */
        PhysicalKey key;

        /**
         * The OS/Platform scancode representing the physical key
         */
        ScanCode scanCode;
    };

    /**
     * Identifies a logical key, irregardless of the specific physical button(s) that were pressed to produce
     * that logical/virtual key
     */
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

    /**
     * Identifies a logical/virtual key by OS/platform virtual keycode
     */
    using VirtualCode = uint32_t;

    /**
     * Data structure which combines all available information about a logical/virtual key
     */
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

        /**
        * The logical/virtual key, if we could determine it, Unknown otherwise.
        */
        LogicalKey key;

        /**
         * The OS/Platform virtual code representing the logical key
         */
        VirtualCode virtualCode;
    };

    /**
     * A modifier key that can be pressed at the same time as other keys
     */
    enum class KeyMod
    {
        Control,
        Shift
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_KEY_H
