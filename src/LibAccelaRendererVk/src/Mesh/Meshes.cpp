#include "Meshes.h"

#include "../PostExecutionOp.h"
#include "../Metrics.h"

#include "../Buffer/IBuffers.h"
#include "../Buffer/CPUDataBuffer.h"
#include "../Buffer/GPUDataBuffer.h"

#include "../Util/VulkanFuncs.h"
#include "../Util/Futures.h"

#include <Accela/Render/Mesh/StaticMesh.h>
#include <Accela/Render/Mesh/BoneMesh.h>

#include <format>
#include <cstring>
#include <algorithm>

namespace Accela::Render
{

Meshes::Meshes(Common::ILogger::Ptr logger,
               Common::IMetrics::Ptr metrics,
               VulkanObjsPtr vulkanObjs,
               Ids::Ptr ids,
               PostExecutionOpsPtr postExecutionOps,
               IBuffersPtr buffers)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_vulkanObjs(std::move(vulkanObjs))
    , m_ids(std::move(ids))
    , m_postExecutionOps(std::move(postExecutionOps))
    , m_buffers(std::move(buffers))
{

}

bool Meshes::Initialize(VulkanCommandPoolPtr transferCommandPool,
                        VkQueue vkTransferQueue)
{
    m_logger->Log(Common::LogLevel::Info, "Meshes: Initializing");

    m_transferCommandPool = transferCommandPool;
    m_vkTransferQueue = vkTransferQueue;

    return true;
}

void Meshes::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "Meshes: Destroying");

    //
    // Destroy each mesh individually
    //
    m_logger->Log(Common::LogLevel::Info, "Meshes: Destroying meshes");

    while (!m_meshes.empty())
    {
        DestroyMesh(m_meshes.cbegin()->first, true);
    }

    //
    // Destroy buffers holding immutable meshes
    //
    m_logger->Log(Common::LogLevel::Info, "Meshes: Destroying immutable buffers");

    for (const auto& it : m_immutableMeshVertexBuffers)
    {
        m_buffers->DestroyBuffer(it.second->GetBuffer()->GetBufferId());
    }
    m_immutableMeshVertexBuffers.clear();

    for (const auto& it : m_immutableMeshIndexBuffers)
    {
        m_buffers->DestroyBuffer(it.second->GetBuffer()->GetBufferId());
    }
    m_immutableMeshIndexBuffers.clear();

    for (const auto& it : m_immutableMeshDataBuffers)
    {
        m_buffers->DestroyBuffer(it.second->GetBuffer()->GetBufferId());
    }
    m_immutableMeshDataBuffers.clear();

    m_meshesLoading.clear();
    m_meshesToDestroy.clear();

    SyncMetrics();
}

bool Meshes::LoadMesh(const Mesh::Ptr& mesh, MeshUsage usage, std::promise<bool> resultPromise)
{
    if (m_meshes.contains(mesh->id))
    {
        m_logger->Log(Common::LogLevel::Error, "Meshes: LoadMesh: Mesh with id {} already exists", mesh->id.id);
        return ErrorResult(resultPromise);
    }

    switch (usage)
    {
        case MeshUsage::Dynamic:    return PromiseResult(LoadCPUMesh(mesh), resultPromise);
        case MeshUsage::Static:     return LoadGPUMesh(mesh, std::move(resultPromise));
        case MeshUsage::Immutable:  return LoadImmutableMesh(mesh, std::move(resultPromise));
    }

    return false;
}

bool Meshes::LoadCPUMesh(const Mesh::Ptr& mesh)
{
    m_logger->Log(Common::LogLevel::Info, "Meshes: Loading CPU mesh {}", mesh->id.id);

    //
    // Create buffers to hold the mesh's vertices, indices, and optional data
    //
    const auto verticesPayload = GetVerticesPayload(mesh);

    const auto verticesBuffer = CPUDataBuffer::Create(
        m_buffers,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        verticesPayload.size(),
        "CPUMeshVertices-" + mesh->tag
    );
    if (!verticesBuffer)
    {
        m_logger->Log(Common::LogLevel::Error, "Meshes: Failed to create vertices buffer for mesh {}", mesh->id.id);
        return false;
    }

    const auto indicesPayload = GetIndicesPayload(mesh);

    const auto indicesBuffer = CPUDataBuffer::Create(
        m_buffers,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        indicesPayload.size(),
        "CPUMeshIndices-" + mesh->tag
    );
    if (!indicesBuffer)
    {
        m_logger->Log(Common::LogLevel::Error, "Meshes: Failed to create indices buffer for mesh {}", mesh->id.id);
        m_buffers->DestroyBuffer((*verticesBuffer)->GetBuffer()->GetBufferId());
        return false;
    }

    const auto dataPayload = GetDataPayload(mesh);

    std::optional<DataBufferPtr> dataBuffer = std::nullopt;
    std::size_t dataByteSize = 0;

    if (dataPayload)
    {
        const auto dataBufferExpect = CPUDataBuffer::Create(
            m_buffers,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            dataPayload->size(),
            "CPUMeshData-" + mesh->tag
        );
        if (!dataBufferExpect)
        {
            m_logger->Log(Common::LogLevel::Error, "Meshes: Failed to create data buffer for mesh {}", mesh->id.id);
            m_buffers->DestroyBuffer((*verticesBuffer)->GetBuffer()->GetBufferId());
            m_buffers->DestroyBuffer((*indicesBuffer)->GetBuffer()->GetBufferId());
            return false;
        }

        dataBuffer = *dataBufferExpect;
        dataByteSize = dataPayload->size();
    }

    //
    // Upload the mesh's data to the newly created buffers
    //
    LoadedMesh loadedMesh{};
    loadedMesh.id = mesh->id;
    loadedMesh.meshType = mesh->type;
    loadedMesh.usage = MeshUsage::Dynamic;
    loadedMesh.verticesBuffer = *verticesBuffer;
    loadedMesh.numVertices = GetVerticesCount(mesh);
    loadedMesh.verticesByteOffset = 0;
    loadedMesh.verticesOffset = 0;
    loadedMesh.verticesByteSize = verticesPayload.size();
    loadedMesh.indicesBuffer = *indicesBuffer;
    loadedMesh.numIndices = GetIndicesCount(mesh);
    loadedMesh.indicesByteOffset = 0;
    loadedMesh.indicesOffset = 0;
    loadedMesh.indicesByteSize = indicesPayload.size();
    loadedMesh.dataBuffer = dataBuffer;
    loadedMesh.dataByteOffset = 0;
    loadedMesh.dataByteSize = dataByteSize;
    loadedMesh.boundingBox_modelSpace = CalculateRenderBoundingBox(mesh);

    if (!TransferCPUMeshData(loadedMesh, mesh))
    {
        m_logger->Log(Common::LogLevel::Error, "Meshes: Failed to upload mesh data to CPU for mesh {}", mesh->id.id);
        m_buffers->DestroyBuffer((*verticesBuffer)->GetBuffer()->GetBufferId());
        m_buffers->DestroyBuffer((*indicesBuffer)->GetBuffer()->GetBufferId());
        return false;
    }

    //
    // Record results
    //
    m_meshes.insert({mesh->id, loadedMesh});

    SyncMetrics();

    return true;
}

bool Meshes::LoadGPUMesh(const Mesh::Ptr& mesh, std::promise<bool> resultPromise)
{
    m_logger->Log(Common::LogLevel::Info, "Meshes: Loading GPU mesh {}", mesh->id.id);

    //
    // Create buffers to hold the mesh's vertices, indices, and optional data buffer
    //
    const auto verticesPayload = GetVerticesPayload(mesh);

    const auto verticesBuffer = GPUDataBuffer::Create(
        m_buffers,
        m_postExecutionOps,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        verticesPayload.size(),
        std::format("GPUMeshVertices-{}-{}", mesh->id.id, mesh->tag)
    );
    if (!verticesBuffer)
    {
        m_logger->Log(Common::LogLevel::Error, "Meshes: Failed to create vertices buffer for mesh {}", mesh->id.id);
        return ErrorResult(resultPromise);
    }

    const auto indicesPayload = GetIndicesPayload(mesh);

    const auto indicesBuffer = GPUDataBuffer::Create(
        m_buffers,
        m_postExecutionOps,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        indicesPayload.size(),
        std::format("GPUMeshIndices-{}-{}", mesh->id.id, mesh->tag)
    );
    if (!indicesBuffer)
    {
        m_logger->Log(Common::LogLevel::Error, "Meshes: Failed to create indices buffer for mesh {}", mesh->id.id);
        m_buffers->DestroyBuffer((*verticesBuffer)->GetBuffer()->GetBufferId());
        return ErrorResult(resultPromise);
    }

    const auto dataPayload = GetDataPayload(mesh);

    std::optional<DataBufferPtr> dataBuffer = std::nullopt;
    std::size_t dataByteSize = 0;

    if (dataPayload)
    {
        const auto dataBufferExpect = GPUDataBuffer::Create(
            m_buffers,
            m_postExecutionOps,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            dataPayload->size(),
            std::format("GPUMeshData-{}-{}", mesh->id.id, mesh->tag)
        );
        if (!dataBufferExpect)
        {
            m_logger->Log(Common::LogLevel::Error, "Meshes: Failed to create data buffer for mesh {}", mesh->id.id);
            m_buffers->DestroyBuffer((*verticesBuffer)->GetBuffer()->GetBufferId());
            m_buffers->DestroyBuffer((*indicesBuffer)->GetBuffer()->GetBufferId());
            return ErrorResult(resultPromise);
        }

        dataBuffer = *dataBufferExpect;
        dataByteSize = dataPayload->size();
    }

    //
    // Record a record of the mesh and start a transfer of its data to the GPU
    //
    LoadedMesh loadedMesh{};
    loadedMesh.id = mesh->id;
    loadedMesh.meshType = mesh->type;
    loadedMesh.usage = MeshUsage::Static;
    loadedMesh.verticesBuffer = *verticesBuffer;
    loadedMesh.numVertices = GetVerticesCount(mesh);
    loadedMesh.verticesByteOffset = 0;
    loadedMesh.verticesOffset = 0;
    loadedMesh.verticesByteSize = verticesPayload.size();
    loadedMesh.indicesBuffer = *indicesBuffer;
    loadedMesh.numIndices = GetIndicesCount(mesh);
    loadedMesh.indicesByteOffset = 0;
    loadedMesh.indicesOffset = 0;
    loadedMesh.indicesByteSize = indicesPayload.size();
    loadedMesh.dataBuffer = dataBuffer;
    loadedMesh.dataByteOffset = 0;
    loadedMesh.dataByteSize = dataByteSize;
    loadedMesh.boundingBox_modelSpace = CalculateRenderBoundingBox(mesh);

    // Create a record of the mesh
    m_meshes.insert({mesh->id, loadedMesh});

    SyncMetrics();

    // Start the mesh data transfer
    return TransferGPUMeshData(loadedMesh, mesh, true, std::move(resultPromise));
}

bool Meshes::LoadImmutableMesh(const Mesh::Ptr& mesh, std::promise<bool> resultPromise)
{
    m_logger->Log(Common::LogLevel::Info, "Meshes: Loading immutable mesh {}", mesh->id.id);

    const auto verticesPayload = GetVerticesPayload(mesh);
    const auto indicesPayload = GetIndicesPayload(mesh);
    const auto dataPayload = GetDataPayload(mesh);

    //
    // Ensure immutable buffers exist for the mesh type
    //
    const auto meshBuffers = EnsureImmutableBuffers(mesh->type);
    if (!meshBuffers)
    {
        m_logger->Log(Common::LogLevel::Info,
          "Meshes: Failed to ensure immutable mesh buffers for mesh type {}", (unsigned int)mesh->type);
        return ErrorResult(resultPromise);
    }

    //
    // Record a record of the mesh and start a transfer of its data to the GPU
    //
    LoadedMesh loadedMesh{};
    loadedMesh.id = mesh->id;
    loadedMesh.meshType = mesh->type;
    loadedMesh.usage = MeshUsage::Immutable;

    loadedMesh.verticesBuffer = meshBuffers->vertexBuffer;
    loadedMesh.numVertices = GetVerticesCount(mesh);
    loadedMesh.verticesByteOffset = meshBuffers->vertexBuffer->GetDataByteSize();
    loadedMesh.verticesOffset = 0; // Set further below
    loadedMesh.verticesByteSize = verticesPayload.size();

    loadedMesh.indicesBuffer = meshBuffers->indexBuffer;
    loadedMesh.numIndices = GetIndicesCount(mesh);
    loadedMesh.indicesByteOffset = meshBuffers->indexBuffer->GetDataByteSize();
    loadedMesh.indicesOffset = meshBuffers->indexBuffer->GetDataByteSize() / sizeof(uint32_t);
    loadedMesh.indicesByteSize = indicesPayload.size();

    loadedMesh.dataBuffer = meshBuffers->dataBuffer;
    loadedMesh.dataByteOffset = 0; // Provided further below
    loadedMesh.dataByteSize = 0; // Provided further below

    loadedMesh.boundingBox_modelSpace = CalculateRenderBoundingBox(mesh);

    switch (mesh->type)
    {
        case MeshType::Static:
        {
            loadedMesh.verticesOffset = meshBuffers->vertexBuffer->GetDataByteSize() / sizeof(StaticMeshVertexPayload);
        }
        break;
        case MeshType::Bone:
        {
            loadedMesh.verticesOffset = meshBuffers->vertexBuffer->GetDataByteSize() / sizeof(BoneMeshVertexPayload);
        }
        break;
    }

    if (dataPayload)
    {
        loadedMesh.dataByteOffset = (*meshBuffers->dataBuffer)->GetDataByteSize();
        loadedMesh.dataByteSize = dataPayload->size();
    }

    // Create a record of the mesh
    m_meshes.insert({mesh->id, loadedMesh});

    SyncMetrics();

    VulkanFuncs vulkanFuncs(m_logger, m_vulkanObjs);

    // Submit the work to transfer the mesh data
    return vulkanFuncs.QueueSubmit<bool>(
        std::format("LoadImmutableMesh-{}", mesh->id.id),
        m_postExecutionOps,
        m_vkTransferQueue,
        m_transferCommandPool,
        [=,this](const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence) {
            const auto executionContext = ExecutionContext::GPU(commandBuffer, vkFence);

            // Mark the mesh as loading
            m_meshesLoading.insert(loadedMesh.id);

            bool allSuccessful = true;

            if (!meshBuffers->vertexBuffer->PushBack(executionContext, BufferAppend{.pData = verticesPayload.data(), .dataByteSize = verticesPayload.size()}))
            {
                m_logger->Log(Common::LogLevel::Error, "LoadImmutableMesh: Failed to push into vertex buffer");
                allSuccessful = false;
            }

            if (!meshBuffers->indexBuffer->PushBack(executionContext, BufferAppend{.pData = indicesPayload.data(), .dataByteSize = indicesPayload.size()}))
            {
                m_logger->Log(Common::LogLevel::Error, "LoadImmutableMesh: Failed to push into index buffer");
                allSuccessful = false;
            }

            if (dataPayload)
            {
                if (!(*meshBuffers->dataBuffer)->PushBack(executionContext, BufferAppend{.pData = dataPayload->data(), .dataByteSize = dataPayload->size()}))
                {
                    m_logger->Log(Common::LogLevel::Error, "LoadImmutableMesh: Failed to push into data buffer");
                    allSuccessful = false;
                }
            }

            return allSuccessful;
        },
        [=,this](bool commandsSuccessful)
        {
            return OnMeshTransferFinished(commandsSuccessful, loadedMesh, true);
        },
        std::move(resultPromise),
        EnqueueType::Frameless
    );
}

std::expected<Meshes::ImmutableMeshBuffers, bool> Meshes::EnsureImmutableBuffers(const MeshType& meshType)
{
    ImmutableMeshBuffers immutableMeshBuffers{};

    //
    // Vertex Buffer
    //
    const auto vertexBufferIt = m_immutableMeshVertexBuffers.find(meshType);
    if (vertexBufferIt == m_immutableMeshVertexBuffers.cend())
    {
        const auto verticesBufferExpect = GPUDataBuffer::Create(
            m_buffers,
            m_postExecutionOps,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            1024,
            std::format("GPUImmutableMeshVertices-{}", (unsigned int)meshType)
        );
        if (!verticesBufferExpect)
        {
            m_logger->Log(Common::LogLevel::Error,
              std::format("Meshes: Failed to create immutable vertices buffer for mesh type: {}", (unsigned int)meshType));
            return std::unexpected(false);
        }

        immutableMeshBuffers.vertexBuffer = *verticesBufferExpect;
        m_immutableMeshVertexBuffers[meshType] = *verticesBufferExpect;
    }
    else
    {
        immutableMeshBuffers.vertexBuffer = vertexBufferIt->second;
    }

    //
    // Index Buffer
    //
    const auto indexBufferIt = m_immutableMeshIndexBuffers.find(meshType);
    if (indexBufferIt == m_immutableMeshIndexBuffers.cend())
    {
        const auto indicesBufferExpect = GPUDataBuffer::Create(
            m_buffers,
            m_postExecutionOps,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            1024,
            std::format("GPUImmutableMeshIndices-{}", (unsigned int)meshType)
        );
        if (!indicesBufferExpect)
        {
            m_logger->Log(Common::LogLevel::Error, "Meshes: Failed to create immutable indices buffer for mesh type {}", (unsigned int)meshType);

            m_buffers->DestroyBuffer(immutableMeshBuffers.vertexBuffer->GetBuffer()->GetBufferId());
            m_immutableMeshVertexBuffers.erase(meshType);

            return std::unexpected(false);
        }

        immutableMeshBuffers.indexBuffer = *indicesBufferExpect;
        m_immutableMeshIndexBuffers[meshType] = *indicesBufferExpect;
    }
    else
    {
        immutableMeshBuffers.indexBuffer = indexBufferIt->second;
    }

    //
    // (Optional) Data Buffer
    //
    bool hasDataPayload = false;

    switch (meshType)
    {
        case MeshType::Static: hasDataPayload = false; break;
        case MeshType::Bone: hasDataPayload = true; break;
    }

    if (hasDataPayload)
    {
        const auto dataBufferIt = m_immutableMeshDataBuffers.find(meshType);
        if (dataBufferIt == m_immutableMeshDataBuffers.cend())
        {
            const auto dataBufferExpect = GPUDataBuffer::Create(
                m_buffers,
                m_postExecutionOps,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                1024,
                std::format("GPUImmutableMeshData-{}", (unsigned int)meshType)
            );
            if (!dataBufferExpect)
            {
                m_logger->Log(Common::LogLevel::Error,
                  "Meshes: Failed to create immutable data buffer for mesh type {}", (unsigned int)meshType);

                m_buffers->DestroyBuffer(immutableMeshBuffers.vertexBuffer->GetBuffer()->GetBufferId());
                m_immutableMeshVertexBuffers.erase(meshType);

                m_buffers->DestroyBuffer(immutableMeshBuffers.indexBuffer->GetBuffer()->GetBufferId());
                m_immutableMeshIndexBuffers.erase(meshType);

                return std::unexpected(false);
            }

            immutableMeshBuffers.dataBuffer = *dataBufferExpect;
            m_immutableMeshDataBuffers[meshType] = *dataBufferExpect;
        }
        else
        {
            immutableMeshBuffers.dataBuffer = dataBufferIt->second;
        }
    }

    return immutableMeshBuffers;
}

std::vector<unsigned char> Meshes::GetVerticesPayload(const Mesh::Ptr& mesh)
{
    std::vector<unsigned char> payloadBytes;

    switch (mesh->type)
    {
        case MeshType::Static:
        {
            auto staticMesh = std::dynamic_pointer_cast<StaticMesh>(mesh);

            std::vector<StaticMeshVertexPayload> verticesPayload(staticMesh->vertices.size());

            std::ranges::transform(staticMesh->vertices, verticesPayload.begin(), [](const MeshVertex& meshVertex){
                StaticMeshVertexPayload meshVertexPayload{};
                meshVertexPayload.position = meshVertex.position;
                meshVertexPayload.normal = meshVertex.normal;
                meshVertexPayload.uv = meshVertex.uv;
                meshVertexPayload.tangent = meshVertex.tangent;
                return meshVertexPayload;
            });

            payloadBytes.resize(verticesPayload.size() * sizeof(StaticMeshVertexPayload));
            memcpy(payloadBytes.data(), verticesPayload.data(), verticesPayload.size() * sizeof(StaticMeshVertexPayload));
        }
        break;
        case MeshType::Bone:
        {
            auto boneMesh = std::dynamic_pointer_cast<BoneMesh>(mesh);

            std::vector<BoneMeshVertexPayload> verticesPayload(boneMesh->vertices.size());

            std::ranges::transform(boneMesh->vertices, verticesPayload.begin(), [](const BoneMeshVertex& meshVertex){
                BoneMeshVertexPayload meshVertexPayload{};
                meshVertexPayload.position = meshVertex.position;
                meshVertexPayload.normal = meshVertex.normal;
                meshVertexPayload.uv = meshVertex.uv;
                meshVertexPayload.tangent = meshVertex.tangent;
                meshVertexPayload.bones = meshVertex.bones;
                meshVertexPayload.boneWeights = meshVertex.boneWeights;
                return meshVertexPayload;
            });

            payloadBytes.resize(verticesPayload.size() * sizeof(BoneMeshVertexPayload));
            memcpy(payloadBytes.data(), verticesPayload.data(), verticesPayload.size() * sizeof(BoneMeshVertexPayload));
        }
        break;
    }

    return payloadBytes;
}

std::size_t Meshes::GetVerticesCount(const Mesh::Ptr& mesh)
{
    switch (mesh->type)
    {
        case MeshType::Static: return std::dynamic_pointer_cast<StaticMesh>(mesh)->vertices.size();
        case MeshType::Bone: return std::dynamic_pointer_cast<BoneMesh>(mesh)->vertices.size();
    }

    assert(false);
    return 0;
}

std::vector<unsigned char> Meshes::GetIndicesPayload(const Mesh::Ptr& mesh)
{
    std::vector<unsigned char> payloadBytes;

    switch (mesh->type)
    {
        case MeshType::Static:
        {
            auto staticMesh = std::dynamic_pointer_cast<StaticMesh>(mesh);

            payloadBytes.resize(staticMesh->indices.size() * sizeof(uint32_t));
            memcpy(payloadBytes.data(), staticMesh->indices.data(), staticMesh->indices.size() * sizeof(uint32_t));
        }
        break;
        case MeshType::Bone:
        {
            auto boneMesh = std::dynamic_pointer_cast<BoneMesh>(mesh);

            payloadBytes.resize(boneMesh->indices.size() * sizeof(uint32_t));
            memcpy(payloadBytes.data(), boneMesh->indices.data(), boneMesh->indices.size() * sizeof(uint32_t));
        }
        break;
    }

    return payloadBytes;
}

std::size_t Meshes::GetIndicesCount(const Mesh::Ptr& mesh)
{
    switch (mesh->type)
    {
        case MeshType::Static: return std::dynamic_pointer_cast<StaticMesh>(mesh)->indices.size();
        case MeshType::Bone: return std::dynamic_pointer_cast<BoneMesh>(mesh)->indices.size();
    }

    assert(false);
    return 0;
}

std::optional<std::vector<unsigned char>> Meshes::GetDataPayload(const Mesh::Ptr& mesh)
{
    switch (mesh->type)
    {
        case MeshType::Static: return std::nullopt; // No extra payload for static meshes
        case MeshType::Bone:
        {
            BoneMeshDataPayload dataPayload{};
            dataPayload.numMeshBones = std::dynamic_pointer_cast<BoneMesh>(mesh)->numBones;

            std::vector<unsigned char> dataPayloadBytes(sizeof(BoneMeshDataPayload));
            memcpy(dataPayloadBytes.data(), &dataPayload, sizeof(BoneMeshDataPayload));

            return dataPayloadBytes;
        }
    }

    assert(false);
    return std::nullopt;
}

bool Meshes::UpdateMesh(const Mesh::Ptr& mesh, std::promise<bool> resultPromise)
{
    const auto it = m_meshes.find(mesh->id);
    if (it == m_meshes.cend())
    {
        m_logger->Log(Common::LogLevel::Error, "Meshes: UpdateMesh: No such mesh: {}", mesh->id.id);
        return ErrorResult(resultPromise);
    }

    //
    // Update CPU mesh state
    //
    it->second.boundingBox_modelSpace = CalculateRenderBoundingBox(mesh);

    //
    // Update mesh buffer data
    //
    switch (it->second.usage)
    {
        case MeshUsage::Dynamic:    return PromiseResult(TransferCPUMeshData(it->second, mesh), resultPromise);
        case MeshUsage::Static:     return TransferGPUMeshData(it->second, mesh, false, std::move(resultPromise));
        case MeshUsage::Immutable:
        {
            m_logger->Log(Common::LogLevel::Error, "Meshes: UpdateMesh: Asked to update immutable mesh: {}", mesh->id.id);
            return ErrorResult(resultPromise);
        }
    }

    assert(false);
    return false;
}

bool Meshes::TransferCPUMeshData(const LoadedMesh& loadedMesh, const Mesh::Ptr& newMeshData)
{
    if (!TransferMeshData(ExecutionContext::CPU(), loadedMesh, newMeshData))
    {
        m_logger->Log(Common::LogLevel::Error, "Meshes: Failed to update CPU mesh {}", newMeshData->id.id);
        return false;
    }

    return true;
}

bool Meshes::TransferGPUMeshData(const LoadedMesh& loadedMesh, const Mesh::Ptr& newMeshData, bool initialDataTransfer, std::promise<bool> resultPromise)
{
    m_logger->Log(Common::LogLevel::Debug,
      "Meshes::TransferGPUMeshData: Starting data transfer for mesh: {}", newMeshData->id.id);

    // If we're already actively transferring data to the mesh, error out
    if (m_meshesLoading.contains(newMeshData->id))
    {
        m_logger->Log(Common::LogLevel::Error,
          "Meshes::TransferGPUMeshData: A data transfer for the mesh is already in progress, id: {}", newMeshData->id.id);
        return ErrorResult(resultPromise);
    }

    VulkanFuncs vulkanFuncs(m_logger, m_vulkanObjs);

    // Submit the work to transfer the mesh data
    return vulkanFuncs.QueueSubmit<bool>(
        std::format("TransferGPUMeshData-{}", newMeshData->id.id),
        m_postExecutionOps,
        m_vkTransferQueue,
        m_transferCommandPool,
        [=,this](const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence)
        {
            // Mark the mesh as loading
            m_meshesLoading.insert(loadedMesh.id);
            SyncMetrics();

            if (!TransferMeshData(ExecutionContext::GPU(commandBuffer, vkFence), loadedMesh, newMeshData))
            {
                m_logger->Log(Common::LogLevel::Error,
                  "Meshes::UpdateGPUMeshBuffers: UpdateMeshBuffers failed for mesh {}", newMeshData->id.id);
                return false;
            }

            return true;
        },
        [=,this](bool commandsSuccessful)
        {
            return OnMeshTransferFinished(commandsSuccessful, loadedMesh, initialDataTransfer);
        },
        std::move(resultPromise),
        EnqueueType::Frameless
    );
}

bool Meshes::TransferMeshData(const ExecutionContext& executionContext, const LoadedMesh& loadedMesh, const Mesh::Ptr& newMeshData)
{
    //
    // Update the mesh's data
    //
    const auto verticesPayload = GetVerticesPayload(newMeshData);

    BufferUpdate verticesBufferUpdate{};
    verticesBufferUpdate.pData = verticesPayload.data();
    verticesBufferUpdate.dataByteSize = verticesPayload.size();
    verticesBufferUpdate.updateOffset = loadedMesh.verticesByteOffset;

    if (verticesBufferUpdate.dataByteSize != loadedMesh.verticesByteSize)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Meshes::UpdateMeshBuffers: Mesh vertices byte size change currently not supported, for mesh: ", newMeshData->id.id);
        return false;
    }

    if (!loadedMesh.verticesBuffer->Update(executionContext, {verticesBufferUpdate}))
    {
        m_logger->Log(Common::LogLevel::Error,
          "Meshes::UpdateMeshBuffers: Failed to update vertex data for mesh {}", newMeshData->id.id);
        return false;
    }

    const auto indicesPayload = GetIndicesPayload(newMeshData);

    BufferUpdate indicesBufferUpdate{};
    indicesBufferUpdate.pData = indicesPayload.data();
    indicesBufferUpdate.dataByteSize = indicesPayload.size();
    indicesBufferUpdate.updateOffset = loadedMesh.indicesByteOffset;

    if (indicesBufferUpdate.dataByteSize != loadedMesh.indicesByteSize)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Meshes::UpdateMeshBuffers: Mesh indices byte size change currently not supported, for mesh: ", newMeshData->id.id);
        return false;
    }

    if (!loadedMesh.indicesBuffer->Update(executionContext, {indicesBufferUpdate}))
    {
        m_logger->Log(Common::LogLevel::Error,
          "Meshes::UpdateMeshBuffers: Failed to update index data for mesh {}", newMeshData->id.id);
        return false;
    }

    const auto dataPayload = GetDataPayload(newMeshData);
    if (dataPayload)
    {
        BufferUpdate dataBufferUpdate{};
        dataBufferUpdate.pData = dataPayload->data();
        dataBufferUpdate.dataByteSize = dataPayload->size();
        dataBufferUpdate.updateOffset = loadedMesh.dataByteOffset;

        if (dataBufferUpdate.dataByteSize != loadedMesh.dataByteSize)
        {
            m_logger->Log(Common::LogLevel::Error,
              "Meshes::UpdateMeshBuffers: Mesh data byte size change currently not supported, for mesh: ", newMeshData->id.id);
            return false;
        }

        if (!loadedMesh.dataBuffer.value()->Update(executionContext, {dataBufferUpdate}))
        {
            m_logger->Log(Common::LogLevel::Error,
              "Meshes::UpdateMeshBuffers: Failed to update payload data for mesh {}", newMeshData->id.id);
            return false;
        }
    }

    return true;
}

bool Meshes::OnMeshTransferFinished(bool transfersSuccessful, const LoadedMesh& loadedMesh, bool initialDataTransfer)
{
    m_logger->Log(Common::LogLevel::Debug, "Meshes: Mesh data transfer finished for mesh: {}", loadedMesh.id.id);

    // Mark the mesh as no longer loading
    m_meshesLoading.erase(loadedMesh.id);

    // Now that the transfer is finished, we want to destroy the mesh in two cases:
    // 1) While the transfer was happening, we received a call to destroy the mesh
    // 2) The transfer was an initial data transfer, which failed
    //
    // Note that for update transfers, we're (currently) allowing the mesh to still
    // exist, even though updating its data failed.
    if (m_meshesToDestroy.contains(loadedMesh.id) || (initialDataTransfer && !transfersSuccessful))
    {
        m_logger->Log(Common::LogLevel::Debug,
          "Meshes::OnMeshTransferFinished: Mesh should be destroyed: {}", loadedMesh.id.id);

        // Erase our records of the mesh
        m_meshes.erase(loadedMesh.id);
        m_meshesToDestroy.erase(loadedMesh.id);

        // Enqueue mesh object destruction
        m_postExecutionOps->Enqueue_Current([=, this]() { DestroyMeshObjects(loadedMesh); });

        SyncMetrics();
        return false;
    }

    SyncMetrics();
    return true;
}

std::optional<LoadedMesh> Meshes::GetLoadedMesh(MeshId meshId) const
{
    const auto it = m_meshes.find(meshId);
    if (it == m_meshes.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

void Meshes::DestroyMesh(MeshId meshId, bool destroyImmediately)
{
    const auto it = m_meshes.find(meshId);
    if (it == m_meshes.cend())
    {
        m_logger->Log(Common::LogLevel::Warning,
          "Meshes: DestroyMesh: Asked to destroy mesh which doesn't exist: {}", meshId.id);
        return;
    }

    const auto loadedMesh = it->second;

    // Whether destroying the mesh's objects immediately or not below, erase our knowledge
    // of the mesh; no future render work is allowed to use it
    m_meshes.erase(it);
    m_meshesToDestroy.erase(meshId);

    SyncMetrics();

    // If a mesh's data transfer is still happening, we need to wait until the transfer has finished before
    // destroying the mesh's Vulkan objects. Mark the mesh as to be deleted and bail out.
    if (m_meshesLoading.contains(meshId) && !destroyImmediately)
    {
        m_logger->Log(Common::LogLevel::Debug, "Meshes: Postponing destroy of mesh: {}", meshId.id);
        m_meshesToDestroy.insert(meshId);
        return;
    }
    else if (destroyImmediately)
    {
        m_logger->Log(Common::LogLevel::Debug, "Meshes: Destroying mesh immediately: {}", meshId.id);
        DestroyMeshObjects(loadedMesh);
    }
    else
    {
        m_logger->Log(Common::LogLevel::Debug, "Meshes: Enqueueing mesh destroy: {}", meshId.id);
        m_postExecutionOps->Enqueue_Current([=,this]() { DestroyMeshObjects(loadedMesh); });
    }
}

void Meshes::DestroyMeshObjects(const LoadedMesh& loadedMesh)
{
    m_logger->Log(Common::LogLevel::Debug, "Meshes: Destroying mesh objects: {}", loadedMesh.id.id);

    switch (loadedMesh.usage)
    {
        case MeshUsage::Static:
        case MeshUsage::Dynamic:
        {
            m_buffers->DestroyBuffer(loadedMesh.verticesBuffer->GetBuffer()->GetBufferId());
            m_buffers->DestroyBuffer(loadedMesh.indicesBuffer->GetBuffer()->GetBufferId());
        }
        break;
        case MeshUsage::Immutable:
        {
            // TODO: Currently don't support deleting immutable meshes. Need to keep better book-keeping
            // so that we when destroy a mesh we can update the offsets as appropriate in the LoadedMeshes
            // for the other meshes in the same immutable buffer that came after the deleted one. Also need
            // to have logic to not destroy anything if we're shutting down; we don't want to queue up async
            // GPU work if we're actively getting torn down, just rely on Meshes::Destroy() straight up
            // destroying all the immutable buffers for us.
        }
        break;
    }

    // Return the id to the pool now that it's fully no longer in use
    m_ids->meshIds.ReturnId(loadedMesh.id);
}

void Meshes::SyncMetrics()
{
    m_metrics->SetCounterValue(Renderer_Meshes_Count, m_meshes.size());
    m_metrics->SetCounterValue(Renderer_Meshes_Loading_Count, m_meshesLoading.size());
    m_metrics->SetCounterValue(Renderer_Meshes_ToDestroy_Count, m_meshesToDestroy.size());

    std::size_t totalByteSize = 0;

    for (const auto& it : m_meshes)
    {
        totalByteSize += it.second.verticesByteSize + it.second.indicesByteSize + it.second.dataByteSize;
    }

    m_metrics->SetCounterValue(Renderer_Meshes_ByteSize, totalByteSize);
}

template<typename T>
AABB CalculateRenderBoundingBoxFromVertices(const std::vector<T>& vertices)
{
    AABB boundingBox{};

    for (const T& vertex : vertices)
    {
        boundingBox.AddPoints({vertex.position});
    }

    return boundingBox;
}

AABB Meshes::CalculateRenderBoundingBox(const Mesh::Ptr& mesh)
{
    switch (mesh->type)
    {
        case MeshType::Static: return CalculateRenderBoundingBoxFromVertices(std::dynamic_pointer_cast<StaticMesh>(mesh)->vertices);
        case MeshType::Bone: return CalculateRenderBoundingBoxFromVertices(std::dynamic_pointer_cast<BoneMesh>(mesh)->vertices);
    }

    assert(false);
    return {};
}

}
