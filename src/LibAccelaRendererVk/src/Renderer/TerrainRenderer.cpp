#include "TerrainRenderer.h"

#include "../PostExecutionOp.h"

#include "../Buffer/CPUItemBuffer.h"
#include "../Renderables/IRenderables.h"
#include "../Program/IPrograms.h"
#include "../Mesh/IMeshes.h"
#include "../Pipeline/IPipelineFactory.h"
#include "../Pipeline/PipelineUtil.h"
#include "../Material/IMaterials.h"
#include "../Texture/ITextures.h"

#include "../Vulkan/VulkanDebug.h"
#include "../Vulkan/VulkanFramebuffer.h"
#include "../Vulkan/VulkanCommandBuffer.h"
#include "../Vulkan/VulkanDescriptorSet.h"
#include "../Vulkan/VulkanPipeline.h"

#include <Accela/Render/Mesh/StaticMesh.h>

#include <set>
#include <algorithm>

namespace Accela::Render
{

TerrainRenderer::TerrainRenderer(Common::ILogger::Ptr logger,
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
    : Renderer(
        std::move(logger),
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

bool TerrainRenderer::Initialize(const RenderSettings& renderSettings)
{
    if (!Renderer::Initialize(renderSettings))
    {
        return false;
    }

    m_programDef = m_programs->GetProgramDef("Terrain");
    if (m_programDef == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "TerrainRenderer: Terrain program doesn't exist");
        return false;
    }

    if (!CreateTerrainMesh()) { return false; }

    return true;
}

bool TerrainRenderer::CreateTerrainMesh()
{
    const auto meshId = m_ids->meshIds.GetId();

    const auto mesh = std::make_shared<StaticMesh>(
        meshId,
        std::vector<MeshVertex>{
            {
                {{-0.5f, 0.0f, 0.5f}, {0, 1, 0}, {0, 1}},
                {{0.5f, 0.0f, 0.5f}, {0, 1, 0}, {1, 1}},
                {{0.5f, 0.0f, -0.5f}, {0, 1, 0}, {1, 0}},
                {{-0.5f, 0.0f, -0.5f}, {0, 1, 0}, {0, 0}}
            }
        },
        // To support triangle / 3 point patches, restore indices below to 6 indices, for two triangles
        std::vector<uint32_t>{ 0, 1, 2, 3 },
        std::format("TerrainMesh-{}", m_frameIndex)
    );

    if (!m_meshes->LoadMesh(mesh, MeshUsage::Static, std::promise<bool>{}))
    {
        m_logger->Log(Common::LogLevel::Error, "TerrainRenderer: Failed to create terrain mesh");
        m_ids->meshIds.ReturnId(mesh->id);
        return false;
    }

    m_terrainMeshId = meshId;

    return true;
}

void TerrainRenderer::Destroy()
{
    if (m_terrainMeshId.IsValid())
    {
        m_meshes->DestroyMesh(m_terrainMeshId, true);
        m_terrainMeshId = MeshId{INVALID_ID};
    }

    m_programDef = nullptr;

    if (m_pipelineHash)
    {
        m_pipelines->DestroyPipeline(*m_pipelineHash);
        m_pipelineHash = std::nullopt;
    }

    Renderer::Destroy();
}

void TerrainRenderer::Render(const std::string& sceneName,
                             const RenderParams& renderParams,
                             const VulkanCommandBufferPtr& commandBuffer,
                             const VulkanRenderPassPtr& renderPass,
                             const VulkanFramebufferPtr& framebuffer,
                             const std::vector<ViewProjection>& viewProjections)
{
    // Bail out early if there's no terrain to be rendered
    if (m_renderables->GetTerrain().GetData().empty()) { return; }

    CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "TerrainRenderer");

    //
    // Compile the batches of terrain to be rendered
    //
    const auto terrainBatches = CompileBatches(sceneName);

    // TODO Perf: Is it worth it to try to have draw batches within each render batch like ObjectRenderer does?

    //
    // Render each terrain batch
    //
    RenderState renderState{};

    for (const auto& terrainBatch : terrainBatches)
    {
        RenderBatch(renderState, terrainBatch, renderParams, commandBuffer, renderPass, framebuffer, viewProjections);
    }
}

std::vector<TerrainRenderer::TerrainBatch> TerrainRenderer::CompileBatches(const std::string& sceneName) const
{
    //
    // Map the scene's visible terrain renderables into batches by batch key
    //
    std::unordered_map<TerrainBatchKey, TerrainBatch, TerrainBatchKey::HashFunction> batchesByKey;

    for (const auto& terrain : m_renderables->GetTerrain().GetData())
    {
        // Skip over invalid (deleted) terrain, don't render them
        if (!terrain.isValid) { continue; }

        // Skip over terrain in a different scene
        if (terrain.renderable.sceneName != sceneName) { continue; }

        const auto terrainBatchKey = GetBatchKey(terrain.renderable);

        const auto batchIt = batchesByKey.find(terrainBatchKey);
        if (batchIt == batchesByKey.cend())
        {
            const auto batch = CreateTerrainBatch(terrain.renderable);
            if (!batch) { continue; }

            batchesByKey[terrainBatchKey] = *batch;
        }

        batchesByKey[terrainBatchKey].terrainIds.push_back(terrain.renderable.terrainId);
    }

    //
    // Sort the batches by material then by height map
    //
    auto batchSort = [](const TerrainBatch& a, const TerrainBatch& b)
    {
        return  std::tie(a.batchKey.materialId, a.batchKey.heightMapTextureId) <
                std::tie(b.batchKey.materialId, b.batchKey.heightMapTextureId);
    };
    auto batchesSet = std::set<TerrainBatch, decltype(batchSort)>(batchSort);

    for (const auto& batchIt : batchesByKey)
    {
        batchesSet.insert(batchIt.second);
    }

    //
    // Transform set to ordered vector for return
    //
    return {batchesSet.cbegin(), batchesSet.cend()};
}

TerrainRenderer::TerrainBatchKey TerrainRenderer::GetBatchKey(const TerrainRenderable& terrainRenderable) const
{
    TerrainBatchKey terrainBatchKey{};
    terrainBatchKey.meshId = m_terrainMeshId;
    terrainBatchKey.materialId = terrainRenderable.materialId;
    terrainBatchKey.heightMapTextureId = terrainRenderable.heightMapTextureId;

    return terrainBatchKey;
}

std::expected<TerrainRenderer::TerrainBatch, bool> TerrainRenderer::CreateTerrainBatch(const TerrainRenderable& terrainRenderable) const
{
    TerrainBatch terrainBatch{};
    terrainBatch.batchKey = GetBatchKey(terrainRenderable);

    //
    // Batch Mesh
    //
    const auto loadedMeshOpt = m_meshes->GetLoadedMesh(m_terrainMeshId);
    if (!loadedMeshOpt)
    {
        m_logger->Log(Common::LogLevel::Error, "TerrainRenderer::CreateTerrainBatch: Terrain mesh doesn't exist");
        return std::unexpected(false);
    }
    terrainBatch.loadedMesh = *loadedMeshOpt;

    //
    // Batch Material
    //
    const auto loadedMaterialOpt = m_materials->GetLoadedMaterial(terrainRenderable.materialId);
    if (!loadedMaterialOpt)
    {
        m_logger->Log(Common::LogLevel::Error,
          "TerrainRenderer::CreateTerrainBatch: No such material exists, {}", terrainRenderable.materialId.id);
        return std::unexpected(false);
    }
    terrainBatch.loadedMaterial = *loadedMaterialOpt;

    //
    // Batch Height Map Texture
    //
    const auto loadedHeightMapTextureOpt = m_textures->GetTexture(terrainRenderable.heightMapTextureId);
    if (!loadedHeightMapTextureOpt)
    {
        m_logger->Log(Common::LogLevel::Error,
          "TerrainRenderer::CreateTerrainBatch: No such height map texture exists, {}", terrainRenderable.heightMapTextureId.id);
        return std::unexpected(false);
    }
    terrainBatch.loadedHeightMapTexture = *loadedHeightMapTextureOpt;

    return terrainBatch;
}

void TerrainRenderer::RenderBatch(RenderState& renderState,
                                  const TerrainRenderer::TerrainBatch& terrainBatch,
                                  const RenderParams& renderParams,
                                  const VulkanCommandBufferPtr& commandBuffer,
                                  const VulkanRenderPassPtr& renderPass,
                                  const VulkanFramebufferPtr& framebuffer,
                                  const std::vector<ViewProjection>& viewProjections)
{
    const auto batchMaterialId = terrainBatch.batchKey.materialId;

    CmdBufferSectionLabel sectionLabel(
        m_vulkanObjs->GetCalls(),
        commandBuffer,
        std::format("TerrainRenderBatch-{}", batchMaterialId.id)
    );

    // We want to bind per-batch draw data to set 3 for every batch, so forcefully mark it as invalidated
    renderState.set3Invalidated = true;

    // Bind Data (as needed)

    if (!BindPipeline(renderState, commandBuffer, renderPass, framebuffer)) { return; }
    if (!BindDescriptorSet0(renderState, renderParams, commandBuffer, viewProjections)) { return; }
    if (!BindDescriptorSet1(renderState, commandBuffer)) { return; }
    if (!BindDescriptorSet2(renderState, terrainBatch, commandBuffer)) { return; }
    if (!BindDescriptorSet3(renderState, terrainBatch, commandBuffer)) { return; }

    // Draw

    const auto verticesBuffer = terrainBatch.loadedMesh.verticesBuffer->GetBuffer();
    const auto indicesBuffer = terrainBatch.loadedMesh.indicesBuffer->GetBuffer();

    BindVertexBuffer(renderState, commandBuffer, terrainBatch.loadedMesh.verticesBuffer->GetBuffer());
    BindIndexBuffer(renderState, commandBuffer, terrainBatch.loadedMesh.indicesBuffer->GetBuffer());

    commandBuffer->CmdDrawIndexed(
        terrainBatch.loadedMesh.numIndices,
        terrainBatch.terrainIds.size(),
        terrainBatch.loadedMesh.indicesOffset,
        (int32_t)terrainBatch.loadedMesh.verticesOffset,
        0
    );
}

bool TerrainRenderer::BindPipeline(RenderState& renderState,
                                   const VulkanCommandBufferPtr& commandBuffer,
                                   const VulkanRenderPassPtr& renderPass,
                                   const VulkanFramebufferPtr& framebuffer)
{
    //
    // If the program is already bound, nothing to do
    //
    if (renderState.programDef == m_programDef) { return true; }

    //
    // Otherwise, get the pipeline for this batch
    //
    const auto pipelineExpect = GetBatchPipeline(renderPass, framebuffer);
    if (!pipelineExpect)
    {
        m_logger->Log(Common::LogLevel::Error, "TerrainRenderer::BindPipeline: GetBatchPipeline failed");
        return false;
    }

    //
    // Bind the pipeline
    //
    commandBuffer->CmdBindPipeline(*pipelineExpect);
    renderState.OnPipelineBound(m_programDef, *pipelineExpect);

    return true;
}

std::expected<VulkanPipelinePtr, bool> TerrainRenderer::GetBatchPipeline(const VulkanRenderPassPtr& renderPass,
                                                                        const VulkanFramebufferPtr& framebuffer)
{
    //
    // Retrieve the pipeline to use for rendering the batch
    //
    const auto viewport = Viewport(0, 0, framebuffer->GetSize()->w, framebuffer->GetSize()->h);

    auto pipeline = GetPipeline(
        m_logger,
        m_vulkanObjs,
        m_shaders,
        m_pipelines,
        m_programDef,
        renderPass,
        Offscreen_GPassOpaqueSubpass_Index,
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
        m_logger->Log(Common::LogLevel::Error, "TerrainRenderer: GetBatchPipeline: Failed to fetch batch pipeline");
        return std::unexpected(false);
    }

    // Keep track of the latest pipeline hash that was used for this program
    m_pipelineHash = (*pipeline)->GetConfigHash();

    return pipeline;
}

bool TerrainRenderer::BindDescriptorSet0(RenderState& renderState,
                                         const RenderParams& renderParams,
                                         const VulkanCommandBufferPtr& commandBuffer,
                                         const std::vector<ViewProjection>& viewProjections) const
{
    //
    // If global data is already bound, nothing to do
    //
    if (!renderState.set0Invalidated) { return true; }

    //
    // Create a descriptor set
    //
    const auto globalDataDescriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        (*renderState.programDef)->GetDescriptorSetLayouts()[0],
        std::format("TerrainRenderer-DS0-{}", m_frameIndex)
    );
    if (!globalDataDescriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error,
          "TerrainRenderer::BindDescriptorSet0: Failed to get or create global data descriptor set");
        return false;
    }

    //
    // Update the descriptor set with data
    //
    if (!BindDescriptorSet0_Global(renderState, renderParams, *globalDataDescriptorSet)) { return false; }
    if (!BindDescriptorSet0_ViewProjection(renderState, *globalDataDescriptorSet, viewProjections)) { return false; }

    //
    // Bind the global data descriptor set
    //
    commandBuffer->CmdBindDescriptorSets(*renderState.pipeline, 0, {(*globalDataDescriptorSet)->GetVkDescriptorSet()});
    renderState.OnSet0Bound();

    return true;
}

bool TerrainRenderer::BindDescriptorSet0_Global(const RenderState& renderState,
                                                const RenderParams& renderParams,
                                                const VulkanDescriptorSetPtr& globalDataDescriptorSet) const
{
    //
    // Create a buffer
    //
    const auto globalDataBuffer = CPUItemBuffer<GlobalPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        1,
        std::format("TerrainRenderer-DS0-GlobalData-{}", m_frameIndex)
    );
    if (!globalDataBuffer)
    {
        m_logger->Log(Common::LogLevel::Error, "TerrainRenderer::BindDescriptorSet0_Global: Failed to create global data buffer");
        return false;
    }

    //
    // Update the global data buffer with the global data
    //
    const GlobalPayload globalPayload = GetGlobalPayload(renderParams, 0);
    (*globalDataBuffer)->PushBack(ExecutionContext::CPU(), {globalPayload});

    //
    // Bind the global data buffer to the global data descriptor set
    //
    globalDataDescriptorSet->WriteBufferBind(
        (*renderState.programDef)->GetBindingDetailsByName("u_globalData"),
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

bool TerrainRenderer::BindDescriptorSet0_ViewProjection(const RenderState& renderState,
                                                        const VulkanDescriptorSetPtr& globalDataDescriptorSet,
                                                        const std::vector<ViewProjection>& viewProjections) const
{
    //
    // Create buffer
    //
    const auto viewProjectionDataBuffer = CPUItemBuffer<ViewProjectionPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        viewProjections.size(),
        std::format("TerrainRenderer-DS0-ViewProjectionData-{}", m_frameIndex)
    );
    if (!viewProjectionDataBuffer)
    {
        m_logger->Log(Common::LogLevel::Error,
          "TerrainRenderer::BindDescriptorSet0_ViewProjection: Failed to create view projection data buffer");
        return false;
    }

    //
    // Set Data
    //
    for (const auto& viewProjection : viewProjections)
    {
        const ViewProjectionPayload viewProjectionPayload = GetViewProjectionPayload(viewProjection);
        (*viewProjectionDataBuffer)->PushBack(ExecutionContext::CPU(), {viewProjectionPayload});
    }

    globalDataDescriptorSet->WriteBufferBind(
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

bool TerrainRenderer::BindDescriptorSet1(RenderState& renderState, const VulkanCommandBufferPtr& commandBuffer) const
{
    //
    // If renderer data is already bound, nothing to do
    //
    if (!renderState.set1Invalidated) { return true; }

    //
    // Otherwise, retrieve a descriptor set for binding renderer data
    //
    const auto rendererDataDescriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        (*renderState.programDef)->GetDescriptorSetLayouts()[1],
        std::format("TerrainRenderer-DS1-{}", m_frameIndex)
    );
    if (!rendererDataDescriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error,
          "TerrainRenderer::BindDescriptorSet1: Failed to get or create renderer data descriptor set");
        return false;
    }

    //
    // Update the descriptor set with data
    //
    const auto terrainPayloadBuffer =  m_renderables->GetTerrain().GetTerrainPayloadBuffer();

    (*rendererDataDescriptorSet)->WriteBufferBind(
        (*renderState.programDef)->GetBindingDetailsByName("i_terrainData"),
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        terrainPayloadBuffer->GetBuffer()->GetVkBuffer(),
        0,
        0
    );

    //
    // Bind the renderer descriptor set
    //
    commandBuffer->CmdBindDescriptorSets(*renderState.pipeline, 1, {(*rendererDataDescriptorSet)->GetVkDescriptorSet()});
    renderState.OnSet1Bound();

    return true;
}

bool TerrainRenderer::BindDescriptorSet2(RenderState& renderState,
                                         const TerrainBatch& terrainBatch,
                                         const VulkanCommandBufferPtr& commandBuffer) const
{
    const auto& loadedMaterial = terrainBatch.loadedMaterial;

    // If renderer data is already bound, nothing to do
    //
    const bool dataBindsMatch =
        renderState.materialDataBufferId == loadedMaterial.payloadBuffer->GetBuffer()->GetBufferId() &&
        renderState.materialTextures == loadedMaterial.textureBinds;

    if (!renderState.set2Invalidated && dataBindsMatch) { return true; }

    //
    // Create a descriptor set
    //
    const auto materialDescriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        (*renderState.programDef)->GetDescriptorSetLayouts()[2],
        std::format("TerrainRenderer-DS2", m_frameIndex)
    );
    if (!materialDescriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error,
          "TerrainRenderer::BindDescriptorSet2: Failed to get or create material descriptor set");
        return false;
    }

    //
    // Update the descriptor set with data
    //

    //
    // Bind the material's payload
    //
    (*materialDescriptorSet)->WriteBufferBind(
        (*renderState.programDef)->GetBindingDetailsByName("i_materialData"),
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        terrainBatch.loadedMaterial.payloadBuffer->GetBuffer()->GetVkBuffer(),
        terrainBatch.loadedMaterial.payloadByteOffset,
        terrainBatch.loadedMaterial.payloadByteSize
    );

    renderState.materialDataBufferId = loadedMaterial.payloadBuffer->GetBuffer()->GetBufferId();

    //
    // Bind the material's textures
    //
    for (const auto& textureBindIt : terrainBatch.loadedMaterial.textureBinds)
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
              "TerrainRenderer: RenderBatch: Failed to fetch any texture for texture: {}", textureBindIt.second.id);
            continue;
        }

        (*materialDescriptorSet)->WriteCombinedSamplerBind(
            (*renderState.programDef)->GetBindingDetailsByName(textureBindIt.first),
            loadedTexture->vkImageViews.at(TextureView::DEFAULT),
            loadedTexture->vkSampler
        );
    }

    renderState.materialTextures = loadedMaterial.textureBinds;

    //
    // Bind the material descriptor set
    //
    commandBuffer->CmdBindDescriptorSets(*renderState.pipeline, 2, {(*materialDescriptorSet)->GetVkDescriptorSet()});
    renderState.OnSet2Bound();

    return true;
}

bool TerrainRenderer::BindDescriptorSet3(RenderState& renderState,
                                         const TerrainBatch& terrainBatch,
                                         const VulkanCommandBufferPtr& commandBuffer) const
{
    //
    // If the set isn't invalidated, bail out. Note: This is just for consistency; we bind new draw
    // data to DS3 for every batch, so set3 is always invalidated at the start of every batch draw.
    //
    if (!renderState.set3Invalidated)
    {
        return true;
    }

    //
    // Retrieve a descriptor set for binding draw data
    //
    const auto drawDescriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        (*renderState.programDef)->GetDescriptorSetLayouts()[3],
        std::format("TerrainRenderer-DS3-{}-{}", terrainBatch.batchKey.materialId.id, m_frameIndex)
    );
    if (!drawDescriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error,
          "TerrainRenderer::BindDescriptorSet3: Failed to get or create draw descriptor set");
        return false;
    }

    //
    // Update the descriptor set with data
    //

    //
    // Update the "draw data" bind, which contains the ids of terrain to be rendered
    //
    if (!BindDescriptorSet3_DrawData(renderState, terrainBatch, *drawDescriptorSet)) { return false; }

    //
    // Update the height map sampler bind
    //
    (*drawDescriptorSet)->WriteCombinedSamplerBind(
        (*renderState.programDef)->GetBindingDetailsByName("i_heightSampler"),
        terrainBatch.loadedHeightMapTexture.vkImageViews.at(TextureView::DEFAULT),
        terrainBatch.loadedHeightMapTexture.vkSampler
    );

    //
    // Bind the draw descriptor set
    //
    commandBuffer->CmdBindDescriptorSets(*renderState.pipeline, 3, {(*drawDescriptorSet)->GetVkDescriptorSet()});
    renderState.OnSet3Bound();

    return true;
}

bool TerrainRenderer::BindDescriptorSet3_DrawData(RenderState& renderState,
                                                  const TerrainBatch& terrainBatch,
                                                  const VulkanDescriptorSetPtr& drawDescriptorSet) const
{
    //
    // Create a per-render CPU buffer to hold draw data
    //
    const auto drawDataBufferExpect = CPUItemBuffer<ObjectDrawPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        terrainBatch.terrainIds.size(),
        std::format("TerrainRenderer-DS3-DrawData-{}-{}", terrainBatch.batchKey.materialId.id, m_frameIndex)
    );
    if (!drawDataBufferExpect)
    {
        m_logger->Log(Common::LogLevel::Error,
          "TerrainRenderer::BindDescriptorSet3_DrawData: Failed to create draw data buffer");
        return false;
    }
    const auto& drawDataBuffer = *drawDataBufferExpect;

    //
    // Convert the batch terrain to be rendered to DrawPayloads
    //
    std::vector<ObjectDrawPayload> drawPayloads(terrainBatch.terrainIds.size());

    std::ranges::transform(terrainBatch.terrainIds, drawPayloads.begin(), [](const TerrainId& id){
        ObjectDrawPayload drawPayload{};
        drawPayload.dataIndex = id.id - 1;
        return drawPayload;
    });

    drawDataBuffer->Resize(ExecutionContext::CPU(), drawPayloads.size());
    drawDataBuffer->Update(ExecutionContext::CPU(), 0, drawPayloads);

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

void TerrainRenderer::BindVertexBuffer(RenderState& renderState,
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

void TerrainRenderer::BindIndexBuffer(RenderState& renderState,
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

}
