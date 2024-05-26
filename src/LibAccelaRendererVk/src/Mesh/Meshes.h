#ifndef LIBACCELARENDERERVK_SRC_MESH_MESHES_H
#define LIBACCELARENDERERVK_SRC_MESH_MESHES_H

#include "IMeshes.h"

#include "../ForwardDeclares.h"

#include "../Util/ExecutionContext.h"
#include "../Util/AABB.h"

#include <Accela/Render/Ids.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <glm/glm.hpp>

#include <expected>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <future>

namespace Accela::Render
{
    class Meshes : public IMeshes
    {
        public:

            Meshes(Common::ILogger::Ptr logger,
                   Common::IMetrics::Ptr metrics,
                   VulkanObjsPtr vulkanObjs,
                   Ids::Ptr ids,
                   PostExecutionOpsPtr postExecutionOps,
                   IBuffersPtr buffers);

            bool Initialize(VulkanCommandPoolPtr transferCommandPool,
                            VkQueue vkTransferQueue) override;
            void Destroy() override;

            [[nodiscard]] bool LoadMesh(const Mesh::Ptr& mesh, MeshUsage usage, std::promise<bool> resultPromise) override;
            [[nodiscard]] bool UpdateMesh(const Mesh::Ptr& mesh, std::promise<bool> resultPromise) override;
            [[nodiscard]] std::optional<LoadedMesh> GetLoadedMesh(MeshId meshId) const override;
            void DestroyMesh(MeshId meshId, bool destroyImmediately) override;

        private:

            // Note: No alignment due to vertex buffer usage
            struct StaticMeshVertexPayload
            {
                glm::vec3 position;
                glm::vec3 normal;
                glm::vec2 uv;
                glm::vec3 tangent;
            };

            // Note: No alignment due to vertex buffer usage
            struct BoneMeshVertexPayload
            {
                glm::vec3 position;
                glm::vec3 normal;
                glm::vec2 uv;
                glm::vec3 tangent;
                glm::ivec4 bones{-1};
                glm::vec4 boneWeights{0.0f};
            };

            struct BoneMeshDataPayload
            {
                alignas(4) uint32_t numMeshBones{0};
            };

            struct ImmutableMeshBuffers
            {
                DataBufferPtr vertexBuffer;
                DataBufferPtr indexBuffer;
                std::optional<DataBufferPtr> dataBuffer;
            };

        private:

            [[nodiscard]] bool LoadCPUMesh(const Mesh::Ptr& mesh);
            [[nodiscard]] bool LoadGPUMesh(const Mesh::Ptr& mesh, std::promise<bool> resultPromise);
            [[nodiscard]] bool LoadImmutableMesh(const Mesh::Ptr& mesh, std::promise<bool> resultPromise);

            [[nodiscard]] static std::vector<unsigned char> GetVerticesPayload(const Mesh::Ptr& mesh);
            [[nodiscard]] static std::size_t GetVerticesCount(const Mesh::Ptr& mesh);
            [[nodiscard]] static std::vector<unsigned char> GetIndicesPayload(const Mesh::Ptr& mesh);
            [[nodiscard]] static std::size_t GetIndicesCount(const Mesh::Ptr& mesh);
            [[nodiscard]] static std::optional<std::vector<unsigned char>> GetDataPayload(const Mesh::Ptr& mesh);

            [[nodiscard]] std::expected<ImmutableMeshBuffers, bool> EnsureImmutableBuffers(const MeshType& meshType);

            [[nodiscard]] bool TransferCPUMeshData(const LoadedMesh& loadedMesh, const Mesh::Ptr& newMeshData);
            [[nodiscard]] bool TransferGPUMeshData(const LoadedMesh& loadedMesh,
                                                   const Mesh::Ptr& newMeshData,
                                                   bool initialDataTransfer,
                                                   std::promise<bool> resultPromise);
            [[nodiscard]] bool TransferMeshData(const ExecutionContext& executionContext, const LoadedMesh& loadedMesh, const Mesh::Ptr& newMeshData);

            [[nodiscard]] bool OnMeshTransferFinished(bool transfersSuccessful, const LoadedMesh& loadedMesh, bool initialDataTransfer);

            void DestroyMeshObjects(const LoadedMesh& loadedMesh);

            void SyncMetrics();

            [[nodiscard]] static AABB CalculateRenderBoundingBox(const Mesh::Ptr& mesh);

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            VulkanObjsPtr m_vulkanObjs;
            Ids::Ptr m_ids;
            PostExecutionOpsPtr m_postExecutionOps;
            IBuffersPtr m_buffers;

            VulkanCommandPoolPtr m_transferCommandPool;
            VkQueue m_vkTransferQueue{VK_NULL_HANDLE};

            std::unordered_map<MeshId, LoadedMesh> m_meshes;
            std::unordered_set<MeshId> m_meshesLoading;
            std::unordered_set<MeshId> m_meshesToDestroy;

            std::unordered_map<MeshType, DataBufferPtr> m_immutableMeshVertexBuffers;
            std::unordered_map<MeshType, DataBufferPtr> m_immutableMeshIndexBuffers;
            std::unordered_map<MeshType, DataBufferPtr> m_immutableMeshDataBuffers;
    };
}

#endif //LIBACCELARENDERERVK_SRC_MESH_MESHES_H
