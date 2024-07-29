/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_FRAMEBUFFER_FRAMEBUFFEROBJS_H
#define LIBACCELARENDERERVK_SRC_FRAMEBUFFER_FRAMEBUFFEROBJS_H

#include "../ForwardDeclares.h"

#include "../Image/LoadedImage.h"
#include "../Image/ImageDefinition.h"

#include "../Vulkan/VulkanRenderPass.h"

#include <Accela/Render/Ids.h>
#include <Accela/Render/Util/Rect.h>

#include <Accela/Common/Log/ILogger.h>

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>
#include <utility>

namespace Accela::Render
{
    class FramebufferObjs
    {
        public:

            FramebufferObjs(Common::ILogger::Ptr logger, Ids::Ptr ids, VulkanObjsPtr vulkanObjs, IImagesPtr images);

            bool CreateOwning(const VulkanRenderPassPtr& renderPass,
                              const std::vector<std::pair<ImageDefinition, ImageViewName>>& attachments,
                              const USize& size,
                              const uint32_t& layers,
                              const std::string& tag);

            bool CreateFromExisting(const VulkanRenderPassPtr& renderPass,
                                    const std::vector<std::pair<ImageId, ImageViewName>>& attachmentImageViews,
                                    const USize& size,
                                    const uint32_t& layers,
                                    const std::string& tag);

            bool CreateFromExistingDefaultViews(const VulkanRenderPassPtr& renderPass,
                                                const std::vector<ImageId>& attachmentImages,
                                                const USize& size,
                                                const uint32_t& layers,
                                                const std::string& tag);

            void Destroy();

            [[nodiscard]] VulkanFramebufferPtr GetFramebuffer() const noexcept { return m_framebuffer; }
            [[nodiscard]] std::size_t GetNumAttachments() const noexcept { return m_attachmentImageViews.size(); }
            [[nodiscard]] std::optional<std::vector<std::pair<LoadedImage, ImageViewName>>> GetAttachmentImages() const;
            [[nodiscard]] std::optional<std::pair<LoadedImage, ImageViewName>> GetAttachmentImage(uint8_t attachmentIndex) const;

        private:

            Common::ILogger::Ptr m_logger;
            Ids::Ptr m_ids;
            VulkanObjsPtr m_vulkanObjs;
            IImagesPtr m_images;

            bool m_ownsAttachments{false};
            std::vector<std::pair<ImageId, ImageViewName>> m_attachmentImageViews;
            VulkanFramebufferPtr m_framebuffer;
    };
}

#endif //LIBACCELARENDERERVK_SRC_FRAMEBUFFER_FRAMEBUFFEROBJS_H
