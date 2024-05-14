/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SkyBoxRenderer.h"
#include "RendererCommon.h"

#include "../PostExecutionOp.h"

#include "../Mesh/IMeshes.h"
#include "../Buffer/CPUItemBuffer.h"
#include "../Program/IPrograms.h"
#include "../Pipeline/IPipelineFactory.h"
#include "../Pipeline/PipelineUtil.h"
#include "../Texture/ITextures.h"

#include "../Vulkan/VulkanFramebuffer.h"
#include "../Vulkan/VulkanPipeline.h"
#include "../Vulkan/VulkanCommandBuffer.h"
#include "../Vulkan/VulkanDebug.h"
#include "../Vulkan/VulkanDescriptorSet.h"

#include <Accela/Render/Mesh/StaticMesh.h>

#include <format>

namespace Accela::Render
{

SkyBoxRenderer::SkyBoxRenderer(Common::ILogger::Ptr logger,
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
               std::move(textures),
               std::move(meshes),
               std::move(lights),
               std::move(renderables),
               frameIndex)
{

}

bool SkyBoxRenderer::Initialize(const RenderSettings& renderSettings)
{
    if (!Renderer::Initialize(renderSettings))
    {
        return false;
    }

    m_programDef = m_programs->GetProgramDef("SkyBox");
    if (m_programDef == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "SkyBoxRenderer: SkyBox program doesn't exist");
        return false;
    }

    if (!CreateSkyBoxMesh())
    {
        return false;
    }

    return true;
}

void SkyBoxRenderer::Destroy()
{
    if (m_skyBoxMesh.id.IsValid())
    {
        m_meshes->DestroyMesh(m_skyBoxMesh.id, true);
        m_skyBoxMesh = {};
    }

    m_programDef = nullptr;

    if (m_pipelineHash)
    {
        m_pipelines->DestroyPipeline(*m_pipelineHash);
        m_pipelineHash = std::nullopt;
    }

    Renderer::Destroy();
}

bool SkyBoxRenderer::CreateSkyBoxMesh()
{
    const auto meshId = m_ids->meshIds.GetId();

    const auto mesh = std::make_shared<StaticMesh>(
        meshId,
        std::vector<MeshVertex>{
            // Back
            {glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0,0,-1), glm::vec2(1,1)},
            {glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0,0,-1), glm::vec2(0,1)},
            {glm::vec3(1.0f,  1.0f, -1.0f), glm::vec3(0,0,-1), glm::vec2(0,0)},
            {glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3(0,0,-1), glm::vec2(1,0)},

            // Front
            {glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0,0,1), glm::vec2(1,1)},
            {glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0,0,1), glm::vec2(0,1)},
            {glm::vec3(-1.0f,  1.0f, 1.0f), glm::vec3(0,0,1), glm::vec2(0,0)},
            {glm::vec3(1.0f,  1.0f, 1.0f), glm::vec3(0,0,1), glm::vec2(1,0)},

            // Left
            {glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(-1,0,0), glm::vec2(1,1)},
            {glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(-1,0,0), glm::vec2(0,1)},
            {glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3(-1,0,0), glm::vec2(0,0)},
            {glm::vec3(-1.0f,  1.0f, 1.0f), glm::vec3(-1,0,0), glm::vec2(1,0)},

            // Right
            {glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(1,0,0), glm::vec2(1,1)},
            {glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(1,0,0), glm::vec2(0,1)},
            {glm::vec3(1.0f,  1.0f, 1.0f), glm::vec3(1,0,0), glm::vec2(0,0)},
            {glm::vec3(1.0f,  1.0f, -1.0f), glm::vec3(1,0,0), glm::vec2(1,0)},

            // Top
            {glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0,1,0), glm::vec2(0,0)},
            {glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0,1,0), glm::vec2(1,0)},
            {glm::vec3(1.0f,  1.0f, 1.0f), glm::vec3(0,1,0), glm::vec2(1,1)},
            {glm::vec3(-1.0f,  1.0f, 1.0f), glm::vec3(0,1,0), glm::vec2(0,1)},

            // Bottom
            {glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0,-1,0), glm::vec2(0,0)},
            {glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0,-1,0), glm::vec2(1,0)},
            {glm::vec3(1.0f,  -1.0f, -1.0f), glm::vec3(0,-1,0), glm::vec2(1,1)},
            {glm::vec3(-1.0f,  -1.0f, -1.0f), glm::vec3(0,-1,0), glm::vec2(0,1)}
        },
        std::vector<uint32_t>{
            0, 1, 2, 0, 2, 3,           // Front
            4, 5, 6, 4, 6, 7,           // Back
            8, 9, 10, 8, 10, 11,        // Left
            12, 13, 14, 12, 14, 15,     // Right
            16, 17, 18, 16, 18, 19,     // Top
            20, 21, 22, 20, 22, 23      // Bottom
        },
        std::format("SkyBoxRenderer-{}", m_frameIndex)
    );

    if (!m_meshes->LoadMesh(mesh, MeshUsage::Static, std::promise<bool>{}))
    {
        m_logger->Log(Common::LogLevel::Error, "SkyBoxRenderer: Failed to create skybox mesh");
        m_ids->meshIds.ReturnId(mesh->id);
        return false;
    }

    m_skyBoxMesh = m_meshes->GetLoadedMesh(meshId).value();

    return true;
}

void SkyBoxRenderer::Render(const RenderParams& renderParams,
                            const VulkanCommandBufferPtr& commandBuffer,
                            const VulkanRenderPassPtr& renderPass,
                            const VulkanFramebufferPtr& framebuffer,
                            const std::vector<ViewProjection>& viewProjections)
{
    //
    // If no sky map is requested, bail out
    //
    if (!renderParams.skyBoxTextureId) { return; }

    CmdBufferSectionLabel sectionLabel(
        m_vulkanObjs->GetCalls(),
        commandBuffer,
        "SkyBoxRenderer"
    );

    //
    // Retrieve the pipeline to use for rendering
    //
    const auto viewport = Viewport(0, 0, framebuffer->GetSize()->w, framebuffer->GetSize()->h);

    auto pipeline = GetPipeline(
        m_logger,
        m_vulkanObjs,
        m_shaders,
        m_pipelines,
        m_programDef,
        renderPass,
        Offscreen_ForwardSubpass_Index,
        viewport,
        CullFace::Back,
        PolygonFillMode::Fill,
        DepthBias::Disabled,
        PushConstantRange::None(),
        m_frameIndex,
        m_pipelineHash
    );
    if (!pipeline)
    {
        m_logger->Log(Common::LogLevel::Error, "SkyBoxRenderer: Failed to fetch pipeline");
        return;
    }

    // Keep track of the latest pipeline hash that was used
    m_pipelineHash = (*pipeline)->GetConfigHash();

    //
    // Bind data
    //
    commandBuffer->CmdBindPipeline(*pipeline);

    if (!BindGlobalDescriptorSet(renderParams, commandBuffer, *pipeline, viewProjections)) { return; }
    if (!BindMaterialDescriptorSet(renderParams, commandBuffer, *pipeline)) { return; }

    BindMeshData(commandBuffer);

    //
    // Draw
    //
    commandBuffer->CmdDrawIndexed(
        m_skyBoxMesh.numIndices,
        1,
        0,
        0,
        0
    );
}

void SkyBoxRenderer::BindMeshData(const VulkanCommandBufferPtr& commandBuffer) const
{
    const auto verticesVkBuffer = m_skyBoxMesh.verticesBuffer->GetBuffer()->GetVkBuffer();
    const auto indicesVkBuffer = m_skyBoxMesh.indicesBuffer->GetBuffer()->GetVkBuffer();

    commandBuffer->CmdBindVertexBuffers(0, 1, {verticesVkBuffer}, {VkDeviceSize(0)});
    commandBuffer->CmdBindIndexBuffer(indicesVkBuffer, 0, VK_INDEX_TYPE_UINT32);
}

bool SkyBoxRenderer::BindGlobalDescriptorSet(const RenderParams& renderParams,
                                             const VulkanCommandBufferPtr& commandBuffer,
                                             const VulkanPipelinePtr& pipeline,
                                             const std::vector<ViewProjection>& viewProjections) const
{
    //
    // Fetch the Descriptor Set
    //
    const auto globalDataDescriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        m_programDef->GetDescriptorSetLayouts()[0],
        std::format("SkyBoxRenderer-GlobalData-{}", m_frameIndex)
    );
    if (!globalDataDescriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error, "SkyBoxRenderer: Failed to get or create global data descriptor set");
        return false;
    }

    //
    // Update the Descriptor Set with global data. (Note that the SkyBox pipeline doesn't need global light data).
    //
    if (!UpdateGlobalDescriptorSet_Global(renderParams, *globalDataDescriptorSet)) { return false; }
    if (!UpdateGlobalDescriptorSet_ViewProjection(*globalDataDescriptorSet, viewProjections)) { return false; }

    //
    // Bind the Descriptor Set
    //
    commandBuffer->CmdBindDescriptorSets(pipeline, 0, {(*globalDataDescriptorSet)->GetVkDescriptorSet()});

    return true;
}

bool SkyBoxRenderer::UpdateGlobalDescriptorSet_Global(const RenderParams& renderParams,
                                                      const VulkanDescriptorSetPtr& globalDataDescriptorSet) const
{
    //
    // Create a per-render CPU buffer for holding global data
    //
    const auto globalDataBuffer = CPUItemBuffer<GlobalPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        1,
        std::format("ObjectRenderer-GlobalData-{}", m_frameIndex)
    );
    if (!globalDataBuffer)
    {
        m_logger->Log(Common::LogLevel::Error, "ObjectRenderer::BindDescriptorSet0_Global: Failed to create global data buffer");
        return false;
    }

    //
    // Update the global data buffer with the global data
    //

    GlobalPayload globalPayload = GetGlobalPayload(renderParams, 0);

    (*globalDataBuffer)->PushBack(ExecutionContext::CPU(), {globalPayload});

    //
    // Bind the global data buffer to the global data descriptor set
    //
    globalDataDescriptorSet->WriteBufferBind(
        m_programDef->GetBindingDetailsByName("u_globalData"),
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

bool SkyBoxRenderer::UpdateGlobalDescriptorSet_ViewProjection(const VulkanDescriptorSetPtr& globalDataDescriptorSet,
                                                              const std::vector<ViewProjection>& viewProjections) const
{
    //
    // Create buffer
    //
    const auto viewProjectionDataBuffer = CPUItemBuffer<ViewProjectionPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        viewProjections.size(),
        std::format("SkyBoxRenderer-ViewProjectionData-{}", m_frameIndex)
    );
    if (!viewProjectionDataBuffer)
    {
        m_logger->Log(Common::LogLevel::Error,
          "SkyBoxRenderer::BindDescriptorSet0_ViewProjection: Failed to create view projection data buffer");
        return false;
    }

    //
    // Set Data
    //
    std::vector<ViewProjectionPayload> viewProjectionPayloads;

    std::ranges::transform(viewProjections, std::back_inserter(viewProjectionPayloads), [](const auto& viewProjection){
        ViewProjectionPayload viewProjectionPayload = GetViewProjectionPayload(viewProjection);

        // Convert view transform to and from a mat3 to keep camera rotation but drop camera translation
        viewProjectionPayload.viewTransform = glm::mat4(glm::mat3(viewProjectionPayload.viewTransform));

        return viewProjectionPayload;
    });

    (*viewProjectionDataBuffer)->PushBack(ExecutionContext::CPU(), viewProjectionPayloads);

    globalDataDescriptorSet->WriteBufferBind(
        m_programDef->GetBindingDetailsByName("i_viewProjectionData"),
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

bool SkyBoxRenderer::BindMaterialDescriptorSet(const RenderParams& renderParams,
                                               const VulkanCommandBufferPtr& commandBuffer,
                                               const VulkanPipelinePtr& pipeline) const
{
    //
    // Retrieve a descriptor set for binding material data
    //
    const auto materialDescriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        m_programDef->GetDescriptorSetLayouts()[2],
        std::format("SkyBoxRenderer-MaterialData-{}", m_frameIndex)
    );
    if (!materialDescriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error, "SkyBoxRenderer::BindDescriptorSet2: Failed to get or create material descriptor set");
        return false;
    }

    //
    // Update the Descriptor Set
    //
    const auto skyBoxTexture = m_textures->GetTexture(*renderParams.skyBoxTextureId);
    if (!skyBoxTexture)
    {
        m_logger->Log(Common::LogLevel::Error,
          "SkyBoxRenderer::BindDescriptorSet2: No such texture exists: {}", (*renderParams.skyBoxTextureId).id);
        return false;
    }

    (*materialDescriptorSet)->WriteCombinedSamplerBind(
        m_programDef->GetBindingDetailsByName("i_skyboxSampler"),
        skyBoxTexture->vkImageViews.at(TextureView::DEFAULT),
        skyBoxTexture->vkSampler
    );

    //
    // Bind the Descriptor Set
    //
    commandBuffer->CmdBindDescriptorSets(pipeline, 2, {(*materialDescriptorSet)->GetVkDescriptorSet()});

    return true;
}

}
