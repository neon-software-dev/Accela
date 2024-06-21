/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_FRAMEBUFFER_FRAMEBUFFEROBJS_H
#define LIBACCELARENDERERVK_SRC_FRAMEBUFFER_FRAMEBUFFEROBJS_H

#include "../ForwardDeclares.h"

#include "../Texture/LoadedTexture.h"

#include "../Vulkan/VulkanRenderPass.h"

#include <Accela/Render/Ids.h>
#include <Accela/Render/Util/Rect.h>
#include <Accela/Render/Texture/TextureDefinition.h>

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

            FramebufferObjs(Common::ILogger::Ptr logger, Ids::Ptr ids, VulkanObjsPtr vulkanObjs, ITexturesPtr textures);

            bool CreateOwning(const VulkanRenderPassPtr& renderPass,
                              const std::vector<std::pair<TextureDefinition, std::string>>& attachments,
                              const USize& size,
                              const uint32_t& layers,
                              const std::string& tag);

            bool CreateFromExisting(const VulkanRenderPassPtr& renderPass,
                                    const std::vector<std::pair<TextureId, std::string>>& attachmentTextureViews,
                                    const USize& size,
                                    const uint32_t& layers,
                                    const std::string& tag);

            bool CreateFromExistingDefaultViews(const VulkanRenderPassPtr& renderPass,
                                                const std::vector<TextureId>& attachmentTextures,
                                                const USize& size,
                                                const uint32_t& layers,
                                                const std::string& tag);

            void Destroy();

            [[nodiscard]] VulkanFramebufferPtr GetFramebuffer() const noexcept { return m_framebuffer; }
            [[nodiscard]] std::size_t GetNumAttachments() const noexcept { return m_attachmentTextureViews.size(); }
            [[nodiscard]] std::optional<std::vector<std::pair<LoadedTexture, std::string>>> GetAttachmentTextures() const;
            [[nodiscard]] std::optional<std::pair<LoadedTexture, std::string>> GetAttachmentTexture(uint8_t attachmentIndex) const;

        private:

            Common::ILogger::Ptr m_logger;
            Ids::Ptr m_ids;
            VulkanObjsPtr m_vulkanObjs;
            ITexturesPtr m_textures;

            bool m_ownsAttachments{false};
            std::vector<std::pair<TextureId, std::string>> m_attachmentTextureViews;
            VulkanFramebufferPtr m_framebuffer;
    };
}

#endif //LIBACCELARENDERERVK_SRC_FRAMEBUFFER_FRAMEBUFFEROBJS_H
