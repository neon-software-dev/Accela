#ifndef LIBACCELARENDERERVK_SRC_RENDERABLES_TERRAINRENDERABLES_H
#define LIBACCELARENDERERVK_SRC_RENDERABLES_TERRAINRENDERABLES_H

#include "RenderableData.h"

#include "../ForwardDeclares.h"

#include "../Renderer/RendererCommon.h"
#include "../Buffer/ItemBuffer.h"

#include <Accela/Render/Ids.h>
#include <Accela/Render/Task/WorldUpdate.h>

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

namespace Accela::Render
{
    class TerrainRenderables
    {
        public:

            TerrainRenderables(Common::ILogger::Ptr logger,
                               Ids::Ptr ids,
                               PostExecutionOpsPtr postExecutionOps,
                               IBuffersPtr buffers,
                               ITexturesPtr textures);

            bool Initialize();
            void Destroy();

            void ProcessUpdate(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence);

            [[nodiscard]] const std::vector<RenderableData<TerrainRenderable>>& GetData() const { return m_terrain; }
            [[nodiscard]] std::shared_ptr<ItemBuffer<TerrainPayload>> GetTerrainPayloadBuffer() const { return m_terrainPayloadBuffer; };

        private:

            void ProcessAddedTerrain(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence);
            void ProcessUpdatedTerrain(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence);
            void ProcessDeletedTerrain(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence);

            static TerrainPayload TerrainToPayload(const TerrainRenderable& terrain);

        private:

            Common::ILogger::Ptr m_logger;
            Ids::Ptr m_ids;
            PostExecutionOpsPtr m_postExecutionOps;
            IBuffersPtr m_buffers;
            ITexturesPtr m_textures;

            // In-memory representation of the scene. Entries in this vector directly map to entries
            // in the GPU payload buffer.
            std::vector<RenderableData<TerrainRenderable>> m_terrain;

            // In-GPU representation of the scene's terrain
            std::shared_ptr<ItemBuffer<TerrainPayload>> m_terrainPayloadBuffer;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERABLES_TERRAINRENDERABLES_H
