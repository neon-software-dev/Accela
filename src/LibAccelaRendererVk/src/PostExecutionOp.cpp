#include "PostExecutionOp.h"

#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanCommandPool.h"

#include "Buffer/IBuffers.h"
#include "Mesh/IMeshes.h"

#include "VMA/IVMA.h"

#include <Accela/Render/IVulkanCalls.h>

namespace Accela::Render
{

PostExecutionOp BufferDeleteOp(const IBuffersPtr& buffers, BufferId bufferId)
{
    return [=]() { buffers->DestroyBuffer(bufferId); };
}

PostExecutionOp MeshDeleteOp(const IMeshesPtr& meshes, MeshId meshId)
{
    return [=]() { meshes->DestroyMesh(meshId, true); };
}

PostExecutionOp DestroyImageAllocationOp(const IVMAPtr& vma, const ImageAllocation& allocation)
{
    return [=]() { vma->DestroyImage(allocation.vkImage, allocation.vmaAllocation); };
}

PostExecutionOp DeleteFenceOp(const IVulkanCallsPtr& vk, const VulkanDevicePtr& device, VkFence vkFence)
{
    return [=]() { vk->vkDestroyFence(device->GetVkDevice(), vkFence, nullptr); };
}

PostExecutionOp FreeCommandBufferOp(const VulkanCommandPoolPtr& commandPool, const VulkanCommandBufferPtr& commandBuffer)
{
    return [=]() { commandPool->FreeCommandBuffer(commandBuffer); };
}

}
