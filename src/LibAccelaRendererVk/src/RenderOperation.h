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

            explicit RenderOperation(std::unordered_map<VkImage, ImageAccess> _imageAccesses);

            [[nodiscard]] std::unordered_map<VkImage, ImageAccess> GetImageAccesses() const noexcept { return m_imageAccesses; }

            [[nodiscard]] static std::optional<RenderOperation> FromRenderPass(const FramebufferObjs& frameBufferObjs, const VulkanRenderPassPtr& renderPass);

        private:

            std::unordered_map<VkImage, ImageAccess> m_imageAccesses;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDEROPERATION_H
