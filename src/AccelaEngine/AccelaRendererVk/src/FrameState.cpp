/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "FrameState.h"
#include "VulkanObjs.h"

#include "Image/IImages.h"

#include "RenderTarget/IRenderTargets.h"

#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanPhysicalDevice.h"
#include "Vulkan/VulkanDebug.h"
#include "Vulkan/VulkanCommandPool.h"
#include "Vulkan/VulkanCommandBuffer.h"

#include <Accela/Render/IVulkanCalls.h>

#include <format>

namespace Accela::Render
{

FrameState::FrameState(Common::ILogger::Ptr logger,
                       VulkanObjsPtr vulkanObjs,
                       IRenderTargetsPtr renderTargets,
                       IImagesPtr images,
                       uint8_t frameIndex)
    : m_logger(std::move(logger))
    , m_vulkanObjs(std::move(vulkanObjs))
    , m_renderTargets(std::move(renderTargets))
    , m_images(std::move(images))
    , m_frameIndex(frameIndex)
{

}

bool FrameState::Initialize(const RenderSettings& renderSettings)
{
    m_logger->Log(Common::LogLevel::Info, "FrameState: Initializing frame {}", m_frameIndex);

    //
    // Graphics Command Pool
    //
    m_graphicsCommandPool = std::make_shared<VulkanCommandPool>(m_logger, m_vulkanObjs->GetCalls(), m_vulkanObjs->GetDevice());

    if (!m_graphicsCommandPool->Create(
        m_vulkanObjs->GetPhysicalDevice()->GetGraphicsQueueFamilyIndex().value(),
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        std::format("Graphics-Frame{}", m_frameIndex)))
    {
        m_logger->Log(Common::LogLevel::Fatal,
          "FrameState: Failed to create graphics command pool for frame: {}", m_frameIndex);
        return false;
    }

    //
    // Render Command Buffer
    //
    const auto renderCommandBufferOpt = m_graphicsCommandPool->AllocateCommandBuffer(
        VulkanCommandPool::CommandBufferType::Primary,
        std::format("Render-Frame{}", m_frameIndex)
    );
    if (!renderCommandBufferOpt)
    {
        m_logger->Log(Common::LogLevel::Fatal,
          "FrameState: Failed to create render command buffer for frame: {}", m_frameIndex);
        return false;
    }
    m_renderCommandBuffer = renderCommandBufferOpt.value();

    //
    // Swap Chain Blit Command Buffer
    //
    const auto swapChainBlitCommandBufferOpt = m_graphicsCommandPool->AllocateCommandBuffer(
        VulkanCommandPool::CommandBufferType::Primary,
        std::format("SwapChainBlit-Frame{}", m_frameIndex)
    );
    if (!swapChainBlitCommandBufferOpt)
    {
        m_logger->Log(Common::LogLevel::Fatal,
          "FrameState: Failed to create swap chain blit command buffer for frame: {}", m_frameIndex);
        return false;
    }
    m_swapChainBlitCommandBuffer = swapChainBlitCommandBufferOpt.value();

    //
    // RenderTexture Available Semaphore
    //
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    m_vulkanObjs->GetCalls()->vkCreateSemaphore(
        m_vulkanObjs->GetDevice()->GetVkDevice(),
        &semaphoreInfo,
        nullptr,
        &m_imageAvailableSemaphore
    );
    SetDebugName(
        m_vulkanObjs->GetCalls(),
        m_vulkanObjs->GetDevice(),
        VK_OBJECT_TYPE_SEMAPHORE,
        (uint64_t)m_imageAvailableSemaphore,
        std::format("Semaphore-ImageAvailable-Frame{}", m_frameIndex)
    );

    //
    // Render Finished Semaphore
    //
    m_vulkanObjs->GetCalls()->vkCreateSemaphore(
        m_vulkanObjs->GetDevice()->GetVkDevice(),
        &semaphoreInfo,
        nullptr,
        &m_renderFinishedSemaphore
    );
    SetDebugName(
        m_vulkanObjs->GetCalls(),
        m_vulkanObjs->GetDevice(),
        VK_OBJECT_TYPE_SEMAPHORE,
        (uint64_t)m_renderFinishedSemaphore,
        std::format("Semaphore-RenderFinished-Frame{}", m_frameIndex)
    );

    //
    // Swap Chain Blit Finished Semaphore
    //
    m_vulkanObjs->GetCalls()->vkCreateSemaphore(
        m_vulkanObjs->GetDevice()->GetVkDevice(),
        &semaphoreInfo,
        nullptr,
        &m_swapChainBlitFinishedSemaphore
    );
    SetDebugName(
        m_vulkanObjs->GetCalls(),
        m_vulkanObjs->GetDevice(),
        VK_OBJECT_TYPE_SEMAPHORE,
        (uint64_t)m_swapChainBlitFinishedSemaphore,
        std::format("Semaphore-SwapChainBlitFinished-Frame{}", m_frameIndex)
    );

    //
    // Pipeline work finished fence
    //
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    m_vulkanObjs->GetCalls()->vkCreateFence(
        m_vulkanObjs->GetDevice()->GetVkDevice(),
        &fenceInfo,
        nullptr,
        &m_pipelineFence
    );
    SetDebugName(
        m_vulkanObjs->GetCalls(),
        m_vulkanObjs->GetDevice(),
        VK_OBJECT_TYPE_FENCE,
        (uint64_t)m_pipelineFence,
        std::format("Fence-PipelineFinished-Frame{}", m_frameIndex)
    );

    //
    // Buffer to receive object detail image data
    //
    const auto image = Image{
        .tag = std::format("ObjectDetail-Frame-{}", m_frameIndex),
        .vkImageType = VK_IMAGE_TYPE_2D,
        .vkFormat = m_renderTargets->GetObjectDetailVkFormat(),
        .vkImageTiling = VK_IMAGE_TILING_LINEAR,
        .vkImageUsageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .size = renderSettings.resolution,
        .numLayers = 1,
        .vmaAllocationCreateFlags =
            VMA_ALLOCATION_CREATE_MAPPED_BIT
            | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
            | VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
    };

    const auto objectDetailImageExpect = m_images->CreateEmptyImage(ImageDefinition(image, {}, {}));
    if (!objectDetailImageExpect)
    {
        m_logger->Log(Common::LogLevel::Fatal,
          "FrameState: Failed to create object detail image for frame: {}", m_frameIndex);
        return false;
    }

    m_objectDetailImageId = *objectDetailImageExpect;

    return true;
}

void FrameState::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "FrameState: Destroying frame {}", m_frameIndex);

    if (m_objectDetailImageId.IsValid())
    {
        m_images->DestroyImage(m_objectDetailImageId, true);
        m_objectDetailImageId = {};
    }

    if (m_pipelineFence != VK_NULL_HANDLE)
    {
        RemoveDebugName(
            m_vulkanObjs->GetCalls(),
            m_vulkanObjs->GetDevice(),
            VK_OBJECT_TYPE_FENCE,
            (uint64_t)m_pipelineFence
        );
        m_vulkanObjs->GetCalls()->vkDestroyFence(
            m_vulkanObjs->GetDevice()->GetVkDevice(),
            m_pipelineFence,
            nullptr
        );
        m_pipelineFence = VK_NULL_HANDLE;
    }

    if (m_renderFinishedSemaphore != VK_NULL_HANDLE)
    {
        RemoveDebugName(
            m_vulkanObjs->GetCalls(),
            m_vulkanObjs->GetDevice(),
            VK_OBJECT_TYPE_SEMAPHORE,
            (uint64_t)m_renderFinishedSemaphore
        );
        m_vulkanObjs->GetCalls()->vkDestroySemaphore(
            m_vulkanObjs->GetDevice()->GetVkDevice(),
            m_renderFinishedSemaphore,
            nullptr
        );
        m_renderFinishedSemaphore = VK_NULL_HANDLE;
    }

    if (m_swapChainBlitFinishedSemaphore != VK_NULL_HANDLE)
    {
        RemoveDebugName(
            m_vulkanObjs->GetCalls(),
            m_vulkanObjs->GetDevice(),
            VK_OBJECT_TYPE_SEMAPHORE,
            (uint64_t)m_swapChainBlitFinishedSemaphore
        );
        m_vulkanObjs->GetCalls()->vkDestroySemaphore(
            m_vulkanObjs->GetDevice()->GetVkDevice(),
            m_swapChainBlitFinishedSemaphore,
            nullptr
        );
        m_swapChainBlitFinishedSemaphore = VK_NULL_HANDLE;
    }

    if (m_imageAvailableSemaphore != VK_NULL_HANDLE)
    {
        RemoveDebugName(
            m_vulkanObjs->GetCalls(),
            m_vulkanObjs->GetDevice(),
            VK_OBJECT_TYPE_SEMAPHORE,
            (uint64_t)m_imageAvailableSemaphore
        );
        m_vulkanObjs->GetCalls()->vkDestroySemaphore(
            m_vulkanObjs->GetDevice()->GetVkDevice(),
            m_imageAvailableSemaphore,
            nullptr
        );
        m_imageAvailableSemaphore = VK_NULL_HANDLE;
    }

    if (m_renderCommandBuffer != nullptr)
    {
        m_graphicsCommandPool->FreeCommandBuffer(m_renderCommandBuffer);
        m_renderCommandBuffer = nullptr;
    }

    if (m_swapChainBlitCommandBuffer != nullptr)
    {
        m_graphicsCommandPool->FreeCommandBuffer(m_swapChainBlitCommandBuffer);
        m_swapChainBlitCommandBuffer = nullptr;
    }

    if (m_graphicsCommandPool != nullptr)
    {
        m_graphicsCommandPool->ResetPool(true);
        m_graphicsCommandPool->Destroy();
        m_graphicsCommandPool = nullptr;
    }
}

}
