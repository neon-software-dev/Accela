#ifndef LIBACCELARENDERERVK_SRC_RENDERABLES_SPRITERENDERABLES_H
#define LIBACCELARENDERERVK_SRC_RENDERABLES_SPRITERENDERABLES_H

#include "RenderableData.h"

#include "../ForwardDeclares.h"

#include "../Renderer/RendererCommon.h"
#include "../Buffer/ItemBuffer.h"
#include "../Texture/LoadedTexture.h"

#include <Accela/Render/Ids.h>

#include <Accela/Render/Task/WorldUpdate.h>

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>

namespace Accela::Render
{
    class SpriteRenderables
    {
        public:

            SpriteRenderables(Common::ILogger::Ptr logger,
                              Ids::Ptr ids,
                              PostExecutionOpsPtr postExecutionOps,
                              IBuffersPtr buffers,
                              ITexturesPtr textures);

            bool Initialize();
            void Destroy();

            void ProcessUpdate(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence);

            [[nodiscard]] const std::vector<RenderableData<SpriteRenderable>>& GetData() const { return m_sprites; }
            [[nodiscard]] std::shared_ptr<ItemBuffer<SpritePayload>> GetPayloadBuffer() const { return m_payloadBuffer; };

        private:

            void ProcessAddedSprites(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence);
            void ProcessUpdatedSprites(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence);
            void ProcessDeletedSprites(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence);

            static SpritePayload SpriteToPayload(const SpriteRenderable& sprite, const LoadedTexture& spriteTexture);

        private:

            Common::ILogger::Ptr m_logger;
            Ids::Ptr m_ids;
            PostExecutionOpsPtr m_postExecutionOps;
            IBuffersPtr m_buffers;
            ITexturesPtr m_textures;

            // In-memory representation of the scene. Entries in this vector directly map to entries
            // in the GPU payload buffer.
            std::vector<RenderableData<SpriteRenderable>> m_sprites;

            // In-GPU representation of the scene
            std::shared_ptr<ItemBuffer<SpritePayload>> m_payloadBuffer;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERABLES_SPRITERENDERABLES_H
