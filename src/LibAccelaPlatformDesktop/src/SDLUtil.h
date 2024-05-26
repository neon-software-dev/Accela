/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMSDL_SRC_SDLUTIL_H
#define LIBACCELAPLATFORMSDL_SRC_SDLUTIL_H

#include <Accela/Platform/Color.h>

#include <Accela/Common/ImageData.h>
#include <Accela/Common/Log/ILogger.h>

#include <SDL2/SDL_surface.h>

namespace Accela::Platform
{
    class SDLUtil
    {
        public:

            [[nodiscard]] static SDL_Color ToSDLColor(const Color& color);

            /**
            * Converts an SDL_Surface to an internal RGBA32 ImageData object.
            *
            * @param pSurface - The SDL surface to be converted.
            */
            [[nodiscard]] static Common::ImageData::Ptr SDLSurfaceToImageData(SDL_Surface *pSurface);

            /**
             * Returns a new surface which contains the supplied surface's pixels but with the surface's
             * dimensions either left the same or adjusted upwards to be a power of two. Does not necessarily
             * return a square surface, only a surface with power of two dimensions.
             *
             * For example, a 110x512 image would be converted to 128x512.
             *
             * The source surface is left unmodified.
             *
             * The result surface is in an RGBA32 format where the extra space that doesn't contain the old
             * surface's pixels is filled with the specified fill pixel color.
             *
             * @param logger Logger to receive any error messages
             * @param pSurface The surface to be resized.
             * @param fillColor The color to fill the background of the new surface with
             *
             * @return A new surface, with power of two dimensions, containing the supplied
             * surface's pixel data, or null on error.
             */
            [[nodiscard]] static SDL_Surface* ResizeToPow2Dimensions(const Common::ILogger::Ptr& logger, SDL_Surface *pSurface, SDL_Color fillColor);
    };
}

#endif //LIBACCELAPLATFORMSDL_SRC_SDLUTIL_H
