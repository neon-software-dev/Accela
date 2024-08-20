/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SDLUtil.h"

namespace Accela::Platform
{

SDL_Color SDLUtil::ToSDLColor(const Color& color)
{
    SDL_Color sdlColor{};
    sdlColor.r = color.r;
    sdlColor.g = color.g;
    sdlColor.b = color.b;
    sdlColor.a = color.a;

    return sdlColor;
}

Common::ImageData::Ptr SDLUtil::SDLSurfaceToImageData(const Common::ILogger::Ptr& logger, SDL_Surface *pSurface)
{
    SDL_Surface* pFormattedSurface = nullptr;
    bool surfaceConverted = false;

    // Lock the provided surface to read its data
    SDL_LockSurface(pSurface);

    // Check if we need to convert the surface to a different format or if it can be used as-is
    if (pSurface->format->format == SDL_PIXELFORMAT_RGBA32)
    {
        // Surface is already in a good format
        pFormattedSurface = pSurface;
    }
    else
    {
        // Convert the surface to RGBA32 as that's what the Renderer wants for textures
        pFormattedSurface = SDL_ConvertSurfaceFormat(pSurface, SDL_PIXELFORMAT_RGBA32, 0);
        surfaceConverted = true;

        // Unlock the old surface as we're not using it any longer
        SDL_UnlockSurface(pSurface);

        if (pFormattedSurface == nullptr)
        {
            logger->Log(Common::LogLevel::Error,
                "SDLSurfaceToImageData: Surface could not be converted to a supported pixel format");
            return nullptr;
        }

        // Lock the new surface for reading its pixels
        SDL_LockSurface(pFormattedSurface);
    }

    // Byte size of the pixel data
    const uint32_t pixelDataByteSize = pFormattedSurface->w * pFormattedSurface->h * pFormattedSurface->format->BytesPerPixel;

    // Copy the surface's pixel data into a vector
    std::vector<std::byte> imageBytes;
    imageBytes.reserve(pixelDataByteSize);
    imageBytes.insert(imageBytes.end(), (std::byte*)pFormattedSurface->pixels, (std::byte*)pFormattedSurface->pixels + pixelDataByteSize);

    Common::ImageData::Ptr loadResult = std::make_shared<Common::ImageData>(
        imageBytes,
        1,
        static_cast<unsigned int>(pFormattedSurface->w),
        static_cast<unsigned int>(pFormattedSurface->h),
        Common::ImageData::PixelFormat::RGBA32
    );

    // Unlock the surface
    SDL_UnlockSurface(pFormattedSurface);

    // If we had to convert the surface format, free the converted surface we made
    if (surfaceConverted)
    {
        SDL_FreeSurface(pFormattedSurface);
    }

    return loadResult;
}

SDL_Surface * SDLUtil::ResizeToPow2Dimensions(const Common::ILogger::Ptr& logger, SDL_Surface *pSurface, SDL_Color fillColor)
{
    SDL_LockSurface(pSurface);

    const auto nextPow2Width = static_cast<unsigned int>(pow(2, ceil(log(pSurface->w)/log(2))));
    const auto nextPow2Height = static_cast<unsigned int>(pow(2, ceil(log(pSurface->h)/log(2))));

    Uint32 rmask, gmask, bmask, amask;

    // SDL interprets each pixel as a 32-bit number, so our masks must depend on the endianness of the machine
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        rmask = 0xff000000;
        gmask = 0x00ff0000;
        bmask = 0x0000ff00;
        amask = 0x000000ff;
    #else
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = 0xff000000;
    #endif

    // Create a new RGBA surface to hold the resized image
    SDL_Surface* pResultSurface = SDL_CreateRGBSurface(
        pSurface->flags,
        (int)nextPow2Width,
        (int)nextPow2Height,
        32,
        rmask,
        gmask,
        bmask,
        amask);

    if (pResultSurface == nullptr)
    {
        logger->Log(Common::LogLevel::Error, "ResizeToPow2Dimensions: Failed to create a new surface, error: {}", SDL_GetError());
        SDL_UnlockSurface(pSurface);
        return nullptr;
    }

    // Fill the newly created surface with a solid color
    SDL_Rect fillRect;
    fillRect.x = 0;
    fillRect.y = 0;
    fillRect.w = pResultSurface->w;
    fillRect.h = pResultSurface->h;

    SDL_FillRect(pResultSurface, &fillRect, SDL_MapRGBA(pResultSurface->format, fillColor.r, fillColor.g, fillColor.b, fillColor.a));

    // Copy the (smaller or equal) source surface to the top left corner of the new result surface
    SDL_Rect targetRect;
    targetRect.x = 0;
    targetRect.y = 0;
    targetRect.w = pSurface->w;
    targetRect.h = pSurface->h;

    SDL_UnlockSurface(pSurface);

    if (SDL_BlitSurface(pSurface, nullptr, pResultSurface, &targetRect) < 0)
    {
        logger->Log(Common::LogLevel::Error, "ResizeToPow2Dimensions: Failed to blit surface, error: {}", SDL_GetError());
        return nullptr;
    }

    return pResultSurface;
}

}
