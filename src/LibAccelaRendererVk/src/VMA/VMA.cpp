/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VMA.h"

#include <vector>

namespace Accela::Render
{

VMA::VMA(const VmaAllocator& vma)
    : m_vma(vma)
{

}

void VMA::DestroyInstance()
{
    vmaDestroyAllocator(m_vma);

    m_vma = nullptr;
}

std::optional<IVMAPtr> VMA::CreateInstance(const Common::ILogger::Ptr& logger,
                                           const VmaAllocatorCreateInfo& createInfo)
{
    VmaAllocator vmaAllocator;

    auto result = vmaCreateAllocator(&createInfo, &vmaAllocator);
    if (result != VK_SUCCESS)
    {
        logger->Log(Common::LogLevel::Fatal, "CreateVMAInstance: Failed to init vma, result code: {}", (uint32_t)result);
        return std::nullopt;
    }

    return std::make_shared<VMA>(vmaAllocator);
}

std::vector<VmaBudget> VMA::GetVmaBudget(unsigned int numPhysicalDeviceMemoryHeaps) const
{
    std::vector<VmaBudget> vmaBudget(numPhysicalDeviceMemoryHeaps, VmaBudget{});
    vmaGetHeapBudgets(m_vma, vmaBudget.data());
    return vmaBudget;
}

VkResult VMA::CreateBuffer(const VkBufferCreateInfo *pBufferCreateInfo,
                           const VmaAllocationCreateInfo *pAllocationCreateInfo, VkBuffer *pBuffer,
                           VmaAllocation *pAllocation, VmaAllocationInfo *pAllocationInfo) const
{
    return vmaCreateBuffer(m_vma, pBufferCreateInfo, pAllocationCreateInfo, pBuffer, pAllocation, pAllocationInfo);
}

void VMA::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation) const
{
    vmaDestroyBuffer(m_vma, buffer, allocation);
}

VkResult
VMA::CreateImage(const VkImageCreateInfo *pImageCreateInfo, const VmaAllocationCreateInfo *pAllocationCreateInfo,
                 VkImage *pImage, VmaAllocation *pAllocation, VmaAllocationInfo *pAllocationInfo) const
{
    return vmaCreateImage(m_vma, pImageCreateInfo, pAllocationCreateInfo, pImage, pAllocation, pAllocationInfo);
}

void VMA::DestroyImage(VkImage image, VmaAllocation allocation) const
{
    vmaDestroyImage(m_vma, image, allocation);
}

VkResult VMA::MapMemory(VmaAllocation allocation, void **ppData) const
{
    return vmaMapMemory(m_vma, allocation, ppData);
}

void VMA::UnmapMemory(VmaAllocation allocation) const
{
    vmaUnmapMemory(m_vma, allocation);
}

}
