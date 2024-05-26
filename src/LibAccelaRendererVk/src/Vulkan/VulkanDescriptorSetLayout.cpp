#include "VulkanDescriptorSetLayout.h"
#include "VulkanDebug.h"
#include "VulkanDevice.h"

#include <Accela/Render/IVulkanCalls.h>

#include <algorithm>

namespace Accela::Render
{

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(Common::ILogger::Ptr logger, IVulkanCallsPtr vk, VulkanDevicePtr device)
    : m_logger(std::move(logger))
    , m_vk(std::move(vk))
    , m_device(std::move(device))
{

}

bool VulkanDescriptorSetLayout::Create(const std::vector<BindingDetails>& bindings, const std::string& tag)
{
    std::vector<VkDescriptorSetLayoutBinding> vkBindings;

    std::transform(bindings.cbegin(), bindings.cend(), std::back_inserter(vkBindings), [](const auto& binding){
        VkDescriptorSetLayoutBinding vkBinding{};
        vkBinding.binding = binding.binding;
        vkBinding.descriptorType = binding.descriptorType;
        vkBinding.descriptorCount = 1;
        vkBinding.stageFlags = binding.stageFlags;
        vkBinding.descriptorCount = binding.descriptorCount;
        vkBinding.pImmutableSamplers = nullptr; // Optional

        return vkBinding;
    });

    // Handle stub descriptor sets that have no bindings
    VkDescriptorSetLayoutBinding* pBindings = nullptr;
    if (!vkBindings.empty())
    {
        pBindings = vkBindings.data();
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = vkBindings.size();
    layoutInfo.pBindings = pBindings;

    const auto result = m_vk->vkCreateDescriptorSetLayout(m_device->GetVkDevice(), &layoutInfo, nullptr, &m_vkDescriptorSetLayout);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error,
          "vkCreateDescriptorSetLayout call failure, result code: {}", (uint32_t)result);
        return false;
    }

    SetDebugName(m_vk, m_device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)m_vkDescriptorSetLayout, "DescriptorSetLayout-" + tag);

    m_bindingDetails = bindings;

    return true;
}

void VulkanDescriptorSetLayout::Destroy()
{
    if (m_vkDescriptorSetLayout == VK_NULL_HANDLE)
    {
        return;
    }

    RemoveDebugName(m_vk, m_device, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (uint64_t)m_vkDescriptorSetLayout);

    m_vk->vkDestroyDescriptorSetLayout(m_device->GetVkDevice(), m_vkDescriptorSetLayout, nullptr);

    m_vkDescriptorSetLayout = VK_NULL_HANDLE;
    m_bindingDetails.clear();
}

}
