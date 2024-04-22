/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PostExecutionOps.h"

#include "VulkanObjs.h"

#include "Vulkan/VulkanDevice.h"

#include <Accela/Render/IVulkanCalls.h>

#include <algorithm>

namespace Accela::Render
{

PostExecutionOps::PostExecutionOps(Common::ILogger::Ptr logger, VulkanObjsPtr vulkanObjs)
    : m_logger(std::move(logger))
    , m_vulkanObjs(std::move(vulkanObjs))
{

}

bool PostExecutionOps::Initialize(const RenderSettings& renderSettings)
{
    return OnRenderSettingsChanged(renderSettings);
}

bool PostExecutionOps::OnRenderSettingsChanged(const RenderSettings& renderSettings)
{
    m_framesInFlight = renderSettings.framesInFlight;

    // Frame fences get destroyed on render settings change, so null out the one
    // we've been holding onto until the next frame is rendered and we're given
    // a new, valid, one. Anything enqueued in the meantime is handled as a fenceless
    // op.
    m_currentFrameFence = VK_NULL_HANDLE;

    //
    // Forcefully fulfill all operations we currently have enqueued. The fences associated
    // with them will no longer be valid to even query in the future, since frame states are
    // destroyed and recreated upon render settings change.
    //
    FulfillAll();

    return true;
}

void PostExecutionOps::Enqueue(VkFence vkFence, const PostExecutionOp& op)
{
    auto it = m_data.find(vkFence);
    if (it == m_data.cend())
    {
        it = m_data.insert({vkFence, ExecutionData(m_framesInFlight)}).first;
    }

    it->second.frameOps.push(op);
}

void PostExecutionOps::EnqueueFrameless(VkFence vkFence, const PostExecutionOp& op)
{
    auto it = m_data.find(vkFence);
    if (it == m_data.cend())
    {
        it = m_data.insert({vkFence, ExecutionData(m_framesInFlight)}).first;
    }

    it->second.framelessOps.push(op);
}

void PostExecutionOps::Enqueue_Current(const PostExecutionOp& op)
{
    if (m_currentFrameFence == VK_NULL_HANDLE)
    {
        m_logger->Log(Common::LogLevel::Debug, "PostExecutionOps: Enqueue_Current: No current frame fence set");
    }

    Enqueue(m_currentFrameFence, op);
}

void PostExecutionOps::SetFrameSynced(uint8_t frameIndex, VkFence vkFence)
{
    //
    // Update our data about this frame's work being synced
    //
    m_currentFrameFence = vkFence;

    for (auto& it : m_data)
    {
        it.second.framesFinished[frameIndex] = true;
    }

    //
    // Fulfill any pending operations which can now be run
    //
    FulfillReadyInternal(false);
}

void PostExecutionOps::FulfillReady()
{
    FulfillReadyInternal(false);
}

void PostExecutionOps::FulfillAll()
{
    // Wait for device idle to ensure all in-progress operations
    // have finished so that enqueued operations can run unrestricted.
    m_vulkanObjs->WaitForDeviceIdle();

    // Forcefully fulfil all pending operations
    FulfillReadyInternal(true);
}

void PostExecutionOps::FulfillReadyInternal(bool forceReady)
{
    std::vector<VkFence> fulfilledFences;

    for (auto& it : m_data)
    {
        // If the entry is tracking a fence, and the fence isn't finished, do nothing
        if (it.first != VK_NULL_HANDLE)
        {
            const auto fenceStatus = m_vulkanObjs->GetCalls()->vkGetFenceStatus(
                m_vulkanObjs->GetDevice()->GetVkDevice(),
                it.first
            );
            if (fenceStatus != VK_SUCCESS)
            {
                continue;
            }
        }

        // Whether all frames have finished rendering once
        const bool allFramesFinished = std::ranges::all_of(
            it.second.framesFinished,
            [](const auto& finished) { return finished; }
        );

        //
        // Frameless Ops
        //
        while (!it.second.framelessOps.empty())
        {
            std::invoke(it.second.framelessOps.top());
            it.second.framelessOps.pop();
        }

        //
        // Frame Ops
        //
        if (forceReady || allFramesFinished)
        {
            while (!it.second.frameOps.empty())
            {
                std::invoke(it.second.frameOps.top());
                it.second.frameOps.pop();
            }
        }

        if (it.second.framelessOps.empty() && it.second.frameOps.empty())
        {
            fulfilledFences.push_back(it.first);
        }
    }

    for (const auto& fence : fulfilledFences)
    {
        m_data.erase(fence);
    }
}

void PostExecutionOps::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "PostExecutionOps: Destroying");
    FulfillAll();
    m_currentFrameFence = VK_NULL_HANDLE;
}

}
