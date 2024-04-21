/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERER_RAWTRIANGLERENDERER_H
#define LIBACCELARENDERERVK_SRC_RENDERER_RAWTRIANGLERENDERER_H

#include "Renderer.h"
#include "RendererCommon.h"

#include "../ForwardDeclares.h"

#include <Accela/Render/Util/Triangle.h>

#include <vector>
#include <expected>

namespace Accela::Render
{
    /**
     * Simple forward-pass rendered which simply renders the raw list of triangles provided.
     */
    class RawTriangleRenderer : public Renderer
    {
        public:

            RawTriangleRenderer(Common::ILogger::Ptr logger,
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

            void Render(const RenderParams& renderParams,
                        const VulkanCommandBufferPtr& commandBuffer,
                        const VulkanRenderPassPtr& renderPass,
                        const VulkanFramebufferPtr& framebuffer,
                        const std::vector<ViewProjection>& viewProjections,
                        const std::vector<Triangle>& triangles);

        private:

            [[nodiscard]] std::expected<MeshId, bool> CreateTrianglesMesh(const std::vector<Triangle>& triangles);

            [[nodiscard]] bool BindGlobalDescriptorSet(const RenderParams& renderParams,
                                                       const VulkanCommandBufferPtr& commandBuffer,
                                                       const VulkanPipelinePtr& pipeline,
                                                       const std::vector<ViewProjection>& viewProjections) const;
            [[nodiscard]] bool UpdateGlobalDescriptorSet_Global(const RenderParams& renderParams,
                                                                const VulkanDescriptorSetPtr& globalDataDescriptorSet) const;
            [[nodiscard]] bool UpdateGlobalDescriptorSet_ViewProjection(const VulkanDescriptorSetPtr& globalDataDescriptorSet,
                                                                        const std::vector<ViewProjection>& viewProjections) const;

        private:

            ProgramDefPtr m_programDef;
            std::optional<std::size_t> m_pipelineHash;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERER_RAWTRIANGLERENDERER_H
