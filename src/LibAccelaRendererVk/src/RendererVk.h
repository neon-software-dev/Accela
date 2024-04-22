/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDERER_RENDERERSDL_H
#define LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDERER_RENDERERSDL_H

#include "ForwardDeclares.h"
#include "Frames.h"

#include "Renderer/RendererGroup.h"
#include "Renderer/SwapChainBlitRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Renderer/ObjectRenderer.h"
#include "Renderer/TerrainRenderer.h"
#include "Renderer/SkyBoxRenderer.h"
#include "Renderer/DeferredLightingRenderer.h"
#include "Renderer/RawTriangleRenderer.h"

#include <Accela/Render/Id.h>
#include <Accela/Render/PresentConfig.h>
#include <Accela/Render/RendererBase.h>
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
                       IVulkanContextPtr vulkanContext);

        protected:

            void OnIdle() override;

            bool OnInitialize(const RenderSettings& renderSettings, const std::vector<ShaderSpec>& shaders) override;
            bool OnShutdown() override;
            bool OnRenderFrame(RenderGraph::Ptr renderGraph) override;
            void OnCreateTexture(std::promise<bool> resultPromise,
                                 const Texture& texture,
                                 const TextureView& textureView,
                                 const TextureSampler& textureSampler,
                                 bool generateMipMaps) override;
            bool OnDestroyTexture(TextureId textureId) override;
            bool OnCreateMesh(const Mesh::Ptr& mesh, MeshUsage meshUsage) override;
            bool OnDestroyMesh(MeshId meshId) override;
            bool OnCreateMaterial(const Material::Ptr& material) override;
            bool OnDestroyMaterial(MaterialId materialId) override;
            bool OnCreateFrameBuffer(FrameBufferId frameBufferId, const std::vector<TextureId>& attachmentTextures) override;
            bool OnDestroyFrameBuffer(FrameBufferId frameBufferId) override;
            bool OnWorldUpdate(const WorldUpdate& update) override;
            bool OnSurfaceChanged() override;
            bool OnChangeRenderSettings(const RenderSettings& renderSettings) override;

        private:

            bool LoadShaders(const std::vector<ShaderSpec>& shaders);
            bool CreatePrograms();

            bool RenderGraphFunc_RenderScene(const RenderGraphNode::Ptr& node);
            bool RenderGraphFunc_Present(const uint32_t& swapChainImageIndex, const RenderGraphNode::Ptr& node);

            bool StartRenderPass(const VulkanRenderPassPtr& renderPass,
                                 const VulkanFramebufferPtr& framebuffer,
                                 const VulkanCommandBufferPtr& commandBuffer);
            void EndRenderPass(const VulkanCommandBufferPtr& commandBuffer);

            void RefreshShadowMapsAsNeeded(const RenderParams& renderParams,
                                           const VulkanCommandBufferPtr& commandBuffer,
                                           const std::vector<ViewProjection>& viewProjections);

            [[nodiscard]] bool RefreshShadowMap(const RenderParams& renderParams,
                                                const VulkanCommandBufferPtr& commandBuffer,
                                                const std::vector<ViewProjection>& viewProjections,
                                                const LoadedLight& loadedLight);

            void OffscreenRender(const std::string& sceneName,
                                 const FramebufferObjs& framebufferObjs,
                                 const RenderParams& renderParams,
                                 const std::vector<ViewProjection>& viewProjections,
                                 const std::unordered_map<LightId, TextureId>& shadowMaps);

        private:

            VulkanObjsPtr m_vulkanObjs;
            IShadersPtr m_shaders;
            IProgramsPtr m_programs;
            IPipelineFactoryPtr m_pipelines;
            PostExecutionOpsPtr m_postExecutionOps;
            IBuffersPtr m_buffers;
            ITexturesPtr m_textures;
            IMeshesPtr m_meshes;
            IFramebuffersPtr m_framebuffers;
            IMaterialsPtr m_materials;
            ILightsPtr m_lights;

            IRenderablesPtr m_renderables;
            Frames m_frames;

            RendererGroup<SwapChainBlitRenderer> m_swapChainRenderers;
            RendererGroup<SpriteRenderer> m_spriteRenderers;
            RendererGroup<ObjectRenderer> m_objectRenderers;
            RendererGroup<TerrainRenderer> m_terrainRenderers;
            RendererGroup<SkyBoxRenderer> m_skyBoxRenderers;
            RendererGroup<DeferredLightingRenderer> m_differedLightingRenderers;
            RendererGroup<RawTriangleRenderer> m_rawTriangleRenderers;
    };
}

#endif //LIBACCELARENDERERVK_INCLUDE_ACCELA_RENDERER_RENDERERSDL_H
