/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_TEXT_TEXTPROPERTIES_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_TEXT_TEXTPROPERTIES_H

#include "../Color.h"

#include <string>
#include <cstdint>

namespace Accela::Platform
{
    /**
     * Describes text properties to be used when rendering text
     */
    struct TextProperties
    {
        TextProperties() = default;

        TextProperties(std::string _fontFileName,
                       uint8_t _fontSize,
                       uint32_t _wrapLength,
                       Color _fgColor,
                       Color _bgColor)
            : fontFileName(std::move(_fontFileName))
            , fontSize(_fontSize)
            , wrapLength(_wrapLength)
            , fgColor(_fgColor)
            , bgColor(_bgColor)
        { }

        bool operator==(const TextProperties& o) const
        {
            return  fontFileName == o.fontFileName &&
                    fontSize == o.fontSize &&
                    wrapLength == o.wrapLength &&
                    fgColor == o.fgColor &&
                    bgColor == o.bgColor;
        }

        std::string fontFileName; // The file name of the font to use
        uint8_t fontSize{0}; // The font size to use
        uint32_t wrapLength{0}; // At what pixel width the text should be wrapped, or 0 for no wrapping
        Color fgColor{Color::Black()}; // The foreground (text) color
        Color bgColor{Color::White()}; // The background color
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_TEXT_TEXTPROPERTIES_H
