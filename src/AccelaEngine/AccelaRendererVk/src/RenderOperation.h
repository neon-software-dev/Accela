/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDEROPERATION_H
#define LIBACCELARENDERERVK_SRC_RENDEROPERATION_H

#include "ForwardDeclares.h"
#include "Framebuffer/FramebufferObjs.h"

#include "Util/Synchronization.h"

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <vector>

namespace Accela::Render
{

    class RenderOperation
    {
        public:

            explicit RenderOperation(std::unordered_map<ImageId, ImageAccess> _imageAccesses);

            [[nodiscard]] std::unordered_map<ImageId, ImageAccess> GetImageAccesses() const noexcept { return m_imageAccesses; }

            /**
             * Creates a RenderOperation for starting a render pass. The operation, when given to RenderState, will
             * transition attachment image layouts as needed to meet the Render Pass's initial layout requirements, and
             * will insert barriers as needed to synchronize attachment usage with any previous usage.
             *
             * Warning: This only prepares/synchronizes for the images associated with the render pass / framebuffer
             * attachments. If the render pass internally samples from or otherwise uses images that aren't attachments,
             * make sure to prepare image access operations for them as well.
             */
            [[nodiscard]] static std::optional<RenderOperation> FromRenderPass(const FramebufferObjs& frameBufferObjs, const VulkanRenderPassPtr& renderPass);

        private:

            std::unordered_map<ImageId, ImageAccess> m_imageAccesses;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDEROPERATION_H
