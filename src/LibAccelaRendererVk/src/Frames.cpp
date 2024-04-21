/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#include "Frames.h"
#include "VulkanObjs.h"

#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanSwapChain.h"

#include <Accela/Render/IVulkanCalls.h>

namespace Accela::Render
{

Frames::Frames(Common::ILogger::Ptr logger,
               Ids::Ptr ids,
               VulkanObjsPtr vulkanObjs,
               ITexturesPtr textures)
    : m_logger(std::move(logger))
    , m_ids(std::move(ids))
    , m_vulkanObjs(std::move(vulkanObjs))
    , m_textures(std::move(textures))
{

}

bool Frames::Initialize(const RenderSettings& renderSettings, const VulkanSwapChainPtr& swapChain)
{
    m_logger->Log(Common::LogLevel::Info,
        "Frames: Initializing for {} frames in flight and {} swap chain images",
          renderSettings.framesInFlight,
          swapChain->GetSwapChainImageViews().size()
    );

    m_currentFrameIndex = 0;

    return CreateFrames(renderSettings);
}

void Frames::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "Frames: Destroying frames");

    for (auto& frame : m_frames)
    {
        frame.Destroy();
    }

    m_frames.clear();

    m_currentFrameIndex = 0;
}

void Frames::OnSwapChainChanged(const VulkanSwapChainPtr& swapChain)
{
    m_logger->Log(Common::LogLevel::Info,
      "Frames: Swap chain changed, new swap chain images size: {}", swapChain->GetSwapChainImageViews().size());
}

bool Frames::OnRenderSettingsChanged(const RenderSettings& renderSettings)
{
    m_logger->Log(Common::LogLevel::Info, "Frames: Render settings changed");

    // If we now have more frames in flight, keep looping through frame indices into the new, expanded, range. If we
    // now have fewer frames in flight, just drop back to the highest index frame we have access to.
    m_currentFrameIndex = std::min(m_currentFrameIndex, (uint32_t)(renderSettings.framesInFlight - 1));

    Destroy();
    return CreateFrames(renderSettings);
}

bool Frames::CreateFrames(const RenderSettings& renderSettings)
{
    for (uint32_t frameIndex = 0; frameIndex < renderSettings.framesInFlight; ++frameIndex)
    {
        m_frames.emplace_back(m_logger, m_ids, m_vulkanObjs, m_textures, frameIndex);
        if (!m_frames[frameIndex].Initialize(renderSettings))
        {
            Destroy();
            return false;
        }
    }

    return true;
}

std::expected<uint32_t, SurfaceIssue> Frames::StartFrame()
{
    //
    // Wait for any prior pipeline work for this frame to finish
    //
    WaitForFrameWorkToFinish(m_currentFrameIndex);

    //
    // Acquire the next swap chain image index to render to and
    // assign this frame as what's rendering to that swap chain
    // image next. (Note that another frame may still be using
    // that swap chain image at this point.)
    //
    const auto swapChainImageIndexExpect = AcquireNextSwapChainImageIndex();
    if (!swapChainImageIndexExpect)
    {
        return std::unexpected(swapChainImageIndexExpect.error());
    }

    return swapChainImageIndexExpect;
}

void Frames::EndFrame()
{
    //
    // Switch to the next frame index
    //
    m_currentFrameIndex = (m_currentFrameIndex + 1) % m_frames.size();
}

void Frames::WaitForFrameWorkToFinish(uint32_t frameIndex)
{
    VkFence pipelineFence = m_frames[frameIndex].GetPipelineFence();

    m_vulkanObjs->GetCalls()->vkWaitForFences(
        m_vulkanObjs->GetDevice()->GetVkDevice(),
        1,
        &pipelineFence,
        VK_TRUE,
        UINT64_MAX
    );
}

std::expected<uint32_t, SurfaceIssue> Frames::AcquireNextSwapChainImageIndex()
{
    VkSemaphore imageAvailableSemaphore = m_frames[m_currentFrameIndex].GetImageAvailableSemaphore();

    uint32_t swapChainImageIndex{0};

    auto result = m_vulkanObjs->GetCalls()->vkAcquireNextImageKHR(
        m_vulkanObjs->GetDevice()->GetVkDevice(),
        m_vulkanObjs->GetSwapChain()->GetVkSwapchainKHR(),
        UINT64_MAX,
        imageAvailableSemaphore,
        VK_NULL_HANDLE,
        &swapChainImageIndex
    );
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        m_logger->Log(Common::LogLevel::Info, "vkAcquireNextImageKHR notified old/suboptimal swap chain");
        return std::unexpected(SurfaceIssue::SurfaceInvalidated);
    }
    else if (result == VK_ERROR_SURFACE_LOST_KHR)
    {
        m_logger->Log(Common::LogLevel::Info, "vkAcquireNextImageKHR notified surface lost");
        return std::unexpected(SurfaceIssue::SurfaceLost);
    }
    else if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Failed to acquire next swap chain image, result code: {}", (uint32_t)result);
        return std::unexpected(SurfaceIssue::SurfaceLost);
    }

    return swapChainImageIndex;
}

FrameState& Frames::GetCurrentFrame()
{
    return m_frames[m_currentFrameIndex];
}

FrameState& Frames::GetNextFrame()
{
    return m_frames[(m_currentFrameIndex + 1) % m_frames.size()];
}

}
