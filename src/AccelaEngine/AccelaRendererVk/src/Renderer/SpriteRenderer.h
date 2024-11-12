/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERER_SPRITERENDERER_H
#define LIBACCELARENDERERVK_SRC_RENDERER_SPRITERENDERER_H

#include "Renderer.h"
#include "RendererCommon.h"

#include "../Mesh/LoadedMesh.h"
#include "../Buffer/ItemBuffer.h"
#include "../Texture/LoadedTexture.h"
#include "../Image/LoadedImage.h"

#include <Accela/Render/Task/RenderParams.h>

#include <unordered_map>
#include <vector>

namespace Accela::Render
{
    /**
     * Sprites are rendered as a quad, with the sprite positioned such that the center of the sprite's final size
     * corresponds to the sprite position (the origin is in the center of the sprite, as opposed to a corner).
     *
     * Rendering is done in Vulkan's right-hand coordinate system, with 0,0 being the top left of the
     * screen, x-axis increasing to the right, y-axis increasing downwards, and z-axis increasing into
     * the screen.
     *
     * A sprite's z-position must fall within the range of [0.0-1.0] to be visible.
     */
    class SpriteRenderer : public Renderer
    {
        public:

            SpriteRenderer(Common::ILogger::Ptr logger,
                           Common::IMetrics::Ptr metrics,
                           Ids::Ptr ids,
                           PostExecutionOpsPtr postExecutionOps,
                           VulkanObjsPtr vulkanObjs,
                           IProgramsPtr programs,
                           IShadersPtr shaders,
                           IPipelineFactoryPtr pipelines,
                           IBuffersPtr buffers,
                           IMaterialsPtr materials,
                           IImagesPtr images,
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
                        const VulkanFramebufferPtr& framebuffer);

        private:

            struct SpriteBatch
            {
                TextureId textureId{INVALID_ID};
                std::vector<SpriteId> spriteIds;
            };

        private:

            bool CreateSpriteMesh();

            std::optional<VulkanDescriptorSetPtr> UpdateGlobalDescriptorSet(const RenderParams& renderParams);
            bool UpdateGlobalDescriptorSet_Global(const VulkanDescriptorSetPtr& descriptorSet);
            bool UpdateGlobalDescriptorSet_ViewProjection(const RenderParams& renderParams,
                                                          const VulkanDescriptorSetPtr& descriptorSet);
            std::optional<VulkanDescriptorSetPtr> UpdateRendererDescriptorSet();
            std::optional<VulkanDescriptorSetPtr> UpdateMaterialDescriptorSet(const std::pair<LoadedTexture, LoadedImage>& loadedTexture);
            std::optional<VulkanDescriptorSetPtr> UpdateDrawDescriptorSet(const SpriteBatch& spriteBatch);

            [[nodiscard]] std::unordered_map<TextureId, SpriteBatch> CompileSpriteBatches(const std::string& sceneName) const;

            void RenderBatch(const LoadedMesh& spriteMesh,
                             const SpriteBatch& spriteBatch,
                             const VulkanPipelinePtr& pipeline,
                             const VulkanCommandBufferPtr& commandBuffer);

        private:

            MeshId m_spriteMeshId{INVALID_ID};
            ProgramDefPtr m_programDef;
            std::optional<std::size_t> m_pipelineHash;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERER_SPRITERENDERER_H
