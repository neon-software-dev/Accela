/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_TEXT_RENDEREDTEXT_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_TEXT_RENDEREDTEXT_H

#include <Accela/Common/ImageData.h>

#include <cstdint>

namespace Accela::Platform
{
    /**
     * Data associated with text that was rendered.
     *
     * Important: imageData will always be resized upwards for its dimensions to be a power
     * of two, for use as a texture. textPixelWidth and textPixelHeight describe the area
     * within the resized image which actually contains the rendered text.
     */
    struct RenderedText
    {
        Common::ImageData::Ptr imageData; // The rendered text's image data
        uint32_t textPixelWidth{0}; // Pixel width of the text, within the rendered image
        uint32_t textPixelHeight{0}; // Pixel height of the text, within the rendered image
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_TEXT_RENDEREDTEXT_H
