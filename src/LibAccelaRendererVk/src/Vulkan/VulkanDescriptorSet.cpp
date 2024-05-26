/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanDescriptorSet.h"
#include "VulkanDevice.h"

#include <Accela/Render/IVulkanCalls.h>

namespace Accela::Render
{

VulkanDescriptorSet::VulkanDescriptorSet(Common::ILogger::Ptr logger,
                                         IVulkanCallsPtr vk,
                                         VulkanDevicePtr device,
                                         VkDescriptorSet vkDescriptorSet)
    : m_logger(std::move(logger))
    , m_vk(std::move(vk))
    , m_device(std::move(device))
    , m_vkDescriptorSet(vkDescriptorSet)
{

}

void VulkanDescriptorSet::WriteBufferBind(const std::optional<VulkanDescriptorSetLayout::BindingDetails>& bindingDetails,
                                          VkDescriptorType vkDescriptorType,
                                          VkBuffer vkBuffer,
                                          std::size_t offset,
                                          uint64_t bufferByteSize)
{
    if (!bindingDetails.has_value()) { return; }

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = vkBuffer;
    bufferInfo.offset = offset;
    bufferInfo.range = bufferByteSize;

    if (bufferInfo.range == 0)
    {
        bufferInfo.range = VK_WHOLE_SIZE;
    }

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_vkDescriptorSet;
    descriptorWrite.dstBinding = bindingDetails->binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = vkDescriptorType;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;
    descriptorWrite.pImageInfo = nullptr;
    descriptorWrite.pTexelBufferView = nullptr;

    m_vk->vkUpdateDescriptorSets(m_device->GetVkDevice(), 1, &descriptorWrite, 0, nullptr);
}

void VulkanDescriptorSet::WriteCombinedSamplerBind(const std::optional<VulkanDescriptorSetLayout::BindingDetails>& bindingDetails,
                                                   VkImageView vkImageView,
                                                   VkSampler vkSampler)
{
    WriteCombinedSamplerBind(bindingDetails, {{vkImageView, vkSampler}});
}

void VulkanDescriptorSet::WriteCombinedSamplerBind(const std::optional<VulkanDescriptorSetLayout::BindingDetails>& bindingDetails,
                                                   const std::vector<std::pair<VkImageView, VkSampler>>& samplers)
{
    if (!bindingDetails.has_value()) { return; }

    std::vector<VkDescriptorImageInfo> imageInfos;

    for (const auto& sampler : samplers)
    {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = sampler.first;
        imageInfo.sampler = sampler.second;

        imageInfos.push_back(imageInfo);
    }

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_vkDescriptorSet;
    descriptorWrite.dstBinding = bindingDetails->binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = imageInfos.size();
    descriptorWrite.pBufferInfo = nullptr;
    descriptorWrite.pImageInfo = imageInfos.data();
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    m_vk->vkUpdateDescriptorSets(m_device->GetVkDevice(), 1, &descriptorWrite, 0, nullptr);
}

void VulkanDescriptorSet::WriteInputAttachmentBind(const std::optional<VulkanDescriptorSetLayout::BindingDetails>& bindingDetails,
                                                   VkImageView vkImageView)
{
    if (!bindingDetails.has_value()) { return; }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = vkImageView;
    imageInfo.sampler = VK_NULL_HANDLE;

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_vkDescriptorSet;
    descriptorWrite.dstBinding = bindingDetails->binding;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = nullptr;
    descriptorWrite.pImageInfo = &imageInfo;
    descriptorWrite.pTexelBufferView = nullptr; // Optional

    m_vk->vkUpdateDescriptorSets(m_device->GetVkDevice(), 1, &descriptorWrite, 0, nullptr);
}

}
