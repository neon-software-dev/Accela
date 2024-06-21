/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERER_OBJECTRENDERER_H
#define LIBACCELARENDERERVK_SRC_RENDERER_OBJECTRENDERER_H

#include "Renderer.h"
#include "RendererCommon.h"
#include "BindState.h"

#include "../InternalId.h"

#include "../Mesh/LoadedMesh.h"
#include "../Material/LoadedMaterial.h"
#include "../Util/ViewProjection.h"
#include "../Renderables/RenderableData.h"

#include <Accela/Render/Task/RenderParams.h>

#include <expected>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <utility>
#include <algorithm>
#include <functional>

namespace Accela::Render
{
    class ObjectRenderer : public Renderer
    {
        public:

            struct ShadowRenderData
            {
                explicit ShadowRenderData(float _lightMaxAffectRange)
                    : lightMaxAffectRange(_lightMaxAffectRange)
                { }

                float lightMaxAffectRange;
            };

        public:

            ObjectRenderer(Common::ILogger::Ptr logger,
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
                        const RenderType& renderType,
                        const RenderParams& renderParams,
                        const VulkanCommandBufferPtr& commandBuffer,
                        const VulkanRenderPassPtr& renderPass,
                        const VulkanFramebufferPtr& framebuffer,
                        const std::vector<ViewProjection>& viewProjections,
                        const std::unordered_map<LightId, TextureId>& shadowMaps,
                        const std::optional<ShadowRenderData>& shadowRenderData);

        private:

            struct ObjectDrawBatchParams
            {
                LoadedMesh loadedMesh;
            };

            // A draw batch contains all objects which can be drawn with the same draw call
            struct ObjectDrawBatch
            {
                using Key = std::size_t;

                Key key;
                ObjectDrawBatchParams params;
                std::vector<ObjectRenderable> objects;
            };

            struct ObjectRenderBatchParams
            {
                ProgramDefPtr programDef;
                LoadedMaterial loadedMaterial;
                std::optional<DataBufferPtr> meshDataBuffer;
            };

            // A render batch contains all objects which can be drawn with the same DS data bound
            struct ObjectRenderBatch
            {
                using Key = std::size_t;

                Key key;
                ObjectRenderBatchParams params;
                std::vector<ObjectDrawBatch> drawBatches;
            };

            struct RenderMetrics
            {
                std::size_t numObjectRendered{0};
                std::size_t numDrawCalls{0};
            };

        private:

            //
            // Batch creation
            //

            [[nodiscard]] std::vector<ObjectRenderBatch> CompileRenderBatches(const std::string& sceneName,
                                                                              const RenderType& renderType,
                                                                              const std::vector<ViewProjection>& viewProjections) const;

            [[nodiscard]] std::vector<ObjectRenderable> GetObjectsToRender(const std::string& sceneName,
                                                                           const RenderType& renderType,
                                                                           const std::vector<ViewProjection>& viewProjections) const;

            [[nodiscard]] std::vector<ObjectRenderBatch> ObjectsToRenderBatches(const RenderType& renderType,
                                                                                const std::vector<ObjectRenderable>& objects) const;

            static void AddObjectToRenderBatch(const ObjectRenderable& object,
                                               const ObjectDrawBatch::Key& drawBatchKey,
                                               const ObjectDrawBatchParams& drawBatchParams,
                                               ObjectRenderBatch& renderBatch);

            [[nodiscard]] static ObjectRenderBatch CreateRenderBatch(const ObjectRenderable& object,
                                                                     const ObjectDrawBatch::Key& drawBatchKey,
                                                                     const ObjectDrawBatchParams& drawBatchParams,
                                                                     const ObjectRenderBatch::Key& renderBatchKey,
                                                                     const ObjectRenderBatchParams& renderBatchParams);

            [[nodiscard]] std::expected<ProgramDefPtr, bool> GetMeshProgramDef(const RenderType& renderType,
                                                                              const LoadedMesh& loadedMesh) const;

            [[nodiscard]] std::expected<ObjectDrawBatchParams, bool> GetDrawBatchParams(const ObjectRenderable& object) const;
            [[nodiscard]] std::expected<ObjectRenderBatchParams, bool> GetRenderBatchParams(const RenderType& renderType,
                                                                                           const ObjectRenderable& object) const;

            [[nodiscard]] static ObjectDrawBatch::Key GetBatchKey(const ObjectDrawBatchParams& params);
            [[nodiscard]] static ObjectRenderBatch::Key GetBatchKey(const ObjectRenderBatchParams& params);

            //
            // Rendering
            //

            void RenderBatch(const std::string& sceneName,
                             BindState& bindState,
                             RenderMetrics& renderMetrics,
                             const RenderType& renderType,
                             const ObjectRenderBatch& renderBatch,
                             const RenderParams& renderParams,
                             const VulkanCommandBufferPtr& commandBuffer,
                             const VulkanRenderPassPtr& renderPass,
                             const VulkanFramebufferPtr& framebuffer,
                             const std::vector<ViewProjection>& viewProjections,
                             const std::unordered_map<LightId, TextureId>& shadowMaps,
                             const std::optional<ShadowRenderData>& shadowRenderData);

            //
            // Pipeline
            //
            [[nodiscard]] bool BindPipeline(BindState& bindState,
                                            const RenderType& renderType,
                                            const ObjectRenderBatch& renderBatch,
                                            const VulkanCommandBufferPtr& commandBuffer,
                                            const VulkanRenderPassPtr& renderPass,
                                            const VulkanFramebufferPtr& framebuffer,
                                            const std::optional<ShadowRenderData>& shadowRenderData);

            [[nodiscard]] bool BindPushConstants(BindState& bindState,
                                                 const RenderType& renderType,
                                                 const VulkanCommandBufferPtr& commandBuffer,
                                                 const std::optional<ShadowRenderData>& shadowRenderData) const;

            //
            // DescriptorSet 0
            //
            [[nodiscard]] bool BindDescriptorSet0(const std::string& sceneName,
                                                  BindState& bindState,
                                                  const RenderType& renderType,
                                                  const RenderParams& renderParams,
                                                  const VulkanCommandBufferPtr& commandBuffer,
                                                  const std::vector<ViewProjection>& viewProjections,
                                                  const std::unordered_map<LightId, TextureId>& shadowMaps);

            [[nodiscard]] bool BindDescriptorSet0_Global(BindState& bindState,
                                                         const RenderParams& renderParams,
                                                         const VulkanDescriptorSetPtr& descriptorSet,
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
            [[nodiscard]] bool BindDescriptorSet1(BindState& bindState,
                                                  const VulkanCommandBufferPtr& commandBuffer);

            void BindDescriptorSet1_RendererData(const BindState& bindState,
                                                 const VulkanDescriptorSetPtr& descriptorSet) const;

            //
            // DescriptorSet 2
            //
            [[nodiscard]] bool BindDescriptorSet2(BindState& bindState,
                                                  const ObjectRenderBatch& renderBatch,
                                                  const VulkanCommandBufferPtr& commandBuffer);

            void BindDescriptorSet2_MaterialData(BindState& bindState,
                                                 const ObjectRenderBatch& renderBatch,
                                                 const VulkanDescriptorSetPtr& descriptorSet);

            //
            // DescriptorSet 3
            //
            [[nodiscard]] bool BindDescriptorSet3(BindState& bindState,
                                                  const ObjectRenderBatch& renderBatch,
                                                  const VulkanCommandBufferPtr& commandBuffer) const;

            [[nodiscard]] bool BindDescriptorSet3_DrawData(const BindState& bindState,
                                                           const ObjectRenderBatch& renderBatch,
                                                           const VulkanDescriptorSetPtr& drawDescriptorSet,
                                                           const BufferId& batchMeshDataBufferId) const;

            static void BindDescriptorSet3_MeshData(const BindState& bindState,
                                                    const ObjectRenderBatch& renderBatch,
                                                    const VulkanDescriptorSetPtr& drawDescriptorSet);

            [[nodiscard]] bool BindDescriptorSet3_BoneData(const BindState& bindState,
                                                           const ObjectRenderBatch& renderBatch,
                                                           const VulkanDescriptorSetPtr& drawDescriptorSet) const;

            //
            // Vertex/Index buffers
            //
            static void BindVertexBuffer(BindState& bindState,
                                         const VulkanCommandBufferPtr& commandBuffer,
                                         const BufferPtr& vertexBuffer);

            static void BindIndexBuffer(BindState& bindState,
                                        const VulkanCommandBufferPtr& commandBuffer,
                                        const BufferPtr& indexBuffer);

            [[nodiscard]] std::expected<VulkanPipelinePtr, bool> GetBatchPipeline(
                const ObjectRenderBatch& renderBatch,
                const RenderType& renderType,
                const VulkanRenderPassPtr& renderPass,
                const VulkanFramebufferPtr& framebuffer);

        private:

            struct ShadowLayerIndexPayload
            {
                alignas(4) float lightMaxAffectRange{0.0f};
            };

            struct LightingSettingPayload
            {
                alignas(4) uint32_t hdr{0};
            };

        private:

            static std::function<bool(const ObjectRenderBatch&, const ObjectRenderBatch&)> BatchSortFunc;

            std::unordered_map<std::string, std::size_t> m_programPipelineHashes;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERER_OBJECTRENDERER_H
