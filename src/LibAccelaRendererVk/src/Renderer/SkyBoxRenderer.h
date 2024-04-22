/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERER_SKYBOXRENDERER_H
#define LIBACCELARENDERERVK_SRC_RENDERER_SKYBOXRENDERER_H

#include "Renderer.h"

#include "../Mesh/LoadedMesh.h"
#include "../Util/ViewProjection.h"

#include <Accela/Render/Task/RenderParams.h>

#include <optional>
#include <vector>

namespace Accela::Render
{
    class SkyBoxRenderer : public Renderer
    {
        public:

            SkyBoxRenderer(Common::ILogger::Ptr logger,
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
                        const std::vector<ViewProjection>& viewProjections);

        private:

            bool CreateSkyBoxMesh();

            void BindMeshData(const VulkanCommandBufferPtr& commandBuffer) const;

            [[nodiscard]] bool BindGlobalDescriptorSet(const RenderParams& renderParams,
                                                       const VulkanCommandBufferPtr& commandBuffer,
                                                       const VulkanPipelinePtr& pipeline,
                                                       const std::vector<ViewProjection>& viewProjections) const;
            [[nodiscard]] bool UpdateGlobalDescriptorSet_Global(const RenderParams& renderParams,
                                                                const VulkanDescriptorSetPtr& globalDataDescriptorSet) const;
            [[nodiscard]] bool UpdateGlobalDescriptorSet_ViewProjection(const VulkanDescriptorSetPtr& globalDataDescriptorSet,
                                                                        const std::vector<ViewProjection>& viewProjections) const;

            [[nodiscard]] bool BindMaterialDescriptorSet(const RenderParams& renderParams,
                                                         const VulkanCommandBufferPtr& commandBuffer,
                                                         const VulkanPipelinePtr& pipeline) const;

        private:

            LoadedMesh m_skyBoxMesh{};
            ProgramDefPtr m_programDef;
            std::optional<std::size_t> m_pipelineHash;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERER_SKYBOXRENDERER_H
