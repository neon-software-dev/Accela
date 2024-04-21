/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#include "TerrainRenderables.h"

#include "../Buffer/IBuffers.h"
#include "../Buffer/GPUItemBuffer.h"

namespace Accela::Render
{


TerrainRenderables::TerrainRenderables(
    Common::ILogger::Ptr logger,
    Ids::Ptr ids,
    PostExecutionOpsPtr postExecutionOps,
    IBuffersPtr buffers,
    ITexturesPtr textures)
    : m_logger(std::move(logger))
    , m_ids(std::move(ids))
    , m_postExecutionOps(std::move(postExecutionOps))
    , m_buffers(std::move(buffers))
    , m_textures(std::move(textures))
{

}

bool TerrainRenderables::Initialize()
{
    assert(m_terrainPayloadBuffer == nullptr);

    const auto dataBuffer = GPUItemBuffer<TerrainPayload>::Create(
        m_buffers,
        m_postExecutionOps,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        16,
        "SceneTerrain-Data"
    );
    if (!dataBuffer)
    {
        m_logger->Log(Common::LogLevel::Fatal, "TerrainRenderables: Failed to create payload buffer");
        return false;
    }

    m_terrainPayloadBuffer = *dataBuffer;

    return true;
}

void TerrainRenderables::Destroy()
{
    if (m_terrainPayloadBuffer != nullptr)
    {
        m_buffers->DestroyBuffer(m_terrainPayloadBuffer->GetBuffer()->GetBufferId());
    }
}

void TerrainRenderables::ProcessUpdate(const WorldUpdate& update,
                                       const VulkanCommandBufferPtr& commandBuffer,
                                       VkFence vkFence)
{
    ProcessAddedTerrain(update, commandBuffer, vkFence);
    ProcessUpdatedTerrain(update, commandBuffer, vkFence);
    ProcessDeletedTerrain(update, commandBuffer, vkFence);
}

void TerrainRenderables::ProcessAddedTerrain(const WorldUpdate& update,
                                             const VulkanCommandBufferPtr& commandBuffer,
                                             VkFence vkFence)
{
    if (update.toAddTerrainRenderables.empty()) { return; }

    //
    // Transform the terrain to terrain payloads
    //
    std::vector<ItemUpdate<TerrainPayload>> updates;
    updates.reserve(update.toAddTerrainRenderables.size());

    IdType highestId{0};

    for (const auto& terrain : update.toAddTerrainRenderables)
    {
        if (terrain.terrainId.id == INVALID_ID)
        {
            m_logger->Log(Common::LogLevel::Warning, "ProcessAddedTerrain: A terrain has an invalid id, ignoring");
            continue;
        }

        updates.emplace_back(TerrainToPayload(terrain), terrain.terrainId.id - 1);

        highestId = std::max(highestId, terrain.terrainId.id);
    }

    //
    // Update the GPU data buffer
    //
    const auto executionContext = ExecutionContext::GPU(commandBuffer, vkFence);

    if (m_terrainPayloadBuffer->GetSize() < highestId)
    {
        m_terrainPayloadBuffer->Resize(executionContext, highestId);
    }

    if (!m_terrainPayloadBuffer->Update(executionContext, updates))
    {
        m_logger->Log(Common::LogLevel::Error, "ProcessAddedTerrain: Failed to update payload buffer");
        return;
    }

    //
    // Update the CPU data buffer
    //
    if (m_terrain.size() < highestId)
    {
        m_terrain.resize(highestId);
    }

    for (const auto& terrain : update.toAddTerrainRenderables)
    {
        RenderableData<TerrainRenderable> terrainData{};
        terrainData.isValid = true;
        terrainData.renderable = terrain;

        m_terrain[terrain.terrainId.id - 1] = terrainData;
    }
}

void TerrainRenderables::ProcessUpdatedTerrain(const WorldUpdate& update,
                                               const VulkanCommandBufferPtr& commandBuffer,
                                               VkFence vkFence)
{
    if (update.toUpdateTerrainRenderables.empty()) { return; }

    std::vector<ItemUpdate<TerrainPayload>> updates;
    updates.reserve(update.toUpdateTerrainRenderables.size());

    for (const auto& terrain : update.toUpdateTerrainRenderables)
    {
        if (terrain.terrainId.id == INVALID_ID)
        {
            m_logger->Log(Common::LogLevel::Warning, "ProcessUpdatedTerrain: A terrain has an invalid id, ignoring");
            continue;
        }

        if (terrain.terrainId.id > m_terrain.size())
        {
            m_logger->Log(Common::LogLevel::Error,
              "ProcessUpdatedTerrain: No such terrain with id {} exists", terrain.terrainId.id);
            continue;
        }

        updates.emplace_back(TerrainToPayload(terrain), terrain.terrainId.id - 1);
    }

    const auto executionContext = ExecutionContext::GPU(commandBuffer, vkFence);

    //
    // Update the GPU data buffer
    //
    if (!m_terrainPayloadBuffer->Update(executionContext, updates))
    {
        m_logger->Log(Common::LogLevel::Error, "ProcessUpdatedTerrain: Failed to update payload buffer");
        return;
    }

    //
    // Update the CPU data buffer
    //
    for (const auto& terrain : update.toUpdateTerrainRenderables)
    {
        RenderableData<TerrainRenderable> terrainData{};
        terrainData.isValid = true;
        terrainData.renderable = terrain;

        m_terrain[terrain.terrainId.id - 1] = terrainData;
    }
}

void TerrainRenderables::ProcessDeletedTerrain(const WorldUpdate& update,
                                               const VulkanCommandBufferPtr&,
                                               VkFence)
{
    if (update.toDeleteTerrainIds.empty()) { return; }

    for (const auto& toDeleteId : update.toDeleteTerrainIds)
    {
        if (toDeleteId.id == INVALID_ID)
        {
            m_logger->Log(Common::LogLevel::Warning, "ProcessDeletedTerrain: A terrain has an invalid id, ignoring");
            continue;
        }

        if (toDeleteId.id > m_terrain.size())
        {
            m_logger->Log(Common::LogLevel::Error,
              "ProcessDeletedTerrain: No such terrain with id {} exists", toDeleteId.id);
            continue;
        }

        m_terrain[toDeleteId.id - 1].isValid = false;
        m_ids->terrainIds.ReturnId(toDeleteId);
    }
}

TerrainPayload TerrainRenderables::TerrainToPayload(const TerrainRenderable& terrain)
{
    const glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(terrain.size.w, 1.0f, terrain.size.h));

    TerrainPayload payload{};
    payload.modelTransform = terrain.modelTransform * scale;
    payload.tesselationLevel = terrain.tesselationLevel;
    payload.displacementFactor = terrain.displacementFactor;

    return payload;
}

}
