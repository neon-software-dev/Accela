/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SwapChainSupportDetails.h"

#include <Accela/Render/IVulkanCalls.h>

namespace Accela::Render
{


SwapChainSupportDetails SwapChainSupportDetails::Load(const IVulkanCallsPtr& vulkanCalls,
                                                      const VkPhysicalDevice& physicalDevice,
                                                      const VkSurfaceKHR& surface)
{
    SwapChainSupportDetails details;

    //
    // Query surface capabilities
    //
    vulkanCalls->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    const uint32_t width = details.capabilities.currentExtent.width;
    const uint32_t height = details.capabilities.currentExtent.height;

    // Important for android devices where rotation changes the transform value. When going into
    // landscape mode we have to manually swap the extent dimension if the surface is transformed.
    if (details.capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
        details.capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
        details.capabilities.currentExtent.height = width;
        details.capabilities.currentExtent.width = height;
    }

    //
    // Query surface formats
    //
    uint32_t formatCount = 0;
    vulkanCalls->vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vulkanCalls->vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
    }

    //
    // Query present modes
    //
    uint32_t presentModeCount = 0;
    vulkanCalls->vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vulkanCalls->vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

}
