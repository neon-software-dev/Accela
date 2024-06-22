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
#include "../Light/ILights.h"

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
                            const std::unordered_map<LightId, TextureId>& shadowMaps,
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
    BindState bindState{};
    RenderMetrics renderMetrics{};

    for (const auto& renderBatch : renderBatches)
    {
        RenderBatch(sceneName, bindState, renderMetrics, renderType, renderBatch, renderParams, commandBuffer,
                    renderPass, framebuffer, viewProjections, shadowMaps, shadowRenderData);
    }

    //
    // Clean Up / metrics
    //
    if (renderType == RenderType::GpassDeferred)
    {
        m_metrics->SetCounterValue(Renderer_Object_Opaque_Objects_Rendered_Count, renderMetrics.numObjectRendered);
        m_metrics->SetCounterValue(Renderer_Object_Opaque_RenderBatch_Count, renderBatches.size());
        m_metrics->SetCounterValue(Renderer_Object_Opaque_DrawCalls_Count, renderMetrics.numDrawCalls);
    }
    else if (renderType == RenderType::GpassForward)
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

        // Determine whether the material has translucency. Anything with an alpha mode of Blend is considered translucent.
        // Anything with an alpha mode of Opaque or Mask is considered non-translucent.
        //
        // An AlphaMode of Mask is considered non-translucent because in the shaders the fragment's alpha values
        // will get set to either fully opaque or fully transparent, depending on the mask blending rules, so it's
        // fine for those materials to go through the opaque flow; it's only materials with actual *translucency*,
        // not transparency, which need to go into the translucent pass.
        //
        // Note: Just because a material has an AlphaMode of blend doesn't mean it *actually* has translucency; all pixels
        // in it could have alphas of 1.0, but unless we're going to inspect the pixels we just have to go off the determined
        // blend mode, even if it's inaccurate. It's better to render stuff that might be translucent but actually isn't
        // using the translucent pass, than the opposite.
        const auto materialHasTranslucency = objectMaterial->properties.alphaMode == AlphaMode::Blend;

        //
        // If we're doing an opaque pass and the object has a translucent material, filter it out.
        //
        if (renderType == RenderType::GpassDeferred && materialHasTranslucency)
        {
            return false;
        }

        //
        // If we're doing a translucent pass and the object doesn't have a translucent material, filter it out
        //
        if (renderType == RenderType::GpassForward && !materialHasTranslucency)
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

    // TODO! When running translucent forward pass need to sort objects by
    //  distance from camera, probably need to have only 1 batch per object?

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
                case RenderType::GpassDeferred: programDef = m_programs->GetProgramDef("ObjectDeferred"); break;
                case RenderType::GpassForward: programDef = m_programs->GetProgramDef("ObjectForward"); break;
                case RenderType::Shadow: programDef = m_programs->GetProgramDef("ObjectShadow"); break;
            }
        }
        break;
        case MeshType::Bone:
        {
            switch (renderType)
            {
                case RenderType::GpassDeferred: programDef = m_programs->GetProgramDef("BoneObjectDeferred"); break;
                case RenderType::GpassForward: programDef = m_programs->GetProgramDef("BoneObjectForward"); break;
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

void ObjectRenderer::RenderBatch(const std::string& sceneName,
                                 BindState& bindState,
                                 RenderMetrics& renderMetrics,
                                 const RenderType& renderType,
                                 const ObjectRenderBatch& renderBatch,
                                 const RenderParams& renderParams,
                                 const VulkanCommandBufferPtr& commandBuffer,
                                 const VulkanRenderPassPtr& renderPass,
                                 const VulkanFramebufferPtr& framebuffer,
                                 const std::vector<ViewProjection>& viewProjections,
                                 const std::unordered_map<LightId, TextureId>& shadowMaps,
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
    bindState.set3Invalidated = true;

    //
    // Bind pipeline
    //
    if (!BindPipeline(bindState, renderType, renderBatch, commandBuffer, renderPass, framebuffer, shadowRenderData)) { return; }

    //
    // Bind Descriptor Sets
    //
    if (!BindDescriptorSet0(sceneName, bindState, renderType, renderParams, commandBuffer, viewProjections, shadowMaps)) { return; }
    if (!BindDescriptorSet1(bindState, commandBuffer)) { return; }
    if (!BindDescriptorSet2(bindState, renderBatch, commandBuffer)) { return; }
    if (!BindDescriptorSet3(bindState, renderBatch, commandBuffer)) { return; }

    //
    // Draw
    //
    std::size_t instanceIndex = 0;

    for (const auto& drawBatch : renderBatch.drawBatches)
    {
        const auto& drawBatchMesh = drawBatch.params.loadedMesh;

        BindVertexBuffer(bindState, commandBuffer, drawBatchMesh.verticesBuffer->GetBuffer());
        BindIndexBuffer(bindState, commandBuffer, drawBatchMesh.indicesBuffer->GetBuffer());

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

bool ObjectRenderer::BindPipeline(BindState& bindState,
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
    if (bindState.pipeline == *pipelineExpect) { return true; }

    //
    // Bind the pipeline
    //
    commandBuffer->CmdBindPipeline(*pipelineExpect);
    bindState.OnPipelineBound(renderBatch.params.programDef, *pipelineExpect);

    //
    // Write pipeline push constants
    //
    if (!BindPushConstants(bindState, renderType, commandBuffer, shadowRenderData)) { return false; }

    return true;
}

bool ObjectRenderer::BindPushConstants(BindState& bindState,
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
            *bindState.pipeline,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(ShadowLayerIndexPayload),
            &payload
        );
    }
    else
    {
        LightingSettingPayload payload{};
        payload.hdr = m_renderSettings.hdr;

        commandBuffer->CmdPushConstants(
            *bindState.pipeline,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(LightingSettingPayload),
            &payload
        );
    }

    return true;
}

bool ObjectRenderer::BindDescriptorSet0(const std::string& sceneName,
                                        BindState& bindState,
                                        const RenderType& renderType,
                                        const RenderParams& renderParams,
                                        const VulkanCommandBufferPtr& commandBuffer,
                                        const std::vector<ViewProjection>& viewProjections,
                                        const std::unordered_map<LightId, TextureId>& shadowMaps)
{
    //
    // If the set isn't invalidated, nothing to do
    //
    if (!bindState.set0Invalidated) { return true; }

    //
    // Create a descriptor set
    //
    const auto descriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        (*bindState.programDef)->GetDescriptorSetLayouts()[0],
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
    const auto sceneLights = m_lights->GetSceneLights(sceneName, viewProjections);

    if (!BindDescriptorSet0_Global(bindState, renderParams, (*descriptorSet), sceneLights)) { return false; }
    if (!BindDescriptorSet0_ViewProjection(bindState, viewProjections, (*descriptorSet))) { return false; }

    // Opaque pass gets lighting done in deferred lighting subpass, shadow doesn't do any lighting, only
    // forward rendering for translucent objects needs light data provided
    if (renderType == RenderType::GpassForward)
    {
        if (!BindDescriptorSet0_Lights(bindState, (*descriptorSet), sceneLights, shadowMaps))
        {
            return false;
        }
    }

    //
    // Bind the descriptor set
    //
    commandBuffer->CmdBindDescriptorSets(*bindState.pipeline, 0, {(*descriptorSet)->GetVkDescriptorSet()});
    bindState.OnSet0Bound();

    return true;
}

bool ObjectRenderer::BindDescriptorSet0_Global(BindState& bindState,
                                               const RenderParams& renderParams,
                                               const VulkanDescriptorSetPtr& descriptorSet,
                                               const std::vector<LoadedLight>& lights) const
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
    // Update the global data buffer with the global data
    //
    const GlobalPayload globalPayload = GetGlobalPayload(renderParams, lights.size());
    (*globalDataBuffer)->PushBack(ExecutionContext::CPU(), {globalPayload});

    //
    // Set Data
    //
    descriptorSet->WriteBufferBind(
        (*bindState.programDef)->GetBindingDetailsByName("u_globalData"),
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

bool ObjectRenderer::BindDescriptorSet0_ViewProjection(BindState& bindState,
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

// TODO: Combine logic with DeferredLightingRenderer
bool ObjectRenderer::BindDescriptorSet0_Lights(const BindState& bindState,
                                               const VulkanDescriptorSetPtr& globalDataDescriptorSet,
                                               const std::vector<LoadedLight>& lights,
                                               const std::unordered_map<LightId, TextureId>& shadowMaps) const
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
    const auto defaultLightTextures = std::vector<TextureId>(Max_Light_Count, Render::TextureId(Render::INVALID_ID));

    std::unordered_map<ShadowMapType, std::vector<TextureId>> shadowMapTextureIds {
        {ShadowMapType::Single, defaultLightTextures},
        {ShadowMapType::Cube, defaultLightTextures}
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
            shadowMapTextureIds[loadedLight.shadowMapType][lightIndex] = lightShadowMapIt->second;
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
    if (!BindDescriptorSet0_ShadowMapTextures(bindState, globalDataDescriptorSet, shadowMapTextureIds))
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

bool ObjectRenderer::BindDescriptorSet0_ShadowMapTextures(const BindState& bindState,
                                                          const VulkanDescriptorSetPtr& globalDataDescriptorSet,
                                                          const std::unordered_map<ShadowMapType, std::vector<TextureId>>& shadowMapTextureIds) const
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

    for (const auto& typeIt : shadowMapTextureIds)
    {
        switch (typeIt.first)
        {
            case ShadowMapType::Single:
            {
                shadowBindingDetails = *shadowMapBindingDetails;
                shadowImageViewName = TextureView::DEFAULT;
                shadowSamplerName = TextureSampler::DEFAULT;
                missingTextureImageView = missingTexture.vkImageViews.at(TextureView::DEFAULT);
                missingTextureSampler = missingTexture.vkSamplers.at(TextureSampler::DEFAULT);
            }
            break;

            case ShadowMapType::Cube:
            {
                shadowBindingDetails = *shadowMapBindingDetails_Cube;
                shadowImageViewName = TextureView::DEFAULT;
                shadowSamplerName = TextureSampler::DEFAULT;
                missingTextureImageView = missingCubeTexture.vkImageViews.at(TextureView::DEFAULT);
                missingTextureSampler = missingCubeTexture.vkSamplers.at(TextureSampler::DEFAULT);
            }
            break;
        }

        std::vector<std::pair<VkImageView, VkSampler>> samplerBinds;

        for (const auto& textureId : typeIt.second)
        {
            const auto textureOpt = m_textures->GetTexture(textureId);

            if (textureOpt)
            {
                samplerBinds.emplace_back(
                    textureOpt->vkImageViews.at(shadowImageViewName),
                    textureOpt->vkSamplers.at(shadowSamplerName)
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

bool ObjectRenderer::BindDescriptorSet1(BindState& bindState, const VulkanCommandBufferPtr& commandBuffer)
{
    //
    // If the set isn't invalidated, nothing to do
    //
    if (!bindState.set1Invalidated) { return true; }

    //
    // Create a descriptor set
    //
    const auto descriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        (*bindState.programDef)->GetDescriptorSetLayouts()[1],
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
    BindDescriptorSet1_RendererData(bindState, *descriptorSet);

    //
    // Bind the descriptor set
    //
    commandBuffer->CmdBindDescriptorSets(*bindState.pipeline, 1, {(*descriptorSet)->GetVkDescriptorSet()});
    bindState.OnSet1Bound();

    return true;
}

void ObjectRenderer::BindDescriptorSet1_RendererData(const BindState& bindState, const VulkanDescriptorSetPtr& descriptorSet) const
{
    //
    // Update the descriptor set with data
    //
    const auto objectPayloadBuffer =  m_renderables->GetObjects().GetObjectPayloadBuffer();

    descriptorSet->WriteBufferBind(
        (*bindState.programDef)->GetBindingDetailsByName("i_objectData"),
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        objectPayloadBuffer->GetBuffer()->GetVkBuffer(),
        0,
        0
    );
}

bool ObjectRenderer::BindDescriptorSet2(BindState& bindState,
                                        const ObjectRenderBatch& renderBatch,
                                        const VulkanCommandBufferPtr& commandBuffer)
{
    const auto& loadedMaterial = renderBatch.params.loadedMaterial;

    //
    // If the set isn't invalidated and the bound data is the same, nothing to do
    //
    const bool dataBindsMatch =
        bindState.materialDataBufferId == loadedMaterial.payloadBuffer->GetBuffer()->GetBufferId() &&
        bindState.materialTextures == loadedMaterial.textureBinds;

    if (!bindState.set2Invalidated && dataBindsMatch) { return true; }

    //
    // Create a descriptor set
    //
    const auto descriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        (*bindState.programDef)->GetDescriptorSetLayouts()[2],
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
    BindDescriptorSet2_MaterialData(bindState, renderBatch, *descriptorSet);

    //
    // Bind the descriptor set
    //
    commandBuffer->CmdBindDescriptorSets(*bindState.pipeline, 2, {(*descriptorSet)->GetVkDescriptorSet()});
    bindState.OnSet2Bound();

    return true;
}

void ObjectRenderer::BindDescriptorSet2_MaterialData(BindState& bindState,
                                                     const ObjectRenderBatch& renderBatch,
                                                     const VulkanDescriptorSetPtr& descriptorSet)
{
    const auto& loadedMaterial = renderBatch.params.loadedMaterial;

    //
    // Update the descriptor set with data
    //
    descriptorSet->WriteBufferBind(
        (*bindState.programDef)->GetBindingDetailsByName("i_materialData"),
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
            (*bindState.programDef)->GetBindingDetailsByName(textureBindIt.first),
            loadedTexture->vkImageViews.at(TextureView::DEFAULT),
            loadedTexture->vkSamplers.at(TextureSampler::DEFAULT)
        );
    }

    //
    // Finish
    //
    bindState.materialDataBufferId = loadedMaterial.payloadBuffer->GetBuffer()->GetBufferId();
    bindState.materialTextures = loadedMaterial.textureBinds;
}

bool ObjectRenderer::BindDescriptorSet3(BindState& bindState,
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
    if (!bindState.set3Invalidated)
    {
        return true;
    }

    //
    // Create a descriptor set
    //
    const auto drawDescriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        (*bindState.programDef)->GetDescriptorSetLayouts()[3],
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
    if (!BindDescriptorSet3_DrawData(bindState, renderBatch, *drawDescriptorSet, batchMeshDataBufferId)) { return false; }
    BindDescriptorSet3_MeshData(bindState, renderBatch, *drawDescriptorSet);
    if (!BindDescriptorSet3_BoneData(bindState, renderBatch, *drawDescriptorSet)) { return false; }

    //
    // Bind the draw descriptor set
    //
    commandBuffer->CmdBindDescriptorSets(*bindState.pipeline, 3, {(*drawDescriptorSet)->GetVkDescriptorSet()});
    bindState.OnSet3Bound();

    return true;
}

bool ObjectRenderer::BindDescriptorSet3_DrawData(const BindState& bindState,
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
        (*bindState.programDef)->GetBindingDetailsByName("i_drawData"),
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

void ObjectRenderer::BindDescriptorSet3_MeshData(const BindState& bindState,
                                                 const ObjectRenderBatch& renderBatch,
                                                 const VulkanDescriptorSetPtr& drawDescriptorSet)
{
    // If there's no mesh data buffer to be bound, nothing to do
    if (!renderBatch.params.meshDataBuffer) { return; }

    //
    // Bind the mesh's data buffer
    //
    drawDescriptorSet->WriteBufferBind(
        (*bindState.programDef)->GetBindingDetailsByName("i_meshData"),
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        (*renderBatch.params.meshDataBuffer)->GetBuffer()->GetVkBuffer(),
        0,
        0
    );
}

bool ObjectRenderer::BindDescriptorSet3_BoneData(const BindState& bindState,
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
        (*bindState.programDef)->GetBindingDetailsByName("i_boneData"),
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

void ObjectRenderer::BindVertexBuffer(BindState& bindState,
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

void ObjectRenderer::BindIndexBuffer(BindState& bindState,
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
        case RenderType::GpassDeferred:
        case RenderType::GpassForward:
        {
            cullFace = CullFace::Back;

            if (std::dynamic_pointer_cast<ObjectMaterial>(renderBatch.params.loadedMaterial.material)->properties.twoSided)
            {
                cullFace = CullFace::None;
            }
        }
        break;
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
    else
    {
        pushConstantRanges = {
            {VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LightingSettingPayload)}
        };
    }

    uint32_t subpassIndex{0};

    switch (renderType)
    {
        case RenderType::GpassDeferred:     subpassIndex = GPassRenderPass_SubPass_DeferredLightingObjects; break;
        case RenderType::GpassForward:      subpassIndex = GPassRenderPass_SubPass_ForwardLightingObjects; break;
        case RenderType::Shadow:            subpassIndex = ShadowRenderPass_ShadowSubpass_Index; break;
    }

    auto fillMode = PolygonFillMode::Fill;

    if (m_vulkanObjs->GetRenderSettings().objectsWireframe)
    {
        fillMode = PolygonFillMode::Line;
    }

    const auto depthBias = renderType == RenderType::Shadow ? DepthBias::Enabled : DepthBias::Disabled;

    auto pipeline = GetGraphicsPipeline(
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
        depthBias,
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
