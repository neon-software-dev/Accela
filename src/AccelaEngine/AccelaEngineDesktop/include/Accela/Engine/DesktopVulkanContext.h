/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINEDESKTOP_INCLUDE_ACCELA_ENGINE_DESKTOPVULKANCONTEXT_H
#define LIBACCELAENGINEDESKTOP_INCLUDE_ACCELA_ENGINE_DESKTOPVULKANCONTEXT_H

#include <Accela/Platform/IPlatform.h>

#include <Accela/Render/IVulkanContext.h>

#include <Accela/Common/SharedLib.h>

namespace Accela::Engine
{
    class ACCELA_PUBLIC DesktopVulkanContext : public Render::IVulkanContext
    {
        public:

            explicit DesktopVulkanContext(Platform::IPlatform::Ptr platform);

            //
            // IVulkanContext
            //
            [[nodiscard]] bool GetRequiredInstanceExtensions(std::set<std::string>& extensions) const override;
            [[nodiscard]] bool GetRequiredDeviceExtensions(VkPhysicalDevice vkPhysicalDevice, std::set<std::string>& extensions) const override;
            [[nodiscard]] bool CreateVulkanSurface(VkInstance instance, VkSurfaceKHR* pSurface) const override;
            [[nodiscard]] bool GetSurfacePixelSize(std::pair<unsigned int, unsigned int>& size) const override;

        private:

            Platform::IPlatform::Ptr m_platform;
    };
}

#endif //LIBACCELAENGINEDESKTOP_INCLUDE_ACCELA_ENGINE_DESKTOPVULKANCONTEXT_H
