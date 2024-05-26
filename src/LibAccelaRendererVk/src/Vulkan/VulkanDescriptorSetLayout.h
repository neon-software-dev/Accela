#ifndef LIBACCELARENDERVK_SRC_VULKAN_VULKANDESCRIPTORSETLAYOUT_H
#define LIBACCELARENDERVK_SRC_VULKAN_VULKANDESCRIPTORSETLAYOUT_H

#include "../ForwardDeclares.h"

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <vector>
#include <string>

namespace Accela::Render
{
    /**
     * Wrapper for working with descriptor set layouts
     */
    class VulkanDescriptorSetLayout
    {
        public:

            /**
             * Encapsulates the details of a binding point within a descriptor set
             */
            struct BindingDetails
            {
                uint32_t descriptorSet{0};          // The descriptor set index of the binding
                uint32_t binding{0};                // The binding index within the descriptor set
                std::string name{};                 // The variable name of the binding in the material's shader
                VkDescriptorType descriptorType{};  // The type of descriptor that can be bound here
                VkShaderStageFlags stageFlags{};    // The module stage flags that can use the binding
                uint32_t descriptorCount;           // Descriptor count; usually 1, but larger for arrays
            };

        public:

            VulkanDescriptorSetLayout(Common::ILogger::Ptr logger, IVulkanCallsPtr vk, VulkanDevicePtr device);

            /**
             * Create a descriptor set layout
             *
             * @param bindings The details of the descriptor set's bindings
             * @param tag A debug name to associate with the descriptor set layout
             *
             * @return Whether the descriptor set was successfully created
             */
            bool Create(const std::vector<BindingDetails>& bindings, const std::string& tag);

            /**
             * Destroy this descriptor set layout
             */
            void Destroy();

            /**
             * @return The VkDescriptorSetLayout object associated with this descriptor set layout
             */
            [[nodiscard]] VkDescriptorSetLayout GetVkDescriptorSetLayout() const { return m_vkDescriptorSetLayout; }

            /**
             * @return The binding details associated with this descriptor set layout
             */
            [[nodiscard]] std::vector<BindingDetails> GetBindingDetails() const { return m_bindingDetails; }

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vk;
            VulkanDevicePtr m_device;

            VkDescriptorSetLayout m_vkDescriptorSetLayout{VK_NULL_HANDLE};
            std::vector<BindingDetails> m_bindingDetails;
    };
}

#endif //LIBACCELARENDERVK_SRC_VULKAN_VULKANDESCRIPTORSETLAYOUT_H
