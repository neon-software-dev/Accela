/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Platform/SDLWindow.h>

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_mouse.h>

namespace Accela::Platform
{

SDLWindow::SDLWindow(Common::ILogger::Ptr logger)
    : m_logger(std::move(logger))
{

}

SDL_Window* SDLWindow::CreateWindow(const std::string& title, unsigned int width, unsigned int height) noexcept
{
    m_pWindow = SDL_CreateWindow(
            title.c_str(),
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            (int)width,
            (int)height,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN
    );

    if (m_pWindow == nullptr)
    {
        m_logger->Log(Common::LogLevel::Fatal, "PlatformSDL: SDL_CreateWindow failed, error: {}", SDL_GetError());
        return nullptr;
    }

    //SDL_SetRelativeMouseMode(SDL_TRUE);

    return m_pWindow;
}

bool SDLWindow::GetVulkanRequiredExtensions(std::vector<std::string>& extensions) const
{
    extensions.clear();

    if (m_pWindow == nullptr)
    {
        m_logger->Log(Common::LogLevel::Fatal, "GetVulkanRequiredExtensions: No active window");
        return false;
    }

    unsigned int extensionsCount{0};
    if (!SDL_Vulkan_GetInstanceExtensions(m_pWindow, &extensionsCount, nullptr))
    {
        m_logger->Log(Common::LogLevel::Fatal, "GetVulkanRequiredExtensions: SDL_Vulkan_GetInstanceExtensions call failed");
        return false;
    }

    std::vector<const char*> extensionsRaw;
    extensionsRaw.resize(extensionsCount);

    if (!SDL_Vulkan_GetInstanceExtensions(m_pWindow, &extensionsCount, extensionsRaw.data()))
    {
        m_logger->Log(Common::LogLevel::Fatal, "GetVulkanRequiredExtensions: SDL_Vulkan_GetInstanceExtensions 2nd call failed");
        return false;
    }

    for (const auto& extensionRaw : extensionsRaw)
    {
        extensions.emplace_back(extensionRaw);
    }

    return true;
}

bool SDLWindow::CreateVulkanSurface(VkInstance instance, VkSurfaceKHR *pSurface) const noexcept
{
    if (m_pWindow == nullptr)
    {
        m_logger->Log(Common::LogLevel::Fatal, "CreateVulkanSurface: No active window");
        return false;
    }

    return SDL_Vulkan_CreateSurface(m_pWindow, instance, pSurface);
}

std::expected<std::pair<unsigned int, unsigned int>, bool> SDLWindow::GetWindowSize() const
{
    if (m_pWindow == nullptr)
    {
        m_logger->Log(Common::LogLevel::Fatal, "GetWindowSize: No active window");
        return std::unexpected(false);
    }

    int width{0};
    int height{0};

    SDL_Vulkan_GetDrawableSize(m_pWindow, &width, &height);

    return std::make_pair(width, height);
}

void SDLWindow::LockCursorToWindow(bool lock) const
{
    if (m_pWindow == nullptr)
    {
        m_logger->Log(Common::LogLevel::Fatal, "LockCursorToWindow: No active window");
        return;
    }

    SDL_SetRelativeMouseMode((SDL_bool)lock);
}

void SDLWindow::Destroy()
{
    if (m_pWindow != nullptr)
    {
        SDL_DestroyWindow(m_pWindow);
        m_pWindow = nullptr;
    }
}

void SDLWindow::SetFullscreen(bool fullscreen) const
{
    if (m_pWindow == nullptr)
    {
        m_logger->Log(Common::LogLevel::Fatal, "SetFullscreen: No active window");
        return;
    }

    uint32_t flags = SDL_GetWindowFlags(m_pWindow);

    if (fullscreen)
    {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    else
    {
        flags &= ~SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    SDL_SetWindowFullscreen(m_pWindow, flags);
}

}
