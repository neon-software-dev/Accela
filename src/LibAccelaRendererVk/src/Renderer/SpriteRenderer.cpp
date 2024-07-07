/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SpriteRenderer.h"

#include "../PostExecutionOp.h"

#include "../Buffer/IBuffers.h"
#include "../Buffer/CPUItemBuffer.h"
#include "../Program/IPrograms.h"
#include "../Pipeline/PipelineUtil.h"
#include "../Pipeline/IPipelineFactory.h"
#include "../Mesh/IMeshes.h"
#include "../Renderables/IRenderables.h"
#include "../Texture/ITextures.h"

#include "../Vulkan/VulkanDebug.h"
#include "../Vulkan/VulkanFramebuffer.h"
#include "../Vulkan/VulkanPipeline.h"
#include "../Vulkan/VulkanDescriptorSet.h"
#include "../Vulkan/VulkanCommandBuffer.h"

#include <Accela/Render/Mesh/StaticMesh.h>

#include <glm/gtc/matrix_transform.hpp>

#include <format>
#include <algorithm>

namespace Accela::Render
{


SpriteRenderer::SpriteRenderer(Common::ILogger::Ptr logger,
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

bool SpriteRenderer::Initialize(const RenderSettings& renderSettings)
{
    if (!Renderer::Initialize(renderSettings))
    {
        return false;
    }

    m_programDef = m_programs->GetProgramDef("Sprite");
    if (m_programDef == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "SpriteRenderer: Sprite program doesn't exist");
        return false;
    }

    if (!CreateSpriteMesh()) { return false; }

    return true;
}

bool SpriteRenderer::CreateSpriteMesh()
{
    const auto meshId = m_ids->meshIds.GetId();

    const auto mesh = std::make_shared<StaticMesh>(
        meshId,
        std::vector<MeshVertex>{
            {
                {{-0.5f, -0.5f, 0}, {0, 0, 0}, {0, 0}},
                {{0.5f, -0.5f, 0}, {0, 0, 0}, {0, 0}},
                {{0.5f, 0.5f, 0}, {0, 0, 0}, {0, 0}},
                {{-0.5f, 0.5f, 0}, {0, 0, 0}, {0, 0}}
            }
        },
        std::vector<uint32_t>{ 0, 2, 1, 0, 3, 2 },
        std::format("SpriteRenderer-{}", m_frameIndex)
    );

    if (!m_meshes->LoadMesh(mesh, MeshUsage::Static, std::promise<bool>{}))
    {
        m_logger->Log(Common::LogLevel::Error, "SpriteRenderer: Failed to create sprite mesh");
        m_ids->meshIds.ReturnId(mesh->id);
        return false;
    }

    m_spriteMeshId = meshId;

    return true;
}

void SpriteRenderer::Destroy()
{
    if (m_spriteMeshId.IsValid())
    {
        m_meshes->DestroyMesh(m_spriteMeshId, true);
        m_spriteMeshId = MeshId{INVALID_ID};
    }

    m_programDef = nullptr;

    if (m_pipelineHash)
    {
        m_pipelines->DestroyPipeline(*m_pipelineHash);
        m_pipelineHash = std::nullopt;
    }

    Renderer::Destroy();
}

void SpriteRenderer::Render(const std::string& sceneName,
                            const RenderParams& renderParams,
                            const VulkanCommandBufferPtr& commandBuffer,
                            const VulkanRenderPassPtr& renderPass,
                            const VulkanFramebufferPtr& framebuffer)
{
    // Bail out early if there's no sprites to be rendered
    if (m_renderables->GetSprites().GetData().empty()) { return; }

    CmdBufferSectionLabel sectionLabel(m_vulkanObjs->GetCalls(), commandBuffer, "SpriteRenderer");

    //
    // Retrieve the pipeline to use for rendering
    //
    const auto viewport = Viewport(0, 0, framebuffer->GetSize()->w, framebuffer->GetSize()->h);

    const auto pipeline = GetGraphicsPipeline(
        m_logger,
        m_vulkanObjs,
        m_shaders,
        m_pipelines,
        m_programDef,
        renderPass,
        BlitRenderPass_SubPass_Blit,
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
        m_logger->Log(Common::LogLevel::Error, "SpriteRenderer: Render: Failed to fetch pipeline");
        return;
    }

    m_pipelineHash = (*pipeline)->GetConfigHash(); // Keep track of the latest pipeline that was used

    //
    // Fetch the sprite mesh's data
    //
    const auto spriteMesh = m_meshes->GetLoadedMesh(m_spriteMeshId);
    if (!spriteMesh)
    {
        m_logger->Log(Common::LogLevel::Error,
          "SpriteRenderer: Render: No such sprite mesh exists: {}", m_spriteMeshId.id);
        return;
    }

    const auto vkMeshVerticesBuffer = spriteMesh->verticesBuffer->GetBuffer()->GetVkBuffer();
    const auto vkMeshIndicesBuffer = spriteMesh->indicesBuffer->GetBuffer()->GetVkBuffer();

    //
    // Update global data descriptor set
    //
    const auto globalDataDescriptorSet = UpdateGlobalDescriptorSet(renderParams);
    if (!globalDataDescriptorSet) { return; }

    //
    // Update renderer data descriptor set
    //
    const auto rendererDataDescriptorSet = UpdateRendererDescriptorSet();
    if (!rendererDataDescriptorSet) { return; }

    //
    // Convert the scene's sprites into batches to be rendered
    //
    const auto spriteBatches = CompileSpriteBatches(sceneName);

    //
    // Start the render
    //
    commandBuffer->CmdBindPipeline(*pipeline);
    commandBuffer->CmdBindVertexBuffers(0, 1, {vkMeshVerticesBuffer}, {VkDeviceSize{0}});
    commandBuffer->CmdBindIndexBuffer(vkMeshIndicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    commandBuffer->CmdBindDescriptorSets(*pipeline, 0, {(*globalDataDescriptorSet)->GetVkDescriptorSet()});
    commandBuffer->CmdBindDescriptorSets(*pipeline, 1, {(*rendererDataDescriptorSet)->GetVkDescriptorSet()});

    //
    // Render each sprite batch
    //
    for (const auto& spriteBatchIt : spriteBatches)
    {
        RenderBatch(*spriteMesh, spriteBatchIt.second, *pipeline, commandBuffer);
    }
}

std::unordered_map<TextureId, SpriteRenderer::SpriteBatch> SpriteRenderer::CompileSpriteBatches(const std::string& sceneName) const
{
    std::unordered_map<TextureId, SpriteBatch> spriteBatches;

    for (const auto& sprite : m_renderables->GetSprites().GetData())
    {
        // Skip over invalid (deleted) sprites, don't render them
        if (!sprite.isValid) { continue; }

        // Skip over objects in a different scene
        if (sprite.renderable.sceneName != sceneName) { continue; }

        // Otherwise, add the sprite to the corresponding per-texture batch
        const auto it = spriteBatches.find(sprite.renderable.textureId);
        if (it == spriteBatches.cend())
        {
            SpriteBatch spriteBatch{};
            spriteBatch.textureId = sprite.renderable.textureId;
            spriteBatch.spriteIds.push_back(sprite.renderable.spriteId);

            spriteBatches.insert({sprite.renderable.textureId, spriteBatch});
        }
        else
        {
            it->second.spriteIds.push_back(sprite.renderable.spriteId);
        }
    }

    return spriteBatches;
}

void SpriteRenderer::RenderBatch(const LoadedMesh& spriteMesh,
                                 const SpriteBatch& spriteBatch,
                                 const VulkanPipelinePtr& pipeline,
                                 const VulkanCommandBufferPtr& commandBuffer)
{
    CmdBufferSectionLabel sectionLabel(
        m_vulkanObjs->GetCalls(),
        commandBuffer,
        std::format("SpriteRenderBatch-{}", spriteBatch.textureId.id)
    );

    //
    // Fetch the texture this batch uses
    //
    auto loadedTexture = m_textures->GetTextureAndImage(spriteBatch.textureId);
    if (!loadedTexture)
    {
        m_logger->Log(Common::LogLevel::Error,
          "SpriteRenderer: RenderBatch: No such texture exists: {}", spriteBatch.textureId.id);

        m_logger->Log(Common::LogLevel::Warning, "SpriteRenderer: RenderBatch: Falling back to missing texture");
        loadedTexture = m_textures->GetMissingTexture();
    }

    if (!loadedTexture)
    {
        m_logger->Log(Common::LogLevel::Error, "SpriteRenderer: RenderBatch: Failed to find texture to use");
        return;
    }

    //
    // Update material descriptor set
    //
    const auto materialDescriptorSet = UpdateMaterialDescriptorSet(*loadedTexture);
    if (!materialDescriptorSet) { return; }

    //
    // Update draw descriptor set
    //
    const auto drawDescriptorSet = UpdateDrawDescriptorSet(spriteBatch);
    if (!drawDescriptorSet) { return; }

    //
    // Render
    //
    commandBuffer->CmdBindDescriptorSets(pipeline, 2, {(*materialDescriptorSet)->GetVkDescriptorSet()});
    commandBuffer->CmdBindDescriptorSets(pipeline, 3, {(*drawDescriptorSet)->GetVkDescriptorSet()});
    commandBuffer->CmdDrawIndexed(
        spriteMesh.numIndices,
        spriteBatch.spriteIds.size(),
        0,
        0,
        0
    );
}

std::optional<VulkanDescriptorSetPtr> SpriteRenderer::UpdateGlobalDescriptorSet(const RenderParams& renderParams)
{
    const auto globalDataDescriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        m_programDef->GetDescriptorSetLayouts()[0],
        std::format("SpriteRenderer-GlobalData-{}", m_frameIndex)
    );
    if (!globalDataDescriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error,
                      "SpriteRenderer: Render: Failed to get or create global data descriptor set");
        return std::nullopt;
    }

    if (!UpdateGlobalDescriptorSet_Global(*globalDataDescriptorSet)) { return std::nullopt; }
    if (!UpdateGlobalDescriptorSet_ViewProjection(renderParams, *globalDataDescriptorSet)) { return std::nullopt; }

    return *globalDataDescriptorSet;
}

bool SpriteRenderer::UpdateGlobalDescriptorSet_Global(const VulkanDescriptorSetPtr& descriptorSet)
{
    //
    // Create a per-render CPU buffer for holding global data
    //
    const auto globalDataBuffer = CPUItemBuffer<GlobalPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        1,
        std::format("SpriteRenderer-GlobalData-{}", m_frameIndex)
    );
    if (!globalDataBuffer)
    {
        m_logger->Log(Common::LogLevel::Error, "SpriteRenderer: Failed to create global data buffer");
        return false;
    }

    //
    // Calculate global data
    //
    GlobalPayload globalPayload{};
    globalPayload.surfaceTransform = glm::mat4(1); // TODO ANDROID: PASS IN

    //
    // Update the global data buffer with the global data
    //
    (*globalDataBuffer)->PushBack(ExecutionContext::CPU(), {globalPayload});

    descriptorSet->WriteBufferBind(
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

bool SpriteRenderer::UpdateGlobalDescriptorSet_ViewProjection(const RenderParams& renderParams,
                                                              const VulkanDescriptorSetPtr& descriptorSet)
{
    //
    // Create a per-render CPU buffer for holding global data
    //
    const auto viewProjectionDataBuffer = CPUItemBuffer<ViewProjectionPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        1,
        std::format("SpriteRenderer-ViewProjectionData-{}", m_frameIndex)
    );
    if (!viewProjectionDataBuffer)
    {
        m_logger->Log(Common::LogLevel::Error,
          "SpriteRenderer::BindDescriptorSet0_ViewProjection: Failed to create view projection data buffer");
        return false;
    }

    //
    // Calculate global data
    //
    const auto camera = renderParams.spriteRenderCamera;
    const auto eye = camera.position;
    const auto center = camera.position - camera.lookUnit;
    const auto up = -camera.upUnit;
    const auto viewTransform = glm::lookAt(eye, center, up);

    ViewProjectionPayload viewProjectionPayload{};
    viewProjectionPayload.viewTransform = viewTransform;
    viewProjectionPayload.projectionTransform = glm::orthoLH_ZO(
        0.0f,(float)m_renderSettings.resolution.w,
        0.0f, (float)m_renderSettings.resolution.h, // Swapped to account for Vulkan flipped y-axis
        0.0f, 1.0f
    );

    //
    // Update the global data buffer with the global data
    //
    (*viewProjectionDataBuffer)->PushBack(ExecutionContext::CPU(), {viewProjectionPayload});

    descriptorSet->WriteBufferBind(
        m_programDef->GetBindingDetailsByName("u_viewProjectionData"),
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        (*viewProjectionDataBuffer)->GetBuffer()->GetVkBuffer(),
        0,
        0
    );

    //
    // Post-frame cleanup
    //
    m_postExecutionOps->Enqueue_Current(BufferDeleteOp(m_buffers, (*viewProjectionDataBuffer)->GetBuffer()->GetBufferId()));

    return true;
}

std::optional<VulkanDescriptorSetPtr> SpriteRenderer::UpdateRendererDescriptorSet()
{
    //
    // Retrieve the buffer containing the scene's sprite data
    //
    const auto spritePayloadBuffer = m_renderables->GetSprites().GetPayloadBuffer();

    //
    // Bind the scene's sprite data to the renderer descriptor set
    //
    const auto rendererDataDescriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        m_programDef->GetDescriptorSetLayouts()[1],
        std::format("SpriteRenderer-RendererData-{}", m_frameIndex)
    );
    if (!rendererDataDescriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error,
          "SpriteRenderer: Render: Failed to get or create renderer data descriptor set");
        return std::nullopt;
    }

    (*rendererDataDescriptorSet)->WriteBufferBind(
        m_programDef->GetBindingDetailsByName("i_spriteData"),
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        spritePayloadBuffer->GetBuffer()->GetVkBuffer(),
        0,
        0
    );

    return *rendererDataDescriptorSet;
}

std::optional<VulkanDescriptorSetPtr> SpriteRenderer::UpdateMaterialDescriptorSet(const std::pair<LoadedTexture, LoadedImage>& loadedTexture)
{
    const auto materialDescriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        m_programDef->GetDescriptorSetLayouts()[2],
        std::format("SpriteRenderer-MaterialData-{}-{}", m_frameIndex, loadedTexture.first.textureDefinition.texture.id.id)
    );
    if (!materialDescriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error, "SpriteRenderer: RenderBatch: Failed to get or create material descriptor set");
        return std::nullopt;
    }

    (*materialDescriptorSet)->WriteCombinedSamplerBind(
        m_programDef->GetBindingDetailsByName("i_spriteSampler"),
        loadedTexture.second.vkImageViews.at(TextureView::DEFAULT),
        loadedTexture.second.vkSamplers.at(TextureSampler::DEFAULT)
    );

    return *materialDescriptorSet;
}

std::optional<VulkanDescriptorSetPtr> SpriteRenderer::UpdateDrawDescriptorSet(const SpriteBatch& spriteBatch)
{
    //
    // Create a per-render CPU buffer to hold draw data
    //
    const auto drawDataBufferExpect = CPUItemBuffer<SpriteDrawPayload>::Create(
        m_buffers,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        spriteBatch.spriteIds.size(),
        std::format("SpriteRenderer-DrawData-{}", m_frameIndex)
    );
    if (!drawDataBufferExpect)
    {
        return std::nullopt;
    }
    const auto& drawDataBuffer = *drawDataBufferExpect;

    //
    // Convert the batch sprites to be rendered to DrawPayloads
    //
    std::vector<SpriteDrawPayload> drawPayloads(spriteBatch.spriteIds.size());

    std::ranges::transform(spriteBatch.spriteIds, drawPayloads.begin(), [](const SpriteId& id){
        SpriteDrawPayload drawPayload{};
        drawPayload.dataIndex = id.id - 1;
        return drawPayload;
    });

    drawDataBuffer->Resize(ExecutionContext::CPU(), drawPayloads.size());
    drawDataBuffer->Update(ExecutionContext::CPU(), 0, drawPayloads);

    //
    // Fetch the draw descriptor set and bind the draw data buffer to it
    //
    const auto drawDescriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        m_programDef->GetDescriptorSetLayouts()[3],
        std::format("SpriteRenderer-DrawData-{}-{}", m_frameIndex, spriteBatch.textureId.id)
    );
    if (!drawDescriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error, "SpriteRenderer: RenderBatch: Failed to get or create draw descriptor set");
        return std::nullopt;
    }

    (*drawDescriptorSet)->WriteBufferBind(
        m_programDef->GetBindingDetailsByName("i_drawData"),
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        drawDataBuffer->GetBuffer()->GetVkBuffer(),
        0,
        0
    );

    //
    // Cleanup
    //
    m_postExecutionOps->Enqueue_Current(BufferDeleteOp(m_buffers, drawDataBuffer->GetBuffer()->GetBufferId()));

    return *drawDescriptorSet;
}

}
