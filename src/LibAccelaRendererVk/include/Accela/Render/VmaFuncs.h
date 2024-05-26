/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDER_VMAFUNCS_H
#define LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDER_VMAFUNCS_H

#include <vulkan/vulkan.h>

namespace Accela::Render
{
    /**
     * Holds function pointers to vulkan functions that the VMA library needs access to.
     */
    struct VmaFuncs
    {
        PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties{nullptr};
        PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties{nullptr};
        PFN_vkAllocateMemory vkAllocateMemory{nullptr};
        PFN_vkFreeMemory vkFreeMemory{nullptr};
        PFN_vkMapMemory vkMapMemory{nullptr};
        PFN_vkUnmapMemory vkUnmapMemory{nullptr};
        PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges{nullptr};
        PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges{nullptr};
        PFN_vkBindBufferMemory vkBindBufferMemory{nullptr};
        PFN_vkBindImageMemory vkBindImageMemory{nullptr};
        PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements{nullptr};
        PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements{nullptr};
        PFN_vkCreateBuffer vkCreateBuffer{nullptr};
        PFN_vkDestroyBuffer vkDestroyBuffer{nullptr};
        PFN_vkCreateImage vkCreateImage{nullptr};
        PFN_vkDestroyImage vkDestroyImage{nullptr};
        PFN_vkCmdCopyBuffer vkCmdCopyBuffer{nullptr};
        PFN_vkGetBufferMemoryRequirements2KHR vkGetBufferMemoryRequirements2KHR{nullptr};
        PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR{nullptr};
        PFN_vkBindBufferMemory2KHR vkBindBufferMemory2KHR{nullptr};
        PFN_vkBindImageMemory2KHR vkBindImageMemory2KHR{nullptr};
        PFN_vkGetPhysicalDeviceMemoryProperties2KHR vkGetPhysicalDeviceMemoryProperties2KHR{nullptr};
    };
}

#endif //LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDER_VMAFUNCS_H
