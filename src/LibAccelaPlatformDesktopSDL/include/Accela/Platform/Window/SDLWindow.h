/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMSDL_SRC_WINDOW_SDLWINDOW_H
#define LIBACCELAPLATFORMSDL_SRC_WINDOW_SDLWINDOW_H

#include <Accela/Platform/Window/IWindow.h>

#include <Accela/Common/Log/ILogger.h>

#include <SDL2/SDL_video.h>

#include <vulkan/vulkan.h>

#include <vector>

namespace Accela::Platform
{
    class SDLWindow : public IWindow
    {
        public:

            using Ptr = std::shared_ptr<SDLWindow>;

        public:

            explicit SDLWindow(Common::ILogger::Ptr logger);

            [[nodiscard]] std::expected<std::pair<unsigned int, unsigned int>, bool> GetWindowSize() const override;
            [[nodiscard]] std::expected<std::pair<unsigned int, unsigned int>, bool> GetWindowDisplaySize() const override;
            [[nodiscard]] bool LockCursorToWindow(bool lock) const override;
            [[nodiscard]] bool SetFullscreen(bool fullscreen) const override;
            [[nodiscard]] bool SetWindowSize(const std::pair<unsigned int, unsigned int>& size) const override;
            [[nodiscard]] bool GetVulkanRequiredExtensions(std::vector<std::string>& extensions) const override;
            [[nodiscard]] bool CreateVulkanSurface(VkInstance instance, VkSurfaceKHR *pSurface) const override;

            void Destroy();

            SDL_Window* CreateWindow(const std::string& title, unsigned int width, unsigned int height) noexcept;

        private:

            Common::ILogger::Ptr m_logger;

            SDL_Window* m_pWindow{nullptr};
    };
}

#endif //LIBACCELAPLATFORMSDL_SRC_WINDOW_SDLWINDOW_H
