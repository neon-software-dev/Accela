/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_FRAMEBUFFER_FRAMEBUFFERS_H
#define LIBACCELARENDERERVK_SRC_FRAMEBUFFER_FRAMEBUFFERS_H

#include "IFramebuffers.h"

#include "../ForwardDeclares.h"

#include <Accela/Render/Ids.h>

#include <Accela/Common/Log/ILogger.h>

#include <unordered_map>

namespace Accela::Render
{
    class Framebuffers : public IFramebuffers
    {
        public:

            Framebuffers(Common::ILogger::Ptr logger,
                         Ids::Ptr ids,
                         VulkanObjsPtr vulkanObjs,
                         IImagesPtr images,
                         PostExecutionOpsPtr postExecutionOps);

            void Destroy() override;

            bool CreateFramebuffer(FrameBufferId id,
                                   const VulkanRenderPassPtr& renderPass,
                                   const std::vector<std::pair<ImageDefinition, std::string>>& attachments,
                                   const USize& size,
                                   const uint32_t& layers,
                                   const std::string& tag) override;

            bool CreateFramebuffer(FrameBufferId id,
                                   const VulkanRenderPassPtr& renderPass,
                                   const std::vector<std::pair<ImageId, std::string>>& attachmentImageViews,
                                   const USize& size,
                                   const uint32_t& layers,
                                   const std::string& tag) override;

            [[nodiscard]] std::optional<FramebufferObjs> GetFramebufferObjs(FrameBufferId frameBufferId) override;

            void DestroyFramebuffer(FrameBufferId frameBufferId, bool destroyImmediately) override;

        private:

            void DestroyFramebufferObjects(FrameBufferId frameBufferId, FramebufferObjs framebufferObjs);

        private:

            Common::ILogger::Ptr m_logger;
            Ids::Ptr m_ids;
            VulkanObjsPtr m_vulkanObjs;
            IImagesPtr m_images;
            PostExecutionOpsPtr m_postExecutionOps;

            std::unordered_map<FrameBufferId, FramebufferObjs> m_framebuffers;
    };
}

#endif //LIBACCELARENDERERVK_SRC_FRAMEBUFFER_FRAMEBUFFERS_H
