#ifndef LIBACCELARENDERERVK_SRC_VMA_VMAUTIL_H
#define LIBACCELARENDERERVK_SRC_VMA_VMAUTIL_H

#include <Accela/Render/VmaFuncs.h>

#include "vma_access.h"

namespace Accela::Render
{
    static VmaVulkanFunctions ToVmaVulkanFunctions(const VmaFuncs& vmaFuncs)
    {
        VmaVulkanFunctions vmaVulkanFuncs{};
        vmaVulkanFuncs.vkGetPhysicalDeviceProperties = vmaFuncs.vkGetPhysicalDeviceProperties;
        vmaVulkanFuncs.vkGetPhysicalDeviceMemoryProperties = vmaFuncs.vkGetPhysicalDeviceMemoryProperties;
        vmaVulkanFuncs.vkAllocateMemory = vmaFuncs.vkAllocateMemory;
        vmaVulkanFuncs.vkFreeMemory = vmaFuncs.vkFreeMemory;
        vmaVulkanFuncs.vkMapMemory = vmaFuncs.vkMapMemory;
        vmaVulkanFuncs.vkUnmapMemory = vmaFuncs.vkUnmapMemory;
        vmaVulkanFuncs.vkFlushMappedMemoryRanges = vmaFuncs.vkFlushMappedMemoryRanges;
        vmaVulkanFuncs.vkInvalidateMappedMemoryRanges = vmaFuncs.vkInvalidateMappedMemoryRanges;
        vmaVulkanFuncs.vkBindBufferMemory = vmaFuncs.vkBindBufferMemory;
        vmaVulkanFuncs.vkBindImageMemory = vmaFuncs.vkBindImageMemory;
        vmaVulkanFuncs.vkGetBufferMemoryRequirements = vmaFuncs.vkGetBufferMemoryRequirements;
        vmaVulkanFuncs.vkGetImageMemoryRequirements = vmaFuncs.vkGetImageMemoryRequirements;
        vmaVulkanFuncs.vkCreateBuffer = vmaFuncs.vkCreateBuffer;
        vmaVulkanFuncs.vkDestroyBuffer = vmaFuncs.vkDestroyBuffer;
        vmaVulkanFuncs.vkCreateImage = vmaFuncs.vkCreateImage;
        vmaVulkanFuncs.vkDestroyImage = vmaFuncs.vkDestroyImage;
        vmaVulkanFuncs.vkCmdCopyBuffer = vmaFuncs.vkCmdCopyBuffer;
        vmaVulkanFuncs.vkGetBufferMemoryRequirements2KHR = vmaFuncs.vkGetBufferMemoryRequirements2KHR;
        vmaVulkanFuncs.vkGetImageMemoryRequirements2KHR = vmaFuncs.vkGetImageMemoryRequirements2KHR;
        vmaVulkanFuncs.vkBindBufferMemory2KHR = vmaFuncs.vkBindBufferMemory2KHR;
        vmaVulkanFuncs.vkBindImageMemory2KHR = vmaFuncs.vkBindImageMemory2KHR;
        vmaVulkanFuncs.vkGetPhysicalDeviceMemoryProperties2KHR = vmaFuncs.vkGetPhysicalDeviceMemoryProperties2KHR;

        return vmaVulkanFuncs;
    }
}

#endif //LIBACCELARENDERERVK_SRC_VMA_VMAUTIL_H
