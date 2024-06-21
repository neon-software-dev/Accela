/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanDevice.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanSurface.h"

#include <Accela/Render/IVulkanCalls.h>
#include <Accela/Render/IVulkanContext.h>

#include <set>
#include <algorithm>
#include <functional>

namespace Accela::Render
{

VulkanDevice::VulkanDevice(Common::ILogger::Ptr logger, IVulkanCallsPtr vulkanCalls, IVulkanContextPtr vulkanContext)
    : m_logger(std::move(logger))
    , m_vulkanCalls(std::move(vulkanCalls))
    , m_vulkanContext(std::move(vulkanContext))
{

}

bool VulkanDevice::Create(const VulkanPhysicalDevicePtr& physicalDevice, const VulkanSurfacePtr& surface)
{
    // From checks in VulkanPhysicalDevice we're guaranteed that the provided physical device
    // has support for graphics, present, and compute queues, and the swap chain extension.
    const uint32_t graphicsQueueFamilyIndex = physicalDevice->GetGraphicsQueueFamilyIndex().value();
    const uint32_t presentQueueFamilyIndex = physicalDevice->GetPresentQueueFamilyIndex(surface).value();
    const uint32_t computeQueueFamilyIndex = physicalDevice->GetComputeQueueFamilyIndex().value();

    std::set<uint32_t> uniqueQueueFamilyIndices =  {graphicsQueueFamilyIndex, presentQueueFamilyIndex, computeQueueFamilyIndex};

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    const float queuePriority = 1.0f; // TODO Perf: Tweak?
    for (const uint32_t& queueFamilyIndex : uniqueQueueFamilyIndices)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceMultiviewFeatures vkPhysicalDeviceMultiviewFeatures{};
    vkPhysicalDeviceMultiviewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;
    vkPhysicalDeviceMultiviewFeatures.multiview = VK_TRUE;
    vkPhysicalDeviceMultiviewFeatures.multiviewTessellationShader = VK_TRUE;

    VkPhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = &vkPhysicalDeviceMultiviewFeatures;
    deviceFeatures.features.tessellationShader = VK_TRUE;
    deviceFeatures.features.independentBlend = VK_TRUE;

    if (physicalDevice->GetPhysicalDeviceFeatures().samplerAnisotropy)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanDevice::Create: Enabling samplerAnisotropy feature");
        deviceFeatures.features.samplerAnisotropy = VK_TRUE;
    }

    if (physicalDevice->GetPhysicalDeviceFeatures().fillModeNonSolid)
    {
        m_logger->Log(Common::LogLevel::Info, "VulkanDevice::Create: Enabling fillModeNonSolid feature");
        deviceFeatures.features.fillModeNonSolid = VK_TRUE;
    }

    // Required device extensions
    std::set<std::string> extensions;
    if (!m_vulkanContext->GetRequiredDeviceExtensions(physicalDevice->GetVkPhysicalDevice(), extensions))
    {
        m_logger->Log(Common::LogLevel::Fatal,
          "Failed to fetch device required extensions: {}", physicalDevice->GetDeviceName());
        return false;
    }

    // Use the multiview extension
    extensions.insert(VK_KHR_MULTIVIEW_EXTENSION_NAME);

    std::vector<const char*> extensionsCStrs;
    std::transform(std::begin(extensions), std::end(extensions),
                   std::back_inserter(extensionsCStrs), std::mem_fn(&std::string::c_str));

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = &deviceFeatures;
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = nullptr;
    createInfo.enabledExtensionCount = extensionsCStrs.size();
    createInfo.ppEnabledExtensionNames = extensionsCStrs.data();

    const auto result = m_vulkanCalls->vkCreateDevice(physicalDevice->GetVkPhysicalDevice(), &createInfo, nullptr, &m_vkDevice);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Fatal, "vkCreateDevice call failure, result code: {}", (uint32_t)result);
        return false;
    }

    //
    // Now that a device is created, load the Vulkan device calls
    //
    m_vulkanCalls->InitDeviceCalls(m_vkDevice);

    //
    // Get access to the created Queues
    //
    m_vulkanCalls->vkGetDeviceQueue(m_vkDevice, graphicsQueueFamilyIndex, 0, &m_vkGraphicsQueue);
    m_vulkanCalls->vkGetDeviceQueue(m_vkDevice, presentQueueFamilyIndex, 0, &m_vkPresentQueue);
    m_vulkanCalls->vkGetDeviceQueue(m_vkDevice, computeQueueFamilyIndex, 0, &m_vkComputeQueue);

    return true;
}

void VulkanDevice::Destroy() noexcept
{
    if (m_vkDevice == VK_NULL_HANDLE)
    {
        return;
    }

    m_vulkanCalls->vkDestroyDevice(m_vkDevice, nullptr);

    m_vkDevice = VK_NULL_HANDLE;
    m_vkGraphicsQueue = VK_NULL_HANDLE;
    m_vkPresentQueue = VK_NULL_HANDLE;
    m_vkComputeQueue = VK_NULL_HANDLE;
}

}
