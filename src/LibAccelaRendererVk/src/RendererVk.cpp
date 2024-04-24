/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RendererVk.h"
#include "VulkanObjs.h"
#include "PostExecutionOp.h"
#include "Metrics.h"

#include "Shader/Shaders.h"
#include "Program/Programs.h"
#include "Pipeline/PipelineFactory.h"
#include "Texture/Textures.h"
#include "Buffer/Buffers.h"
#include "Util/VulkanFuncs.h"
#include "Util/Synchronization.h"
#include "Mesh/Meshes.h"
#include "Framebuffer/Framebuffers.h"
#include "Renderables/Renderables.h"
#include "Material/Materials.h"
#include "Light/Lights.h"

#include "Vulkan/VulkanDebug.h"
#include "Vulkan/VulkanSwapChain.h"
#include "Vulkan/VulkanInstance.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanPhysicalDevice.h"
#include "Vulkan/VulkanCommandPool.h"
#include "Vulkan/VulkanCommandBuffer.h"
#include "Vulkan/VulkanFramebuffer.h"

#include <Accela/Render/IVulkanCalls.h>
#include <Accela/Render/IVulkanContext.h>
#include <Accela/Render/Graph/RenderGraphNodes.h>

#include <Accela/Common/Timer.h>
#include <Accela/Common/BuildInfo.h>

#include <format>
#include <algorithm>
#include <stack>

namespace Accela::Render
{

RendererVk::RendererVk(std::string appName,
                       uint32_t appVersion,
                       Common::ILogger::Ptr logger,
                       Common::IMetrics::Ptr metrics,
                       IVulkanCallsPtr vulkanCalls,
                       IVulkanContextPtr vulkanContext)
    : RendererBase(std::move(logger), std::move(metrics))
    , m_vulkanObjs(std::make_shared<VulkanObjs>(std::move(appName), appVersion, m_logger, std::move(vulkanCalls), std::move(vulkanContext)))
    , m_shaders(std::make_shared<Shaders>(m_logger, m_vulkanObjs))
    , m_programs(std::make_shared<Programs>(m_logger, m_vulkanObjs, m_shaders))
    , m_pipelines(std::make_shared<PipelineFactory>(m_logger, m_vulkanObjs, m_shaders))
    , m_postExecutionOps(std::make_shared<PostExecutionOps>(m_logger, m_vulkanObjs))
    , m_buffers(std::make_shared<Buffers>(m_logger, m_metrics, m_vulkanObjs, m_postExecutionOps))
    , m_textures(std::make_shared<Textures>(m_logger, m_metrics, m_vulkanObjs, m_buffers, m_postExecutionOps, m_ids))
    , m_meshes(std::make_shared<Meshes>(m_logger, m_metrics, m_vulkanObjs, m_ids, m_postExecutionOps, m_buffers))
    , m_framebuffers(std::make_shared<Framebuffers>(m_logger, m_ids, m_vulkanObjs, m_textures, m_postExecutionOps))
    , m_materials(std::make_shared<Materials>(m_logger, m_vulkanObjs, m_postExecutionOps, m_ids, m_textures, m_buffers))
    , m_lights(std::make_shared<Lights>(m_logger, m_metrics, m_vulkanObjs, m_framebuffers, m_ids))
    , m_renderables(std::make_shared<Renderables>(m_logger, m_ids, m_postExecutionOps, m_textures, m_buffers, m_meshes, m_lights))
    , m_frames(m_logger, m_ids, m_vulkanObjs, m_textures)
    , m_swapChainRenderers(m_logger, m_metrics, m_ids, m_postExecutionOps, m_vulkanObjs, m_programs, m_shaders, m_pipelines, m_buffers, m_materials, m_textures, m_meshes, m_lights, m_renderables)
    , m_spriteRenderers(m_logger, m_metrics, m_ids, m_postExecutionOps, m_vulkanObjs, m_programs, m_shaders, m_pipelines, m_buffers, m_materials, m_textures, m_meshes, m_lights, m_renderables)
    , m_objectRenderers(m_logger, m_metrics, m_ids, m_postExecutionOps, m_vulkanObjs, m_programs, m_shaders, m_pipelines, m_buffers, m_materials, m_textures, m_meshes, m_lights, m_renderables)
    , m_terrainRenderers(m_logger, m_metrics, m_ids, m_postExecutionOps, m_vulkanObjs, m_programs, m_shaders, m_pipelines, m_buffers, m_materials, m_textures, m_meshes, m_lights, m_renderables)
    , m_skyBoxRenderers(m_logger, m_metrics, m_ids, m_postExecutionOps, m_vulkanObjs, m_programs, m_shaders, m_pipelines, m_buffers, m_materials, m_textures, m_meshes, m_lights, m_renderables)
    , m_differedLightingRenderers(m_logger, m_metrics, m_ids, m_postExecutionOps, m_vulkanObjs, m_programs, m_shaders, m_pipelines, m_buffers, m_materials, m_textures, m_meshes, m_lights, m_renderables)
    , m_rawTriangleRenderers(m_logger, m_metrics, m_ids, m_postExecutionOps, m_vulkanObjs, m_programs, m_shaders, m_pipelines, m_buffers, m_materials, m_textures, m_meshes, m_lights, m_renderables)
{ }

bool RendererVk::OnInitialize(const RenderSettings& renderSettings, const std::vector<ShaderSpec>& shaders)
{
    m_logger->Log(Common::LogLevel::Info, "RendererVk: Initializing");

    if (renderSettings.presentToHeadset && !m_vulkanObjs->GetContext()->VR_InitOutput()) { return false; }

    if (!m_vulkanObjs->Initialize(Common::BuildInfo::IsDebugBuild(), renderSettings)) { return false; }
    if (!m_postExecutionOps->Initialize(renderSettings)) { return false; }
    if (!LoadShaders(shaders)) { return false; }
    if (!CreatePrograms()) { return false; }
    if (!m_buffers->Initialize()) { return false; }
    if (!m_textures->Initialize(m_vulkanObjs->GetTransferCommandPool(), m_vulkanObjs->GetDevice()->GetVkGraphicsQueue())) { return false; }
    if (!m_meshes->Initialize(m_vulkanObjs->GetTransferCommandPool(), m_vulkanObjs->GetDevice()->GetVkGraphicsQueue())) { return false; }
    if (!m_materials->Initialize(m_vulkanObjs->GetTransferCommandPool(), m_vulkanObjs->GetDevice()->GetVkGraphicsQueue())) { return false; }
    if (!m_renderables->Initialize()) { return false; }
    if (!m_frames.Initialize(renderSettings, m_vulkanObjs->GetSwapChain())) { return false; }
    if (!m_swapChainRenderers.Initialize(renderSettings)) { return false; }
    if (!m_spriteRenderers.Initialize(renderSettings)) { return false; }
    if (!m_objectRenderers.Initialize(renderSettings)) { return false; }
    if (!m_terrainRenderers.Initialize(renderSettings)) { return false; }
    if (!m_skyBoxRenderers.Initialize(renderSettings)) { return false; }
    if (!m_differedLightingRenderers.Initialize(renderSettings)) { return false; }
    if (!m_rawTriangleRenderers.Initialize(renderSettings)) { return false; }

    return true;
}

bool RendererVk::OnShutdown()
{
    m_logger->Log(Common::LogLevel::Info, "RendererVk: Shutting down");

    m_vulkanObjs->WaitForDeviceIdle();

    m_vulkanObjs->GetContext()->VR_DestroyOutput();

    m_postExecutionOps->Destroy();

    m_rawTriangleRenderers.Destroy();
    m_differedLightingRenderers.Destroy();
    m_skyBoxRenderers.Destroy();
    m_terrainRenderers.Destroy();
    m_objectRenderers.Destroy();
    m_spriteRenderers.Destroy();
    m_swapChainRenderers.Destroy();
    m_frames.Destroy();
    m_renderables->Destroy();

    m_lights->Destroy();
    m_materials->Destroy();
    m_framebuffers->Destroy();
    m_meshes->Destroy();
    m_textures->Destroy();
    m_buffers->Destroy();
    m_pipelines->Destroy();
    m_programs->Destroy();
    m_shaders->Destroy();
    m_vulkanObjs->Destroy();

    return true;
}

bool RendererVk::LoadShaders(const std::vector<ShaderSpec>& shaders)
{
    const bool allLoaded = std::ranges::all_of(shaders, [this](const auto& shader){
        return m_shaders->LoadShader(shader);
    });

    if (!allLoaded)
    {
        m_logger->Log(Common::LogLevel::Error, "LoadShaders: Not all shaders loaded");
    }

    return allLoaded;
}

bool RendererVk::CreatePrograms()
{
    if (!m_programs->CreateProgram("Sprite", {"Sprite.vert.spv", "Sprite.frag.spv"}))
    {
        m_logger->Log(Common::LogLevel::Error, "CreatePrograms: Failed to create Sprite program");
        return false;
    }

    if (!m_programs->CreateProgram("Object", {"Object.vert.spv", "Object.frag.spv"}))
    {
        m_logger->Log(Common::LogLevel::Error, "CreatePrograms: Failed to create Object program");
        return false;
    }

    if (!m_programs->CreateProgram("ObjectShadow", {"ObjectShadow.vert.spv", "Shadow.frag.spv"}))
    {
        m_logger->Log(Common::LogLevel::Error, "CreatePrograms: Failed to create ObjectShadow program");
        return false;
    }

    if (!m_programs->CreateProgram("BoneObject", {"BoneObject.vert.spv", "Object.frag.spv"}))
    {
        m_logger->Log(Common::LogLevel::Error, "CreatePrograms: Failed to create BoneObject program");
        return false;
    }

    if (!m_programs->CreateProgram("BoneObjectShadow", {"BoneObjectShadow.vert.spv", "Shadow.frag.spv"}))
    {
        m_logger->Log(Common::LogLevel::Error, "CreatePrograms: Failed to create BoneObjectShadow program");
        return false;
    }

    if (!m_programs->CreateProgram("Terrain", {"Terrain.tesc.spv", "Terrain.tese.spv", "Terrain.vert.spv", "Object.frag.spv"}))
    {
        m_logger->Log(Common::LogLevel::Error, "CreatePrograms: Failed to create Terrain program");
        return false;
    }

    if (!m_programs->CreateProgram("SkyBox", {"SkyBox.vert.spv", "SkyBox.frag.spv"}))
    {
        m_logger->Log(Common::LogLevel::Error, "CreatePrograms: Failed to create SkyBox program");
        return false;
    }

    if (!m_programs->CreateProgram("DeferredLighting", {"DeferredLighting.vert.spv", "DeferredLighting.frag.spv"}))
    {
        m_logger->Log(Common::LogLevel::Error, "CreatePrograms: Failed to create DeferredLighting program");
        return false;
    }

    if (!m_programs->CreateProgram("RawTriangle", {"RawTriangle.vert.spv", "RawTriangle.frag.spv"}))
    {
        m_logger->Log(Common::LogLevel::Error, "CreatePrograms: Failed to create RawTriangle program");
        return false;
    }

    if (!m_programs->CreateProgram("SwapChainBlit", {"SwapChainBlit.vert.spv", "SwapChainBlit.frag.spv"}))
    {
        m_logger->Log(Common::LogLevel::Error, "CreatePrograms: Failed to create SwapChainBlit program");
        return false;
    }

    return true;
}

void RendererVk::OnIdle()
{
    // If we're idle (not receiving any actual work messages), just pump the post
    // execution ops to see if any non-frame work (e.g. texture transfers) can
    // be finished out, without having to wait for frame renders to be requested
    m_postExecutionOps->FulfillReady();
}

void RendererVk::OnCreateTexture(std::promise<bool> resultPromise,
                                 const Texture& texture,
                                 const TextureView& textureView,
                                 const TextureSampler& textureSampler,
                                 bool generateMipMaps)
{
    if (texture.data.has_value())
    {
        m_textures->CreateTextureFilled(texture, {textureView}, textureSampler, generateMipMaps, std::move(resultPromise));
    }
    else
    {
        resultPromise.set_value(m_textures->CreateTextureEmpty(texture, {textureView}, textureSampler));
    }
}

bool RendererVk::OnDestroyTexture(TextureId textureId)
{
    m_textures->DestroyTexture(textureId, false);
    return true;
}

bool RendererVk::OnCreateMesh(const Mesh::Ptr& mesh, MeshUsage meshUsage)
{
    return m_meshes->LoadMesh(mesh, meshUsage);
}

bool RendererVk::OnDestroyMesh(MeshId meshId)
{
    m_meshes->DestroyMesh(meshId, false);
    return true;
}

bool RendererVk::OnCreateMaterial(const Material::Ptr& material)
{
    return m_materials->CreateMaterial(material);
}

bool RendererVk::OnDestroyMaterial(MaterialId materialId)
{
    m_materials->DestroyMaterial(materialId, false);
    return true;
}

bool RendererVk::OnCreateFrameBuffer(FrameBufferId frameBufferId, const std::vector<TextureId>& attachmentTextures)
{
    if (attachmentTextures.empty())
    {
        m_logger->Log(Common::LogLevel::Error, "OnCreateFrameBuffer: No textures provided");
        return false;
    }

    std::stringstream ss;
    bool first = true;

    USize attachmentSize{};

    std::vector<std::pair<TextureId, std::string>> attachmentTextureViews;

    for (const auto& textureId : attachmentTextures)
    {
        if (!first) { ss << "-"; }

        const auto loadedTexture = m_textures->GetTexture(textureId);
        if (!loadedTexture)
        {
            m_logger->Log(Common::LogLevel::Error, "OnCreateFrameBuffer: No such texture exists: {}", textureId.id);
            return false;
        }

        if (!first)
        {
            if (attachmentSize != loadedTexture->pixelSize)
            {
                m_logger->Log(Common::LogLevel::Error, "OnCreateFrameBuffer: Attachments have mismatched sizes");
                return false;
            }
        }

        ss << textureId.id;
        attachmentSize = loadedTexture->pixelSize;
        first = false;

        attachmentTextureViews.emplace_back(textureId, TextureView::DEFAULT);
    }

    return m_framebuffers->CreateFramebuffer(
        frameBufferId,
        m_vulkanObjs->GetOffscreenRenderPass(),
        attachmentTextureViews,
        attachmentSize,
        1,
        std::format("ClientCreated-{}", ss.str())
    );
}

bool RendererVk::OnDestroyFrameBuffer(FrameBufferId frameBufferId)
{
    m_framebuffers->DestroyFramebuffer(frameBufferId, false);
    return true;
}

bool RendererVk::OnRenderFrame(RenderGraph::Ptr renderGraph)
{
    Common::Timer frameRenderTotalTimer(Renderer_FrameRenderTotal_Time);

    //
    // Start the frame
    // - Waits for the frame's previous work to finish
    // - Returns the swap chain image index to render to (or an error)
    //
    const auto swapChainImageIndexExpect = m_frames.StartFrame();
    if (!swapChainImageIndexExpect)
    {
        switch (swapChainImageIndexExpect.error())
        {
            case SurfaceIssue::SurfaceInvalidated: { m_vulkanObjs->OnSurfaceInvalidated(); } break;
            case SurfaceIssue::SurfaceLost: { m_vulkanObjs->OnSurfaceLost(); } break;
        }

        return false;
    }
    const auto swapChainImageIndex = swapChainImageIndexExpect.value();

    ///////////////
    // CPU and GPU are synchronized for the frame at this point
    ///////////////

    Common::Timer frameRenderWorkTimer(Renderer_FrameRenderWork_Time);

    auto& currentFrame = m_frames.GetCurrentFrame();
    auto framePipelineFence = currentFrame.GetPipelineFence();
    auto graphicsCommandPool = currentFrame.GetGraphicsCommandPool();
    auto graphicsCommandBuffer = currentFrame.GetGraphicsCommandBuffer();

    //
    // Initialize frame state
    //

    // Mark the current frame's work as finished/synced and fulfill any pending work for it
    m_postExecutionOps->SetFrameSynced(currentFrame.GetFrameIndex(), framePipelineFence);

    // Reset the frame's execution fence
    m_vulkanObjs->GetCalls()->vkResetFences(m_vulkanObjs->GetDevice()->GetVkDevice(), 1, &framePipelineFence);

    // Mark frame-specific resources as not currently in use
    m_swapChainRenderers.GetRendererForFrame(currentFrame.GetFrameIndex()).OnFrameSynced();
    m_spriteRenderers.GetRendererForFrame(currentFrame.GetFrameIndex()).OnFrameSynced();
    m_objectRenderers.GetRendererForFrame(currentFrame.GetFrameIndex()).OnFrameSynced();
    m_terrainRenderers.GetRendererForFrame(currentFrame.GetFrameIndex()).OnFrameSynced();
    m_skyBoxRenderers.GetRendererForFrame(currentFrame.GetFrameIndex()).OnFrameSynced();
    m_differedLightingRenderers.GetRendererForFrame(currentFrame.GetFrameIndex()).OnFrameSynced();
    m_rawTriangleRenderers.GetRendererForFrame(currentFrame.GetFrameIndex()).OnFrameSynced();

    // Reset the graphics command buffer, to prepare for recording new commands
    graphicsCommandPool->ResetCommandBuffer(graphicsCommandBuffer, false);

    ////////////////////////////////////
    // Query the VR headset for input, if needed
    ////////////////////////////////////

    if (m_vulkanObjs->GetRenderSettings().presentToHeadset)
    {
        m_vulkanObjs->GetContext()->VR_WaitGetPoses();
    }

    ////////////////////////////////////
    // Start recording graphics commands
    ////////////////////////////////////

    graphicsCommandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    //
    // DFS process the render graph, fulfilling its tasks
    //
    std::stack<RenderGraphNode::Ptr> nodeStack;
    nodeStack.push(renderGraph->root);

    bool graphProcessSuccess = true;

    while (!nodeStack.empty())
    {
        RenderGraphNode::Ptr node = nodeStack.top();
        nodeStack.pop();

        for (const auto& child : node->children)
        {
            nodeStack.push(child);
        }

        switch (node->GetType())
        {
            case RenderGraphNodeType::RenderScene: if (!RenderGraphFunc_RenderScene(node)) { graphProcessSuccess = false; } break;
            case RenderGraphNodeType::Present: if (!RenderGraphFunc_Present(swapChainImageIndex, node)) { graphProcessSuccess = false; } break;
        }
    }

    m_frames.EndFrame();

    frameRenderWorkTimer.StopTimer(m_metrics);
    frameRenderTotalTimer.StopTimer(m_metrics);

    return graphProcessSuccess;
}

bool RendererVk::RenderGraphFunc_RenderScene(const RenderGraphNode::Ptr& node)
{
    //
    // Gather Data
    //
    const auto renderSceneNode = std::static_pointer_cast<RenderGraphNode_RenderScene>(node);
    const auto sceneName = std::get<std::string>(renderSceneNode->data);
    const auto frameBufferId = std::get<FrameBufferId>(renderSceneNode->data);
    const auto renderParams = std::get<RenderParams>(renderSceneNode->data);

    auto& currentFrame = m_frames.GetCurrentFrame();
    auto graphicsCommandBuffer = currentFrame.GetGraphicsCommandBuffer();

    const auto framebufferObjs = m_framebuffers->GetFramebufferObjs(frameBufferId);
    if (!framebufferObjs)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RenderGraphFunc_RenderScene: No such framebuffer exists: {}", frameBufferId.id);
        return false;
    }

    std::vector<ViewProjection> viewProjections;

    if (m_vulkanObjs->GetRenderSettings().presentToHeadset)
    {
        const auto leftViewProjection = GetCameraViewProjection(m_vulkanObjs->GetContext(), renderParams.worldRenderCamera, Eye::Left);
        const auto rightViewProjection = GetCameraViewProjection(m_vulkanObjs->GetContext(), renderParams.worldRenderCamera, Eye::Right);

        if (!leftViewProjection || !rightViewProjection)
        {
            m_logger->Log(Common::LogLevel::Error, "RenderGraphFunc_RenderScene: Failed to generate an eye ViewProjection");
            return false;
        }

        viewProjections.push_back(*leftViewProjection);
        viewProjections.push_back(*rightViewProjection);
    }
    else
    {
        const auto viewProjection = GetCameraViewProjection(m_vulkanObjs->GetContext(), renderParams.worldRenderCamera);
        if (!viewProjection)
        {
            m_logger->Log(Common::LogLevel::Error, "RenderGraphFunc_RenderScene: Failed to generate camera ViewProjection");
            return false;
        }

        viewProjections.push_back(*viewProjection);
    }

    //
    // Validation
    //
    const auto sceneLights = m_lights->GetSceneLights(sceneName, viewProjections);

    const std::vector<LoadedLight> renderLights(
        sceneLights.cbegin(),
        sceneLights.cbegin() + std::min((uint32_t)sceneLights.size(), Max_Light_Count)
    );

    if (renderLights.size() != sceneLights.size())
    {
        m_logger->Log(Common::LogLevel::Error,
            "RenderGraphFunc_RenderScene: Scene has too many lights, dropped some, {} vs max of {}", sceneLights.size(), Max_Light_Count);
    }

    m_metrics->SetCounterValue(Renderer_Scene_Lights_Count, renderLights.size());

    //
    // Shadow Passes
    //

    // Run shadow passes to render any shadow maps which are invalidated
    RefreshShadowMapsAsNeeded(renderParams, graphicsCommandBuffer, viewProjections);

    //
    // Offscreen Render
    //

    // Create a mapping of light -> shadow map texture
    std::unordered_map<LightId, TextureId> shadowMaps;

    for (const auto& renderLight : renderLights)
    {
        if (!renderLight.light.castsShadows || !renderLight.shadowFrameBufferId) { continue; }

        const auto shadowFramebuffer = m_framebuffers->GetFramebufferObjs(*renderLight.shadowFrameBufferId);
        const auto shadowTextureId = shadowFramebuffer->GetAttachmentTextureView(0)->first.textureId;

        shadowMaps.insert({renderLight.light.lightId, shadowTextureId});
    }

    m_metrics->SetCounterValue(Renderer_Scene_Shadow_Map_Count, shadowMaps.size());

    {
        CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), graphicsCommandBuffer, "OffscreenRender");

        if (!StartRenderPass(m_vulkanObjs->GetOffscreenRenderPass(), (*framebufferObjs).GetFramebuffer(), graphicsCommandBuffer)) { return false; }
            OffscreenRender(sceneName, *framebufferObjs, renderParams, viewProjections, shadowMaps);
        EndRenderPass(graphicsCommandBuffer);
    }

    return true;
}

bool RendererVk::RenderGraphFunc_Present(const uint32_t& swapChainImageIndex, const RenderGraphNode::Ptr& node)
{
    //
    // Gather Data
    //
    VulkanFuncs vulkanFuncs(m_logger, m_vulkanObjs);

    auto& currentFrame = m_frames.GetCurrentFrame();
    auto framePipelineFence = currentFrame.GetPipelineFence();
    auto graphicsCommandBuffer = currentFrame.GetGraphicsCommandBuffer();
    const auto swapChainRenderPass = m_vulkanObjs->GetSwapChainRenderPass();
    const auto swapChainFrameBuffer = m_vulkanObjs->GetSwapChainFrameBuffer(swapChainImageIndex);

    const auto presentToScreenNode = std::static_pointer_cast<RenderGraphNode_Present>(node);
    const auto screenTextureId = std::get<TextureId>(presentToScreenNode->data);
    const auto presentConfig = std::get<PresentConfig>(presentToScreenNode->data);

    const auto screenColorTexture = m_textures->GetTexture(screenTextureId);
    if (!screenColorTexture)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RenderGraphFunc_PresentTexture: No such screen color texture exists: {}", screenTextureId.id);
        return false;
    }

    ////////////////////////////////////////////////////
    // Run the SwapChainBlitRenderer to blit the texture to be presented
    // to the SwapChain framebuffer
    ////////////////////////////////////////////////////

    {
        CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), graphicsCommandBuffer, "SwapChainBlit");

        std::vector<VkClearValue> swapChainClearValues(1);
        swapChainClearValues[0].color = {{presentConfig.clearColor.r, presentConfig.clearColor.g, presentConfig.clearColor.b, 1.0f}};

        graphicsCommandBuffer->CmdBeginRenderPass(
            swapChainRenderPass,
            swapChainFrameBuffer,
            VK_SUBPASS_CONTENTS_INLINE,
            swapChainClearValues
        );

        m_swapChainRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
            .Render(
                graphicsCommandBuffer,
                swapChainRenderPass,
                swapChainFrameBuffer,
                *screenColorTexture
            );

        graphicsCommandBuffer->CmdEndRenderPass();
    }

    ////////////////////////////////////////////////////
    // If outputting to a headset, add pipeline barriers to transition the
    // eye textures to VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL so that VR can
    // transfer from it, after the swap chain renderer has finished its work.
    ////////////////////////////////////////////////////

    if (m_vulkanObjs->GetRenderSettings().presentToHeadset)
    {
        InsertPipelineBarrier_Image(
            m_vulkanObjs->GetCalls(),
            graphicsCommandBuffer,
            screenColorTexture->allocation.vkImage,
            Layers(0, 2),
            Levels(0, 1),
            VK_IMAGE_ASPECT_COLOR_BIT,
            // Swap chain fragment shader must finish sampling from the image texture
            BarrierPoint(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT),
            // Before VR starts transferring from it
            BarrierPoint(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT),
            // Convert image to transfer src optimal
            ImageTransition(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        );
    }

    ////////////////////////////////////////////////////
    // Finish recording graphics commands for this frame
    ////////////////////////////////////////////////////

    graphicsCommandBuffer->End();

    /////////////////////////////////////////////////////
    // Submit the graphics commands to the graphics queue
    /////////////////////////////////////////////////////

    vulkanFuncs.QueueSubmit(
        std::format("FrameRender-{}", currentFrame.GetFrameIndex()),
        m_vulkanObjs->GetDevice()->GetVkGraphicsQueue(),
        {graphicsCommandBuffer->GetVkCommandBuffer()},
        WaitOn({
           // Swap chain image must be available before we can write to it
           {currentFrame.GetImageAvailableSemaphore(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}
        }),
        SignalOn({
             currentFrame.GetRenderFinishedSemaphore()
        }),
        // This fence tracks the work submitted for this frame
        framePipelineFence
    );

    /////////////////////////////////////////////////////
    // Present the eye textures to the VR system
    /////////////////////////////////////////////////////

    if (m_vulkanObjs->GetRenderSettings().presentToHeadset)
    {
        HeadsetEyeRenderData eyeRenderData{};
        eyeRenderData.vkInstance = m_vulkanObjs->GetInstance()->GetVkInstance();
        eyeRenderData.vkPhysicalDevice = m_vulkanObjs->GetPhysicalDevice()->GetVkPhysicalDevice();
        eyeRenderData.vkDevice = m_vulkanObjs->GetDevice()->GetVkDevice();
        eyeRenderData.vkQueue = m_vulkanObjs->GetDevice()->GetVkGraphicsQueue();
        eyeRenderData.vkImage = screenColorTexture->allocation.vkImage;
        eyeRenderData.queueFamilyIndex = m_vulkanObjs->GetPhysicalDevice()->GetGraphicsQueueFamilyIndex().value();
        eyeRenderData.width = screenColorTexture->pixelSize.w;
        eyeRenderData.height = screenColorTexture->pixelSize.h;
        eyeRenderData.format = VK_FORMAT_R8G8B8A8_SRGB; // TODO: Get from texture
        eyeRenderData.sampleCount = 0;

        m_vulkanObjs->GetContext()->VR_SubmitEyeRender(Eye::Left, eyeRenderData);
        m_vulkanObjs->GetContext()->VR_SubmitEyeRender(Eye::Right, eyeRenderData);
    }

    /////////////////////////////////////////////////////
    // Present the swap chain image to the screen
    /////////////////////////////////////////////////////

    std::vector<VkSwapchainKHR> swapChains = {m_vulkanObjs->GetSwapChain()->GetVkSwapchainKHR()};

    // Present must wait until submitted render graphics commands have finished
    VkSemaphore presentWaitSemaphores[] = {currentFrame.GetRenderFinishedSemaphore()};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains.data();
    presentInfo.pImageIndices = &swapChainImageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = presentWaitSemaphores;

    auto result = m_vulkanObjs->GetCalls()->vkQueuePresentKHR(m_vulkanObjs->GetDevice()->GetVkPresentQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        m_logger->Log(Common::LogLevel::Info, "vkQueuePresentKHR notified bad swap chain");
        m_vulkanObjs->OnSurfaceInvalidated();
        return false;
    }
    else if (result == VK_ERROR_SURFACE_LOST_KHR)
    {
        m_vulkanObjs->OnSurfaceLost();
        return false;
    }

    return true;
}

bool RendererVk::StartRenderPass(const VulkanRenderPassPtr& renderPass,
                                 const VulkanFramebufferPtr& framebuffer,
                                 const VulkanCommandBufferPtr& commandBuffer)
{
    //
    // Start Render Pass
    //
    const auto framebufferAttachments = framebuffer->GetAttachments();

    const auto attachments = renderPass->GetAttachments();

    if (framebufferAttachments.size() != attachments.size())
    {
        m_logger->Log(Common::LogLevel::Error, "StartRenderPass: Framebuffer attachments size doesn't match render pass attachments size");
        return false;
    }

    std::vector<VkClearValue> clearValues(framebufferAttachments.size());

    for (unsigned int x = 0; x < framebufferAttachments.size(); ++x)
    {
        switch (attachments[x].type)
        {
            case VulkanRenderPass::AttachmentType::Color:
                clearValues[x].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
            break;
            case VulkanRenderPass::AttachmentType::Depth:
                clearValues[x].depthStencil.depth = 1.0f;
                clearValues[x].depthStencil.stencil = 0.0f;
            break;
        }
    }

    commandBuffer->CmdBeginRenderPass(
        renderPass,
        framebuffer,
        VK_SUBPASS_CONTENTS_INLINE,
        clearValues
    );

    return true;
}

void RendererVk::EndRenderPass(const VulkanCommandBufferPtr& commandBuffer)
{
    commandBuffer->CmdEndRenderPass();
}

void RendererVk::OffscreenRender(const std::string& sceneName,
                                 const FramebufferObjs& framebufferObjs,
                                 const RenderParams& renderParams,
                                 const std::vector<ViewProjection>& viewProjections,
                                 const std::unordered_map<LightId, TextureId>& shadowMaps)
{
    //
    // Gather Data
    //
    auto& currentFrame = m_frames.GetCurrentFrame();
    auto graphicsCommandBuffer = currentFrame.GetGraphicsCommandBuffer();
    const auto offscreenRenderPass = m_vulkanObjs->GetOffscreenRenderPass();

    //
    // GPass Subpass
    //
    {
        CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), graphicsCommandBuffer, "GPass");

        m_objectRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
            .Render(
                sceneName,
                RenderType::Gpass,
                renderParams,
                graphicsCommandBuffer,
                offscreenRenderPass,
                framebufferObjs.GetFramebuffer(),
                viewProjections,
                std::nullopt
            );

        m_terrainRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
            .Render(
                sceneName,
                renderParams,
                graphicsCommandBuffer,
                offscreenRenderPass,
                framebufferObjs.GetFramebuffer(),
                viewProjections
            );
    }

    //
    // Deferred Lighting Subpass
    //
    {
        CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), graphicsCommandBuffer, "Lighting");

        graphicsCommandBuffer->CmdNextSubpass();

        // TODO: If we ever support more material types then we need separate deferred lighting flows for each
        //  material type, as the material index used as an input is specific to a material type's data buffer
        //  Output two values: (mat type, mat index) from object frag instead? and run lighting x times, ignoring
        //  mat types that are wrong?
        m_differedLightingRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
            .Render(
                sceneName,
                Material::Type::Object,
                renderParams,
                graphicsCommandBuffer,
                offscreenRenderPass,
                framebufferObjs.GetFramebuffer(),
                viewProjections,
                shadowMaps
            );
    }

    //
    // Forward Subpass
    //
    {
        CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), graphicsCommandBuffer, "Forward");

        graphicsCommandBuffer->CmdNextSubpass();

        m_skyBoxRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
            .Render(
                renderParams,
                graphicsCommandBuffer,
                offscreenRenderPass,
                framebufferObjs.GetFramebuffer(),
                viewProjections
            );

        // TODO: Figure out what to do with sprite renderers when in VR mode. They're currently getting
        //  multiviewed in the pass they're in and drawn twice onto the screen, in a way that isn't even
        //  good for VR. Works fine in non-VR mode though.
        m_spriteRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
            .Render(
                sceneName,
                renderParams,
                graphicsCommandBuffer,
                offscreenRenderPass,
                framebufferObjs.GetFramebuffer()
            );

        // Render debug triangles
        m_rawTriangleRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
            .Render(
                renderParams,
                graphicsCommandBuffer,
                offscreenRenderPass,
                framebufferObjs.GetFramebuffer(),
                viewProjections,
                renderParams.debugTriangles
            );
    }
}

bool RendererVk::OnWorldUpdate(const WorldUpdate& update)
{
    Common::Timer sceneUpdateTimer(Renderer_Scene_Update_Time);

    VulkanFuncs vulkanFuncs(m_logger, m_vulkanObjs);

    //
    // Update renderable data in the GPU
    //
    vulkanFuncs.QueueSubmit(
        "WorldUpdate",
        m_postExecutionOps,
        m_vulkanObjs->GetDevice()->GetVkGraphicsQueue(),
        m_frames.GetNextFrame().GetGraphicsCommandPool(),
        [&](const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence){
            m_renderables->ProcessUpdate(update, commandBuffer, vkFence);
            m_lights->ProcessUpdate(update, commandBuffer, vkFence);
        }
    );

    sceneUpdateTimer.StopTimer(m_metrics);

    return true;
}

bool RendererVk::OnSurfaceChanged()
{
    m_logger->Log(Common::LogLevel::Info, "OnSurfaceChanged: Notified surface changed");

    if (!m_vulkanObjs->OnSurfaceInvalidated())
    {
        return false;
    }

    m_frames.OnSwapChainChanged(m_vulkanObjs->GetSwapChain());

    return true;
}

bool RendererVk::OnChangeRenderSettings(const RenderSettings& renderSettings)
{
    std::stringstream ss;
    ss  << "RendererVk::OnChangeRenderSetting: Applying new render settings: "
        << "[Present Mode: " << (unsigned int)renderSettings.presentMode << "] "
        << "[Present Scaling: " << (unsigned int)renderSettings.presentScaling << "] "
        << "[Resolution: " << renderSettings.resolution.w << "x" << renderSettings.resolution.h  << "] "
        << "[Frames in Flight: " << (unsigned int)renderSettings.framesInFlight << "]";

    m_logger->Log(Common::LogLevel::Info, ss.str());

    m_vulkanObjs->WaitForDeviceIdle();

    bool successful = true;

    if (!m_postExecutionOps->OnRenderSettingsChanged(renderSettings)) { successful = false; }
    if (!m_vulkanObjs->OnRenderSettingsChanged(renderSettings)) { successful = false; }
    if (!m_frames.OnRenderSettingsChanged(renderSettings)) { successful = false; }
    if (!m_swapChainRenderers.OnRenderSettingsChanged(renderSettings)) { successful = false; }
    if (!m_spriteRenderers.OnRenderSettingsChanged(renderSettings)) { successful = false; }
    if (!m_objectRenderers.OnRenderSettingsChanged(renderSettings)) { successful = false; }
    if (!m_terrainRenderers.OnRenderSettingsChanged(renderSettings)) { successful = false; }
    if (!m_skyBoxRenderers.OnRenderSettingsChanged(renderSettings)) { successful = false; }
    if (!m_differedLightingRenderers.OnRenderSettingsChanged(renderSettings)) { successful = false; }
    if (!m_rawTriangleRenderers.OnRenderSettingsChanged(renderSettings)) { successful = false; }
    if (!m_lights->OnRenderSettingsChanged(renderSettings)) { successful = false; }

    return successful;
}

void RendererVk::RefreshShadowMapsAsNeeded(const RenderParams& renderParams,
                                           const VulkanCommandBufferPtr& commandBuffer,
                                           const std::vector<ViewProjection>& viewProjections)
{
    CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "ShadowMapRenders");

    const auto loadedLights = m_lights->GetAllLights();

    for (const auto& loadedLight : loadedLights)
    {
        const bool lightCastsShadows = loadedLight.light.castsShadows && loadedLight.shadowFrameBufferId;
        const auto lightShadowInvalidated = loadedLight.shadowInvalidated;

        if (lightCastsShadows && lightShadowInvalidated)
        {
            if (!RefreshShadowMap(renderParams, commandBuffer, viewProjections, loadedLight))
            {
                m_logger->Log(Common::LogLevel::Error,
                  "RefreshShadowMaps: Failed to refresh shadow map for light id: {}", loadedLight.light.lightId.id);
            }
        }
    }
}

bool RendererVk::RefreshShadowMap(const RenderParams& renderParams,
                                  const VulkanCommandBufferPtr& commandBuffer,
                                  const std::vector<ViewProjection>&,
                                  const LoadedLight& loadedLight)
{
    //
    // Gather data / validation
    //
    auto& currentFrame = m_frames.GetCurrentFrame();

    VulkanRenderPassPtr shadowRenderPass{};
    unsigned int numShadowMapLayers = 1;

    switch (loadedLight.shadowMapType)
    {
        case ShadowMapType::Single:
        {
            shadowRenderPass = m_vulkanObjs->GetShadow2DRenderPass();
            numShadowMapLayers = 1;
        }
            break;
        case ShadowMapType::Cube:
        {
            shadowRenderPass = m_vulkanObjs->GetShadowCubeRenderPass();
            numShadowMapLayers = 6;
        }
        break;
    }

    if (!loadedLight.shadowFrameBufferId)
    {
        m_logger->Log(Common::LogLevel::Warning,
          "RendererVk::RefreshShadowMap: Light doesn't have a shadow framebuffer, light id: {}", loadedLight.light.lightId.id);
        return false;
    }

    const auto shadowFramebufferId = *loadedLight.shadowFrameBufferId;
    const auto shadowFramebuffer = m_framebuffers->GetFramebufferObjs(shadowFramebufferId);

    if (!shadowFramebuffer || shadowFramebuffer->GetAttachmentTextureViews()->size() != 1)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RendererVk::RefreshShadowMap: Shadow framebuffer doesn't exist or wrong attachment count, light id: {}, fb id: {}",
              loadedLight.light.lightId.id, shadowFramebufferId.id);
        return false;
    }

    const auto shadowMapTexture = shadowFramebuffer->GetAttachmentTextureViews()->at(0).first;

    const float lightMaxAffectRange = GetLightMaxAffectRange(loadedLight.light);

    //
    // Set up and run a shadow render for the light for each shadow map cube face that's invalidated
    //
    {
        CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, std::format("ShadowMapRender-Light-{}", loadedLight.light.lightId.id));

        //
        // Pipeline barrier to wait for parallel reads of the shadow map from other frames to finish before rendering over it.
        // Note: The shadow render pass handles synchronization for future frames that read from its output.
        //
        InsertPipelineBarrier_Image(
            m_vulkanObjs->GetCalls(),
            commandBuffer,
            shadowMapTexture.allocation.vkImage,
            Layers(0, numShadowMapLayers),
            Levels(0, 1),
            VK_IMAGE_ASPECT_DEPTH_BIT,
            // Deferred lighting fragment shader must finish sampling from the shadow map
            BarrierPoint(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT),
            // Before renderer depth stage(s) can write to the shadow map as a depth attachment
            BarrierPoint(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT),
            // Whatever layout it was in before, transition it to depth attachment optimal as a depth target for shadow rendering
            ImageTransition(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
        );

        if (!StartRenderPass(shadowRenderPass, shadowFramebuffer->GetFramebuffer(), commandBuffer)) { return false; }

            //
            // Clear any existing shadow map data
            //
            VkClearAttachment vkClearAttachment{};
            vkClearAttachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            vkClearAttachment.clearValue.depthStencil.depth = 1.0f;
            vkClearAttachment.clearValue.depthStencil.stencil = 0.0f;

            VkClearRect vkClearRect = {};
            vkClearRect.rect = {{0, 0}, {shadowMapTexture.pixelSize.w, shadowMapTexture.pixelSize.h}};
            vkClearRect.baseArrayLayer = 0;
            vkClearRect.layerCount = 1;

            commandBuffer->CmdClearAttachments({vkClearAttachment}, {vkClearRect});

            //
            // Render the shadow map
            //
            std::vector<ViewProjection> shadowViewProjections;

            switch (loadedLight.shadowMapType)
            {
                case ShadowMapType::Single:
                {
                    const auto viewProjection = GetShadowMapViewProjection(loadedLight);
                    if (!viewProjection)
                    {
                        m_logger->Log(Common::LogLevel::Error, "RendererVk::RefreshShadowMap: Failed to generate shadow map ViewProjection");
                        return false;
                    }

                    shadowViewProjections.push_back(*viewProjection);
                }
                break;
                case ShadowMapType::Cube:
                {
                    for (unsigned int cubeFaceIndex = 0; cubeFaceIndex < 6; ++cubeFaceIndex)
                    {
                        const auto viewProjection = GetShadowMapCubeViewProjection(loadedLight, static_cast<CubeFace>(cubeFaceIndex));
                        if (!viewProjection)
                        {
                            m_logger->Log(Common::LogLevel::Error, "RendererVk::RefreshShadowMap: Failed to generate shadow map ViewProjection");
                            return false;
                        }

                        shadowViewProjections.push_back(*viewProjection);
                    }
                }
                break;
            }

            m_objectRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
                .Render(
                    loadedLight.light.sceneName,
                    RenderType::Shadow,
                    renderParams,
                    commandBuffer,
                    shadowRenderPass,
                    shadowFramebuffer->GetFramebuffer(),
                    shadowViewProjections,
                    ObjectRenderer::ShadowRenderData(lightMaxAffectRange)
                );

        EndRenderPass(commandBuffer);
    }

    //
    // Mark the light's shadow map as now synced
    //
    m_lights->OnShadowMapSynced(loadedLight.light.lightId);

    return true;
}

}
