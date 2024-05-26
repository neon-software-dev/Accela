#ifndef LIBACCELARENDERERVK_SRC_RENDERER_TERRAINRENDERER_H
#define LIBACCELARENDERERVK_SRC_RENDERER_TERRAINRENDERER_H

#include "Renderer.h"
#include "RenderState.h"

#include "../Mesh/LoadedMesh.h"
#include "../Material/LoadedMaterial.h"
#include "../Texture/LoadedTexture.h"
#include "../Util/ViewProjection.h"

#include <Accela/Render/Task/RenderParams.h>

#include <format>
#include <expected>

namespace Accela::Render
{
    class TerrainRenderer : public Renderer
    {
        public:

            TerrainRenderer(Common::ILogger::Ptr logger,
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
                        const RenderParams& renderParams,
                        const VulkanCommandBufferPtr& commandBuffer,
                        const VulkanRenderPassPtr& renderPass,
                        const VulkanFramebufferPtr& framebuffer,
                        const std::vector<ViewProjection>& viewProjections);

        private:

            struct TerrainBatchKey
            {
                MeshId meshId;
                MaterialId materialId;
                TextureId heightMapTextureId;

                bool operator==(const TerrainBatchKey& other) const
                {
                    return  meshId == other.meshId &&
                            materialId == other.materialId &&
                            heightMapTextureId == other.heightMapTextureId;
                }

                struct HashFunction
                {
                    std::size_t operator()(const TerrainBatchKey& key) const {
                        return std::hash<std::string>{}(
                            std::format("{}-{}-{}", key.meshId.id, key.materialId.id, key.heightMapTextureId.id)
                        );
                    }
                };
            };

            struct TerrainBatch
            {
                TerrainBatchKey batchKey;
                LoadedMesh loadedMesh;
                LoadedMaterial loadedMaterial;
                LoadedTexture loadedHeightMapTexture;
                std::vector<TerrainId> terrainIds;
            };

        private:

            bool CreateTerrainMesh();

            [[nodiscard]] std::vector<TerrainBatch> CompileBatches(const std::string& sceneName) const;

            [[nodiscard]] TerrainBatchKey GetBatchKey(const TerrainRenderable& terrainRenderable) const;

            [[nodiscard]] std::expected<TerrainBatch, bool> CreateTerrainBatch(const TerrainRenderable& terrainRenderable) const;

            void RenderBatch(RenderState& renderState,
                             const TerrainBatch& terrainBatch,
                             const RenderParams& renderParams,
                             const VulkanCommandBufferPtr& commandBuffer,
                             const VulkanRenderPassPtr& renderPass,
                             const VulkanFramebufferPtr& framebuffer,
                             const std::vector<ViewProjection>& viewProjections);

            [[nodiscard]] bool BindPipeline(RenderState& renderState,
                                            const VulkanCommandBufferPtr& commandBuffer,
                                            const VulkanRenderPassPtr& renderPass,
                                            const VulkanFramebufferPtr& framebuffer);

            [[nodiscard]] std::expected<VulkanPipelinePtr, bool> GetBatchPipeline(
                const VulkanRenderPassPtr& renderPass,
                const VulkanFramebufferPtr& framebuffer);

            //
            // Descriptor Set 0 - Global Data
            //
            [[nodiscard]] bool BindDescriptorSet0(RenderState& renderState,
                                                  const RenderParams& renderParams,
                                                  const VulkanCommandBufferPtr& commandBuffer,
                                                  const std::vector<ViewProjection>& viewProjections) const;

            [[nodiscard]] bool BindDescriptorSet0_Global(const RenderState& renderState,
                                                         const RenderParams& renderParams,
                                                         const VulkanDescriptorSetPtr& globalDataDescriptorSet) const;

            [[nodiscard]] bool BindDescriptorSet0_ViewProjection(const RenderState& renderState,
                                                                 const VulkanDescriptorSetPtr& globalDataDescriptorSet,
                                                                 const std::vector<ViewProjection>& viewProjections) const;

            //
            // Descriptor Set 1 - Renderer Data
            //
            [[nodiscard]] bool BindDescriptorSet1(RenderState& renderState,
                                                  const VulkanCommandBufferPtr& commandBuffer) const;

            //
            // Descriptor Set 2 - Material Data
            //
            [[nodiscard]] bool BindDescriptorSet2(RenderState& renderState,
                                                  const TerrainBatch& terrainBatch,
                                                  const VulkanCommandBufferPtr& commandBuffer) const;

            //
            // Descriptor Set 3 - Draw Data
            //
            [[nodiscard]] bool BindDescriptorSet3(RenderState& renderState,
                                                  const TerrainBatch& terrainBatch,
                                                  const VulkanCommandBufferPtr& commandBuffer) const;
            [[nodiscard]] bool BindDescriptorSet3_DrawData(RenderState& renderState,
                                                           const TerrainBatch& terrainBatch,
                                                           const VulkanDescriptorSetPtr& drawDescriptorSet) const;

            //
            // Vertex/Index buffers
            //
            static void BindVertexBuffer(RenderState& renderState,
                                         const VulkanCommandBufferPtr& commandBuffer,
                                         const BufferPtr& vertexBuffer);

            static void BindIndexBuffer(RenderState& renderState,
                                        const VulkanCommandBufferPtr& commandBuffer,
                                        const BufferPtr& indexBuffer);

        private:

            MeshId m_terrainMeshId{Render::INVALID_ID};
            ProgramDefPtr m_programDef;
            std::optional<std::size_t> m_pipelineHash;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERER_TERRAINRENDERER_H
