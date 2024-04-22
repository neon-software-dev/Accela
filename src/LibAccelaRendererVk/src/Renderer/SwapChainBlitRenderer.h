/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERER_SWAPCHAINBLITRENDERER_H
#define LIBACCELARENDERERVK_SRC_RENDERER_SWAPCHAINBLITRENDERER_H

#include "Renderer.h"

#include "../ForwardDeclares.h"

#include "../Framebuffer/FramebufferObjs.h"

#include <Accela/Render/Ids.h>
#include <Accela/Render/RenderSettings.h>

#include <Accela/Common/Log/ILogger.h>

#include <expected>
#include <cstdint>
#include <optional>

namespace Accela::Render
{
    class SwapChainBlitRenderer : public Renderer
    {
        public:

            SwapChainBlitRenderer(Common::ILogger::Ptr logger,
                                  Common::IMetrics::Ptr metrics,
                                  Ids::Ptr ids,
                                  PostExecutionOpsPtr postExecutionOps,
                                  VulkanObjsPtr vulkanObjs,
                                  IProgramsPtr programs,
                                  IShadersPtr shaders,
                                  IPipelineFactoryPtr pipelines,
                                  IBuffersPtr buffers,
                                  IMaterialsPtr materials,
                                  ITexturesPtr textures,
                                  IMeshesPtr meshes,
                                  ILightsPtr lights,
                                  IRenderablesPtr renderables,
                                  uint8_t frameIndex);

            bool Initialize(const RenderSettings& renderSettings) override;
            void Destroy() override;

            void Render(const VulkanCommandBufferPtr& commandBuffer,
                        const VulkanRenderPassPtr& renderPass,
                        const VulkanFramebufferPtr& swapChainFramebuffer,
                        const LoadedTexture& offscreenColorAttachment);

        private:

            bool ConfigureMeshFor(const RenderSettings& renderSettings, const USize& targetSize);

        private:

            ProgramDefPtr m_programDef;
            VulkanDescriptorSetPtr m_descriptorSet;

            MeshId m_meshId{MeshId(INVALID_ID)};
            std::optional<USize> m_renderSize;
            std::optional<USize> m_targetSize;
            std::optional<std::size_t> m_pipelineHash;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERER_SWAPCHAINBLITRENDERER_H
