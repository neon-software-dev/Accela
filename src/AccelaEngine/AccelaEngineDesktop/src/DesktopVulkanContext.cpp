/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/DesktopVulkanContext.h>

#include <vector>

namespace Accela::Engine
{

DesktopVulkanContext::DesktopVulkanContext(Platform::IPlatform::Ptr platform)
    : m_platform(std::move(platform))
{

}

std::vector<std::string> ExtensionBytesToVec(const std::vector<char>& extensionsBytes)
{
    std::vector<std::string> extensions;

    std::string curExtStr;

    for (const char& c : extensionsBytes)
    {
        if (c == ' ')
        {
            extensions.push_back(curExtStr);
            curExtStr.clear();
        }
        else
        {
            curExtStr += c;
        }
    }

    if (!curExtStr.empty())
    {
        extensions.push_back(curExtStr);
    }

    return extensions;
}

bool DesktopVulkanContext::GetRequiredInstanceExtensions(std::set<std::string>& extensions) const
{
    //
    // Get the extensions that SDL reports are required for it to create a Vulkan surface
    //
    std::vector<std::string> windowExtensions;

    if (!m_platform->GetWindow()->GetVulkanRequiredExtensions(windowExtensions))
    {
        return false;
    }

    //
    // Create list of all required extensions
    //
    for (const auto& extension : windowExtensions) { extensions.insert(extension); }

    return true;
}

bool DesktopVulkanContext::GetRequiredDeviceExtensions(VkPhysicalDevice, std::set<std::string>& extensions) const
{
    // Require swap chain extension for presenting to surface
    extensions.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    return true;
}

bool DesktopVulkanContext::CreateVulkanSurface(VkInstance instance, VkSurfaceKHR *pSurface) const
{
    return m_platform->GetWindow()->CreateVulkanSurface(instance, pSurface);
}

bool DesktopVulkanContext::GetSurfacePixelSize(std::pair<unsigned int, unsigned int>& size) const
{
    const auto sizeExpect = m_platform->GetWindow()->GetWindowSize();
    if (!sizeExpect)
    {
        return false;
    }

    size = *sizeExpect;

    return true;
}

}
