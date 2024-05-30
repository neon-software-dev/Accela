/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Platform/Text/SDLText.h>

#include "../SDLUtil.h"

#include <algorithm>

namespace Accela::Platform
{

SDLText::SDLText(Common::ILogger::Ptr logger)
    : m_logger(std::move(logger))
{

}

void SDLText::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "SDLText: Destroying");

    while (!m_fonts.empty())
    {
        UnloadFont(m_fonts.cbegin()->first);
    }
}

bool SDLText::LoadFontBlocking(const std::string& fontFileName, const std::vector<std::byte>& fontData, uint8_t fontSize)
{
    if (IsFontLoaded(fontFileName, fontSize))
    {
        m_logger->Log(Common::LogLevel::Debug, "LoadFont: Font {}x{} already loaded", fontFileName, fontSize);
        return true;
    }

    m_logger->Log(Common::LogLevel::Info, "LoadFont: Loading font: {}x{}", fontFileName, fontSize);

    auto loadedFont = std::make_shared<LoadedFont>();
    loadedFont->fontData = fontData;

    // Needed to make a persistent heap copy of font data in memory for this RWops, as the font data needs to stay
    // alive until the TTF_Font that's created is closed
    const auto pRwOps = SDL_RWFromConstMem((void*)loadedFont->fontData.data(), (int)loadedFont->fontData.size());

    loadedFont->pFont = TTF_OpenFontRW(pRwOps, 1, fontSize);
    if (loadedFont->pFont == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadFont: TTF_OpenFont failed for font: {}x{}", fontFileName, fontSize);
        return false;
    }

    {
        std::lock_guard<std::recursive_mutex> lock(m_fontsMutex);

        if (IsFontLoaded(fontFileName, fontSize))
        {
            TTF_CloseFont(loadedFont->pFont);
            return true;
        }

        auto it = m_fonts.find(fontFileName);
        if (it == m_fonts.cend())
        {
            it = m_fonts.insert({fontFileName, {}}).first;
        }

        it->second.insert({fontSize, loadedFont});
    }

    return true;
}

bool SDLText::IsFontLoaded(const std::string& fontFileName, uint8_t fontSize)
{
    const auto it = m_fonts.find(fontFileName);
    if (it == m_fonts.cend())
    {
        return false;
    }

    return it->second.find(fontSize) != it->second.cend();
}

void SDLText::UnloadFont(const std::string& fontFileName)
{
    std::lock_guard<std::recursive_mutex> lock(m_fontsMutex);

    const auto it = m_fonts.find(fontFileName);

    std::vector<uint8_t> fontSizes;

    for (const auto& fontIt : it->second)
    {
        fontSizes.push_back(fontIt.first);
    }

    for (const auto& fontSize : fontSizes)
    {
        UnloadFont(fontFileName, fontSize);
    }
}

void SDLText::UnloadFont(const std::string& fontFileName, uint8_t fontSize)
{
    std::lock_guard<std::recursive_mutex> lock(m_fontsMutex);

    const auto it = m_fonts.find(fontFileName);
    if (it == m_fonts.cend())
    {
        m_logger->Log(Common::LogLevel::Debug, "UnloadFont: Font {} not loaded", fontFileName);
        return;
    }

    const auto sizeIt = it->second.find(fontSize);
    if (sizeIt == it->second.cend())
    {
        m_logger->Log(Common::LogLevel::Debug, "UnloadFont: Font {}x{} not loaded", fontFileName, fontSize);
        return;
    }

    m_logger->Log(Common::LogLevel::Info, "UnloadFont: Unloading font: {}x{}", fontFileName, fontSize);
    TTF_CloseFont(sizeIt->second->pFont);

    it->second.erase(sizeIt);

    if (it->second.empty())
    {
        m_fonts.erase(it);
    }
}

void SDLText::UnloadAllFonts()
{
    std::lock_guard<std::recursive_mutex> lock(m_fontsMutex);

    while (!m_fonts.empty())
    {
        UnloadFont(m_fonts.cbegin()->first);
    }
}

std::expected<RenderedText, bool>
SDLText::RenderText(const std::string& text, const TextProperties& properties) const
{
    const SDL_Color sdlFgColor = SDLUtil::ToSDLColor(properties.fgColor);
    const SDL_Color sdlBgColor = SDLUtil::ToSDLColor(properties.bgColor);

    //
    // Fetch the font
    //
    const auto font = GetLoadedFont(properties.fontFileName, properties.fontSize);
    if (font == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RenderText: Font not loaded: {}x{}", properties.fontFileName, properties.fontSize);
        return std::unexpected(false);
    }

    //
    // Render the text
    //
    SDL_Surface *pSurface;

    if (properties.wrapLength == 0)
    {
        pSurface = TTF_RenderUTF8_Blended(font, text.c_str(), sdlFgColor);
    }
    else
    {
        pSurface = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), sdlFgColor, properties.wrapLength);
    }

    if (pSurface == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "RenderText: Failed to render text, error: {}", TTF_GetError());
        return std::unexpected(false);
    }

    //
    // Record relevant data about the rendered text before we modify the surface for use as a texture
    //
    RenderedText renderedText{};
    renderedText.textPixelWidth = pSurface->w;
    renderedText.textPixelHeight = pSurface->h;

    //
    // Resize the surface so that it can be used as a texture
    //
    SDL_Surface *pResizedSurface = SDLUtil::ResizeToPow2Dimensions(m_logger, pSurface, sdlBgColor);
    SDL_FreeSurface(pSurface);

    if (pResizedSurface == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "RenderText: Failed to resize surface to power of two");
        return std::unexpected(false);
    }

    renderedText.imageData = SDLUtil::SDLSurfaceToImageData(pResizedSurface);
    SDL_FreeSurface(pResizedSurface);

    return renderedText;
}

TTF_Font* SDLText::GetLoadedFont(const std::string& fontFileName, uint8_t fontSize) const
{
    std::lock_guard<std::recursive_mutex> lock(m_fontsMutex);

    const auto it = m_fonts.find(fontFileName);
    if (it == m_fonts.cend()) { return nullptr; }

    const auto sizeIt = it->second.find(fontSize);
    if (sizeIt == it->second.cend()) { return nullptr; }

    return sizeIt->second->pFont;
}

}
