/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERTARGET_RENDERTARGETS_H
#define LIBACCELARENDERERVK_SRC_RENDERTARGET_RENDERTARGETS_H

#include "IRenderTargets.h"

#include "../ForwardDeclares.h"

#include <Accela/Render/Ids.h>

#include <Accela/Common/Log/ILogger.h>

#include <unordered_map>

namespace Accela::Render
{
    class RenderTargets : public IRenderTargets
    {
        public:

            RenderTargets(Common::ILogger::Ptr logger,
                          VulkanObjsPtr vulkanObjs,
                          PostExecutionOpsPtr postExecutionOps,
                          IFramebuffersPtr framebuffers,
                          ITexturesPtr textures,
                          Ids::Ptr ids);

            //
            // IRenderTargets
            //
            [[nodiscard]] bool CreateRenderTarget(const RenderTargetId& renderTargetId, const std::string& tag) override;
            void DestroyRenderTarget(const RenderTargetId& renderTargetId, bool destroyImmediately) override;
            [[nodiscard]] std::optional<RenderTarget> GetRenderTarget(const RenderTargetId& renderTargetId) const override;
            [[nodiscard]] bool OnRenderSettingsChanged(const RenderSettings& renderSettings) override;
            void Destroy() override;

        private:

            [[nodiscard]] std::optional<FrameBufferId> CreateGPassFramebuffer(const std::string& tag) const;
            [[nodiscard]] std::optional<FrameBufferId> CreateScreenFramebuffer(const std::string& tag) const;
            [[nodiscard]] std::optional<TextureId> CreatePostProcessOutputTexture(const std::string& tag) const;

        private:

            Common::ILogger::Ptr m_logger;
            VulkanObjsPtr m_vulkanObjs;
            PostExecutionOpsPtr m_postExecutionOps;
            IFramebuffersPtr m_framebuffers;
            ITexturesPtr m_textures;
            Ids::Ptr m_ids;

            std::unordered_map<RenderTargetId, RenderTarget> m_renderTargets;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERTARGET_RENDERTARGETS_H
