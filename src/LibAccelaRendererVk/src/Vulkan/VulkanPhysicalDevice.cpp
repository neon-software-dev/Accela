/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanPhysicalDevice.h"
#include "SwapChainSupportDetails.h"
#include "VulkanSurface.h"
#include "VulkanInstance.h"

#include <Accela/Render/IVulkanCalls.h>
#include <Accela/Render/IVulkanContext.h>

#include <cstring>
#include <algorithm>

namespace Accela::Render
{

VulkanPhysicalDevice::VulkanPhysicalDevice(Common::ILogger::Ptr logger,
                                           IVulkanCallsPtr vulkanCalls,
                                           IVulkanContextPtr vulkanContext,
                                           VkPhysicalDevice vkPhysicalDevice)
    : m_logger(std::move(logger))
    , m_vulkanCalls(std::move(vulkanCalls))
    , m_vulkanContext(std::move(vulkanContext))
    , m_vkPhysicalDevice(vkPhysicalDevice)
{
    // Query for device properties and features
    m_vulkanCalls->vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &m_vkPhysicalDeviceProperties);
    m_vulkanCalls->vkGetPhysicalDeviceFeatures(m_vkPhysicalDevice, &m_vkPhysicalDeviceFeatures);

    // Query for queue family information
    uint32_t queueFamilyCount = 0;
    m_vulkanCalls->vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, nullptr);
    m_vkQueueFamilyProperties.resize(queueFamilyCount);
    m_vulkanCalls->vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, m_vkQueueFamilyProperties.data());

    // Query for supported extensions
    uint32_t extensionCount = 0;
    m_vulkanCalls->vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, nullptr, &extensionCount, nullptr);
    m_vkExtensionProperties.resize(extensionCount);
    m_vulkanCalls->vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, nullptr, &extensionCount, m_vkExtensionProperties.data());
}

std::vector<VulkanPhysicalDevicePtr> VulkanPhysicalDevice::EnumerateAll(const Common::ILogger::Ptr& logger,
                                                                        const IVulkanCallsPtr& vulkanCalls,
                                                                        const IVulkanContextPtr& vulkanContext,
                                                                        const VulkanInstancePtr& instance)
{
    uint32_t deviceCount = 0;
    vulkanCalls->vkEnumeratePhysicalDevices(instance->GetVkInstance(), &deviceCount, nullptr);

    if (deviceCount == 0) { return {}; }

    std::vector<VkPhysicalDevice> vkPhysicalDevices(deviceCount);
    vulkanCalls->vkEnumeratePhysicalDevices(instance->GetVkInstance(), &deviceCount, vkPhysicalDevices.data());

    std::vector<VulkanPhysicalDevicePtr> physicalDevices(deviceCount);

    std::ranges::transform(vkPhysicalDevices, physicalDevices.begin(),
       [&](const auto& vkPhysicalDevice){
            return std::make_shared<VulkanPhysicalDevice>(logger, vulkanCalls, vulkanContext, vkPhysicalDevice);
    });

    return physicalDevices;
}

bool VulkanPhysicalDevice::IsDeviceSuitable(const VulkanSurfacePtr& surface) const
{
    // Only allow running on discrete or integrated GPUs for the moment, no cpu/virtualization
    bool suitableType =
        m_vkPhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
        m_vkPhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

    if (!suitableType)
    {
        m_logger->Log(Common::LogLevel::Info,
          "Rejecting device due to unsuitable device type: {}", m_vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    // Device must support graphics operations
    if (!GetGraphicsQueueFamilyIndex().has_value())
    {
        m_logger->Log(Common::LogLevel::Info,
          "Rejecting device due to no graphics-capable queue family: {}", m_vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    // Device must support present operations for the specified surface
    if (!GetPresentQueueFamilyIndex(surface).has_value())
    {
        m_logger->Log(Common::LogLevel::Info,
          "Rejecting device due to no present-capable queue family: {}", m_vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    // Device must support the required extensions
    std::set<std::string> requiredExtensions;
    if (!m_vulkanContext->GetRequiredDeviceExtensions(m_vkPhysicalDevice, requiredExtensions))
    {
        m_logger->Log(Common::LogLevel::Error,
          "Rejecting device as we failed to fetch required device extensions: {}", m_vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    // Device must support independent blend (to support deferred lighting renderer's non-blended material id attachment)
    if (!m_vkPhysicalDeviceFeatures.independentBlend)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Rejecting device as it doesn't support independentBlend feature: {}", m_vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    // Device must support the multiview extension
    requiredExtensions.insert(VK_KHR_MULTIVIEW_EXTENSION_NAME);

    for (const auto& extension : requiredExtensions)
    {
        if (!SupportsExtension(extension))
        {
            m_logger->Log(Common::LogLevel::Info,
              "Rejecting device: {} due to missing required extension: {}",
              m_vkPhysicalDeviceProperties.deviceName,
              extension
            );
            return false;
        }
    }

    // Swap chain capabilities must be sufficient
    const SwapChainSupportDetails swapChainSupportDetails = SwapChainSupportDetails::Load(
        m_vulkanCalls,
        m_vkPhysicalDevice,
        surface->GetVkSurface()
    );

    const auto swapChainAdequate = !swapChainSupportDetails.formats.empty() && !swapChainSupportDetails.presentModes.empty();
    if (!swapChainAdequate)
    {
        m_logger->Log(Common::LogLevel::Info,
            "Rejecting device due to insufficient swap chain: {}", m_vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    // Device must support tesselation shaders
    if (!m_vkPhysicalDeviceFeatures.tessellationShader)
    {
        m_logger->Log(Common::LogLevel::Info,
          "Rejecting device due to missing tesselation shader support: {}", m_vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    return true;
}

uint32_t VulkanPhysicalDevice::GetDeviceRating() const
{
    uint32_t rating = 0;

    //
    // Discrete GPUs rank above Integrated GPUs
    //
    if (m_vkPhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        rating += 1000;
    }

    m_logger->Log(Common::LogLevel::Info,
      "Rating of {} for device: {}", rating, m_vkPhysicalDeviceProperties.deviceName);

    return rating;
}

std::string VulkanPhysicalDevice::GetDeviceName() const
{
    return m_vkPhysicalDeviceProperties.deviceName;
}

std::optional<uint32_t> VulkanPhysicalDevice::GetGraphicsQueueFamilyIndex() const
{
    for (uint32_t x = 0; x < m_vkQueueFamilyProperties.size(); ++x)
    {
        if (m_vkQueueFamilyProperties[x].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            return x;
        }
    }

    return std::nullopt;
}

std::optional<uint32_t> VulkanPhysicalDevice::GetPresentQueueFamilyIndex(const VulkanSurfacePtr& surface) const
{
    for (uint32_t x = 0; x < m_vkQueueFamilyProperties.size(); ++x)
    {
        VkBool32 presentSupport = false;
        m_vulkanCalls->vkGetPhysicalDeviceSurfaceSupportKHR(m_vkPhysicalDevice, x, surface->GetVkSurface(), &presentSupport);

        if (presentSupport == VK_TRUE)
        {
            return x;
        }
    }

    return std::nullopt;
}

bool VulkanPhysicalDevice::SupportsExtension(const std::string& extensionName) const
{
    return std::any_of(m_vkExtensionProperties.begin(), m_vkExtensionProperties.end(), [&](const auto& extension) {
       return strcmp(extension.extensionName, extensionName.c_str()) == 0;
    });
}

}
