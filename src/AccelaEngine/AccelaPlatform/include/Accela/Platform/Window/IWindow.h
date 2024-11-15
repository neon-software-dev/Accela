/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_WINDOW_IWINDOW_H
#define LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_WINDOW_IWINDOW_H

#include <Accela/Common/SharedLib.h>

#include <memory>
#include <utility>
#include <expected>
#include <string>
#include <vector>

namespace Accela::Platform
{
    /**
     * Interface to OS windowing functionality
     */
    class ACCELA_PUBLIC IWindow
    {
        public:

            using Ptr = std::shared_ptr<IWindow>;

        public:

            virtual ~IWindow() = default;

            [[nodiscard]] virtual std::expected<std::pair<unsigned int, unsigned int>, bool> GetWindowSize() const = 0;
            [[nodiscard]] virtual std::expected<std::pair<unsigned int, unsigned int>, bool> GetWindowDisplaySize() const = 0;
            [[nodiscard]] virtual bool LockCursorToWindow(bool lock) const = 0;
            [[nodiscard]] virtual bool SetFullscreen(bool fullscreen) const = 0;
            [[nodiscard]] virtual bool SetWindowSize(const std::pair<unsigned int, unsigned int>& size) const = 0;
            [[nodiscard]] virtual bool GetVulkanRequiredExtensions(std::vector<std::string>& extensions) const = 0;
            [[nodiscard]] virtual bool CreateVulkanSurface(void* pVkInstance, void* pVkSurface) const = 0;
    };
}

#endif //LIBACCELAPLATFORM_INCLUDE_ACCELA_PLATFORM_WINDOW_IWINDOW_H
