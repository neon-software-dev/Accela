/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_POSTEXECUTIONOP_H
#define LIBACCELARENDERERVK_SRC_POSTEXECUTIONOP_H

#include "PostExecutionOps.h"
#include "InternalId.h"
#include "ForwardDeclares.h"
#include "Util/ImageAllocation.h"

namespace Accela::Render
{
    PostExecutionOp BufferDeleteOp(const IBuffersPtr& buffers, BufferId bufferId);
    PostExecutionOp MeshDeleteOp(const IMeshesPtr& meshes, MeshId meshId);
    PostExecutionOp DestroyImageAllocationOp(const IVMAPtr& vma, const ImageAllocation& allocation);
    PostExecutionOp DeleteFenceOp(const IVulkanCallsPtr& vk, const VulkanDevicePtr& device, VkFence vkFence);
    PostExecutionOp FreeCommandBufferOp(const VulkanCommandPoolPtr& commandPool, const VulkanCommandBufferPtr& commandBuffer);
}

#endif //LIBACCELARENDERERVK_SRC_POSTEXECUTIONOP_H
