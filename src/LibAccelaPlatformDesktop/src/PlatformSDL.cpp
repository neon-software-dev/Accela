/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#include <Accela/Platform/PlatformSDL.h>
#include <Accela/Platform/SDLWindow.h>

#include "Event/SDLEvents.h"
#include "File/SDLFiles.h"
#include "Text/SDLText.h"
#include "VR/OpenVR.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

namespace Accela::Platform
{

PlatformSDL::PlatformSDL(Common::ILogger::Ptr logger)
    : m_logger(std::move(logger))
    , m_events(std::make_shared<SDLEvents>())
    , m_files(std::make_shared<SDLFiles>(m_logger))
    , m_text(std::make_shared<SDLText>(m_logger, m_files))
    , m_window(std::make_shared<SDLWindow>(m_logger))
    , m_vr(std::make_shared<OpenVR>(m_logger))
{

}

bool PlatformSDL::Startup() noexcept
{
    m_logger->Log(Common::LogLevel::Info, "PlatformSDL: Starting");

    auto result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    if (result != 0)
    {
        m_logger->Log(Common::LogLevel::Fatal, "PlatformSDL: SDL_Init failed, error: {}", SDL_GetError());
        return false;
    }

    result = IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
    if (result == 0)
    {
        m_logger->Log(Common::LogLevel::Fatal, "PlatformSDL: IMG_Init failed, error: {}", IMG_GetError());
        return false;
    }

    result = TTF_Init();
    if (result != 0)
    {
        m_logger->Log(Common::LogLevel::Fatal, "PlatformSDL: TTF_Init failed, error: {}", TTF_GetError());
        return false;
    }

    return true;
}

void PlatformSDL::Shutdown() noexcept
{
    m_logger->Log(Common::LogLevel::Info, "PlatformSDL: Shutting down");

    m_text->Destroy();
    std::dynamic_pointer_cast<SDLWindow>(m_window)->Destroy();

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
}

IEvents::Ptr PlatformSDL::GetEvents() const noexcept { return m_events; }
IFiles::Ptr PlatformSDL::GetFiles() const noexcept { return m_files; }
IText::Ptr PlatformSDL::GetText() const noexcept { return m_text; }
IWindow::Ptr PlatformSDL::GetWindow() const noexcept { return m_window; }
IVR::Ptr PlatformSDL::GetVR() const noexcept{ return m_vr; }

}
