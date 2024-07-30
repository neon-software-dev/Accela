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
#include "RenderTarget/RenderTargets.h"
#include "Renderer/PostProcessingEffects.h"
#include "VMA/IVMA.h"
#include "Image/Images.h"

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

#include <glm/gtc/color_space.hpp>

#include <format>
#include <algorithm>
#include <stack>
#include <array>

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
    , m_images(std::make_shared<Images>(m_logger, m_metrics, m_vulkanObjs, m_buffers, m_postExecutionOps))
    , m_textures(std::make_shared<Textures>(m_logger, m_metrics, m_vulkanObjs, m_images, m_buffers, m_postExecutionOps, m_ids))
    , m_meshes(std::make_shared<Meshes>(m_logger, m_metrics, m_vulkanObjs, m_ids, m_postExecutionOps, m_buffers))
    , m_framebuffers(std::make_shared<Framebuffers>(m_logger, m_ids, m_vulkanObjs, m_images, m_postExecutionOps))
    , m_materials(std::make_shared<Materials>(m_logger, m_metrics, m_vulkanObjs, m_postExecutionOps, m_ids, m_textures, m_buffers))
    , m_lights(std::make_shared<Lights>(m_logger, m_metrics, m_vulkanObjs, m_framebuffers, m_ids))
    , m_renderTargets(std::make_shared<RenderTargets>(m_logger, m_vulkanObjs, m_postExecutionOps, m_framebuffers, m_images, m_ids))
    , m_renderables(std::make_shared<Renderables>(m_logger, m_ids, m_postExecutionOps, m_textures, m_buffers, m_meshes, m_lights))
    , m_frames(m_logger, m_vulkanObjs, m_renderTargets, m_images)
    , m_renderState(m_logger, m_vulkanObjs->GetCalls())
    , m_swapChainRenderers(m_logger, m_metrics, m_ids, m_postExecutionOps, m_vulkanObjs, m_programs, m_shaders, m_pipelines, m_buffers, m_materials, m_images, m_textures, m_meshes, m_lights, m_renderables)
    , m_spriteRenderers(m_logger, m_metrics, m_ids, m_postExecutionOps, m_vulkanObjs, m_programs, m_shaders, m_pipelines, m_buffers, m_materials, m_images, m_textures, m_meshes, m_lights, m_renderables)
    , m_objectRenderers(m_logger, m_metrics, m_ids, m_postExecutionOps, m_vulkanObjs, m_programs, m_shaders, m_pipelines, m_buffers, m_materials, m_images, m_textures, m_meshes, m_lights, m_renderables)
    , m_terrainRenderers(m_logger, m_metrics, m_ids, m_postExecutionOps, m_vulkanObjs, m_programs, m_shaders, m_pipelines, m_buffers, m_materials, m_images, m_textures, m_meshes, m_lights, m_renderables)
    , m_skyBoxRenderers(m_logger, m_metrics, m_ids, m_postExecutionOps, m_vulkanObjs, m_programs, m_shaders, m_pipelines, m_buffers, m_materials, m_images, m_textures, m_meshes, m_lights, m_renderables)
    , m_differedLightingRenderers(m_logger, m_metrics, m_ids, m_postExecutionOps, m_vulkanObjs, m_programs, m_shaders, m_pipelines, m_buffers, m_materials, m_images, m_textures, m_meshes, m_lights, m_renderables)
    , m_rawTriangleRenderers(m_logger, m_metrics, m_ids, m_postExecutionOps, m_vulkanObjs, m_programs, m_shaders, m_pipelines, m_buffers, m_materials, m_images, m_textures, m_meshes, m_lights, m_renderables)
    , m_postProcessingRenderers(m_logger, m_metrics, m_ids, m_postExecutionOps, m_vulkanObjs, m_programs, m_shaders, m_pipelines, m_buffers, m_materials, m_images, m_textures, m_meshes, m_lights, m_renderables)
{ }

bool RendererVk::OnInitialize(const RenderSettings& renderSettings, const std::vector<ShaderSpec>& shaders)
{
    m_logger->Log(Common::LogLevel::Info, "RendererVk: Initializing");

    if (renderSettings.presentToHeadset && !m_vulkanObjs->GetContext()->VR_InitOutput()) { return false; }

    if (!m_vulkanObjs->Initialize(Common::BuildInfo::IsDebugBuild(), renderSettings)) { return false; }

    const auto transferCommandPool = m_vulkanObjs->GetTransferCommandPool();
    const auto transferQueue = m_vulkanObjs->GetDevice()->GetVkGraphicsQueue(); // TODO Perf: Separate transfer queue than graphics queue

    if (!m_postExecutionOps->Initialize(renderSettings)) { return false; }
    if (!LoadShaders(shaders)) { return false; }
    if (!CreatePrograms()) { return false; }
    if (!m_buffers->Initialize()) { return false; }
    if (!m_images->Initialize(transferCommandPool, transferQueue)) { return false; }
    if (!m_textures->Initialize()) { return false; }
    if (!m_meshes->Initialize(transferCommandPool, transferQueue)) { return false; }
    if (!m_materials->Initialize(transferCommandPool, transferQueue)) { return false; }
    if (!m_renderables->Initialize()) { return false; }
    if (!m_frames.Initialize(renderSettings, m_vulkanObjs->GetSwapChain())) { return false; }
    if (!m_swapChainRenderers.Initialize(renderSettings)) { return false; }
    if (!m_spriteRenderers.Initialize(renderSettings)) { return false; }
    if (!m_objectRenderers.Initialize(renderSettings)) { return false; }
    if (!m_terrainRenderers.Initialize(renderSettings)) { return false; }
    if (!m_skyBoxRenderers.Initialize(renderSettings)) { return false; }
    if (!m_differedLightingRenderers.Initialize(renderSettings)) { return false; }
    if (!m_rawTriangleRenderers.Initialize(renderSettings)) { return false; }
    if (!m_postProcessingRenderers.Initialize(renderSettings)) { return false; }

    return true;
}

bool RendererVk::OnShutdown()
{
    m_logger->Log(Common::LogLevel::Info, "RendererVk: Shutting down");

    m_vulkanObjs->WaitForDeviceIdle();

    m_latestObjectDetailImageId = std::nullopt;

    m_vulkanObjs->GetContext()->VR_DestroyOutput();

    m_postExecutionOps->Destroy();

    m_postProcessingRenderers.Destroy();
    m_rawTriangleRenderers.Destroy();
    m_differedLightingRenderers.Destroy();
    m_skyBoxRenderers.Destroy();
    m_terrainRenderers.Destroy();
    m_objectRenderers.Destroy();
    m_spriteRenderers.Destroy();
    m_swapChainRenderers.Destroy();
    m_renderState.Destroy();
    m_frames.Destroy();
    m_renderables->Destroy();

    m_renderTargets->Destroy();
    m_lights->Destroy();
    m_materials->Destroy();
    m_framebuffers->Destroy();
    m_meshes->Destroy();
    m_textures->Destroy();
    m_images->Destroy();
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
    const std::unordered_map<std::string, std::vector<std::string>> programs = {
        {"Sprite", {"Sprite.vert.spv", "Sprite.frag.spv"}},
        {"ObjectDeferred", {"Object.vert.spv", "ObjectDeferred.frag.spv"}},
        {"ObjectForward", {"Object.vert.spv", "ObjectForward.frag.spv"}},
        {"ObjectShadow", {"ObjectShadow.vert.spv", "Shadow.frag.spv"}},
        {"BoneObjectDeferred", {"BoneObject.vert.spv", "ObjectDeferred.frag.spv"}},
        {"BoneObjectForward", {"BoneObject.vert.spv", "ObjectForward.frag.spv"}},
        {"BoneObjectShadow", {"BoneObjectShadow.vert.spv", "Shadow.frag.spv"}},
        {"TerrainDeferred", {"Terrain.tesc.spv", "Terrain.tese.spv", "Terrain.vert.spv", "ObjectDeferred.frag.spv"}},
        {"SkyBox", {"SkyBox.vert.spv", "SkyBox.frag.spv"}},
        {"DeferredLighting", {"DeferredLighting.vert.spv", "DeferredLighting.frag.spv"}},
        {"RawTriangle", {"RawTriangle.vert.spv", "RawTriangle.frag.spv"}},
        {"SwapChainBlit", {"SwapChainBlit.vert.spv", "SwapChainBlit.frag.spv"}},
        {"ColorCorrection", {"ColorCorrection.comp.spv"}},
        {"FXAA", {"FXAA.comp.spv"}},
        {"ObjectHighlight", {"ObjectHighlight.comp.spv"}}
    };

    return std::ranges::all_of(programs, [this](const auto& program){
        if (!m_programs->CreateProgram(program.first, program.second))
        {
            m_logger->Log(Common::LogLevel::Error, "CreatePrograms: Failed to create program: {}", program.first);
            return false;
        }

        return true;
    });
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
                                 const TextureSampler& textureSampler)
{
    m_textures->CreateTexture(TextureDefinition(texture, {textureView}, {textureSampler}), std::move(resultPromise));
}

bool RendererVk::OnDestroyTexture(TextureId textureId)
{
    m_textures->DestroyTexture(textureId, false);
    return true;
}

bool RendererVk::OnCreateMesh(std::promise<bool> resultPromise,
                              const Mesh::Ptr& mesh,
                              MeshUsage meshUsage)
{
    return m_meshes->LoadMesh(mesh, meshUsage, std::move(resultPromise));
}

bool RendererVk::OnDestroyMesh(MeshId meshId)
{
    m_meshes->DestroyMesh(meshId, false);
    return true;
}

bool RendererVk::OnCreateMaterial(std::promise<bool> resultPromise, const Material::Ptr& material)
{
    return m_materials->CreateMaterial(material, std::move(resultPromise));
}

bool RendererVk::OnDestroyMaterial(MaterialId materialId)
{
    m_materials->DestroyMaterial(materialId, false);
    return true;
}

bool RendererVk::OnCreateRenderTarget(RenderTargetId renderTargetId, const std::string& tag)
{
    return m_renderTargets->CreateRenderTarget(renderTargetId, tag);
}

bool RendererVk::OnDestroyRenderTarget(RenderTargetId renderTargetId)
{
    m_renderTargets->DestroyRenderTarget(renderTargetId, false);
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
    const auto framePipelineFence = currentFrame.GetPipelineFence();
    const auto graphicsCommandPool = currentFrame.GetGraphicsCommandPool();
    const auto renderCommandBuffer = currentFrame.GetRenderCommandBuffer();

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
    m_postProcessingRenderers.GetRendererForFrame(currentFrame.GetFrameIndex()).OnFrameSynced();

    // Reset the frame's command buffers, to prepare for recording new commands
    graphicsCommandPool->ResetCommandBuffer(renderCommandBuffer, false);
    graphicsCommandPool->ResetCommandBuffer(currentFrame.GetSwapChainBlitCommandBuffer(), false);

    //////////////////////////////////////////
    // Update the latest object detail buffer
    //////////////////////////////////////////

    {
        std::lock_guard<std::mutex> lock(m_latestObjectDetailTextureIdMutex);
        m_latestObjectDetailImageId = currentFrame.GetObjectDetailImageId();
    }

    ////////////////////////////////////
    // Query the VR headset for input, if needed
    ////////////////////////////////////

    if (m_vulkanObjs->GetRenderSettings().presentToHeadset)
    {
        m_vulkanObjs->GetContext()->VR_WaitGetPoses();
    }

    ////////////////////////////////////
    // Start recording render commands
    ////////////////////////////////////

    renderCommandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

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

    ////////////////////////////////////
    // Record Metrics
    ////////////////////////////////////

    const auto numMemoryHeaps = m_vulkanObjs->GetPhysicalDevice()->GetPhysicalDeviceMemoryProperties().memoryHeapCount;
    const auto vmaBudgets = m_vulkanObjs->GetVMA()->GetVmaBudget(numMemoryHeaps);
    std::size_t memoryUsageBytes = 0;
    std::size_t memoryAvailableBytes = 0;

    for (const auto& vmaBudget : vmaBudgets)
    {
        memoryUsageBytes += vmaBudget.usage;
        memoryAvailableBytes += vmaBudget.budget;
    }

    m_metrics->SetCounterValue(Renderer_Memory_Usage, memoryUsageBytes);
    m_metrics->SetCounterValue(Renderer_Memory_Available, memoryAvailableBytes);

    return graphProcessSuccess;
}

bool RendererVk::RenderGraphFunc_RenderScene(const RenderGraphNode::Ptr& node)
{
    //
    // Gather Data
    //
    const auto renderSceneNode = std::static_pointer_cast<RenderGraphNode_RenderScene>(node);
    const auto sceneName = std::get<std::string>(renderSceneNode->data);
    const auto renderTargetId = std::get<RenderTargetId>(renderSceneNode->data);
    const auto renderParams = std::get<RenderParams>(renderSceneNode->data);

    auto& currentFrame = m_frames.GetCurrentFrame();
    auto renderCommandBuffer = currentFrame.GetRenderCommandBuffer();

    const auto renderTarget = m_renderTargets->GetRenderTarget(renderTargetId);
    if (!renderTarget)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RenderGraphFunc_RenderScene: No such render target exists: {}", renderTargetId.id);
        return false;
    }

    std::vector<ViewProjection> viewProjections;

    if (m_vulkanObjs->GetRenderSettings().presentToHeadset)
    {
        const auto leftViewProjection = GetCameraViewProjection(m_vulkanObjs->GetRenderSettings(), m_vulkanObjs->GetContext(), renderParams.worldRenderCamera, Eye::Left);
        const auto rightViewProjection = GetCameraViewProjection(m_vulkanObjs->GetRenderSettings(), m_vulkanObjs->GetContext(), renderParams.worldRenderCamera, Eye::Right);

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
        const auto viewProjection = GetCameraViewProjection(m_vulkanObjs->GetRenderSettings(), m_vulkanObjs->GetContext(), renderParams.worldRenderCamera);
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
    // Shadow Pass Renders
    //

    // Run shadow passes to render any shadow maps which are invalidated
    RefreshShadowMapsAsNeeded(renderParams, renderCommandBuffer);

    // Create a mapping of light -> shadow map texture
    std::unordered_map<LightId, ImageId> shadowMaps;

    for (const auto& renderLight : renderLights)
    {
        if (!renderLight.light.castsShadows || !renderLight.shadowFrameBufferId) { continue; }

        const auto shadowFramebuffer = m_framebuffers->GetFramebufferObjs(*renderLight.shadowFrameBufferId);
        const auto shadowImageId = shadowFramebuffer->GetAttachmentImage(0)->first.id;

        shadowMaps.insert({renderLight.light.lightId, shadowImageId});
    }

    m_metrics->SetCounterValue(Renderer_Scene_Shadow_Map_Count, shadowMaps.size());

    //
    // Scene Render
    //

    RunSceneRender(sceneName, *renderTarget, renderParams, viewProjections, shadowMaps);

    return true;
}

void RendererVk::RunSceneRender(const std::string& sceneName,
                                const RenderTarget& renderTarget,
                                const RenderParams& renderParams,
                                const std::vector<ViewProjection>& viewProjections,
                                const std::unordered_map<LightId, ImageId>& shadowMaps)
{
    ////////////////////
    // Setup
    ////////////////////

    auto& currentFrame = m_frames.GetCurrentFrame();
    const auto commandBuffer = currentFrame.GetRenderCommandBuffer();
    const auto gPassRenderPass = m_vulkanObjs->GetGPassRenderPass();
    const auto screenRenderPass = m_vulkanObjs->GetScreenRenderPass();
    const auto renderSettings = m_vulkanObjs->GetRenderSettings();

    const auto gPassFramebufferObjs = m_framebuffers->GetFramebufferObjs(renderTarget.gPassFramebuffer);
    if (!gPassFramebufferObjs)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RunSceneRender: No such gpass framebuffer exists: {}", renderTarget.gPassFramebuffer.id);
        return;
    }

    const auto screenFramebufferObjs = m_framebuffers->GetFramebufferObjs(renderTarget.screenFramebuffer);
    if (!screenFramebufferObjs)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RunSceneRender: No such screen framebuffer exists: {}", renderTarget.screenFramebuffer.id);
        return;
    }

    const auto gPassColorAttachment = gPassFramebufferObjs->GetAttachmentImage(Offscreen_Attachment_Color);
    const auto gPassColorImage = gPassColorAttachment->first;

    const auto gPassDepthAttachment = gPassFramebufferObjs->GetAttachmentImage(Offscreen_Attachment_Depth);
    const auto gPassDepthImage = gPassDepthAttachment->first;

    const auto gPassObjectDetailAttachment = gPassFramebufferObjs->GetAttachmentImage(Offscreen_Attachment_ObjectDetail);
    const auto gPassObjectDetailImage = gPassObjectDetailAttachment->first;

    const auto screenColorAttachment = screenFramebufferObjs->GetAttachmentImage(Screen_Attachment_Color);
    const auto screenColorImage = screenColorAttachment->first;

    const auto postProcessingOutputImage = *m_images->GetImage(renderTarget.postProcessOutputImage);

    const auto frameObjectDetailResultImage = *m_images->GetImage(currentFrame.GetObjectDetailImageId());

    //////////////////////////
    // GPass Render Pass
    //////////////////////////

    StartRenderPass(gPassRenderPass, *gPassFramebufferObjs, commandBuffer);
        RenderObjects(sceneName, *gPassFramebufferObjs, renderParams, viewProjections, shadowMaps);
    EndRenderPass(commandBuffer);

    // Post-process the gpass color output to highlight objects
    if (!renderParams.highlightedObjects.empty())
    {
        RunPostProcessing(gPassColorImage,
                          postProcessingOutputImage,
                          ObjectHighlightEffect(
                              m_vulkanObjs->GetRenderSettings(),
                              gPassObjectDetailImage,
                              gPassDepthImage,
                              renderParams.highlightedObjects));
    }

    //////////////////////////
    // Screen Render Pass
    //////////////////////////

    StartRenderPass(screenRenderPass, *screenFramebufferObjs, commandBuffer);
        RenderScreen(sceneName, *screenFramebufferObjs, renderParams);
    EndRenderPass(commandBuffer);

    //////////////////////////
    // GPass Color Correction
    //////////////////////////

    std::unordered_set<ColorCorrection> colorCorrections{ColorCorrection::GammaCorrection};

    if (renderSettings.hdr)
    {
        colorCorrections.insert(ColorCorrection::ToneMapping);
    }

    RunPostProcessing(gPassColorImage, postProcessingOutputImage, ColorCorrectionEffect(m_vulkanObjs->GetRenderSettings(), colorCorrections));

    //////////////////////////
    // Screen Color Correction
    //////////////////////////

    RunPostProcessing(screenColorImage, postProcessingOutputImage, ColorCorrectionEffect(m_vulkanObjs->GetRenderSettings(), {ColorCorrection::GammaCorrection}));

    //////////////////////////
    // FXAA
    //////////////////////////

    if (renderSettings.fxaa)
    {
        RunPostProcessing(gPassColorImage, postProcessingOutputImage, FXAAEffect(m_vulkanObjs->GetRenderSettings()));
    }
}

bool RendererVk::RenderGraphFunc_Present(const uint32_t& swapChainImageIndex, const RenderGraphNode::Ptr& node)
{
    //
    // Setup
    //
    VulkanFuncs vulkanFuncs(m_logger, m_vulkanObjs);

    auto& currentFrame = m_frames.GetCurrentFrame();
    auto framePipelineFence = currentFrame.GetPipelineFence();
    auto renderCommandBuffer = currentFrame.GetRenderCommandBuffer();
    auto swapChainBlitCommandBuffer = currentFrame.GetSwapChainBlitCommandBuffer();

    const auto swapChainBlitRenderPass = m_vulkanObjs->GetSwapChainBlitRenderPass();
    const auto swapChainFrameBuffer = m_vulkanObjs->GetSwapChainFrameBuffer(swapChainImageIndex);

    const auto presentToScreenNode = std::static_pointer_cast<RenderGraphNode_Present>(node);
    const auto renderTargetId = std::get<RenderTargetId>(presentToScreenNode->data);
    const auto presentConfig = std::get<PresentConfig>(presentToScreenNode->data);

    const auto renderTarget = m_renderTargets->GetRenderTarget(renderTargetId);
    if (!renderTarget)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RenderGraphFunc_PresentTexture: No such render target exists: {}", renderTargetId.id);
        return false;
    }

    const auto gPassFramebufferObjs = m_framebuffers->GetFramebufferObjs(renderTarget->gPassFramebuffer);
    if (!gPassFramebufferObjs)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RenderGraphFunc_PresentTexture: No such gpass framebuffer exists: {}", renderTarget->gPassFramebuffer.id);
        return false;
    }

    const auto screenFramebufferObjs = m_framebuffers->GetFramebufferObjs(renderTarget->screenFramebuffer);
    if (!screenFramebufferObjs)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RenderGraphFunc_PresentTexture: No such screen framebuffer exists: {}", renderTarget->screenFramebuffer.id);
        return false;
    }

    const auto gPassColorImage = gPassFramebufferObjs->GetAttachmentImage(Offscreen_Attachment_Color)->first;
    const auto gPassColorVkImage = gPassColorImage.allocation.vkImage;

    const auto gPassObjectDetailAttachment = gPassFramebufferObjs->GetAttachmentImage(Offscreen_Attachment_ObjectDetail);
    const auto gPassObjectDetailImage = gPassObjectDetailAttachment->first;

    const auto screenColorImage = screenFramebufferObjs->GetAttachmentImage(Screen_Attachment_Color)->first;
    const auto screenColorVkImage = screenColorImage.allocation.vkImage;

    const auto frameObjectDetailResultImage = *m_images->GetImage(currentFrame.GetObjectDetailImageId());

    //////////////////////////
    // Object Detail Transfer
    //////////////////////////

    // The final object detail image is transferred to per-frame CPU-mapped image for client access
    TransferObjectDetailImage(gPassObjectDetailImage, frameObjectDetailResultImage, renderCommandBuffer);

    ////////////////////////////////////////////////////
    // Finish the Render work command buffer and submit it
    ////////////////////////////////////////////////////

    renderCommandBuffer->End();

    vulkanFuncs.QueueSubmit(
        std::format("FrameRender-{}", currentFrame.GetFrameIndex()),
        m_vulkanObjs->GetDevice()->GetVkGraphicsQueue(),
        {renderCommandBuffer->GetVkCommandBuffer()},
        WaitOn::None(),
        SignalOn({
             currentFrame.GetRenderFinishedSemaphore()
         }),
        VK_NULL_HANDLE
    );

    ////////////////////////////////////////////////////////////
    // Record and Submit the SwapChainBlit work
    ////////////////////////////////////////////////////////////

    swapChainBlitCommandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    // Convert the linearly-specified clear color to SRGB space as gamma correction was already done before this pass
    const auto swapChainBlitClearColor =
        glm::convertLinearToSRGB(presentConfig.clearColor, m_vulkanObjs->GetRenderSettings().gamma);

    // Prepare the input textures for read by the swap chain blit pass
    m_renderState.PrepareOperation(swapChainBlitCommandBuffer, RenderOperation({
       // We're going to read from the offscreen texture
       {gPassColorVkImage, ImageAccess(
           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
           BarrierPoint(
               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
               VK_ACCESS_SHADER_READ_BIT),
           BarrierPoint(
               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
               VK_ACCESS_SHADER_READ_BIT),
           Layers(0, gPassColorImage.image.numLayers),
           Levels(0, 1),
           VK_IMAGE_ASPECT_COLOR_BIT
       )},
       // We're going to read from the screen texture
       {screenColorVkImage, ImageAccess(
           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
           BarrierPoint(
               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
               VK_ACCESS_SHADER_READ_BIT),
           BarrierPoint(
               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
               VK_ACCESS_SHADER_READ_BIT),
           Layers(0, 1),
           Levels(0, 1),
           VK_IMAGE_ASPECT_COLOR_BIT
       )}
    } ));

    StartRenderPass(swapChainBlitRenderPass, swapChainFrameBuffer, swapChainBlitCommandBuffer, swapChainBlitClearColor);
        RunSwapChainBlitPass(swapChainFrameBuffer, gPassColorImage, screenColorImage);
    EndRenderPass(swapChainBlitCommandBuffer);

    // If outputting to a headset, insert one last operation that will record that when we
    // submit the present textures to the headset it transfers data from them
    if (m_vulkanObjs->GetRenderSettings().presentToHeadset)
    {
        m_renderState.PrepareOperation(swapChainBlitCommandBuffer, RenderOperation({
            // OpenVR is going to transfer from the present texture when we submit eye renders below
            {gPassColorVkImage, ImageAccess(
               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
               BarrierPoint(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT),
               BarrierPoint(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT),
               Layers(0, gPassColorImage.image.numLayers),
               Levels(0,1),
               VK_IMAGE_ASPECT_COLOR_BIT
            )}
        }));
    }

    swapChainBlitCommandBuffer->End();

    vulkanFuncs.QueueSubmit(
        std::format("FrameSwapChainBlit-{}", currentFrame.GetFrameIndex()),
        m_vulkanObjs->GetDevice()->GetVkGraphicsQueue(),
        {swapChainBlitCommandBuffer->GetVkCommandBuffer()},
        WaitOn({
           // Render work must be finished before we can read from its output
           {currentFrame.GetRenderFinishedSemaphore(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
           // Swap chain image must be available before we can write to it
           {currentFrame.GetImageAvailableSemaphore(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}
        }),
        SignalOn({
             currentFrame.GetSwapChainBlitFinishedSemaphore()
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
        eyeRenderData.vkImage = gPassColorImage.allocation.vkImage;
        eyeRenderData.queueFamilyIndex = m_vulkanObjs->GetPhysicalDevice()->GetGraphicsQueueFamilyIndex().value();
        eyeRenderData.width = gPassColorImage.image.size.w;
        eyeRenderData.height = gPassColorImage.image.size.h;
        eyeRenderData.format = gPassColorImage.image.vkFormat;
        eyeRenderData.sampleCount = 0;

        m_vulkanObjs->GetContext()->VR_SubmitEyeRender(Eye::Left, eyeRenderData);
        m_vulkanObjs->GetContext()->VR_SubmitEyeRender(Eye::Right, eyeRenderData);
    }

    /////////////////////////////////////////////////////
    // Present the swap chain image to the screen
    /////////////////////////////////////////////////////

    std::vector<VkSwapchainKHR> swapChains = {m_vulkanObjs->GetSwapChain()->GetVkSwapchainKHR()};

    // Present must wait until submitted render graphics commands have finished
    VkSemaphore presentWaitSemaphores[] = {currentFrame.GetSwapChainBlitFinishedSemaphore()};

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

void RendererVk::RenderObjects(const std::string& sceneName,
                               const FramebufferObjs& framebufferObjs,
                               const RenderParams& renderParams,
                               const std::vector<ViewProjection>& viewProjections,
                               const std::unordered_map<LightId, ImageId>& shadowMaps)
{
    auto& currentFrame = m_frames.GetCurrentFrame();
    const auto commandBuffer = currentFrame.GetRenderCommandBuffer();
    const auto renderPass = m_vulkanObjs->GetGPassRenderPass();

    //
    // Deferred Lighting Objects SubPass
    //
    {
        CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "DeferredRender");

        // Objects
        {
            CmdBufferSectionLabel innerSectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "Objects");

            m_objectRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
                .Render(
                    sceneName,
                    RenderType::GpassDeferred,
                    renderParams,
                    commandBuffer,
                    renderPass,
                    framebufferObjs.GetFramebuffer(),
                    viewProjections,
                    shadowMaps,
                    std::nullopt
                );
        }

        // Terrain
        {
            CmdBufferSectionLabel innerSectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "Terrain");
            m_terrainRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
                .Render(
                    sceneName,
                    renderParams,
                    commandBuffer,
                    renderPass,
                    framebufferObjs.GetFramebuffer(),
                    viewProjections
                );
        }
    }

    //
    // Deferred Lighting Subpass
    //
    {
        CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "DeferredLighting");

        commandBuffer->CmdNextSubpass();

        m_differedLightingRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
            .Render(
                sceneName,
                Material::Type::Object,
                renderParams,
                commandBuffer,
                renderPass,
                framebufferObjs.GetFramebuffer(),
                viewProjections,
                shadowMaps
            );
    }

    //
    // Forward Lighting Objects Subpass
    //
    {
        CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "ForwardRender");

        commandBuffer->CmdNextSubpass();

        // Debug Triangles
        {
            CmdBufferSectionLabel innerSectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "Triangle");

            m_rawTriangleRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
                .Render(
                    renderParams,
                    commandBuffer,
                    renderPass,
                    framebufferObjs.GetFramebuffer(),
                    viewProjections,
                    renderParams.debugTriangles
                );
        }

        // Skybox
        {
            CmdBufferSectionLabel innerSectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "SkyBox");

            m_skyBoxRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
                .Render(
                    renderParams,
                    commandBuffer,
                    renderPass,
                    framebufferObjs.GetFramebuffer(),
                    viewProjections
                );
        }

        // Forward Objects
        {
            CmdBufferSectionLabel innerSectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "Objects");

            m_objectRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
                .Render(
                    sceneName,
                    RenderType::GpassForward,
                    renderParams,
                    commandBuffer,
                    renderPass,
                    framebufferObjs.GetFramebuffer(),
                    viewProjections,
                    shadowMaps,
                    std::nullopt
                );
        }
    }
}

void RendererVk::RunPostProcessing(const LoadedImage& inputImage, const LoadedImage& outputImage, const PostProcessEffect& effect)
{
    auto& currentFrame = m_frames.GetCurrentFrame();
    const auto commandBuffer = currentFrame.GetRenderCommandBuffer();

    //
    // Prepare post-processing memory access
    //
    std::unordered_map<VkImage, ImageAccess> imageAccesses = {
        // We're going to read from the input texture
        {inputImage.allocation.vkImage, ImageAccess(
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            BarrierPoint(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT),
            BarrierPoint(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT),
            Layers(0, inputImage.image.numLayers),
            Levels(0,1),
            VK_IMAGE_ASPECT_COLOR_BIT
        )},
        // We're going to write to the output texture
        {outputImage.allocation.vkImage, ImageAccess(
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_GENERAL,
            BarrierPoint(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT),
            BarrierPoint(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT),
            Layers(0, outputImage.image.numLayers),
            Levels(0,1),
            VK_IMAGE_ASPECT_COLOR_BIT
        )}
    };

    for (const auto& inputSampler : effect.additionalSamplers)
    {
        const auto& loadedImage = std::get<1>(inputSampler);
        const auto& vkImageAspectFlags = std::get<2>(inputSampler);

        imageAccesses.insert({
            loadedImage.allocation.vkImage,
            ImageAccess(
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                BarrierPoint(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT),
                BarrierPoint(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT),
                Layers(0, loadedImage.image.numLayers),
                Levels(0, 1),
                vkImageAspectFlags
            )
        });
    }

    m_renderState.PrepareOperation(commandBuffer, RenderOperation(imageAccesses));

    //
    // Execute the post-processing effect shader
    //
    CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, effect.tag);

    m_postProcessingRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
        .Render(commandBuffer, inputImage, outputImage, effect);

    //
    // Blit the post-process output back to the input texture
    //
    m_renderState.PrepareOperation(commandBuffer, RenderOperation({
          // We're going to transfer/blit the result to the input texture
          {inputImage.allocation.vkImage,  ImageAccess(
              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
              BarrierPoint(
                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                  VK_ACCESS_TRANSFER_WRITE_BIT),
              BarrierPoint(
                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                  VK_ACCESS_TRANSFER_WRITE_BIT),
              Layers(0, inputImage.image.numLayers),
              Levels(0, 1),
              VK_IMAGE_ASPECT_COLOR_BIT
          )},
          // We're going to transfer/blit the result from the output texture
          {outputImage.allocation.vkImage, ImageAccess(
              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
              BarrierPoint(
                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                  VK_ACCESS_TRANSFER_READ_BIT),
              BarrierPoint(
                  VK_PIPELINE_STAGE_TRANSFER_BIT,
                  VK_ACCESS_TRANSFER_READ_BIT),
              Layers(0, outputImage.image.numLayers),
              Levels(0, 1),
              VK_IMAGE_ASPECT_COLOR_BIT
          )}
      }));

    VkImageBlit blit{};
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {(int)outputImage.image.size.w, (int)outputImage.image.size.h, 1};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = outputImage.image.numLayers;
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {(int)inputImage.image.size.w, (int)inputImage.image.size.h, 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = inputImage.image.numLayers;

    m_vulkanObjs->GetCalls()->vkCmdBlitImage(
        commandBuffer->GetVkCommandBuffer(),
        outputImage.allocation.vkImage,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        inputImage.allocation.vkImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &blit,
        VK_FILTER_LINEAR
    );
}

void RendererVk::RenderScreen(const std::string& sceneName,
                              const FramebufferObjs& framebufferObjs,
                              const RenderParams& renderParams)
{
    auto& currentFrame = m_frames.GetCurrentFrame();
    const auto commandBuffer = currentFrame.GetRenderCommandBuffer();
    const auto renderPass = m_vulkanObjs->GetScreenRenderPass();

    {
        CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "Screen");

        // Sprites
        {
            CmdBufferSectionLabel innerSectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "Sprite");

            m_spriteRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
                .Render(
                    sceneName,
                    renderParams,
                    commandBuffer,
                    renderPass,
                    framebufferObjs.GetFramebuffer()
                );
        }
    }
}

void RendererVk::RunSwapChainBlitPass(const VulkanFramebufferPtr& framebuffer,
                                      const LoadedImage& renderImage,
                                      const LoadedImage& screenImage)
{
    auto& currentFrame = m_frames.GetCurrentFrame();
    const auto commandBuffer = currentFrame.GetSwapChainBlitCommandBuffer();
    const auto renderPass = m_vulkanObjs->GetSwapChainBlitRenderPass();

    //
    // Blit Render SubPass
    //
    {
        CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "SwapChainBlit");

        m_swapChainRenderers.GetRendererForFrame(currentFrame.GetFrameIndex())
            .Render(
                commandBuffer,
                renderPass,
                framebuffer,
                renderImage,
                screenImage
            );
    }
}

bool RendererVk::StartRenderPass(const VulkanRenderPassPtr& renderPass,
                                 const FramebufferObjs& framebufferObjs,
                                 const VulkanCommandBufferPtr& commandBuffer,
                                 const glm::vec4& colorClearColor)
{
    //
    // Prepare a Render Operation associated with the Render Pass execution
    //
    const auto operation = RenderOperation::FromRenderPass(framebufferObjs, renderPass);
    if (!operation)
    {
        m_logger->Log(Common::LogLevel::Error, "StartRenderPass: Failed to create the render operation");
        return false;
    }

    m_renderState.PrepareOperation(commandBuffer, *operation);

    //
    // Start Render Pass
    //
    return StartRenderPass(renderPass, framebufferObjs.GetFramebuffer(), commandBuffer, colorClearColor);
}

bool RendererVk::StartRenderPass(const VulkanRenderPassPtr& renderPass,
                                 const VulkanFramebufferPtr& framebuffer,
                                 const VulkanCommandBufferPtr& commandBuffer,
                                 const glm::vec4& colorClearColor)
{
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
                clearValues[x].color = {{colorClearColor.r, colorClearColor.g, colorClearColor.b, colorClearColor.a}};
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
            return true;
        }
    );

    sceneUpdateTimer.StopTimer(m_metrics);

    return true;
}

void RendererVk::TransferObjectDetailImage(const LoadedImage& gPassObjectDetailImage,
                                           const LoadedImage& frameObjectDetailImage,
                                           const VulkanCommandBufferPtr& commandBuffer)
{
    CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "TransferObjectDetailImage");

    const auto renderSettings = m_vulkanObjs->GetRenderSettings();
    const auto presentLayerIndex = (uint32_t)renderSettings.presentEye;

    // Prepare access to the object detail image
    m_renderState.PrepareOperation(commandBuffer, RenderOperation({
        // We're going to transfer from the gpass object detail image
        {gPassObjectDetailImage.allocation.vkImage, ImageAccess(
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            BarrierPoint(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT),
            BarrierPoint(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT),
            Layers(presentLayerIndex, 1),
            Levels(0,1),
            VK_IMAGE_ASPECT_COLOR_BIT
        )},
        // We're going to transfer to the frame object detail result image
        {frameObjectDetailImage.allocation.vkImage, ImageAccess(
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            BarrierPoint(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT),
            BarrierPoint(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT),
            Layers(presentLayerIndex, 1),
            Levels(0,1),
            VK_IMAGE_ASPECT_COLOR_BIT
        )}
    }));

    //
    // Copy the gpass object detail image to the linearly tiled, host-accessible, per-frame object
    // detail image.
    //
    VkImageCopy vkImageCopy{};
    vkImageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkImageCopy.srcSubresource.mipLevel = 0;
    vkImageCopy.srcSubresource.baseArrayLayer = presentLayerIndex;
    vkImageCopy.srcSubresource.layerCount = 1;
    vkImageCopy.srcOffset = {0, 0, 0};
    vkImageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    vkImageCopy.dstSubresource.mipLevel = 0;
    vkImageCopy.dstSubresource.baseArrayLayer = presentLayerIndex;
    vkImageCopy.dstSubresource.layerCount = 1;
    vkImageCopy.dstOffset = {0, 0, 0};
    vkImageCopy.extent.width = gPassObjectDetailImage.image.size.w;
    vkImageCopy.extent.height = gPassObjectDetailImage.image.size.h;
    vkImageCopy.extent.depth = 1;

    m_vulkanObjs->GetCalls()->vkCmdCopyImage(
        commandBuffer->GetVkCommandBuffer(),
        gPassObjectDetailImage.allocation.vkImage,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        frameObjectDetailImage.allocation.vkImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &vkImageCopy
    );
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

    bool allSuccessful = true;

    if (!m_postExecutionOps->OnRenderSettingsChanged(renderSettings)) { allSuccessful = false; }
    if (!m_vulkanObjs->OnRenderSettingsChanged(renderSettings)) { allSuccessful = false; }
    if (!m_frames.OnRenderSettingsChanged(renderSettings)) { allSuccessful = false; }
    if (!m_swapChainRenderers.OnRenderSettingsChanged(renderSettings)) { allSuccessful = false; }
    if (!m_spriteRenderers.OnRenderSettingsChanged(renderSettings)) { allSuccessful = false; }
    if (!m_objectRenderers.OnRenderSettingsChanged(renderSettings)) { allSuccessful = false; }
    if (!m_terrainRenderers.OnRenderSettingsChanged(renderSettings)) { allSuccessful = false; }
    if (!m_skyBoxRenderers.OnRenderSettingsChanged(renderSettings)) { allSuccessful = false; }
    if (!m_differedLightingRenderers.OnRenderSettingsChanged(renderSettings)) { allSuccessful = false; }
    if (!m_rawTriangleRenderers.OnRenderSettingsChanged(renderSettings)) { allSuccessful = false; }
    if (!m_postProcessingRenderers.OnRenderSettingsChanged(renderSettings)) { allSuccessful = false; }
    if (!m_lights->OnRenderSettingsChanged(renderSettings)) { allSuccessful = false; }
    if (!m_renderTargets->OnRenderSettingsChanged(renderSettings)) { allSuccessful = false; }

    m_latestObjectDetailImageId = std::nullopt;

    return allSuccessful;
}

std::optional<ObjectId> RendererVk::GetTopObjectAtRenderPoint(const glm::vec2& renderPoint) const
{
    // Warning! All of this in run on engine thread, not render thread, to provide an instant answer
    // rather than an asynchronous answer. Can fail in scenarios such as parallel render settings change.

    std::lock_guard<std::mutex> lock(m_latestObjectDetailTextureIdMutex);

    if (!m_latestObjectDetailImageId)
    {
        return std::nullopt;
    }

    const auto objectDetailImage = m_images->GetImage(*m_latestObjectDetailImageId);
    if (!objectDetailImage)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RendererVk::GetTopObjectAtRenderPoint: Failed to fetch latest object detail image: {}", m_latestObjectDetailImageId->id);
        return std::nullopt;
    }

    const auto pObjectDetailImage = (unsigned char*)objectDetailImage->allocation.vmaAllocationInfo.pMappedData;

    const auto pixelByteStartOffset =
        ((unsigned int)renderPoint.x +
        ((unsigned int)renderPoint.y * m_vulkanObjs->GetRenderSettings().resolution.w))
        * m_renderTargets->GetObjectDetailPerPixelByteSize();

    // Note that ObjectId is stored in the first 4 of 8 bytes of each object detail pixel (material id is the second half)
    const IdType objectId = *(pObjectDetailImage + pixelByteStartOffset);

    return ObjectId(objectId);
}

void RendererVk::RefreshShadowMapsAsNeeded(const RenderParams& renderParams, const VulkanCommandBufferPtr& commandBuffer)
{
    CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "ShadowMapRenders");

    //
    // Let the lighting system invalidate shadow maps as needed, given the render camera parameters
    //
    m_lights->UpdateShadowMapsForCamera(renderParams.worldRenderCamera);

    //
    // Loop over all lights and run shadow renders for any which cast shadows and which have an
    // invalidated shadow map
    //
    for (const auto& loadedLight : m_lights->GetAllLights())
    {
        const bool lightCastsShadows = loadedLight.light.castsShadows && loadedLight.shadowFrameBufferId;
        const bool needsRefresh = lightCastsShadows && loadedLight.shadowInvalidated;

        if (!needsRefresh)
        {
            continue;
        }

        if (!RefreshShadowMap(renderParams, commandBuffer, loadedLight))
        {
            m_logger->Log(Common::LogLevel::Error,
              "RefreshShadowMapsAsNeeded: Failed to refresh shadow map for light id: {}", loadedLight.light.lightId.id);
        }
    }
}

bool RendererVk::RefreshShadowMap(const RenderParams& renderParams,
                                  const VulkanCommandBufferPtr& commandBuffer,
                                  const LoadedLight& loadedLight)
{
    //
    // Gather data / validation
    //
    auto& currentFrame = m_frames.GetCurrentFrame();

    VulkanRenderPassPtr shadowRenderPass{};

    switch (loadedLight.shadowMapType)
    {
        case ShadowMapType::Cascaded: { shadowRenderPass = m_vulkanObjs->GetShadowCascadedRenderPass(); } break;
        case ShadowMapType::Single: { shadowRenderPass = m_vulkanObjs->GetShadowSingleRenderPass(); } break;
        case ShadowMapType::Cube: { shadowRenderPass = m_vulkanObjs->GetShadowCubeRenderPass(); } break;
    }

    if (!loadedLight.shadowFrameBufferId)
    {
        m_logger->Log(Common::LogLevel::Warning,
          "RendererVk::RefreshShadowMap: Light doesn't have a shadow framebuffer, light id: {}", loadedLight.light.lightId.id);
        return false;
    }

    const auto shadowFramebufferId = *loadedLight.shadowFrameBufferId;
    const auto shadowFramebuffer = m_framebuffers->GetFramebufferObjs(shadowFramebufferId);

    if (!shadowFramebuffer || shadowFramebuffer->GetAttachmentImages()->size() != 1)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RendererVk::RefreshShadowMap: Shadow framebuffer doesn't exist or wrong attachment count, light id: {}, fb id: {}",
              loadedLight.light.lightId.id, shadowFramebufferId.id);
        return false;
    }

    const auto shadowMapImage = shadowFramebuffer->GetAttachmentImages()->at(0).first;

    const float lightMaxAffectRange = GetLightMaxAffectRange(m_vulkanObjs->GetRenderSettings(), loadedLight.light);

    //
    // Set up and run a shadow render for the light for each shadow map cube face that's invalidated
    //
    {
        CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, std::format("ShadowMapRender-Light-{}", loadedLight.light.lightId.id));

        if (!StartRenderPass(shadowRenderPass, *shadowFramebuffer, commandBuffer)) { return false; }

            //
            // Clear any existing shadow map data
            //
            VkClearAttachment vkClearAttachment{};
            vkClearAttachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            vkClearAttachment.clearValue.depthStencil.depth = 1.0f;
            vkClearAttachment.clearValue.depthStencil.stencil = 0.0f;

            VkClearRect vkClearRect = {};
            vkClearRect.rect = {{0, 0}, {shadowMapImage.image.size.w, shadowMapImage.image.size.h}};
            vkClearRect.baseArrayLayer = 0;
            vkClearRect.layerCount = 1;

            commandBuffer->CmdClearAttachments({vkClearAttachment}, {vkClearRect});

            //
            // Render the shadow map
            //
            std::vector<ViewProjection> shadowViewProjections;

            for (const auto& shadowRender : loadedLight.shadowRenders)
            {
                shadowViewProjections.push_back(shadowRender.viewProjection);
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
                    {},
                    ObjectRenderer::ShadowRenderData(loadedLight.shadowMapType, lightMaxAffectRange)
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
