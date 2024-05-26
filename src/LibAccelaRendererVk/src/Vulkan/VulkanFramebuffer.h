#ifndef LIBACCELARENDERERVK_SRC_VULKAN_VULKANFRAMEBUFFER
#define LIBACCELARENDERERVK_SRC_VULKAN_VULKANFRAMEBUFFER

#include "../ForwardDeclares.h"

#include <Accela/Render/Util/Rect.h>

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <optional>

namespace Accela::Render
{
    /**
     * Wrapper for working with a framebuffer
     */
    class VulkanFramebuffer
    {
        public:

            VulkanFramebuffer(Common::ILogger::Ptr logger,
                              IVulkanCallsPtr vk,
                              VulkanDevicePtr device);

            /**
             * Create this framebuffer object
             *
             * @param compatibleRenderPass A render pass defining which render passes the framebuffer is compatible with
             * @param attachments The *index ordered* image view attachments that make up the framebuffer
             * @param size The dimensions of the framebuffer (dimensions of the attached images)
             * @param tag A debug tag to associate with the framebuffer
             *
             * @return Whether the framebuffer was created successfully
             */
            bool Create(const VulkanRenderPassPtr& compatibleRenderPass,
                        const std::vector<VkImageView>& attachments,
                        const USize& size,
                        const uint32_t& layers,
                        const std::string& tag);

            [[nodiscard]] std::optional<USize> GetSize() const noexcept { return m_size; }
            [[nodiscard]] VkFramebuffer GetVkFramebuffer() const noexcept { return m_vkFrameBuffer; }
            [[nodiscard]] std::vector<VkImageView> GetAttachments() const noexcept { return m_attachments; }

            void Destroy();

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vk;
            VulkanDevicePtr m_device;

            std::vector<VkImageView> m_attachments;
            std::optional<USize> m_size;
            VkFramebuffer m_vkFrameBuffer{VK_NULL_HANDLE};
    };
}

#endif //LIBACCELARENDERERVK_SRC_VULKAN_VULKANFRAMEBUFFER
