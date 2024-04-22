/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_TEXT_ITEXT_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_TEXT_ITEXT_H

#include "RenderedText.h"
#include "TextProperties.h"

#include <expected>
#include <memory>
#include <string>

namespace Accela::Platform
{
    /**
     * Interface to text-based operations
     */
    class IText
    {
        public:

            using Ptr = std::shared_ptr<IText>;

        public:

            virtual ~IText() = default;

            virtual void Destroy() = 0;

            virtual bool LoadFontBlocking(const std::string& fontFileName, uint8_t fontSize) = 0;
            virtual bool IsFontLoaded(const std::string& fontFileName, uint8_t fontSize) = 0;
            virtual void UnloadFont(const std::string& fontFileName) = 0;
            virtual void UnloadFont(const std::string& fontFileName, uint8_t fontSize) = 0;

            [[nodiscard]] virtual std::expected<RenderedText, bool> RenderText(const std::string& text, const TextProperties& properties) const = 0;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_TEXT_ITEXT_H
