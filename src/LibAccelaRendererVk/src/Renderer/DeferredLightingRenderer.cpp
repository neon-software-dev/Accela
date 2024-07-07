/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "DeferredLightingRenderer.h"
#include "RendererCommon.h"

#include "../PostExecutionOp.h"

#include "../Program/IPrograms.h"
#include "../Pipeline/PipelineUtil.h"
#include "../Mesh/IMeshes.h"
#include "../Buffer/CPUItemBuffer.h"
#include "../Texture/ITextures.h"
#include "../Image/IImages.h"
#include "../Light/ILights.h"
#include "../Material/IMaterials.h"

#include "../Vulkan/VulkanPipeline.h"
#include "../Vulkan/VulkanFramebuffer.h"
#include "../Vulkan/VulkanCommandBuffer.h"
#include "../Vulkan/VulkanDescriptorSet.h"

#include <Accela/Render/Mesh/StaticMesh.h>

#include <format>

namespace Accela::Render
{

DeferredLightingRenderer::DeferredLightingRenderer(
    Common::ILogger::Ptr logger,
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
    uint8_t frameIndex)
    : Renderer(std::move(logger),
               std::move(metrics),
               std::move(ids),
               std::move(postExecutionOps),
               std::move(vulkanObjs),
               std::move(programs),
               std::move(shaders),
               std::move(pipelines),
               std::move(buffers),
               std::move(materials),
               std::move(images),
               std::move(textures),
               std::move(meshes),
               std::move(lights),
               std::move(renderables),
               frameIndex)
{

}

bool DeferredLightingRenderer::Initialize(const RenderSettings& renderSettings)
{
    if (!Renderer::Initialize(renderSettings))
    {
        return false;
    }

    const auto programDef = m_programs->GetProgramDef("DeferredLighting");
    if (programDef == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "DeferredLightingRenderer: Failed to find DeferredLighting program");
        return false;
    }

    if (!CreateMesh())
    {
        m_logger->Log(Common::LogLevel::Error, "DeferredLightingRenderer: Failed to create mesh");
        return false;
    }

    //
    // Update state
    //
    m_programDef = programDef;

    return true;
}

bool DeferredLightingRenderer::CreateMesh()
{
    const auto meshId = m_ids->meshIds.GetId();

    // TODO: Why does the winding order differ than what swap chain blit renderer has?
    const auto mesh = std::make_shared<StaticMesh>(
        meshId,
        std::vector<MeshVertex>{
            {
                {{-1.0f, -1.0f, 0}, {0, 0, 0}, {0, 0}},
                {{1.0f, -1.0f, 0}, {0, 0, 0}, {0, 0}},
                {{1.0f, 1.0f, 0}, {0, 0, 0}, {0, 0}},
                {{-1.0f, 1.0f, 0}, {0, 0, 0}, {0, 0}}
            }
        },
        std::vector<uint32_t>{ 0, 2, 1, 0, 3, 2 },
        std::format("DeferredLighting-{}", m_frameIndex)
    );

    if (!m_meshes->LoadMesh(mesh, MeshUsage::Static, std::promise<bool>{}))
    {
        m_logger->Log(Common::LogLevel::Error, "DeferredLightingRenderer: Failed to create mesh");
        m_ids->meshIds.ReturnId(mesh->id);
        return false;
    }

    m_mesh = *m_meshes->GetLoadedMesh(meshId);

    return true;
}

void DeferredLightingRenderer::Destroy()
{
    if (m_mesh.id.IsValid())
    {
        m_meshes->DestroyMesh(m_mesh.id, true);
        m_mesh = {};
    }

    m_programDef = nullptr;

    Renderer::Destroy();
}

void DeferredLightingRenderer::Render(const std::string& sceneName,
                                      const Material::Type& materialType,
                                      const RenderParams& renderParams,
                                      const VulkanCommandBufferPtr& commandBuffer,
                                      const VulkanRenderPassPtr& renderPass,
                                      const VulkanFramebufferPtr& framebuffer,
                                      const std::vector<ViewProjection>& viewProjections,
                                      const std::unordered_map<LightId, ImageId>& shadowMaps)
{
    BindState bindState{};

    //
    // Bind Pipeline
    //
    if (!BindPipeline(bindState, commandBuffer, renderPass, framebuffer)) { return; }

    //
    // Bind PushConstants
    //
    LightingSettingPayload payload{};
    payload.hdr = m_renderSettings.hdr;

    commandBuffer->CmdPushConstants(*bindState.pipeline, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LightingSettingPayload), &payload);

    //
    // Bind Descriptor Sets
    //
    if (!BindDescriptorSet0(sceneName, bindState, renderParams, commandBuffer, viewProjections, shadowMaps)) { return; }
    if (!BindDescriptorSet1(bindState, commandBuffer, framebuffer)) { return; }
    if (!BindDescriptorSet2(bindState, materialType, commandBuffer)) { return; }

    //
    // Draw
    //
    BindVertexBuffer(bindState, commandBuffer, m_mesh.verticesBuffer->GetBuffer());
    BindIndexBuffer(bindState, commandBuffer, m_mesh.indicesBuffer->GetBuffer());

    commandBuffer->CmdDrawIndexed(
        m_mesh.numIndices,
        1,
        0,
        0,
        0
    );
}

std::expected<VulkanPipelinePtr, bool> DeferredLightingRenderer::BindPipeline(BindState& bindState,
                                                                              const VulkanCommandBufferPtr& commandBuffer,
                                                                              const VulkanRenderPassPtr& renderPass,
                                                                              const VulkanFramebufferPtr& framebuffer)
{
    //
    // Retrieve the pipeline to use for rendering the batch
    //
    const auto viewport = Viewport(0, 0, framebuffer->GetSize()->w, framebuffer->GetSize()->h);

    std::vector<PushConstantRange> pushConstantRanges;

    pushConstantRanges = {
        {VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LightingSettingPayload)}
    };

    auto pipeline = GetGraphicsPipeline(
        m_logger,
        m_vulkanObjs,
        m_shaders,
        m_pipelines,
        m_programDef,
        renderPass,
        GPassRenderPass_SubPass_DeferredLightingRender,
        viewport,
        CullFace::Back,
        PolygonFillMode::Fill,
        DepthBias::Disabled,
        pushConstantRanges,
        m_frameIndex,
        m_pipelineHash
    );
    if (!pipeline)
    {
        m_logger->Log(Common::LogLevel::Error, "ObjectRenderer: GetBatchPipeline: Failed to fetch batch pipeline");
        return std::unexpected(false);
    }

    // Keep track of the latest pipeline hash that was used for this program
    m_pipelineHash = (*pipeline)->GetConfigHash();

    // Bind the pipeline
    commandBuffer->CmdBindPipeline(*pipeline);
    bindState.OnPipelineBound(m_programDef, *pipeline);

    return *pipeline;
}

bool DeferredLightingRenderer::BindDescriptorSet0(const std::string& sceneName,
                                                  BindState& bindState,
                                                  const RenderParams& renderParams,
                                                  const VulkanCommandBufferPtr& commandBuffer,
                                                  const std::vector<ViewProjection>& viewProjections,
                                                  const std::unordered_map<LightId, ImageId>& shadowMaps) const
{
    //
    // If the set isn't invalidated, nothing to do
    //
    if (!bindState.set0Invalidated) { return true; }

    //
    // Create a descriptor set
    //
    const auto globalDataDescriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        (*bindState.programDef)->GetDescriptorSetLayouts()[0],
        std::format("DeferredLightingRenderer-DS0-{}", m_frameIndex)
    );
    if (!globalDataDescriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error,
          "DeferredLightingRenderer::BindDescriptorSet0: Failed to get or create global data descriptor set");
        return false;
    }

    //
    // Update the descriptor set with data
    //
    const auto sceneLights = m_lights->GetSceneLights(sceneName, viewProjections);

    if (!BindDescriptorSet0_Global(bindState, renderParams, *globalDataDescriptorSet, sceneLights)) { return false; }
    if (!BindDescriptorSet0_ViewProjection(bindState, viewProjections, *globalDataDescriptorSet)) { return false; }
    if (!BindDescriptorSet0_Lights(bindState, *globalDataDescriptorSet, sceneLights, shadowMaps)) { return false; }

    //
    // Bind the global data descriptor set
    //
    commandBuffer->CmdBindDescriptorSets(*bindState.pipeline, 0, {(*globalDataDescriptorSet)->GetVkDescriptorSet()});
    bindState.OnSet0Bound();

    return true;
}

bool DeferredLightingRenderer::BindDescriptorSet0_Global(const BindState& bindState,
                                                         const RenderParams& renderParams,
                                                         const VulkanDescriptorSetPtr& globalDataDescriptorSet,
                                                         const std::vector<LoadedLight>& lights) const
{
    //
    // Create a buffer
    //
    const auto globalDataBuffer = CPUItemBuffer<GlobalPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        1,
        std::format("DeferredLightingRenderer-DS0-{}", m_frameIndex)
    );
    if (!globalDataBuffer)
    {
        m_logger->Log(Common::LogLevel::Error, "DeferredLightingRenderer::BindDescriptorSet0_Global: Failed to create global data buffer");
        return false;
    }

    //
    // Update the global data buffer with the global data
    //
    const GlobalPayload globalPayload = GetGlobalPayload(renderParams, lights.size());
    (*globalDataBuffer)->PushBack(ExecutionContext::CPU(), {globalPayload});

    //
    // Bind the global data buffer to the global data descriptor set
    //
    globalDataDescriptorSet->WriteBufferBind(
        (*bindState.programDef)->GetBindingDetailsByName("u_globalData"),
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        (*globalDataBuffer)->GetBuffer()->GetVkBuffer(),
        0,
        0
    );

    //
    // Post-frame cleanup
    //
    m_postExecutionOps->Enqueue_Current(BufferDeleteOp(m_buffers, (*globalDataBuffer)->GetBuffer()->GetBufferId()));

    return true;
}

bool DeferredLightingRenderer::BindDescriptorSet0_ViewProjection(BindState& bindState,
                                                                 const std::vector<ViewProjection>& viewProjections,
                                                                 const VulkanDescriptorSetPtr& descriptorSet) const
{
    //
    // Create buffer
    //
    const auto viewProjectionDataBuffer = CPUItemBuffer<ViewProjectionPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        1,
        std::format("DeferredLightingRenderer-DS0-ViewProjectionData-{}", m_frameIndex)
    );
    if (!viewProjectionDataBuffer)
    {
        m_logger->Log(Common::LogLevel::Error,
          "DeferredLightingRenderer::BindGlobalData_ViewProjection: Failed to create view projection data buffer");
        return false;
    }

    //
    // Set Data
    //
    std::vector<ViewProjectionPayload> viewProjectionPayloads;

    for (const auto& viewProjection : viewProjections)
    {
        viewProjectionPayloads.push_back(GetViewProjectionPayload(viewProjection));
    }

    (*viewProjectionDataBuffer)->PushBack(ExecutionContext::CPU(), viewProjectionPayloads);

    descriptorSet->WriteBufferBind(
        (*bindState.programDef)->GetBindingDetailsByName("i_viewProjectionData"),
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        (*viewProjectionDataBuffer)->GetBuffer()->GetVkBuffer(),
        0,
        0
    );

    //
    // Post-Frame Cleanup
    //
    m_postExecutionOps->Enqueue_Current(BufferDeleteOp(m_buffers, (*viewProjectionDataBuffer)->GetBuffer()->GetBufferId()));

    return true;
}

bool DeferredLightingRenderer::BindDescriptorSet0_Lights(const BindState& bindState,
                                                         const VulkanDescriptorSetPtr& globalDataDescriptorSet,
                                                         const std::vector<LoadedLight>& lights,
                                                         const std::unordered_map<LightId, ImageId>& shadowMaps) const
{
    //
    // Create a per-render CPU buffer for holding light data
    //
    // (Note that it reserves space for at least one light, so that the buffer creation doesn't fail).
    //
    const auto lightDataBuffer = CPUItemBuffer<LightPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        std::max((std::size_t)1U, lights.size()),
        std::format("DeferredLightingRenderer-DS0-LightData-{}", m_frameIndex)
    );
    if (!lightDataBuffer)
    {
        m_logger->Log(Common::LogLevel::Error, "DeferredLightingRenderer::BindDescriptorSet0_Lights: Failed to create light data buffer");
        return false;
    }

    //
    // Calculate light data
    //
    const auto defaultLightImages = std::vector<ImageId>(Max_Light_Count, Render::ImageId(Render::INVALID_ID));

    std::unordered_map<ShadowMapType, std::vector<ImageId>> shadowMapImageIds {
        {ShadowMapType::Single, defaultLightImages},
        {ShadowMapType::Cube, defaultLightImages}
    };

    // TODO Perf: Cull out lights that are a certain distance away from the camera
    for (unsigned int lightIndex = 0; lightIndex < lights.size(); ++lightIndex)
    {
        const LoadedLight& loadedLight = lights[lightIndex];
        const Light& light = loadedLight.light;

        LightPayload lightPayload{};
        lightPayload.shadowMapType = static_cast<uint32_t>(loadedLight.shadowMapType);
        lightPayload.worldPos = light.worldPos;
        lightPayload.maxAffectRange = GetLightMaxAffectRange(m_renderSettings, light);
        lightPayload.attenuationMode = static_cast<uint32_t>(light.lightProperties.attenuationMode);
        lightPayload.diffuseColor = light.lightProperties.diffuseColor;
        lightPayload.diffuseIntensity = light.lightProperties.diffuseIntensity;
        lightPayload.specularColor = light.lightProperties.specularColor;
        lightPayload.specularIntensity = light.lightProperties.specularIntensity;
        lightPayload.directionUnit = light.lightProperties.directionUnit;
        lightPayload.coneFovDegrees = light.lightProperties.coneFovDegrees;

        // Single shadow maps need their light-space transform supplied to the lighting shader. Cube
        // shadow maps don't as we can do the transformation manually.
        if (loadedLight.shadowMapType == ShadowMapType::Single)
        {
            const auto lightViewProjection = GetShadowMapViewProjection(m_renderSettings, loadedLight);
            assert(lightViewProjection);

            lightPayload.lightTransform = lightViewProjection->GetTransformation();
        }

        // If the light has a shadow map, update its payload to know about it (from its -1 default),
        // and record the texture id for texture binding further on
        const auto lightShadowMapIt = shadowMaps.find(light.lightId);
        if (lightShadowMapIt != shadowMaps.cend())
        {
            shadowMapImageIds[loadedLight.shadowMapType][lightIndex] = lightShadowMapIt->second;
            lightPayload.shadowMapIndex = (int)lightIndex;
        }

        //
        // Update the light data buffer with the light data
        //
        (*lightDataBuffer)->PushBack(ExecutionContext::CPU(), {lightPayload});
    }

    //
    // Bind the light data buffer to the global data descriptor set
    //
    globalDataDescriptorSet->WriteBufferBind(
        (*bindState.programDef)->GetBindingDetailsByName("i_lightData"),
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        (*lightDataBuffer)->GetBuffer()->GetVkBuffer(),
        0,
        0
    );

    //
    // Bind shadow map textures
    //
    if (!BindDescriptorSet0_ShadowMapTextures(bindState, globalDataDescriptorSet, shadowMapImageIds))
    {
        m_logger->Log(Common::LogLevel::Error, "DeferredLightingRenderer::BindDescriptorSet0_Lights: Failed to bind shadow maps");
        return false;
    }

    //
    // Post-frame cleanup
    //
    m_postExecutionOps->Enqueue_Current(BufferDeleteOp(m_buffers, (*lightDataBuffer)->GetBuffer()->GetBufferId()));

    return true;
}

bool DeferredLightingRenderer::BindDescriptorSet0_ShadowMapTextures(const BindState& bindState,
                                                                    const VulkanDescriptorSetPtr& globalDataDescriptorSet,
                                                                    const std::unordered_map<ShadowMapType, std::vector<ImageId>>& shadowMapImageIds) const
{
    //
    // Cube shadow map binding details
    //
    const auto shadowMapBindingDetails = (*bindState.programDef)->GetBindingDetailsByName("i_shadowSampler");
    if (!shadowMapBindingDetails)
    {
        m_logger->Log(Common::LogLevel::Error,
          "DeferredLightingRenderer::BindDescriptorSet0_ShadowMapTextures: No such shadow map binding point exists: i_shadowSampler");
        return false;
    }

    const auto shadowMapBindingDetails_Cube = (*bindState.programDef)->GetBindingDetailsByName("i_shadowSampler_cubeMap");
    if (!shadowMapBindingDetails_Cube)
    {
        m_logger->Log(Common::LogLevel::Error,
          "DeferredLightingRenderer::BindDescriptorSet0_ShadowMapTextures: No such shadow map binding point exists: i_shadowSampler_cubeMap");
        return false;
    }

    //
    // Missing texture binds
    //
    const auto missingTexture = m_textures->GetMissingTexture();
    const auto missingCubeTexture = m_textures->GetMissingCubeTexture();

    //
    // Parameters for binding shadow map textures for a particular light type
    //
    VulkanDescriptorSetLayout::BindingDetails shadowBindingDetails{};
    std::string shadowImageViewName;
    std::string shadowSamplerName;
    VkImageView missingTextureImageView{VK_NULL_HANDLE};
    VkSampler missingTextureSampler{VK_NULL_HANDLE};

    for (const auto& typeIt : shadowMapImageIds)
    {
        switch (typeIt.first)
        {
            case ShadowMapType::Single:
            {
                shadowBindingDetails = *shadowMapBindingDetails;
                shadowImageViewName = ImageView::DEFAULT;
                shadowSamplerName = ImageSampler::DEFAULT;
                missingTextureImageView = missingTexture.second.vkImageViews.at(ImageView::DEFAULT);
                missingTextureSampler = missingTexture.second.vkSamplers.at(ImageSampler::DEFAULT);
            }
                break;

            case ShadowMapType::Cube:
            {
                shadowBindingDetails = *shadowMapBindingDetails_Cube;
                shadowImageViewName = ImageView::DEFAULT;
                shadowSamplerName = ImageSampler::DEFAULT;
                missingTextureImageView = missingCubeTexture.second.vkImageViews.at(ImageView::DEFAULT);
                missingTextureSampler = missingCubeTexture.second.vkSamplers.at(ImageSampler::DEFAULT);
            }
            break;
        }

        std::vector<std::pair<VkImageView, VkSampler>> samplerBinds;

        for (const auto& imageId : typeIt.second)
        {
            const auto imageOpt = m_images->GetImage(imageId);

            if (imageOpt)
            {
                samplerBinds.emplace_back(
                    imageOpt->vkImageViews.at(shadowImageViewName),
                    imageOpt->vkSamplers.at(shadowSamplerName)
                );
            }
            else
            {
                samplerBinds.emplace_back(missingTextureImageView, missingTextureSampler);
            }
        }

        globalDataDescriptorSet->WriteCombinedSamplerBind(shadowBindingDetails, samplerBinds);
    }

    return true;
}

bool DeferredLightingRenderer::BindDescriptorSet1(BindState& bindState,
                                                  const VulkanCommandBufferPtr& commandBuffer,
                                                  const VulkanFramebufferPtr& framebuffer)
{
    //
    // If the set isn't invalidated, nothing to do
    //
    if (!bindState.set1Invalidated) { return true; }

    //
    // Create a descriptor set
    //
    const auto descriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        m_programDef->GetDescriptorSetLayouts()[1],
        std::format("DeferredLightingRenderer-DS1-{}", m_frameIndex)
    );
    if (!descriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error,
          "DeferredLightingRenderer::BindDescriptorSet1: Failed to get or create descriptor set");
        return false;
    }

    //
    // Update the descriptor set
    //
    (*descriptorSet)->WriteInputAttachmentBind(
        m_programDef->GetBindingDetailsByName("i_vertexPosition_worldSpace"),
        framebuffer->GetAttachments()[1]
    );
    (*descriptorSet)->WriteInputAttachmentBind(
        m_programDef->GetBindingDetailsByName("i_vertexNormal_viewSpace"),
        framebuffer->GetAttachments()[2]
    );
    (*descriptorSet)->WriteInputAttachmentBind(
        m_programDef->GetBindingDetailsByName("i_vertexObjectDetail"),
        framebuffer->GetAttachments()[3]
    );
    (*descriptorSet)->WriteInputAttachmentBind(
        m_programDef->GetBindingDetailsByName("i_vertexAmbientColor"),
        framebuffer->GetAttachments()[4]
    );
    (*descriptorSet)->WriteInputAttachmentBind(
        m_programDef->GetBindingDetailsByName("i_vertexDiffuseColor"),
        framebuffer->GetAttachments()[5]
    );
    (*descriptorSet)->WriteInputAttachmentBind(
        m_programDef->GetBindingDetailsByName("i_vertexSpecularColor"),
        framebuffer->GetAttachments()[6]
    );

    //
    // Bind the descriptor set
    //
    commandBuffer->CmdBindDescriptorSets(*bindState.pipeline, 1, {(*descriptorSet)->GetVkDescriptorSet()});
    bindState.OnSet1Bound();

    return true;
}

bool DeferredLightingRenderer::BindDescriptorSet2(BindState& bindState,
                                                  const Material::Type& materialType,
                                                  const VulkanCommandBufferPtr& commandBuffer)
{
    //
    // If descriptor set is already valid, nothing to do
    //
    if (!bindState.set2Invalidated) { return true; }

    //
    // Otherwise, retrieve a descriptor set for binding material data
    //
    const auto materialDataDescriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        (*bindState.programDef)->GetDescriptorSetLayouts()[2],
        std::format("DeferredLightingRenderer-DS2-MaterialData-{}", m_frameIndex)
    );
    if (!materialDataDescriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error,
          "DeferredLightingRenderer::BindDescriptorSet2: Failed to get or create material data descriptor set");
        return false;
    }

    //
    // Update the descriptor set with data
    //
    if (!BindDescriptorSet2_MaterialData(bindState, materialType, *materialDataDescriptorSet)) { return false; }

    //
    // Bind the material data descriptor set
    //
    commandBuffer->CmdBindDescriptorSets(*bindState.pipeline, 2, {(*materialDataDescriptorSet)->GetVkDescriptorSet()});
    bindState.OnSet2Bound();

    return true;
}

bool DeferredLightingRenderer::BindDescriptorSet2_MaterialData(BindState& bindState,
                                                               const Material::Type& materialType,
                                                               const VulkanDescriptorSetPtr& materialDataDescriptorSet)
{
    const auto materialDataBuffer = m_materials->GetMaterialBufferForType(materialType);
    if (!materialDataBuffer)
    {
        return false;
    }

    materialDataDescriptorSet->WriteBufferBind(
        (*bindState.programDef)->GetBindingDetailsByName("i_materialData"),
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        (*materialDataBuffer)->GetBuffer()->GetVkBuffer(),
        0,
        (*materialDataBuffer)->GetDataByteSize()
    );

    return true;
}

void DeferredLightingRenderer::BindVertexBuffer(BindState& bindState,
                                                const VulkanCommandBufferPtr& commandBuffer,
                                                const BufferPtr& vertexBuffer)
{
    //
    // If the vertex buffer is already bound, nothing to do
    //
    if (vertexBuffer == bindState.vertexBuffer) { return; }

    //
    // Bind the vertex buffer
    //
    commandBuffer->CmdBindVertexBuffers(0, 1, {vertexBuffer->GetVkBuffer()}, {VkDeviceSize(0)});

    //
    // Update render state
    //
    bindState.OnVertexBufferBound(vertexBuffer);
}

void DeferredLightingRenderer::BindIndexBuffer(BindState& bindState,
                                               const VulkanCommandBufferPtr& commandBuffer,
                                               const BufferPtr& indexBuffer)
{
    //
    // If the index buffer is already bound, nothing to do
    //
    if (indexBuffer == bindState.indexBuffer) { return; }

    //
    // Bind the index buffer
    //
    commandBuffer->CmdBindIndexBuffer(indexBuffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT32);

    //
    // Update render state
    //
    bindState.OnIndexBufferBound(indexBuffer);
}

}
