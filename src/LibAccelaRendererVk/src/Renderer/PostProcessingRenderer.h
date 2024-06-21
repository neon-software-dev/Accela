/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERER_POSTPROCESSINGRENDERER_H
#define LIBACCELARENDERERVK_SRC_RENDERER_POSTPROCESSINGRENDERER_H

#include "Renderer.h"

#include "../ForwardDeclares.h"

#include "../Framebuffer/FramebufferObjs.h"

#include <Accela/Render/Ids.h>
#include <Accela/Render/RenderSettings.h>

#include <Accela/Common/Log/ILogger.h>

#include <expected>
#include <cstdint>
#include <cstddef>
#include <optional>

namespace Accela::Render
{
    struct PostProcessEffect
    {
        std::string programName;
        std::vector<std::byte> pushPayload;
        std::string tag;
    };

    /**
     * Base class for standard compute-based post-processing passes
     */
    class PostProcessingRenderer : public Renderer
    {
        public:

            PostProcessingRenderer(Common::ILogger::Ptr logger,
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
                        const LoadedTexture& colorAttachment,
                        const PostProcessEffect& effect);

        private:

            [[nodiscard]] std::pair<uint32_t, uint32_t> CalculateWorkGroupSize() const;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERER_POSTPROCESSINGRENDERER_H
