/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Framebuffers.h"

#include "../PostExecutionOp.h"

#include "../Vulkan/VulkanFramebuffer.h"

namespace Accela::Render
{

Framebuffers::Framebuffers(Common::ILogger::Ptr logger,
                           Ids::Ptr ids,
                           VulkanObjsPtr vulkanObjs,
                           IImagesPtr images,
                           PostExecutionOpsPtr postExecutionOps)
   : m_logger(std::move(logger))
   , m_ids(std::move(ids))
   , m_vulkanObjs(std::move(vulkanObjs))
   , m_images(std::move(images))
   , m_postExecutionOps(std::move(postExecutionOps))
{

}

void Framebuffers::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "Framebuffers: Destroying");

    while (!m_framebuffers.empty())
    {
        DestroyFramebuffer(m_framebuffers.cbegin()->first, true);
    }
}

bool Framebuffers::CreateFramebuffer(FrameBufferId id,
                                     const VulkanRenderPassPtr& renderPass,
                                     const std::vector<std::pair<ImageDefinition, std::string>>& attachments,
                                     const USize& size,
                                     const uint32_t& layers,
                                     const std::string& tag)
{
    const auto it = m_framebuffers.find(id);
    if (it != m_framebuffers.cend())
    {
        m_logger->Log(Common::LogLevel::Warning,
          "Framebuffers: CreateFramebuffer: Framebuffer already exists: {}", id.id);
        return false;
    }

    FramebufferObjs framebufferObjs(m_logger, m_ids, m_vulkanObjs, m_images);
    if (!framebufferObjs.CreateOwning(renderPass, attachments, size, layers, tag))
    {
        return false;
    }

    m_framebuffers.insert({id, framebufferObjs});

    return true;
}

bool Framebuffers::CreateFramebuffer(FrameBufferId id,
                                     const VulkanRenderPassPtr& renderPass,
                                     const std::vector<std::pair<ImageId, std::string>>& attachmentImageViews,
                                     const USize& size,
                                     const uint32_t& layers,
                                     const std::string& tag)
{
    const auto it = m_framebuffers.find(id);
    if (it != m_framebuffers.cend())
    {
        m_logger->Log(Common::LogLevel::Warning,
          "Framebuffers: CreateFramebuffer: Framebuffer already exists: {}", id.id);
        return false;
    }

    FramebufferObjs framebufferObjs(m_logger, m_ids, m_vulkanObjs, m_images);
    if (!framebufferObjs.CreateFromExisting(renderPass, attachmentImageViews, size, layers, tag))
    {
        return false;
    }

    m_framebuffers.insert({id, framebufferObjs});

    return true;
}

std::optional<FramebufferObjs> Framebuffers::GetFramebufferObjs(FrameBufferId frameBufferId)
{
    const auto it = m_framebuffers.find(frameBufferId);
    if (it != m_framebuffers.cend())
    {
        return it->second;
    }

    return std::nullopt;
}

void Framebuffers::DestroyFramebuffer(FrameBufferId frameBufferId, bool destroyImmediately)
{
    const auto it = m_framebuffers.find(frameBufferId);
    if (it == m_framebuffers.cend())
    {
        return;
    }

    FramebufferObjs framebufferObjs = it->second;

    // Immediately erase our knowledge of the framebuffer
    m_framebuffers.erase(it);

    if (destroyImmediately)
    {
        m_logger->Log(Common::LogLevel::Debug, "Framebuffers: Destroying framebuffer immediately: {}", frameBufferId.id);

        DestroyFramebufferObjects(frameBufferId, framebufferObjs);
    }
    else
    {
        m_logger->Log(Common::LogLevel::Info,
          "Framebuffers: Enqueuing framebuffer destroy: {}", frameBufferId.id);

        m_postExecutionOps->Enqueue_Current([=, this]() { DestroyFramebufferObjects(frameBufferId, framebufferObjs); });
    }
}

void Framebuffers::DestroyFramebufferObjects(FrameBufferId frameBufferId, FramebufferObjs framebufferObjs)
{
    m_logger->Log(Common::LogLevel::Debug, "Framebuffers: Destroying framebuffer objects: {}", frameBufferId.id);
    framebufferObjs.Destroy();

    // Return the framebuffer id to the pool now that it's no longer in use
    m_ids->frameBufferIds.ReturnId(frameBufferId);
}

}
