/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_FRAMEBUFFER_IFRAMEBUFFERS_H
#define LIBACCELARENDERERVK_SRC_FRAMEBUFFER_IFRAMEBUFFERS_H

#include "FramebufferObjs.h"

#include "../ForwardDeclares.h"

#include <Accela/Render/Id.h>
#include <Accela/Render/Util/Rect.h>
#include <Accela/Render/Texture/TextureDefinition.h>

#include <string>
#include <optional>
#include <vector>
#include <utility>

namespace Accela::Render
{
    class IFramebuffers
    {
        public:

            virtual ~IFramebuffers() = default;

            virtual void Destroy() = 0;

            virtual bool CreateFramebuffer(FrameBufferId id,
                                           const VulkanRenderPassPtr& renderPass,
                                           const std::vector<std::pair<TextureDefinition, std::string>>& attachments,
                                           const USize& size,
                                           const uint32_t& layers,
                                           const std::string& tag) = 0;

            virtual bool CreateFramebuffer(FrameBufferId id,
                                           const VulkanRenderPassPtr& renderPass,
                                           const std::vector<std::pair<TextureId, std::string>>& attachmentTextureViews,
                                           const USize& size,
                                           const uint32_t& layers,
                                           const std::string& tag) = 0;

            [[nodiscard]] virtual std::optional<FramebufferObjs> GetFramebufferObjs(FrameBufferId frameBufferId) = 0;

            virtual void DestroyFramebuffer(FrameBufferId id, bool destroyImmediately) = 0;
    };
}

#endif //LIBACCELARENDERERVK_SRC_FRAMEBUFFER_IFRAMEBUFFERS_H
