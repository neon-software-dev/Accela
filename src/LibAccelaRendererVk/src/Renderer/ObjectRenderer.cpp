/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ObjectRenderer.h"

#include "../PostExecutionOp.h"
#include "../Metrics.h"

#include "../Buffer/IBuffers.h"
#include "../Buffer/CPUItemBuffer.h"
#include "../Program/IPrograms.h"
#include "../Pipeline/PipelineUtil.h"
#include "../Pipeline/IPipelineFactory.h"
#include "../Renderables/IRenderables.h"
#include "../Mesh/IMeshes.h"
#include "../Material/IMaterials.h"
#include "../Texture/ITextures.h"

#include "../Vulkan/VulkanDebug.h"
#include "../Vulkan/VulkanFramebuffer.h"
#include "../Vulkan/VulkanPipeline.h"
#include "../Vulkan/VulkanDescriptorSet.h"
#include "../Vulkan/VulkanCommandBuffer.h"

#include <Accela/Render/Material/ObjectMaterial.h>

#include <format>
#include <algorithm>
#include <set>
#include <ranges>

namespace Accela::Render
{


ObjectRenderer::ObjectRenderer(Common::ILogger::Ptr logger,
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

bool ObjectRenderer::Initialize(const RenderSettings& renderSettings)
{
    return Renderer::Initialize(renderSettings);
}

void ObjectRenderer::Destroy()
{
    // Destroy any pipelines that were created for object rendering
    for (const auto& pipelineHashIt : m_programPipelineHashes)
    {
        m_pipelines->DestroyPipeline(pipelineHashIt.second);
    }
    m_programPipelineHashes.clear();

    Renderer::Destroy();
}

void ObjectRenderer::Render(const std::string& sceneName,
                            const RenderType& renderType,
                            const RenderParams& renderParams,
                            const VulkanCommandBufferPtr& commandBuffer,
                            const VulkanRenderPassPtr& renderPass,
                            const VulkanFramebufferPtr& framebuffer,
                            const std::vector<ViewProjection>& viewProjections,
                            const std::optional<ShadowRenderData>& shadowRenderData)
{
    CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "ObjectRenderer");

    // Early bail out if there's no objects to be rendered
    if (m_renderables->GetObjects().GetData().empty()) { return; }

    // If render settings has object rendering turned off, bail out
    if (!m_vulkanObjs->GetRenderSettings().renderObjects) { return; }

    //
    // Compile render batches from the scene's objects
    //
    const auto renderBatches = CompileRenderBatches(sceneName, renderType, viewProjections);

    //
    // Render each render batch
    //
    RenderState renderState{};
    RenderMetrics renderMetrics{};

    for (const auto& renderBatch : renderBatches)
    {
        RenderBatch(renderState, renderMetrics, renderType, renderBatch, renderParams, commandBuffer,
                    renderPass, framebuffer, viewProjections, shadowRenderData);
    }

    //
    // Clean Up / metrics
    //
    if (renderType == RenderType::GpassOpaque)
    {
        m_metrics->SetCounterValue(Renderer_Object_Opaque_Objects_Rendered_Count, renderMetrics.numObjectRendered);
        m_metrics->SetCounterValue(Renderer_Object_Opaque_RenderBatch_Count, renderBatches.size());
        m_metrics->SetCounterValue(Renderer_Object_Opaque_DrawCalls_Count, renderMetrics.numDrawCalls);
    }
    else if (renderType == RenderType::GpassTranslucent)
    {
        m_metrics->SetCounterValue(Renderer_Object_Transparent_Objects_Rendered_Count, renderMetrics.numObjectRendered);
        m_metrics->SetCounterValue(Renderer_Object_Transparent_RenderBatch_Count, renderBatches.size());
        m_metrics->SetCounterValue(Renderer_Object_Transparent_DrawCalls_Count, renderMetrics.numDrawCalls);
    }
}

std::vector<ObjectRenderer::ObjectRenderBatch> ObjectRenderer::CompileRenderBatches(
    const std::string& sceneName,
    const RenderType& renderType,
    const std::vector<ViewProjection>& viewProjections) const
{
    //
    // Compile the list of objects that should be rendered
    //
    const auto objectsToRender = GetObjectsToRender(sceneName, renderType, viewProjections);

    //
    // Transform the objects to be rendered into sorted render batches
    //
    return ObjectsToRenderBatches(renderType, objectsToRender);
}

std::vector<ObjectRenderable> ObjectRenderer::GetObjectsToRender(const std::string& sceneName,
                                                                 const RenderType& renderType,
                                                                 const std::vector<ViewProjection>& viewProjections) const
{
    AABB totalViewSpaceAABB;

    //
    // As we can be rendering for any number of view projections, create one total view space AABB which encompasses
    // the AABBs of all the render view projections
    //
    for (const auto& viewProjection : viewProjections)
    {
        // Adjust the far plane of the view projection so that we're only looking at objects within the max object render distance.
        auto objectViewProjection = viewProjection;

        if (!objectViewProjection.projectionTransform->SetFarPlaneDistance(m_vulkanObjs->GetRenderSettings().objectRenderDistance))
        {
            m_logger->Log(Common::LogLevel::Error, "GetObjectsToRender: Failed to reduce far plane distance");
        }

        // Add this view projection's AABB to the total view space AABB
        totalViewSpaceAABB.AddVolume(objectViewProjection.GetWorldSpaceAABB().GetVolume());
    }

    //
    // Query ObjectRenderables for all valid objects in the scene within the bounds of the total view projection
    //
    auto objectsToRender = m_renderables->GetObjects()
        .GetVisibleObjects(sceneName, totalViewSpaceAABB.GetVolume());

    //
    // Filter the objects by the render operation we're performing
    //
    auto filteredObjectsView = objectsToRender | std::ranges::views::filter([&](const ObjectRenderable& objectRenderable){
        //
        // If we're doing a shadow pass and the object shouldn't be included in shadow passes, filter it out
        //
        if (renderType == RenderType::Shadow && !objectRenderable.shadowPass)
        {
            return false;
        }

        const auto loadedMaterial = m_materials->GetLoadedMaterial(objectRenderable.materialId);
        if (!loadedMaterial)
        {
            return false;
        }

        const auto objectMaterial = std::dynamic_pointer_cast<ObjectMaterial>(loadedMaterial->material);

        //
        // If we're doing an opaque pass and the object doesn't have an opaque material, filter it out
        //
        if (renderType == RenderType::GpassOpaque && objectMaterial->properties.alphaMode != AlphaMode::Opaque)
        {
            return false;
        }

        //
        // If we're doing a translucent pass and the object has an opaque material, filter it out
        //
        if (renderType == RenderType::GpassTranslucent && objectMaterial->properties.alphaMode == AlphaMode::Opaque)
        {
            return false;
        }

        return true;
    });

    objectsToRender = std::vector<ObjectRenderable>(filteredObjectsView.begin(), filteredObjectsView.end());

    return objectsToRender;
}

std::function<bool(const ObjectRenderer::ObjectRenderBatch&, const ObjectRenderer::ObjectRenderBatch&)> ObjectRenderer::BatchSortFunc =
    [](const ObjectRenderer::ObjectRenderBatch& a, const ObjectRenderer::ObjectRenderBatch& b)
{
    // a
    const auto aProgramName = a.params.programDef->GetProgramName();
    const auto aMaterialId = a.params.loadedMaterial.material->materialId;
    const auto aMaterialType = a.params.loadedMaterial.material->type;
    BufferId aMeshDataBufferId{};
    if (a.params.meshDataBuffer)
    {
        aMeshDataBufferId = (*a.params.meshDataBuffer)->GetBuffer()->GetBufferId();
    }

    // b
    const auto bProgramName = b.params.programDef->GetProgramName();
    const auto bMaterialId = b.params.loadedMaterial.material->materialId;
    const auto bMaterialType = b.params.loadedMaterial.material->type;
    BufferId bMeshDataBufferId{};
    if (b.params.meshDataBuffer)
    {
        bMeshDataBufferId = (*b.params.meshDataBuffer)->GetBuffer()->GetBufferId();
    }

    //
    // Sort batches by program, then by material type, then by material, then by (optional) mesh data buffer
    //
    return  std::tie(aProgramName, aMaterialType, aMaterialId, aMeshDataBufferId) <
            std::tie(bProgramName, bMaterialType, bMaterialId, bMeshDataBufferId);
};

std::vector<ObjectRenderer::ObjectRenderBatch> ObjectRenderer::ObjectsToRenderBatches(const RenderType& renderType,
                                                                                      const std::vector<ObjectRenderable>& objects) const
{
    //
    // Add every object to its appropriate render batch
    //
    std::unordered_map<ObjectRenderBatch::Key, ObjectRenderBatch> renderBatches;

    for (const auto& object : objects)
    {
        //
        // Get the object's batch parameters
        //
        const auto renderBatchParams = GetRenderBatchParams(renderType, object);
        const auto drawBatchParams = GetDrawBatchParams(object);

        if (!renderBatchParams || !drawBatchParams)
        {
            continue;
        }

        const auto renderBatchKey = GetBatchKey(*renderBatchParams);
        const auto drawBatchKey = GetBatchKey(*drawBatchParams);

        //
        // Insert the object into its render batch, or create the render batch if it doesn't exist yet
        //
        const auto renderBatchIt = renderBatches.find(renderBatchKey);
        if (renderBatchIt != renderBatches.cend())
        {
            AddObjectToRenderBatch(object, drawBatchKey, *drawBatchParams, renderBatchIt->second);
        }
        else
        {
            const auto renderBatch = CreateRenderBatch(
                object,
                drawBatchKey,
                *drawBatchParams,
                renderBatchKey,
                *renderBatchParams
            );

            renderBatches.insert({renderBatchKey, renderBatch});
        }
    }

    //
    // Transform the batches map to a batches vector
    //
    std::vector<ObjectRenderBatch> batchesVec;
    batchesVec.reserve(renderBatches.size());

    std::ranges::transform(renderBatches, std::back_inserter(batchesVec),[](const auto& kv){
        return kv.second;
    });

    //
    // Sort the render batches for efficient rendering with minimal descriptor set changes
    //
    std::ranges::sort(batchesVec, BatchSortFunc);

    return batchesVec;
}

void ObjectRenderer::AddObjectToRenderBatch(const ObjectRenderable& object,
                                            const ObjectDrawBatch::Key& drawBatchKey,
                                            const ObjectDrawBatchParams& drawBatchParams,
                                            ObjectRenderBatch& renderBatch)
{
    //
    // Add the object to an existing draw batch, if possible
    //
    for (auto& drawBatch : renderBatch.drawBatches)
    {
        if (drawBatch.key == drawBatchKey)
        {
            drawBatch.objects.push_back(object);
            return;
        }
    }

    //
    // Otherwise, create a new draw batch
    //
    ObjectDrawBatch drawBatch{};
    drawBatch.key = drawBatchKey;
    drawBatch.params = drawBatchParams;
    drawBatch.objects.push_back(object);

    renderBatch.drawBatches.push_back(drawBatch);
}

ObjectRenderer::ObjectRenderBatch ObjectRenderer::CreateRenderBatch(
    const ObjectRenderable& object,
    const ObjectDrawBatch::Key& drawBatchKey,
    const ObjectDrawBatchParams& drawBatchParams,
    const ObjectRenderBatch::Key& renderBatchKey,
    const ObjectRenderBatchParams& renderBatchParams)
{
    ObjectDrawBatch drawBatch{};
    drawBatch.key = drawBatchKey;
    drawBatch.params = drawBatchParams;
    drawBatch.objects.push_back(object);

    ObjectRenderBatch renderBatch{};
    renderBatch.key = renderBatchKey;
    renderBatch.params = renderBatchParams;
    renderBatch.drawBatches.push_back(drawBatch);

    return renderBatch;
}

std::expected<ProgramDefPtr, bool> ObjectRenderer::GetMeshProgramDef(const RenderType& renderType, const LoadedMesh& loadedMesh) const
{
    ProgramDefPtr programDef;

    switch (loadedMesh.meshType)
    {
        case MeshType::Static:
        {
            switch (renderType)
            {
                case RenderType::GpassOpaque: case RenderType::GpassTranslucent: programDef = m_programs->GetProgramDef("Object"); break;
                case RenderType::Shadow: programDef = m_programs->GetProgramDef("ObjectShadow"); break;
            }
        }
        break;
        case MeshType::Bone:
        {
            switch (renderType)
            {
                case RenderType::GpassOpaque: case RenderType::GpassTranslucent: programDef = m_programs->GetProgramDef("BoneObject"); break;
                case RenderType::Shadow: programDef = m_programs->GetProgramDef("BoneObjectShadow"); break;
            }
        }
        break;
    }

    if (programDef == nullptr)
    {
        return std::unexpected(false);
    }

    return programDef;
}

void ObjectRenderer::RenderBatch(RenderState& renderState,
                                 RenderMetrics& renderMetrics,
                                 const RenderType& renderType,
                                 const ObjectRenderBatch& renderBatch,
                                 const RenderParams& renderParams,
                                 const VulkanCommandBufferPtr& commandBuffer,
                                 const VulkanRenderPassPtr& renderPass,
                                 const VulkanFramebufferPtr& framebuffer,
                                 const std::vector<ViewProjection>& viewProjections,
                                 const std::optional<ShadowRenderData>& shadowRenderData)
{
    //
    // Setup
    //
    const auto batchProgramName = renderBatch.params.programDef->GetProgramName();
    const auto batchMaterialId = renderBatch.params.loadedMaterial.material->materialId;

    BufferId batchMeshDataBufferId{};
    if (renderBatch.params.meshDataBuffer)
    {
        batchMeshDataBufferId = (*renderBatch.params.meshDataBuffer)->GetBuffer()->GetBufferId();
    }

    CmdBufferSectionLabel sectionLabel(
        m_vulkanObjs->GetCalls(),
        commandBuffer,
        std::format("ObjectRenderBatch-{}-{}-{}", batchProgramName, batchMaterialId.id, batchMeshDataBufferId.id)
    );

    // We want to bind per-batch draw data to set 3 for every batch, so forcefully mark it as invalidated
    renderState.set3Invalidated = true;

    //
    // Bind pipeline
    //
    if (!BindPipeline(renderState, renderType, renderBatch, commandBuffer, renderPass, framebuffer, shadowRenderData)) { return; }

    //
    // Bind Descriptor Sets
    //
    if (!BindDescriptorSet0(renderState, renderParams, commandBuffer, viewProjections)) { return; }
    if (!BindDescriptorSet1(renderState, commandBuffer)) { return; }
    if (!BindDescriptorSet2(renderState, renderBatch, commandBuffer)) { return; }
    if (!BindDescriptorSet3(renderState, renderBatch, commandBuffer)) { return; }

    //
    // Draw
    //
    std::size_t instanceIndex = 0;

    for (const auto& drawBatch : renderBatch.drawBatches)
    {
        const auto& drawBatchMesh = drawBatch.params.loadedMesh;

        BindVertexBuffer(renderState, commandBuffer, drawBatchMesh.verticesBuffer->GetBuffer());
        BindIndexBuffer(renderState, commandBuffer, drawBatchMesh.indicesBuffer->GetBuffer());

        commandBuffer->CmdDrawIndexed(
            drawBatchMesh.numIndices,
            drawBatch.objects.size(),
            drawBatchMesh.indicesOffset,
            (int32_t)drawBatchMesh.verticesOffset,
            instanceIndex
        );

        instanceIndex += drawBatch.objects.size();

        renderMetrics.numObjectRendered += drawBatch.objects.size();
        renderMetrics.numDrawCalls++;
    }
}

bool ObjectRenderer::BindPipeline(RenderState& renderState,
                                  const RenderType& renderType,
                                  const ObjectRenderBatch& renderBatch,
                                  const VulkanCommandBufferPtr& commandBuffer,
                                  const VulkanRenderPassPtr& renderPass,
                                  const VulkanFramebufferPtr& framebuffer,
                                  const std::optional<ShadowRenderData>& shadowRenderData)
{
    //
    // Get the pipeline for this batch
    //
    const auto pipelineExpect = GetBatchPipeline(renderBatch, renderType, renderPass, framebuffer);
    if (!pipelineExpect)
    {
        m_logger->Log(Common::LogLevel::Error, "ObjectRenderer::BindPipeline: GetBatchPipeline failed");
        return false;
    }

    //
    // If the pipeline is already bound, nothing to do
    //
    if (renderState.pipeline == *pipelineExpect) { return true; }

    //
    // Bind the pipeline
    //
    commandBuffer->CmdBindPipeline(*pipelineExpect);
    renderState.OnPipelineBound(renderBatch.params.programDef, *pipelineExpect);

    //
    // Write pipeline push constants
    //
    if (!BindPushConstants(renderState, renderType, commandBuffer, shadowRenderData)) { return false; }

    return true;
}

bool ObjectRenderer::BindPushConstants(RenderState& renderState,
                                       const RenderType& renderType,
                                       const VulkanCommandBufferPtr& commandBuffer,
                                       const std::optional<ShadowRenderData>& shadowRenderData) const
{
    // If we're rendering a shadow map, install details about the light/shadow map as push constants
    if (renderType == RenderType::Shadow)
    {
        assert(shadowRenderData);
        if (!shadowRenderData)
        {
            m_logger->Log(Common::LogLevel::Error,
                          "ObjectRenderer::BindPushConstants: Running shadow pass but no shadow render data provided");
            return false;
        }

        ShadowLayerIndexPayload payload{};
        payload.lightMaxAffectRange = shadowRenderData->lightMaxAffectRange;

        commandBuffer->CmdPushConstants(
            *renderState.pipeline,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(ShadowLayerIndexPayload),
            &payload
        );
    }

    return true;
}

bool ObjectRenderer::BindDescriptorSet0(RenderState& renderState,
                                        const RenderParams& renderParams,
                                        const VulkanCommandBufferPtr& commandBuffer,
                                        const std::vector<ViewProjection>& viewProjections)
{
    //
    // If the set isn't invalidated, nothing to do
    //
    if (!renderState.set0Invalidated) { return true; }

    //
    // Create a descriptor set
    //
    const auto descriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        (*renderState.programDef)->GetDescriptorSetLayouts()[0],
        std::format("ObjectRenderer-DS0-{}", m_frameIndex)
    );
    if (!descriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error,
          "ObjectRenderer::BindDescriptorSet0: Failed to get or create descriptor set");
        return false;
    }

    //
    // Update the descriptor set with data
    //
    if (!BindDescriptorSet0_Global(renderState, renderParams, (*descriptorSet))) { return false; }
    if (!BindDescriptorSet0_ViewProjection(renderState, viewProjections, (*descriptorSet))) { return false; }

    //
    // Bind the descriptor set
    //
    commandBuffer->CmdBindDescriptorSets(*renderState.pipeline, 0, {(*descriptorSet)->GetVkDescriptorSet()});
    renderState.OnSet0Bound();

    return true;
}

bool ObjectRenderer::BindDescriptorSet0_Global(RenderState& renderState,
                                               const RenderParams& renderParams,
                                               const VulkanDescriptorSetPtr& descriptorSet) const
{
    //
    // Create a buffer
    //
    const auto globalDataBuffer = CPUItemBuffer<GlobalPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        1,
        std::format("ObjectRenderer-DS0-GlobalData-{}", m_frameIndex)
    );
    if (!globalDataBuffer)
    {
        m_logger->Log(Common::LogLevel::Error,
          "ObjectRenderer::BindDescriptorSet0_Global: Failed to create global data buffer");
        return false;
    }

    //
    // Set Data
    //
    const GlobalPayload globalPayload = GetGlobalPayload(renderParams, 0);
    (*globalDataBuffer)->PushBack(ExecutionContext::CPU(), {globalPayload});

    descriptorSet->WriteBufferBind(
        (*renderState.programDef)->GetBindingDetailsByName("u_globalData"),
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        (*globalDataBuffer)->GetBuffer()->GetVkBuffer(),
        0,
        0
    );

    //
    // Post-Frame Cleanup
    //
    m_postExecutionOps->Enqueue_Current(BufferDeleteOp(m_buffers, (*globalDataBuffer)->GetBuffer()->GetBufferId()));

    return true;
}

bool ObjectRenderer::BindDescriptorSet0_ViewProjection(RenderState& renderState,
                                                       const std::vector<ViewProjection>& viewProjections,
                                                       const VulkanDescriptorSetPtr& descriptorSet) const
{
    //
    // Create buffer
    //
    const auto viewProjectionDataBuffer = CPUItemBuffer<ViewProjectionPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        viewProjections.size(),
        std::format("ObjectRenderer-DS0-ViewProjectionData-{}", m_frameIndex)
    );
    if (!viewProjectionDataBuffer)
    {
        m_logger->Log(Common::LogLevel::Error,
          "ObjectRenderer::BindDescriptorSet0_ViewProjection: Failed to create view projection data buffer");
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

    descriptorSet->WriteBufferBind(
        (*renderState.programDef)->GetBindingDetailsByName("i_viewProjectionData"),
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

bool ObjectRenderer::BindDescriptorSet1(RenderState& renderState, const VulkanCommandBufferPtr& commandBuffer)
{
    //
    // If the set isn't invalidated, nothing to do
    //
    if (!renderState.set1Invalidated) { return true; }

    //
    // Create a descriptor set
    //
    const auto descriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        (*renderState.programDef)->GetDescriptorSetLayouts()[1],
        std::format("ObjectRenderer-DS1-{}", m_frameIndex)
    );
    if (!descriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error,
          "ObjectRenderer::BindDescriptorSet1: Failed to get or create renderer data descriptor set");
        return false;
    }

    //
    // Update the descriptor set with data
    //
    BindDescriptorSet1_RendererData(renderState, *descriptorSet);

    //
    // Bind the descriptor set
    //
    commandBuffer->CmdBindDescriptorSets(*renderState.pipeline, 1, {(*descriptorSet)->GetVkDescriptorSet()});
    renderState.OnSet1Bound();

    return true;
}

void ObjectRenderer::BindDescriptorSet1_RendererData(const RenderState& renderState, const VulkanDescriptorSetPtr& descriptorSet) const
{
    //
    // Update the descriptor set with data
    //
    const auto objectPayloadBuffer =  m_renderables->GetObjects().GetObjectPayloadBuffer();

    descriptorSet->WriteBufferBind(
        (*renderState.programDef)->GetBindingDetailsByName("i_objectData"),
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        objectPayloadBuffer->GetBuffer()->GetVkBuffer(),
        0,
        0
    );
}

bool ObjectRenderer::BindDescriptorSet2(RenderState& renderState,
                                        const ObjectRenderBatch& renderBatch,
                                        const VulkanCommandBufferPtr& commandBuffer)
{
    const auto& loadedMaterial = renderBatch.params.loadedMaterial;

    //
    // If the set isn't invalidated and the bound data is the same, nothing to do
    //
    const bool dataBindsMatch =
        renderState.materialDataBufferId == loadedMaterial.payloadBuffer->GetBuffer()->GetBufferId() &&
        renderState.materialTextures == loadedMaterial.textureBinds;

    if (!renderState.set2Invalidated && dataBindsMatch) { return true; }

    //
    // Create a descriptor set
    //
    const auto descriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        (*renderState.programDef)->GetDescriptorSetLayouts()[2],
        std::format("ObjectRenderer-DS2-{}", m_frameIndex)
    );
    if (!descriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error, "ObjectRenderer::BindDescriptorSet2: Failed to get or create material descriptor set");
        return false;
    }

    //
    // Update the descriptor set with data
    //
    BindDescriptorSet2_MaterialData(renderState, renderBatch, *descriptorSet);

    //
    // Bind the descriptor set
    //
    commandBuffer->CmdBindDescriptorSets(*renderState.pipeline, 2, {(*descriptorSet)->GetVkDescriptorSet()});
    renderState.OnSet2Bound();

    return true;
}

void ObjectRenderer::BindDescriptorSet2_MaterialData(RenderState& renderState,
                                                     const ObjectRenderBatch& renderBatch,
                                                     const VulkanDescriptorSetPtr& descriptorSet)
{
    const auto& loadedMaterial = renderBatch.params.loadedMaterial;

    //
    // Update the descriptor set with data
    //
    descriptorSet->WriteBufferBind(
        (*renderState.programDef)->GetBindingDetailsByName("i_materialData"),
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        loadedMaterial.payloadBuffer->GetBuffer()->GetVkBuffer(),
        0,
        loadedMaterial.payloadBuffer->GetDataByteSize()
    );

    //
    // Bind the material's textures
    //
    for (const auto& textureBindIt : loadedMaterial.textureBinds)
    {
        std::optional<LoadedTexture> loadedTexture{};

        if (textureBindIt.second == TextureId{INVALID_ID})
        {
            loadedTexture = m_textures->GetMissingTexture();
        }
        else
        {
            loadedTexture = m_textures->GetTexture(textureBindIt.second);

            // Fallback to missing texture as needed
            if (!loadedTexture)
            {
                loadedTexture = m_textures->GetMissingTexture();
            }
        }

        if (!loadedTexture)
        {
            m_logger->Log(Common::LogLevel::Error,
              "ObjectRenderer: BindDescriptorSet2_MaterialData: Failed to fetch any texture for texture: {}", textureBindIt.second.id);
            continue;
        }

        descriptorSet->WriteCombinedSamplerBind(
            (*renderState.programDef)->GetBindingDetailsByName(textureBindIt.first),
            loadedTexture->vkImageViews.at(TextureView::DEFAULT),
            loadedTexture->vkSampler
        );
    }

    //
    // Finish
    //
    renderState.materialDataBufferId = loadedMaterial.payloadBuffer->GetBuffer()->GetBufferId();
    renderState.materialTextures = loadedMaterial.textureBinds;
}

bool ObjectRenderer::BindDescriptorSet3(RenderState& renderState,
                                        const ObjectRenderBatch& renderBatch,
                                        const VulkanCommandBufferPtr& commandBuffer) const
{
    BufferId batchMeshDataBufferId{};
    if (renderBatch.params.meshDataBuffer)
    {
        batchMeshDataBufferId = (*renderBatch.params.meshDataBuffer)->GetBuffer()->GetBufferId();
    }

    //
    // If the set isn't invalidated, bail out. Note: This is just for consistency; we bind new draw
    // data to DS3 for every batch, so set3 is always invalidated at the start of every batch draw.
    //
    if (!renderState.set3Invalidated)
    {
        return true;
    }

    //
    // Create a descriptor set
    //
    const auto drawDescriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        (*renderState.programDef)->GetDescriptorSetLayouts()[3],
        std::format("ObjectRenderer-DS3-{}-{}-{}", batchMeshDataBufferId.id, renderBatch.params.loadedMaterial.material->materialId.id, m_frameIndex)
    );
    if (!drawDescriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error, "ObjectRenderer::BindDescriptorSet3: Failed to get or create draw descriptor set");
        return false;
    }

    //
    // Update the descriptor set with data
    //
    if (!BindDescriptorSet3_DrawData(renderState, renderBatch, *drawDescriptorSet, batchMeshDataBufferId)) { return false; }
    BindDescriptorSet3_MeshData(renderState, renderBatch, *drawDescriptorSet);
    if (!BindDescriptorSet3_BoneData(renderState, renderBatch, *drawDescriptorSet)) { return false; }

    //
    // Bind the draw descriptor set
    //
    commandBuffer->CmdBindDescriptorSets(*renderState.pipeline, 3, {(*drawDescriptorSet)->GetVkDescriptorSet()});
    renderState.OnSet3Bound();

    return true;
}

bool ObjectRenderer::BindDescriptorSet3_DrawData(const RenderState& renderState,
                                                 const ObjectRenderBatch& renderBatch,
                                                 const VulkanDescriptorSetPtr& drawDescriptorSet,
                                                 const BufferId& batchMeshDataBufferId) const
{
    std::size_t renderBatchNumObjects = 0;

    for (const auto& drawBatch : renderBatch.drawBatches)
    {
        renderBatchNumObjects += drawBatch.objects.size();
    }

    //
    // Create a per-render CPU buffer to hold draw data
    //
    const auto drawDataBufferExpect = CPUItemBuffer<ObjectDrawPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        renderBatchNumObjects,
        std::format("ObjectRenderer-DrawData-{}-{}-{}", m_frameIndex, batchMeshDataBufferId.id, renderBatch.params.loadedMaterial.material->materialId.id)
    );
    if (!drawDataBufferExpect)
    {
        m_logger->Log(Common::LogLevel::Error, "ObjectRenderer::BindDescriptorSet3_DrawData: Failed to create draw data buffer");
        return false;
    }
    const auto& drawDataBuffer = *drawDataBufferExpect;

    //
    // Convert the batch objects to be rendered to DrawPayloads
    //
    std::vector<ObjectDrawPayload> drawPayloads;

    for (const auto& drawBatch : renderBatch.drawBatches)
    {
        std::ranges::transform(drawBatch.objects, std::back_inserter(drawPayloads), [&](const ObjectRenderable& object) {
            ObjectDrawPayload drawPayload{};
            drawPayload.dataIndex = object.objectId.id - 1;
            drawPayload.materialIndex = renderBatch.params.loadedMaterial.payloadIndex;
            return drawPayload;
        });
    }

    drawDataBuffer->PushBack(ExecutionContext::CPU(), drawPayloads);

    drawDescriptorSet->WriteBufferBind(
        (*renderState.programDef)->GetBindingDetailsByName("i_drawData"),
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        drawDataBuffer->GetBuffer()->GetVkBuffer(),
        0,
        0
    );

    //
    // Cleanup
    //
    m_postExecutionOps->Enqueue_Current(BufferDeleteOp(m_buffers, drawDataBuffer->GetBuffer()->GetBufferId()));

    return true;
}

void ObjectRenderer::BindDescriptorSet3_MeshData(const RenderState& renderState,
                                                 const ObjectRenderBatch& renderBatch,
                                                 const VulkanDescriptorSetPtr& drawDescriptorSet)
{
    // If there's no mesh data buffer to be bound, nothing to do
    if (!renderBatch.params.meshDataBuffer) { return; }

    //
    // Bind the mesh's data buffer
    //
    drawDescriptorSet->WriteBufferBind(
        (*renderState.programDef)->GetBindingDetailsByName("i_meshData"),
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        (*renderBatch.params.meshDataBuffer)->GetBuffer()->GetVkBuffer(),
        0,
        0
    );
}

bool ObjectRenderer::BindDescriptorSet3_BoneData(const RenderState& renderState,
                                                 const ObjectRenderBatch& renderBatch,
                                                 const VulkanDescriptorSetPtr& drawDescriptorSet) const
{
    //
    // Look at a sample object in the batch to determine whether the batch's objects have bone data or not
    //
    const auto& objectsData = m_renderables->GetObjects().GetData();

    const auto sampleBoneTransforms =
        objectsData[renderBatch.drawBatches.at(0).objects.at(0).objectId.id - 1]
            .renderable.boneTransforms;

    // If there's no bone data to be bound, nothing to do
    if (!sampleBoneTransforms)
    {
        return true;
    }

    //
    // Compile bone data for the render batch
    //
    std::size_t renderBatchNumObjects = 0;

    for (const auto& drawBatch : renderBatch.drawBatches)
    {
        renderBatchNumObjects += drawBatch.objects.size();
    }

    const auto meshNumBones = sampleBoneTransforms->size();
    const auto meshBonesByteSize = meshNumBones * sizeof(glm::mat4);

    std::vector<glm::mat4> allObjectsBoneTransforms;
    allObjectsBoneTransforms.resize(renderBatchNumObjects * meshNumBones);

    std::size_t boneTransformIndex = 0;

    for (const auto& drawBatch : renderBatch.drawBatches)
    {
        for (const auto& object : drawBatch.objects)
        {
            memcpy(
                (unsigned char *)allObjectsBoneTransforms.data() + (boneTransformIndex * meshBonesByteSize),
                objectsData[object.objectId.id - 1].renderable.boneTransforms->data(),
                meshBonesByteSize
            );

            boneTransformIndex++;
        }
    }

    //
    // Create and update a buffer to hold bone transforms
    //
    const auto boneTransformsBufferExpect = CPUItemBuffer<glm::mat4>::Create(
        m_buffers,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        allObjectsBoneTransforms.size(),
        std::format("ObjectRenderer-DS3-BoneTransforms-{}-{}", renderBatch.params.loadedMaterial.material->materialId.id, m_frameIndex)
    );
    if (!boneTransformsBufferExpect)
    {
        m_logger->Log(Common::LogLevel::Error, "ObjectRenderer::UpdateDrawDescriptorSet_BoneData: Failed to create bone data buffer");
        return false;
    }

    (*boneTransformsBufferExpect)->Update(ExecutionContext::CPU(), 0, allObjectsBoneTransforms);

    //
    // Bind bone data
    //
    drawDescriptorSet->WriteBufferBind(
        (*renderState.programDef)->GetBindingDetailsByName("i_boneData"),
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        (*boneTransformsBufferExpect)->GetBuffer()->GetVkBuffer(),
        0,
        0
    );

    //
    // Cleanup
    //
    m_postExecutionOps->Enqueue_Current(BufferDeleteOp(m_buffers, (*boneTransformsBufferExpect)->GetBuffer()->GetBufferId()));

    return true;
}

void ObjectRenderer::BindVertexBuffer(RenderState& renderState,
                                      const VulkanCommandBufferPtr& commandBuffer,
                                      const BufferPtr& vertexBuffer)
{
    //
    // If the vertex buffer is already bound, nothing to do
    //
    if (vertexBuffer == renderState.vertexBuffer) { return; }

    //
    // Bind the vertex buffer
    //
    commandBuffer->CmdBindVertexBuffers(0, 1, {vertexBuffer->GetVkBuffer()}, {VkDeviceSize(0)});

    //
    // Update render state
    //
    renderState.OnVertexBufferBound(vertexBuffer);
}

void ObjectRenderer::BindIndexBuffer(RenderState& renderState,
                                      const VulkanCommandBufferPtr& commandBuffer,
                                      const BufferPtr& indexBuffer)
{
    //
    // If the index buffer is already bound, nothing to do
    //
    if (indexBuffer == renderState.indexBuffer) { return; }

    //
    // Bind the index buffer
    //
    commandBuffer->CmdBindIndexBuffer(indexBuffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT32);

    //
    // Update render state
    //
    renderState.OnIndexBufferBound(indexBuffer);
}

std::expected<VulkanPipelinePtr, bool> ObjectRenderer::GetBatchPipeline(
    const ObjectRenderBatch& renderBatch,
    const RenderType& renderType,
    const VulkanRenderPassPtr& renderPass,
    const VulkanFramebufferPtr& framebuffer)
{
    const auto batchProgram = renderBatch.params.programDef;

    std::optional<std::size_t> oldPipelineHash;

    if (m_programPipelineHashes.contains(batchProgram->GetProgramName()))
    {
        oldPipelineHash = m_programPipelineHashes[batchProgram->GetProgramName()];
    }

    //
    // Retrieve the pipeline to use for rendering the batch
    //
    const auto viewport = Viewport(0, 0, framebuffer->GetSize()->w, framebuffer->GetSize()->h);

    auto cullFace = CullFace::Back;

    switch (renderType)
    {
        case RenderType::GpassOpaque: case RenderType::GpassTranslucent: cullFace = CullFace::Back; break;
        case RenderType::Shadow: cullFace = CullFace::Front; break; // Fixes peter-panning effect
    }

    std::vector<PushConstantRange> pushConstantRanges;

    if (renderType == RenderType::Shadow)
    {
        // Providing light/shadow data push constants to vertex and fragment stages when doing a shadow pass
        pushConstantRanges = {
            {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ShadowLayerIndexPayload)},
            {VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ShadowLayerIndexPayload)}
        };
    }

    uint32_t subpassIndex{0};

    switch (renderType)
    {
        case RenderType::GpassOpaque:       subpassIndex = Offscreen_GPassOpaqueSubpass_Index; break;
        case RenderType::GpassTranslucent:  subpassIndex = Offscreen_GPassTranslucentSubpass_Index; break;
        case RenderType::Shadow:            subpassIndex = Offscreen_GPassOpaqueSubpass_Index; break;
    }

    auto fillMode = PolygonFillMode::Fill;

    if (m_vulkanObjs->GetRenderSettings().objectsWireframe)
    {
        fillMode = PolygonFillMode::Line;
    }

    auto pipeline = GetPipeline(
        m_logger,
        m_vulkanObjs,
        m_shaders,
        m_pipelines,
        batchProgram,
        renderPass,
        subpassIndex,
        viewport,
        cullFace,
        fillMode,
        pushConstantRanges,
        m_frameIndex,
        oldPipelineHash
    );
    if (!pipeline)
    {
        m_logger->Log(Common::LogLevel::Error, "ObjectRenderer: GetBatchPipeline: Failed to fetch batch pipeline");
        return std::unexpected(false);
    }

    // Keep track of the latest pipeline hash that was used for this program
    m_programPipelineHashes[batchProgram->GetProgramName()] = (*pipeline)->GetConfigHash();

    return pipeline;
}

std::expected<ObjectRenderer::ObjectDrawBatchParams, bool> ObjectRenderer::GetDrawBatchParams(const ObjectRenderable& object) const
{
    const auto loadedMeshOpt = m_meshes->GetLoadedMesh(object.meshId);
    if (!loadedMeshOpt)
    {
        return std::unexpected(false);
    }

    ObjectDrawBatchParams params{};
    params.loadedMesh = *loadedMeshOpt;

    return params;
}

std::expected<ObjectRenderer::ObjectRenderBatchParams, bool> ObjectRenderer::GetRenderBatchParams(const RenderType& renderType,
                                                                                                 const ObjectRenderable& object) const
{
    const auto loadedMeshOpt = m_meshes->GetLoadedMesh(object.meshId);
    if (!loadedMeshOpt)
    {
        return std::unexpected(false);
    }

    const auto loadedMaterialOpt = m_materials->GetLoadedMaterial(object.materialId);
    if (!loadedMaterialOpt)
    {
        return std::unexpected(false);
    }

    const auto programDefExpect = GetMeshProgramDef(renderType, *loadedMeshOpt);
    if (!programDefExpect)
    {
        return std::unexpected(false);
    }

    ObjectRenderBatchParams params{};
    params.programDef = *programDefExpect;
    params.loadedMaterial = *loadedMaterialOpt;
    params.meshDataBuffer = loadedMeshOpt->dataBuffer;

    return params;
}

ObjectRenderer::ObjectDrawBatch::Key ObjectRenderer::GetBatchKey(const ObjectDrawBatchParams& params)
{
    return std::hash<std::string>{}(std::format("{}", params.loadedMesh.id.id));
}

ObjectRenderer::ObjectRenderBatch::Key ObjectRenderer::GetBatchKey(const ObjectRenderBatchParams& params)
{
    BufferId meshDataBufferId{};

    if (params.meshDataBuffer)
    {
        meshDataBufferId = (*params.meshDataBuffer)->GetBuffer()->GetBufferId();
    }

    return std::hash<std::string>{}(std::format("{}-{}-{}",
        params.programDef->GetProgramName(), params.loadedMaterial.material->materialId.id, meshDataBufferId.id));
}

}
