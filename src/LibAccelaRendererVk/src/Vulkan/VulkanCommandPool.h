#ifndef LIBACCELARENDERERVK_SRC_VULKAN_VULKANCOMMANDPOOL
#define LIBACCELARENDERERVK_SRC_VULKAN_VULKANCOMMANDPOOL

#include "../ForwardDeclares.h"

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <set>
#include <optional>
#include <string>

namespace Accela::Render
{
    /**
     * Wrapper for working with Vulkan command pools
     */
    class VulkanCommandPool
    {
        public:

            // The valid types of command buffers
            enum class CommandBufferType
            {
                Primary,
                Secondary
            };

        public:

            VulkanCommandPool(Common::ILogger::Ptr logger, IVulkanCallsPtr vk, VulkanDevicePtr device);

            /**
             * Create a new command pool
             *
             * @param queueFamilyIndex The index of the device queue family to use
             * @param flags Flags that describe how the command pool should operate
             * @param tag A debug tag to associate with this command pool
             *
             * @return Whether the command pool was created successfully
             */
            bool Create(
                const uint32_t& queueFamilyIndex,
                const VkCommandPoolCreateFlags& flags,
                const std::string& tag);

            /**
             * Destroys this pool and frees any resources associated with the pool or outstanding command
             * buffers created from it
             */
            void Destroy();

            /**
             * @return The underlying VkCommandPool associated with this command pool
             */
            [[nodiscard]] VkCommandPool GetVkCommandPool() const { return m_vkCommandPool; }

            /**
             * Allocate a command buffer from this command pool
             *
             * @param type The type of command buffer to allocate
             * @param tag A debug tag to associate with the command buffer
             *
             * @return The VulkanCommandBuffer that was allocated, or std::nullopt on error
             */
            [[nodiscard]] std::optional<VulkanCommandBufferPtr> AllocateCommandBuffer(CommandBufferType type, const std::string& tag);

            /**
             * Frees a previously allocated VkCommandBuffer. Memory used by it is reclaimed.
             *
             * @param commandBuffer The command buffer to be freed
             */
            void FreeCommandBuffer(const VulkanCommandBufferPtr& commandBuffer);

            /**
             * Resets a previously allocated VkCommandBuffer. This pool must have been
             * created with the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag.
             *
             * @param commandBuffer The command buffer to be reset
             * @param trimMemory Whether to forcefully free memory previously used by the command buffer. If true, the
             * command buffer will release its memory and allocate new memory for recording commands. If false, the
             * command buffer will reuse its previously allocated memory for recording commands.
             */
            void ResetCommandBuffer(const VulkanCommandBufferPtr& commandBuffer, bool trimMemory);

            /**
             * Resets all command buffers associated with this pool, but does not free them.
             *
             * @param trimMemory Whether memory previously associated with the pool should be given back to
             * the system and a new allocation created, or the previous allocation reused.
             */
            void ResetPool(bool trimMemory);

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vk;
            VulkanDevicePtr m_device;

            VkCommandPool m_vkCommandPool{VK_NULL_HANDLE};
            VkCommandPoolCreateFlags m_createFlags{0};

            std::set<VulkanCommandBufferPtr> m_allocatedBuffers;
    };
}

#endif //LIBACCELARENDERERVK_SRC_VULKAN_VULKANCOMMANDPOOL
