/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Platform/PlatformDesktop.h>
#include <Accela/Platform/File/SDLFiles.h>
#include <Accela/Platform/Text/SDLText.h>
#include <Accela/Platform/VR/OpenVR.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

namespace Accela::Platform
{

PlatformDesktop::PlatformDesktop(Common::ILogger::Ptr logger)
    : m_logger(std::move(logger))
    , m_files(std::make_shared<SDLFiles>(m_logger))
    , m_text(std::make_shared<SDLText>(m_logger, m_files))
    , m_vr(std::make_shared<OpenVR>(m_logger))
{

}

bool PlatformDesktop::Startup()
{
    m_logger->Log(Common::LogLevel::Info, "PlatformDesktop: Starting");

    auto result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if (result != 0)
    {
        m_logger->Log(Common::LogLevel::Fatal, "PlatformDesktop: SDL_Init failed, error: {}", SDL_GetError());
        return false;
    }

    result = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
    if (result == 0)
    {
        m_logger->Log(Common::LogLevel::Fatal, "PlatformDesktop: IMG_Init failed, error: {}", IMG_GetError());
        return false;
    }

    result = TTF_Init();
    if (result != 0)
    {
        m_logger->Log(Common::LogLevel::Fatal, "PlatformDesktop: TTF_Init failed, error: {}", TTF_GetError());
        return false;
    }

    return true;
}

void PlatformDesktop::Shutdown()
{
    m_logger->Log(Common::LogLevel::Info, "PlatformDesktop: Shutting down");

    m_text->Destroy();

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

IFiles::Ptr PlatformDesktop::GetFiles() const noexcept { return m_files; }
IText::Ptr PlatformDesktop::GetText() const noexcept { return m_text; }
IVR::Ptr PlatformDesktop::GetVR() const noexcept{ return m_vr; }

}
