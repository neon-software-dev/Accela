#include "VulkanSwapChain.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanDevice.h"
#include "VulkanSurface.h"
#include "VulkanDebug.h"

#include "../VMA/IVMA.h"

#include <Accela/Render/IVulkanCalls.h>

#include <format>
#include <cstdint>
#include <algorithm>

namespace Accela::Render
{

VulkanSwapChain::VulkanSwapChain(Common::ILogger::Ptr logger,
                                 IVulkanCallsPtr vk,
                                 IVMAPtr vma,
                                 VulkanPhysicalDevicePtr physicalDevice,
                                 VulkanDevicePtr device)
    : m_logger(std::move(logger))
    , m_vk(std::move(vk))
    , m_vma(std::move(vma))
    , m_physicalDevice(std::move(physicalDevice))
    , m_device(std::move(device))
{

}

bool VulkanSwapChain::Create(const VulkanSurfacePtr& surface,
                             const VulkanSwapChainPtr& previousSwapChain,
                             const PresentMode& desiredPresentMode)
{
    // Query for the surface capabilities of the device+surface
    const SwapChainSupportDetails supportDetails = SwapChainSupportDetails::Load(
        m_vk,
        m_physicalDevice->GetVkPhysicalDevice(),
        surface->GetVkSurface()
    );

    //
    // Choose the swap chain's configuration, from the device+surface reported capabilities
    //
    const VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(supportDetails);
    const VkPresentModeKHR presentMode = ChoosePresentMode(supportDetails, desiredPresentMode);
    const VkExtent2D swapChainImageExtent = ChooseExtent(surface, supportDetails);

    m_logger->Log(Common::LogLevel::Info,
      "VulkanSwapChain: Chosen surface format: {}, color space: {}", (uint32_t)surfaceFormat.format, (uint32_t)surfaceFormat.colorSpace);
    m_logger->Log(Common::LogLevel::Info,
      "VulkanSwapChain: Chosen Vulkan present mode: {}", (uint32_t)presentMode);
    m_logger->Log(Common::LogLevel::Info,
      "VulkanSwapChain: Chosen image extent: {}x{}", swapChainImageExtent.width, swapChainImageExtent.height);

    uint32_t imageCount = supportDetails.capabilities.minImageCount + 1;
    if (supportDetails.capabilities.maxImageCount > 0 && imageCount > supportDetails.capabilities.maxImageCount)
    {
        imageCount = supportDetails.capabilities.maxImageCount;
    }

    VkCompositeAlphaFlagBitsKHR compositeAlphaFlags = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    if ((supportDetails.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) == 0)
    {
        m_logger->Log(Common::LogLevel::Warning,"Device doesn't support opaque alpha bit, using inherit instead");
        compositeAlphaFlags = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }

    //
    // Create the swap chain
    //
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface->GetVkSurface();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapChainImageExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = supportDetails.capabilities.currentTransform;
    createInfo.compositeAlpha = compositeAlphaFlags;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    // Set sharing mode as needed depending on if graphics and present queues are in different queue families
    const uint32_t graphicsQueueFamilyIndex = m_physicalDevice->GetGraphicsQueueFamilyIndex().value();
    const uint32_t presentQueueFamilyIndex = m_physicalDevice->GetPresentQueueFamilyIndex(surface).value();

    if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
    {
        const uint32_t queueFamilyIndices[] = {graphicsQueueFamilyIndex, presentQueueFamilyIndex};

        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    if (previousSwapChain != nullptr)
    {
        createInfo.oldSwapchain = previousSwapChain->GetVkSwapchainKHR();
    }

    auto result = m_vk->vkCreateSwapchainKHR(m_device->GetVkDevice(), &createInfo, nullptr, &m_vkSwapChain);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Fatal, "vkCreateSwapchainKHR failed, result code: {}", (uint32_t)result);
        return false;
    }

    m_swapChainConfig = SwapChainConfig(surfaceFormat, presentMode, swapChainImageExtent, createInfo.preTransform);

    //
    // Get references to the swap view's Images
    //
    m_vk->vkGetSwapchainImagesKHR(m_device->GetVkDevice(), m_vkSwapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    m_vk->vkGetSwapchainImagesKHR(m_device->GetVkDevice(), m_vkSwapChain, &imageCount, m_swapChainImages.data());

    //
    // Create ImageViews for accessing the swap chain Images
    //
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (unsigned int x = 0; x < m_swapChainImages.size(); ++x)
    {
        VkImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = m_swapChainImages[x];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = surfaceFormat.format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        result = m_vk->vkCreateImageView(m_device->GetVkDevice(), &imageViewCreateInfo, nullptr, &m_swapChainImageViews[x]);
        if (result != VK_SUCCESS)
        {
            m_logger->Log(Common::LogLevel::Fatal, "Swap chain vkCreateImageView failed, result code: {}", (uint32_t)result);
            Destroy();
            return false;
        }

        SetDebugName(m_vk, m_device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)m_swapChainImageViews[x], std::format("ImageView-SwapChain-{}", x));
    }

    return true;
}

void VulkanSwapChain::Destroy()
{
    if (m_vkSwapChain == VK_NULL_HANDLE)
    {
        return;
    }

    for (const auto& imageView : m_swapChainImageViews)
    {
        RemoveDebugName(m_vk, m_device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)imageView);
        m_vk->vkDestroyImageView(m_device->GetVkDevice(), imageView, nullptr);
    }
    m_swapChainImageViews.clear();

    m_swapChainConfig.reset();
    m_swapChainImages.clear();

    m_vk->vkDestroySwapchainKHR(m_device->GetVkDevice(), m_vkSwapChain, nullptr);
    m_vkSwapChain = VK_NULL_HANDLE;
}

VkSurfaceFormatKHR VulkanSwapChain::ChooseSurfaceFormat(const SwapChainSupportDetails& supportDetails)
{
    for (const auto& availableFormat : supportDetails.formats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return supportDetails.formats.front();
}

VkPresentModeKHR VulkanSwapChain::ChoosePresentMode(const SwapChainSupportDetails& supportDetails, const PresentMode& desiredPresentMode)
{
    // The only present mode guaranteed to be made available
    const VkPresentModeKHR fallbackPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    VkPresentModeKHR vkDesiredPresentMode;

    switch (desiredPresentMode)
    {
        case PresentMode::Immediate:
            vkDesiredPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        break;
        case PresentMode::VSync:
            vkDesiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        break;
        default:
            vkDesiredPresentMode = fallbackPresentMode;
    }

    if (HasSupportForPresentMode(supportDetails, vkDesiredPresentMode))
    {
        m_logger->Log(Common::LogLevel::Info,
          "VulkanSwapChain: Has support for desired present mode: {}", (int)desiredPresentMode);
        return vkDesiredPresentMode;
    }

    m_logger->Log(Common::LogLevel::Info, "VulkanSwapChain: Using fallback present mode");
    return fallbackPresentMode;
}

VkExtent2D VulkanSwapChain::ChooseExtent(const VulkanSurfacePtr& surface, const SwapChainSupportDetails& supportDetails)
{
    if (supportDetails.capabilities.currentExtent.width != UINT32_MAX)
    {
        return supportDetails.capabilities.currentExtent;
    }
    else
    {
        // Use the surface size as reported by the IVulkanContext if it's being left to
        // us to pick an extent
        std::pair<unsigned int, unsigned int> surfaceSize = surface->GetSurfaceSize();

        VkExtent2D actualExtent = {static_cast<uint32_t>(surfaceSize.first), static_cast<uint32_t>(surfaceSize.second)};

        actualExtent.width = std::clamp(
            actualExtent.width,
            supportDetails.capabilities.minImageExtent.width,
            supportDetails.capabilities.maxImageExtent.width);

        actualExtent.height = std::clamp(
            actualExtent.height,
            supportDetails.capabilities.minImageExtent.height,
            supportDetails.capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

bool VulkanSwapChain::HasSupportForPresentMode(const SwapChainSupportDetails& supportDetails, VkPresentModeKHR presentMode)
{
    return std::any_of(supportDetails.presentModes.cbegin(), supportDetails.presentModes.cend(), [&](const auto& supportedMode){
        return supportedMode == presentMode;
    });
}

}
