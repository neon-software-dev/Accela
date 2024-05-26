#ifndef LIBACCELARENDERERVK_SRC_VULKAN_VULKANRENDERPASS
#define LIBACCELARENDERERVK_SRC_VULKAN_VULKANRENDERPASS

#include "../ForwardDeclares.h"

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

namespace Accela::Render
{
    /**
     * Wrapper for working with a render pass
     */
    class VulkanRenderPass
    {
        public:

            enum class AttachmentType
            {
                Color,
                Depth
            };

            struct Attachment
            {
                explicit Attachment(AttachmentType _type) : type(_type) {}

                AttachmentType type;
                VkAttachmentDescription description{};
            };

            struct Subpass
            {
                // The color attachments the subpass uses
                std::vector<VkAttachmentReference> colorAttachmentRefs;
                // The optional depth attachment the subpass uses
                std::optional<VkAttachmentReference> depthAttachmentRef;
                // The input attachments the subpass uses
                std::vector<VkAttachmentReference> inputAttachmentRefs;
            };

        public:

            VulkanRenderPass(Common::ILogger::Ptr logger,
                             IVulkanCallsPtr vk,
                             VulkanPhysicalDevicePtr physicalDevice,
                             VulkanDevicePtr device);

            /**
             * Create this render pass
             *
             * @param attachments Descriptions of the attachments that the render pass uses
             * @param subpasses Definition of the subpasses the render pass contains
             * @param tag A debug tag to associate with the render pass
             *
             * @return Whether the render pass was created successfully
             */
            bool Create(const std::vector<Attachment>& attachments,
                        const std::vector<Subpass>& subpasses,
                        const std::vector<VkSubpassDependency>& vkDependencies,
                        const std::optional<std::vector<uint32_t>>& multiViewMasks,
                        const std::optional<uint32_t>& multiViewCorrelationMask,
                        const std::string& tag);

            [[nodiscard]] VkRenderPass GetVkRenderPass() const { return m_vkRenderPass; }
            [[nodiscard]] std::vector<Attachment> GetAttachments() const { return m_attachments; }
            [[nodiscard]] std::vector<Subpass> GetSubpasses() const { return m_subpasses; }
            [[nodiscard]] bool HasDepthAttachment() const;

            void Destroy();

        private:

            Common::ILogger::Ptr m_logger;
            IVulkanCallsPtr m_vk;
            VulkanPhysicalDevicePtr m_physicalDevice;
            VulkanDevicePtr m_device;

            VkRenderPass m_vkRenderPass{VK_NULL_HANDLE};
            std::vector<Attachment> m_attachments;
            std::vector<Subpass> m_subpasses;
    };
}

#endif //LIBACCELARENDERERVK_SRC_VULKAN_VULKANRENDERPASS
