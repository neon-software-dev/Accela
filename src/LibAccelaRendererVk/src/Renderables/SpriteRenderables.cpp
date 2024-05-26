#include "SpriteRenderables.h"

#include "../Buffer/IBuffers.h"
#include "../Buffer/GPUItemBuffer.h"

#include "../Texture/ITextures.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cassert>

namespace Accela::Render
{

SpriteRenderables::SpriteRenderables(
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

bool SpriteRenderables::Initialize()
{
    assert(m_payloadBuffer == nullptr);

    const auto dataBuffer = GPUItemBuffer<SpritePayload>::Create(
        m_buffers,
        m_postExecutionOps,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        16,
        "SceneSprites-Data"
    );
    if (!dataBuffer)
    {
        m_logger->Log(Common::LogLevel::Fatal, "SpriteRenderables: Failed to create payload buffer");
        return false;
    }

    m_payloadBuffer = *dataBuffer;

    return true;
}

void SpriteRenderables::Destroy()
{
    if (m_payloadBuffer != nullptr)
    {
        m_buffers->DestroyBuffer(m_payloadBuffer->GetBuffer()->GetBufferId());
    }
}

void SpriteRenderables::ProcessUpdate(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence)
{
    ProcessAddedSprites(update, commandBuffer, vkFence);
    ProcessUpdatedSprites(update, commandBuffer, vkFence);
    ProcessDeletedSprites(update, commandBuffer, vkFence);
}

void SpriteRenderables::ProcessAddedSprites(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence)
{
    if (update.toAddSpriteRenderables.empty()) { return; }

    //
    // Transform the sprites to sprite payloads
    //
    std::vector<ItemUpdate<SpritePayload>> updates;
    updates.reserve(update.toAddSpriteRenderables.size());

    IdType highestId{0};

    for (const auto& sprite : update.toAddSpriteRenderables)
    {
        const auto texture = m_textures->GetTexture(sprite.textureId);
        if (!texture)
        {
            m_logger->Log(Common::LogLevel::Error,
              "ProcessAddedSprites: No such texture exists: {}", sprite.textureId.id);
            continue;
        }

        if (sprite.spriteId.id == INVALID_ID)
        {
            m_logger->Log(Common::LogLevel::Warning, "ProcessAddedSprites: A sprite has an invalid id, ignoring");
            continue;
        }

        updates.emplace_back(SpriteToPayload(sprite, *texture), sprite.spriteId.id - 1);

        highestId = std::max(highestId, sprite.spriteId.id);
    }

    //
    // Update the GPU data buffer
    //
    const auto executionContext = ExecutionContext::GPU(commandBuffer, vkFence);

    if (m_payloadBuffer->GetSize() < highestId)
    {
        m_payloadBuffer->Resize(executionContext, highestId);
    }

    if (!m_payloadBuffer->Update(executionContext, updates))
    {
        m_logger->Log(Common::LogLevel::Error, "ProcessAddedSprites: Failed to update payload buffer");
        return;
    }

    //
    // Update the CPU data buffer
    //
    if (m_sprites.size() < highestId)
    {
        m_sprites.resize(highestId);
    }

    for (const auto& sprite : update.toAddSpriteRenderables)
    {
        RenderableData<SpriteRenderable> spriteData{};
        spriteData.isValid = true;
        spriteData.renderable = sprite;

        m_sprites[sprite.spriteId.id - 1] = spriteData;
    }
}

void SpriteRenderables::ProcessUpdatedSprites(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence)
{
    if (update.toUpdateSpriteRenderables.empty()) { return; }

    std::vector<ItemUpdate<SpritePayload>> updates;
    updates.reserve(update.toUpdateSpriteRenderables.size());

    for (const auto& sprite : update.toUpdateSpriteRenderables)
    {
        const auto texture = m_textures->GetTexture(sprite.textureId);
        if (!texture)
        {
            m_logger->Log(Common::LogLevel::Error,
              "ProcessUpdatedSprites: No such texture exists: {}", sprite.textureId.id);
            continue;
        }

        if (sprite.spriteId.id == INVALID_ID)
        {
            m_logger->Log(Common::LogLevel::Warning, "ProcessUpdatedSprites: A sprite has an invalid id, ignoring");
            continue;
        }

        if (sprite.spriteId.id > m_sprites.size())
        {
            m_logger->Log(Common::LogLevel::Error,
              "ProcessUpdatedSprites: No such sprite with id {} exists", sprite.spriteId.id);
            continue;
        }

        updates.emplace_back(SpriteToPayload(sprite, *texture), sprite.spriteId.id - 1);
    }

    const auto executionContext = ExecutionContext::GPU(commandBuffer, vkFence);

    //
    // Update the GPU data buffer
    //
    if (!m_payloadBuffer->Update(executionContext, updates))
    {
        m_logger->Log(Common::LogLevel::Error, "ProcessUpdatedSprites: Failed to update payload buffer");
        return;
    }

    //
    // Update the CPU data buffer
    //
    for (const auto& sprite : update.toUpdateSpriteRenderables)
    {
        RenderableData<SpriteRenderable> spriteData{};
        spriteData.isValid = true;
        spriteData.renderable = sprite;

        m_sprites[sprite.spriteId.id - 1] = spriteData;
    }
}

void SpriteRenderables::ProcessDeletedSprites(const WorldUpdate& update, const VulkanCommandBufferPtr&, VkFence)
{
    if (update.toDeleteSpriteIds.empty()) { return; }

    for (const auto& toDeleteId : update.toDeleteSpriteIds)
    {
        if (toDeleteId.id == INVALID_ID)
        {
            m_logger->Log(Common::LogLevel::Warning, "ProcessDeletedSprites: A sprite has an invalid id, ignoring");
            continue;
        }

        if (toDeleteId.id > m_sprites.size())
        {
            m_logger->Log(Common::LogLevel::Error,
              "ProcessDeletedSprites: No such sprite with id {} exists", toDeleteId.id);
            continue;
        }

        m_sprites[toDeleteId.id - 1].isValid = false;
        m_ids->spriteIds.ReturnId(toDeleteId);
    }
}

SpritePayload SpriteRenderables::SpriteToPayload(const SpriteRenderable& sprite, const LoadedTexture& spriteTexture)
{
    auto sourceRect = URect(spriteTexture.pixelSize);
    if (sprite.srcPixelRect.has_value()) { sourceRect = sprite.srcPixelRect.value(); }

    FSize destSize = {(float)sourceRect.w, (float)sourceRect.h};
    if (sprite.dstSize.has_value()) { destSize = *sprite.dstSize; }

    const float selectPercentX      = (float)sourceRect.x / (float)spriteTexture.pixelSize.w;
    const float selectPercentY      = (float)sourceRect.y / (float)spriteTexture.pixelSize.h;
    const float selectPercentWidth  = (float)sourceRect.w / (float)spriteTexture.pixelSize.w;
    const float selectPercentHeight = (float)sourceRect.h / (float)spriteTexture.pixelSize.h;

    const glm::mat4 translation = glm::translate(glm::mat4(1), sprite.position);

    const glm::mat4 rotation = glm::mat4_cast(sprite.orientation);

    // Scale the sprite by its destination size to make it the correct pixel size on the screen
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(destSize.w, destSize.h, 0));

    // Additionally, scale by a general scaling factor as needed
    scale = glm::scale(scale, sprite.scale);

    SpritePayload payload{};
    payload.modelTransform = translation * rotation * scale;
    payload.uvTranslation = glm::vec2(selectPercentX, selectPercentY);
    payload.uvSize = glm::vec2(selectPercentWidth, selectPercentHeight);

    return payload;
}

}
