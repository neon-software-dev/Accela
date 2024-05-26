#ifndef LIBACCELARENDERERVK_SRC_RENDERABLES_OBJECTRENDERABLES_H
#define LIBACCELARENDERERVK_SRC_RENDERABLES_OBJECTRENDERABLES_H

#include "RenderableData.h"

#include "../ForwardDeclares.h"

#include "../Renderer/RendererCommon.h"
#include "../Buffer/ItemBuffer.h"
#include "../Util/KDTree.h"
#include "../Util/RTree.h"

#include <Accela/Render/Ids.h>

#include <Accela/Render/Task/WorldUpdate.h>

#include <Accela/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <expected>
#include <memory>
#include <unordered_map>

namespace Accela::Render
{
    using ObjectsRTree = RTree<ObjectId, float, 3>;

    class ObjectRenderables
    {
        public:

            ObjectRenderables(Common::ILogger::Ptr logger,
                              Ids::Ptr ids,
                              PostExecutionOpsPtr postExecutionOps,
                              IBuffersPtr buffers,
                              ITexturesPtr textures,
                              IMeshesPtr meshes,
                              ILightsPtr lights);

            bool Initialize();
            void Destroy();

            void ProcessUpdate(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence);

            [[nodiscard]] const std::vector<RenderableData<ObjectRenderable>>& GetData() const { return m_objects; }
            [[nodiscard]] const ObjectsRTree& GetDataRTree(const std::string& sceneName) const noexcept;
            [[nodiscard]] std::shared_ptr<ItemBuffer<ObjectPayload>> GetObjectPayloadBuffer() const { return m_objectPayloadBuffer; };

            [[nodiscard]] std::vector<ObjectRenderable> GetVisibleObjects(const std::string& sceneName, const Volume& volume) const;

        private:

            struct ModifiedWorldAreas
            {
                // The world-space bounding boxes of ObjectRenderables which were added, updated, or deleted
                std::vector<AABB> boundingBoxes_worldSpace;
            };

        private:

            void ProcessAddedObjects(const WorldUpdate& update, ModifiedWorldAreas& modifiedWorldAreas, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence);
            void ProcessUpdatedObjects(const WorldUpdate& update, ModifiedWorldAreas& modifiedWorldAreas, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence);
            void ProcessDeletedObjects(const WorldUpdate& update, ModifiedWorldAreas& modifiedWorldAreas, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence);

            static ObjectPayload ObjectToPayload(const ObjectRenderable& object);
            [[nodiscard]] std::expected<AABB, bool> GetObjectAABB(const ObjectRenderable& object) const;

        private:

            Common::ILogger::Ptr m_logger;
            Ids::Ptr m_ids;
            PostExecutionOpsPtr m_postExecutionOps;
            IBuffersPtr m_buffers;
            ITexturesPtr m_textures;
            IMeshesPtr m_meshes;
            ILightsPtr m_lights;

            // In-memory representation of the scene. Entries in this vector directly map to entries
            // in the GPU payload buffer.
            std::vector<RenderableData<ObjectRenderable>> m_objects;
            std::unordered_map<std::string, ObjectsRTree> m_objectsRTree;

            // In-GPU representation of the scene's objects
            std::shared_ptr<ItemBuffer<ObjectPayload>> m_objectPayloadBuffer;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERABLES_OBJECTRENDERABLES_H
