#ifndef LIBACCELARENDERVK_SRC_VULKAN_VULKANDEBUG
#define LIBACCELARENDERVK_SRC_VULKAN_VULKANDEBUG

#include "../ForwardDeclares.h"

#include <vulkan/vulkan.h>

#include <string>

namespace Accela::Render
{
    // Associates a debug name with the specified vulkan object
    void SetDebugName(const IVulkanCallsPtr& vk, const VulkanDevicePtr& device, VkObjectType objType, uint64_t obj, const std::string& name);
    // Removes debug name assocaited with a vulkan object
    void RemoveDebugName(const IVulkanCallsPtr& vk, const VulkanDevicePtr& device, VkObjectType objType, uint64_t obj);

    /**
     * Scoped object that annotates usage of a VkCommandBuffer
     */
    struct CmdBufferSectionLabel
    {
        CmdBufferSectionLabel(IVulkanCallsPtr vk, const VulkanCommandBufferPtr& cmdBuffer, const std::string& sectionName);
        ~CmdBufferSectionLabel();

        private:

            IVulkanCallsPtr m_vk;
            VkCommandBuffer m_vkCmdBuffer;
    };

    /**
     * Scoped object that annotates usage of a VkQueue
     */
    struct QueueSectionLabel
    {
            QueueSectionLabel(IVulkanCallsPtr vk, VkQueue vkQueue, const std::string& sectionName);
            ~QueueSectionLabel();

        private:

            IVulkanCallsPtr m_vk;
            VkQueue m_vkQueue;
    };
}

#endif //LIBACCELARENDERVK_SRC_VULKAN_VULKANDEBUG
