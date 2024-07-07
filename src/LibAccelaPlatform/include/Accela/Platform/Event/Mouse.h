/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_MOUSE_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_MOUSE_H

namespace Accela::Platform
{
    enum class MouseButton
    {
        Left,
        Middle,
        Right,
        X1,
        X2
    };

    enum class ClickType
    {
        Press,
        Release
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_EVENT_MOUSE_H
