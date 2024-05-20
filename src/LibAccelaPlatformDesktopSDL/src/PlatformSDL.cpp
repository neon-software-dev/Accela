/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Platform/PlatformSDL.h>
#include <Accela/Platform/Window/SDLWindow.h>

#include "Event/SDLEvents.h"

namespace Accela::Platform
{

PlatformSDL::PlatformSDL(Common::ILogger::Ptr logger)
    : PlatformDesktop(std::move(logger))
    , m_events(std::make_shared<SDLEvents>())
    , m_window(std::make_shared<SDLWindow>(m_logger))
{

}

bool PlatformSDL::Startup()
{
    if (!PlatformDesktop::Startup()) { return false; }

    m_logger->Log(Common::LogLevel::Info, "PlatformSDL: Starting");

    return true;
}

void PlatformSDL::Shutdown()
{
    m_logger->Log(Common::LogLevel::Info, "PlatformSDL: Shutting down");

    std::dynamic_pointer_cast<SDLWindow>(m_window)->Destroy();

    PlatformDesktop::Shutdown();
}

IEvents::Ptr PlatformSDL::GetEvents() const noexcept { return m_events; }
IWindow::Ptr PlatformSDL::GetWindow() const noexcept { return m_window; }

}
