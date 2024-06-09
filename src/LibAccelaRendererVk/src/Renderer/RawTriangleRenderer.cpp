/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RawTriangleRenderer.h"

#include "../Pipeline/IPipelineFactory.h"
#include "../Pipeline/PipelineUtil.h"
#include "../Program/IPrograms.h"
#include "../Buffer/DataBuffer.h"
#include "../Buffer/CPUItemBuffer.h"
#include "../Mesh/IMeshes.h"
#include "../PostExecutionOp.h"

#include "../Vulkan/VulkanDebug.h"
#include "../Vulkan/VulkanFramebuffer.h"
#include "../Vulkan/VulkanPipeline.h"
#include "../Vulkan/VulkanCommandBuffer.h"
#include "../Vulkan/VulkanDescriptorSet.h"

#include <Accela/Render/Mesh/StaticMesh.h>

namespace Accela::Render
{


RawTriangleRenderer::RawTriangleRenderer(Common::ILogger::Ptr logger,
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

bool RawTriangleRenderer::Initialize(const RenderSettings& renderSettings)
{
    if (!Renderer::Initialize(renderSettings))
    {
        return false;
    }

    m_programDef = m_programs->GetProgramDef("RawTriangle");
    if (m_programDef == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "RawTriangleRenderer: RawTriangle program doesn't exist");
        return false;
    }

    return true;
}

void RawTriangleRenderer::Destroy()
{
    m_programDef = nullptr;

    if (m_pipelineHash)
    {
        m_pipelines->DestroyPipeline(*m_pipelineHash);
        m_pipelineHash = std::nullopt;
    }

    Renderer::Destroy();
}

void RawTriangleRenderer::Render(const RenderParams& renderParams,
                                 const VulkanCommandBufferPtr& commandBuffer,
                                 const VulkanRenderPassPtr& renderPass,
                                 const VulkanFramebufferPtr& framebuffer,
                                 const std::vector<ViewProjection>& viewProjections,
                                 const std::vector<Triangle>& triangles)
{
    CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "RawTriangleRenderer");

    // Bail out early if there's no triangles to be rendered
    if (triangles.empty()) { return; }

    //
    // Retrieve the pipeline to use for rendering
    //
    const auto viewport = Viewport(0, 0, framebuffer->GetSize()->w, framebuffer->GetSize()->h);

    const auto pipeline = GetPipeline(
        m_logger,
        m_vulkanObjs,
        m_shaders,
        m_pipelines,
        m_programDef,
        renderPass,
        OffscreenRenderPass_ForwardSubpass_Index,
        viewport,
        CullFace::Back,
        PolygonFillMode::Line,
        DepthBias::Disabled,
        PushConstantRange::None(),
        m_frameIndex,
        m_pipelineHash
    );
    if (!pipeline)
    {
        m_logger->Log(Common::LogLevel::Error, "RawTriangleRenderer: Render: Failed to fetch pipeline");
        return;
    }

    m_pipelineHash = (*pipeline)->GetConfigHash(); // Keep track of the latest pipeline that was used

    //
    // Create a mesh to hold the triangles data
    //
    const auto meshIdExpect = CreateTrianglesMesh(triangles);
    if (!meshIdExpect)
    {
        m_logger->Log(Common::LogLevel::Error, "RawTriangleRenderer: Render: Failed to create triangles mesh");
        return;
    }

    const auto loadedMesh = m_meshes->GetLoadedMesh(*meshIdExpect);

    const auto vkMeshVerticesBuffer = loadedMesh->verticesBuffer->GetBuffer()->GetVkBuffer();
    const auto vkMeshIndicesBuffer = loadedMesh->indicesBuffer->GetBuffer()->GetVkBuffer();

    //
    // Bind Data
    //
    commandBuffer->CmdBindPipeline(*pipeline);

    if (!BindGlobalDescriptorSet(renderParams, commandBuffer, *pipeline, viewProjections)) { return; }

    commandBuffer->CmdBindVertexBuffers(0, 1, {vkMeshVerticesBuffer}, {VkDeviceSize(0)});
    commandBuffer->CmdBindIndexBuffer(vkMeshIndicesBuffer, 0, VK_INDEX_TYPE_UINT32);

    //
    // Draw
    //
    commandBuffer->CmdDrawIndexed(
        loadedMesh->numIndices,
        1,
        0,
        0,
        0
    );

    //
    // Cleanup
    //
    m_postExecutionOps->Enqueue_Current(MeshDeleteOp(m_meshes, *meshIdExpect));
}

std::expected<MeshId, bool> RawTriangleRenderer::CreateTrianglesMesh(const std::vector<Triangle>& triangles)
{
    std::vector<MeshVertex> vertices;
    vertices.reserve(triangles.size() * 3);

    std::vector<uint32_t> indices;
    indices.reserve(triangles.size() * 3);

    for (const auto& triangle : triangles)
    {
        vertices.emplace_back(
            triangle.p1,
            glm::vec3{0,0,0},
            glm::vec2{0,0}
        );

        vertices.emplace_back(
            triangle.p2,
            glm::vec3{0,0,0},
            glm::vec2{0,0}
        );

        vertices.emplace_back(
            triangle.p3,
            glm::vec3{0,0,0},
            glm::vec2{0,0}
        );

        indices.emplace_back(vertices.size() - 3);
        indices.emplace_back(vertices.size() - 2);
        indices.emplace_back(vertices.size() - 1);
    }

    const auto meshId = m_ids->meshIds.GetId();

    const auto mesh = std::make_shared<StaticMesh>(meshId, vertices, indices, "RawTriangles");

    if (!m_meshes->LoadMesh(mesh, MeshUsage::Dynamic, std::promise<bool>{}))
    {
        m_ids->meshIds.ReturnId(meshId);
        return std::unexpected(false);
    }

    return meshId;
}

bool RawTriangleRenderer::BindGlobalDescriptorSet(const RenderParams& renderParams,
                                                  const VulkanCommandBufferPtr& commandBuffer,
                                                  const VulkanPipelinePtr& pipeline,
                                                  const std::vector<ViewProjection>& viewProjections) const
{
    //
    // Fetch the Descriptor Set
    //
    const auto globalDataDescriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        m_programDef->GetDescriptorSetLayouts()[0],
        std::format("RawTriangleRenderer-GlobalData-{}", m_frameIndex)
    );
    if (!globalDataDescriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error, "RawTriangleRenderer: Failed to get or create global data descriptor set");
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

bool RawTriangleRenderer::UpdateGlobalDescriptorSet_Global(const RenderParams& renderParams,
                                                           const VulkanDescriptorSetPtr& globalDataDescriptorSet) const
{
    //
    // Create a per-render CPU buffer for holding global data
    //
    const auto globalDataBuffer = CPUItemBuffer<GlobalPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        1,
        std::format("RawTriangleRenderer-GlobalData-{}", m_frameIndex)
    );
    if (!globalDataBuffer)
    {
        m_logger->Log(Common::LogLevel::Error, "RawTriangleRenderer::BindDescriptorSet0_Global: Failed to create global data buffer");
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

bool RawTriangleRenderer::UpdateGlobalDescriptorSet_ViewProjection(const VulkanDescriptorSetPtr& globalDataDescriptorSet,
                                                                   const std::vector<ViewProjection>& viewProjections) const
{
    //
    // Create buffer
    //
    const auto viewProjectionDataBuffer = CPUItemBuffer<ViewProjectionPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        viewProjections.size(),
        std::format("RawTriangleRenderer-ViewProjectionData-{}", m_frameIndex)
    );
    if (!viewProjectionDataBuffer)
    {
        m_logger->Log(Common::LogLevel::Error,
          "RawTriangleRenderer::BindDescriptorSet0_ViewProjection: Failed to create view projection data buffer");
        return false;
    }

    //
    // Set Data
    //
    std::vector<ViewProjectionPayload> viewProjectionPayloads;

    std::ranges::transform(viewProjections, std::back_inserter(viewProjectionPayloads),
       [](const auto& viewProjection){
           return GetViewProjectionPayload(viewProjection);
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

}
