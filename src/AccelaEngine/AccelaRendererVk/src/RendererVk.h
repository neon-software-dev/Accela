/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDERER_RENDERERSDL_H
#define LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDERER_RENDERERSDL_H

#include "ForwardDeclares.h"
#include "Frames.h"
#include "RenderState.h"

#include "Renderer/RendererGroup.h"
#include "Renderer/SwapChainBlitRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Renderer/ObjectRenderer.h"
#include "Renderer/TerrainRenderer.h"
#include "Renderer/SkyBoxRenderer.h"
#include "Renderer/DeferredLightingRenderer.h"
#include "Renderer/RawTriangleRenderer.h"
#include "Renderer/PostProcessingRenderer.h"

#include "RenderTarget/RenderTarget.h"

#include <Accela/Render/Id.h>
#include <Accela/Render/PresentConfig.h>
#include <Accela/Render/RendererBase.h>
#include <Accela/Render/IOpenXR.h>
#include <Accela/Render/Task/RenderParams.h>

#include <vector>

namespace Accela::Render
{
    class RendererVk : public RendererBase
    {
        public:

            RendererVk(std::string appName,
                       uint32_t appVersion,
                       Common::ILogger::Ptr logger,
                       Common::IMetrics::Ptr metrics,
                       IVulkanCallsPtr vulkanCalls,
                       IVulkanContextPtr vulkanContext,
                       IOpenXR::Ptr openXR);

            [[nodiscard]] std::optional<ObjectId> GetTopObjectAtRenderPoint(const glm::vec2& renderPoint) const override;

        protected:

            void OnIdle() override;

            bool OnInitialize(const RenderInit& renderInit, const RenderSettings& renderSettings) override;
            bool OnShutdown() override;
            bool OnRenderFrame(RenderGraph::Ptr renderGraph) override;
            void OnCreateTexture(std::promise<bool> resultPromise,
                                 const Texture& texture,
                                 const TextureView& textureView,
                                 const TextureSampler& textureSampler) override;
            void OnUpdateTexture(std::promise<bool> resultPromise,
                                 const TextureId& textureId,
                                 const Common::ImageData::Ptr& imageData) override;
            bool OnDestroyTexture(TextureId textureId) override;
            bool OnCreateMesh(std::promise<bool> resultPromise,
                              const Mesh::Ptr& mesh,
                              MeshUsage meshUsage) override;
            bool OnDestroyMesh(MeshId meshId) override;
            bool OnCreateMaterial(std::promise<bool> resultPromise,
                                  const Material::Ptr& material) override;
            bool OnDestroyMaterial(MaterialId materialId) override;
            bool OnCreateRenderTarget(RenderTargetId renderTargetId, const std::string& tag) override;
            bool OnDestroyRenderTarget(RenderTargetId renderTargetId) override;
            bool OnWorldUpdate(const WorldUpdate& update) override;
            bool OnSurfaceChanged() override;
            bool OnChangeRenderSettings(const RenderSettings& renderSettings) override;

        private:

            bool LoadShaders(const std::vector<ShaderSpec>& shaders);
            bool CreatePrograms();

            bool RenderGraphFunc_RenderScene(const RenderGraphNode::Ptr& node);
            bool RenderGraphFunc_Present(const uint32_t& swapChainImageIndex, const RenderGraphNode::Ptr& node);

            bool StartRenderPass(const VulkanRenderPassPtr& renderPass,
                                 const FramebufferObjs& framebufferObjs,
                                 const VulkanCommandBufferPtr& commandBuffer,
                                 const glm::vec4& colorClearColor = {0,0,0,0});
            bool StartRenderPass(const VulkanRenderPassPtr& renderPass,
                                 const VulkanFramebufferPtr& framebuffer,
                                 const VulkanCommandBufferPtr& commandBuffer,
                                 const glm::vec4& colorClearColor = {0,0,0,0});
            void EndRenderPass(const VulkanCommandBufferPtr& commandBuffer);

            void RefreshShadowMapsAsNeeded(const RenderParams& renderParams,
                                           const VulkanCommandBufferPtr& commandBuffer);

            [[nodiscard]] bool RefreshShadowMap(const RenderParams& renderParams,
                                                const VulkanCommandBufferPtr& commandBuffer,
                                                const LoadedLight& loadedLight);

            void RunSceneRender(const std::string& sceneName,
                                const RenderTarget& renderTarget,
                                const RenderParams& renderParams,
                                const std::vector<ViewProjection>& viewProjections,
                                const std::unordered_map<LightId, ImageId>& shadowMaps);

            void RenderObjects(const std::string& sceneName,
                               const FramebufferObjs& framebufferObjs,
                               const RenderParams& renderParams,
                               const std::vector<ViewProjection>& viewProjections,
                               const std::unordered_map<LightId, ImageId>& shadowMaps);

            void RunPostProcessing(const LoadedImage& inputImage,
                                   const LoadedImage& outputImage,
                                   const PostProcessEffect& effect);

            void RenderScreen(const std::string& sceneName,
                              const FramebufferObjs& framebufferObjs,
                              const RenderParams& renderParams);

            void RunSwapChainBlitPass(const VulkanFramebufferPtr& framebuffer,
                                      const LoadedImage& renderImage,
                                      const LoadedImage& screenImage);

            void TransferObjectDetailImage(const LoadedImage& gPassObjectDetailImage,
                                           const LoadedImage& frameObjectDetailImage,
                                           const VulkanCommandBufferPtr& commandBuffer);

            void BlitEyeRendersToOpenXR(const VulkanCommandBufferPtr& commandBuffer, const LoadedImage& renderImage);

        private:

            IOpenXR::Ptr m_openXR;
            VulkanObjsPtr m_vulkanObjs;
            IShadersPtr m_shaders;
            IProgramsPtr m_programs;
            IPipelineFactoryPtr m_pipelines;
            PostExecutionOpsPtr m_postExecutionOps;
            IBuffersPtr m_buffers;
            IImagesPtr m_images;
            ITexturesPtr m_textures;
            IMeshesPtr m_meshes;
            IFramebuffersPtr m_framebuffers;
            IMaterialsPtr m_materials;
            ILightsPtr m_lights;
            IRenderTargetsPtr m_renderTargets;

            IRenderablesPtr m_renderables;
            Frames m_frames;
            RenderState m_renderState;

            mutable std::mutex m_latestObjectDetailTextureIdMutex;
            std::optional<ImageId> m_latestObjectDetailImageId;

            RendererGroup<SwapChainBlitRenderer> m_swapChainRenderers;
            RendererGroup<SpriteRenderer> m_spriteRenderers;
            RendererGroup<ObjectRenderer> m_objectRenderers;
            RendererGroup<TerrainRenderer> m_terrainRenderers;
            RendererGroup<SkyBoxRenderer> m_skyBoxRenderers;
            RendererGroup<DeferredLightingRenderer> m_differedLightingRenderers;
            RendererGroup<RawTriangleRenderer> m_rawTriangleRenderers;
            RendererGroup<PostProcessingRenderer> m_postProcessingRenderers;
    };
}

#endif //LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDERER_RENDERERSDL_H
