/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERER_DEFERREDLIGHTINGRENDERER_H
#define LIBACCELARENDERERVK_SRC_RENDERER_DEFERREDLIGHTINGRENDERER_H

#include "Renderer.h"
#include "BindState.h"

#include "../ForwardDeclares.h"

#include "../Texture/LoadedTexture.h"
#include "../Mesh/LoadedMesh.h"
#include "../Light/LoadedLight.h"
#include "../Util/ViewProjection.h"

#include <Accela/Render/Material/Material.h>
#include <Accela/Render/Task/RenderParams.h>

#include <expected>
#include <unordered_map>

namespace Accela::Render
{
    class DeferredLightingRenderer : public Renderer
    {
        public:

            DeferredLightingRenderer(Common::ILogger::Ptr logger,
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

            void Render(const std::string& sceneName,
                        const Material::Type& materialType,
                        const RenderParams& renderParams,
                        const VulkanCommandBufferPtr& commandBuffer,
                        const VulkanRenderPassPtr& renderPass,
                        const VulkanFramebufferPtr& framebuffer,
                        const std::vector<ViewProjection>& viewProjections,
                        const std::unordered_map<LightId, TextureId>& shadowMaps);

        private:

            struct LightingSettingPayload
            {
                alignas(4) uint32_t hdr{0};
            };

        private:

            bool CreateMesh();

            //
            // Pipeline
            //
            std::expected<VulkanPipelinePtr, bool> BindPipeline(BindState& bindState,
                                                                const VulkanCommandBufferPtr& commandBuffer,
                                                                const VulkanRenderPassPtr& renderPass,
                                                                const VulkanFramebufferPtr& framebuffer);

            //
            // DescriptorSet 0
            //
            [[nodiscard]] bool BindDescriptorSet0(const std::string& sceneName,
                                                  BindState& bindState,
                                                  const RenderParams& renderParams,
                                                  const VulkanCommandBufferPtr& commandBuffer,
                                                  const std::vector<ViewProjection>& viewProjections,
                                                  const std::unordered_map<LightId, TextureId>& shadowMaps) const;
            [[nodiscard]] bool BindDescriptorSet0_Global(const BindState& bindState,
                                                         const RenderParams& renderParams,
                                                         const VulkanDescriptorSetPtr& globalDataDescriptorSet,
                                                         const std::vector<LoadedLight>& lights) const;
            [[nodiscard]] bool BindDescriptorSet0_ViewProjection(BindState& bindState,
                                                                 const std::vector<ViewProjection>& viewProjections,
                                                                 const VulkanDescriptorSetPtr& descriptorSet) const;
            [[nodiscard]] bool BindDescriptorSet0_Lights(const BindState& bindState,
                                                         const VulkanDescriptorSetPtr& globalDataDescriptorSet,
                                                         const std::vector<LoadedLight>& lights,
                                                         const std::unordered_map<LightId, TextureId>& shadowMaps) const;

            [[nodiscard]] bool BindDescriptorSet0_ShadowMapTextures(const BindState& bindState,
                                                                    const VulkanDescriptorSetPtr& globalDataDescriptorSet,
                                                                    const std::unordered_map<ShadowMapType, std::vector<TextureId>>& shadowMapTextureIds) const;

            //
            // DescriptorSet 1
            //
            bool BindDescriptorSet1(BindState& bindState,
                                    const VulkanCommandBufferPtr& commandBuffer,
                                    const VulkanFramebufferPtr& framebuffer);

            //
            // DescriptorSet 2
            //
            [[nodiscard]] bool BindDescriptorSet2(BindState& bindState,
                                                  const Material::Type& materialType,
                                                  const VulkanCommandBufferPtr& commandBuffer);

            [[nodiscard]] bool BindDescriptorSet2_MaterialData(BindState& bindState,
                                                               const Material::Type& materialType,
                                                               const VulkanDescriptorSetPtr& materialDataDescriptorSet);

            //
            // Vertex/Index buffers
            //
            static void BindVertexBuffer(BindState& bindState,
                                         const VulkanCommandBufferPtr& commandBuffer,
                                         const BufferPtr& vertexBuffer);

            static void BindIndexBuffer(BindState& bindState,
                                        const VulkanCommandBufferPtr& commandBuffer,
                                        const BufferPtr& indexBuffer);

        private:

            ProgramDefPtr m_programDef;
            LoadedMesh m_mesh{};
            std::optional<std::size_t> m_pipelineHash;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERER_DEFERREDLIGHTINGRENDERER_H
