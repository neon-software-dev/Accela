/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORMSDL_SRC_WINDOW_SDLWINDOW_H
#define LIBACCELAPLATFORMSDL_SRC_WINDOW_SDLWINDOW_H

#include <Accela/Platform/Window/IWindow.h>

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <vector>

class SDL_Window;

namespace Accela::Platform
{
    class SDLWindow : public IWindow
    {
        public:

            using Ptr = std::shared_ptr<SDLWindow>;

        public:

            explicit SDLWindow(Common::ILogger::Ptr logger);

            [[nodiscard]] std::expected<std::pair<unsigned int, unsigned int>, bool> GetWindowSize() const override;
            void LockCursorToWindow(bool lock) const override;
            void SetFullscreen(bool fullscreen) const override;

            void Destroy();

            SDL_Window* CreateWindow(const std::string& title, unsigned int width, unsigned int height) noexcept;
            bool GetVulkanRequiredExtensions(std::vector<std::string>& extensions) const;
            bool CreateVulkanSurface(VkInstance instance, VkSurfaceKHR *pSurface) const noexcept;

        private:

            Common::ILogger::Ptr m_logger;

            SDL_Window* m_pWindow{nullptr};
    };
}

#endif //LIBACCELAPLATFORMSDL_SRC_WINDOW_SDLWINDOW_H
