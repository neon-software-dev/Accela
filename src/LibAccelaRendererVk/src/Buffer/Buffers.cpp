#include "Buffers.h"

#include "../VulkanObjs.h"
#include "../PostExecutionOp.h"
#include "../Metrics.h"

#include "../VMA/IVMA.h"
#include "../Util/Synchronization.h"

#include "../Vulkan/VulkanDebug.h"
#include "../Vulkan/VulkanCommandBuffer.h"

#include <Accela/Render/IVulkanCalls.h>

#include <format>
#include <cassert>
#include <set>

namespace Accela::Render
{

Buffers::Buffers(Common::ILogger::Ptr logger,
                 Common::IMetrics::Ptr metrics,
                 VulkanObjsPtr vulkanObjs,
                 PostExecutionOpsPtr postExecutionOps)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_vulkanObjs(std::move(vulkanObjs))
    , m_postExecutionOps(std::move(postExecutionOps))
{

}

bool Buffers::Initialize()
{
    m_logger->Log(Common::LogLevel::Info, "Buffers: Initializing");

    return true;
}

void Buffers::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "Buffers: Destroying");

    while (!m_buffers.empty())
    {
        DestroyBuffer(m_buffers.cbegin()->first);
    }

    m_bufferIds.Reset();

    SyncMetrics();
}

std::expected<BufferPtr, Buffers::BufferCreateError> Buffers::CreateBuffer(
    VkBufferUsageFlags vkUsageFlags,
    VmaMemoryUsage vmaMemoryUsage,
    const std::size_t& byteSize,
    const std::string& tag)
{
    //
    // Create a VMA allocation for the buffer
    //
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = byteSize;
    bufferInfo.usage = vkUsageFlags;

    VmaAllocationCreateInfo vmaAllocCreateInfo{};
    vmaAllocCreateInfo.usage = vmaMemoryUsage;

    BufferAllocation bufferAllocation{};
    bufferAllocation.vkBufferUsageFlags = vkUsageFlags;
    bufferAllocation.vmaMemoryUsage = vmaMemoryUsage;

    auto result = m_vulkanObjs->GetVMA()->CreateBuffer(
        &bufferInfo,
        &vmaAllocCreateInfo,
        &bufferAllocation.vkBuffer,
        &bufferAllocation.vmaAllocation,
        nullptr);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error,
          "CreateBuffer: vmaCreateBuffer call failure, result code: {}", (uint32_t)result);
        return std::unexpected(IBuffers::BufferCreateError::AllocationFailed);
    }

    //
    // Track the buffer
    //
    const auto bufferId = m_bufferIds.GetId();

    auto buffer = std::make_shared<Buffer>(bufferId, vkUsageFlags, bufferAllocation, byteSize, tag);

    // Attach a debug name to the buffer
    SetDebugName(
        m_vulkanObjs->GetCalls(),
        m_vulkanObjs->GetDevice(),
        VK_OBJECT_TYPE_BUFFER,
        (uint64_t)bufferAllocation.vkBuffer,
        std::format("Buffer-{}", tag)
    );

    m_buffers[bufferId] = buffer;
    SyncMetrics();

    return buffer;
}

bool Buffers::DestroyBuffer(BufferId bufferId)
{
    const auto bufferIt = m_buffers.find(bufferId);
    if (bufferIt == m_buffers.cend())
    {
        m_logger->Log(Common::LogLevel::Warning,
          "DestroyBuffer: Asked to delete buffer which doesn't exist, buffer id: {}", bufferId.id);
        return true;
    }

    const auto buffer = bufferIt->second;

    RemoveDebugName(
        m_vulkanObjs->GetCalls(),
        m_vulkanObjs->GetDevice(),
        VK_OBJECT_TYPE_BUFFER,
        (uint64_t)buffer->GetVkBuffer()
    );

    m_vulkanObjs->GetVMA()->DestroyBuffer(buffer->GetVkBuffer(), buffer->GetVmaAllocation());

    //
    // Stop tracking the buffer
    //
    m_buffers.erase(bufferIt);
    m_bufferIds.ReturnId(bufferId);
    SyncMetrics();

    return true;
}

static bool CanBufferBeMapped(const BufferPtr& buffer)
{
    const bool isSupportedAllocationType =
        buffer->GetAllocation().vmaMemoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU ||
        buffer->GetAllocation().vmaMemoryUsage == VMA_MEMORY_USAGE_CPU_ONLY;

    return isSupportedAllocationType;
}

bool Buffers::MappedUpdateBuffer(const BufferPtr& buffer, const std::vector<BufferUpdate>& updates) const
{
    //
    // Verify preconditions
    //
    const bool canBufferBeMapped = CanBufferBeMapped(buffer);

    if (!canBufferBeMapped)
    {
        assert(canBufferBeMapped);

        m_logger->Log(Common::LogLevel::Error,
          "Buffers: MappedUpdateBuffer: The supplied buffer, id: {}, is not a mappable type", buffer->GetBufferId().id);
        return false;
    }

    //
    // Map the buffer into memory and copy the updates into it
    //
    void *pMappedBuffer{nullptr};
    m_vulkanObjs->GetVMA()->MapMemory(buffer->GetVmaAllocation(), &pMappedBuffer);

    for (const auto& update : updates)
    {
        assert(buffer->GetByteSize() >= (update.updateOffset + update.dataByteSize));

        memcpy((unsigned char*)pMappedBuffer + update.updateOffset, update.pData, update.dataByteSize);
    }

    m_vulkanObjs->GetVMA()->UnmapMemory(buffer->GetVmaAllocation());

    return true;
}

bool Buffers::MappedCopyBufferData(const BufferPtr& srcBuffer,
                                   const size_t& srcOffset,
                                   const size_t& copyByteSize,
                                   const BufferPtr& dstBuffer,
                                   const size_t& dstOffset) const
{
    assert(srcBuffer->GetByteSize() >= (srcOffset + copyByteSize));
    assert(dstBuffer->GetByteSize() >= (dstOffset + copyByteSize));

    void *pMappedSrcBuffer{nullptr};
    void *pMappedDstBuffer{nullptr};

    m_vulkanObjs->GetVMA()->MapMemory(srcBuffer->GetVmaAllocation(), &pMappedSrcBuffer);
    m_vulkanObjs->GetVMA()->MapMemory(dstBuffer->GetVmaAllocation(), &pMappedDstBuffer);

    memcpy((unsigned char*)pMappedDstBuffer + dstOffset, (unsigned char*)pMappedSrcBuffer + srcOffset, copyByteSize);

    m_vulkanObjs->GetVMA()->UnmapMemory(srcBuffer->GetVmaAllocation());
    m_vulkanObjs->GetVMA()->UnmapMemory(dstBuffer->GetVmaAllocation());

    return true;
}

bool Buffers::MappedDeleteData(const BufferPtr& buffer, const std::vector<BufferDelete>& deletes) const
{
    if (deletes.empty()) { return true; }

    //
    // Verify preconditions
    //
    const bool canBufferBeMapped = CanBufferBeMapped(buffer);

    assert(canBufferBeMapped);

    if (!canBufferBeMapped)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Buffers: MappedUpdateBuffer: The supplied buffer {} is not a mappable type", buffer->GetBufferId().id);
        return false;
    }

    //
    // Sort the buffer deletes by offset into the buffer to be deleted
    //
    std::set<BufferDelete, DeleteOffsetSort> offsetSortedDeletes;

    for (const auto& bufferDelete : deletes)
    {
        offsetSortedDeletes.insert(bufferDelete);
    }

    //
    // Given the sections to be deleted, create an inverse of the buffer, which
    // are the sections that we don't want to delete.
    //
    std::vector<BufferSection> savedSections;

    std::size_t workingOffset{0};

    for (const auto& bufferDelete : offsetSortedDeletes)
    {
        if (bufferDelete.deleteOffset - workingOffset > 0)
        {
            savedSections.push_back({workingOffset, bufferDelete.deleteOffset - workingOffset});
        }

        workingOffset = bufferDelete.deleteOffset + bufferDelete.deleteByteSize;
    }

    //
    // Rewrite the buffer, starting at the beginning, by copying the sections to be saved forwards,
    // tightly packing them, overwriting the deleted sections
    //
    workingOffset = 0;

    void *pMappedBuffer{nullptr};
    m_vulkanObjs->GetVMA()->MapMemory(buffer->GetVmaAllocation(), &pMappedBuffer);

    for (const auto& savedSection : savedSections)
    {
        memcpy((unsigned char*)pMappedBuffer + workingOffset, (unsigned char*)pMappedBuffer + savedSection.offset, savedSection.byteSize);
        workingOffset += savedSection.byteSize;
    }

    m_vulkanObjs->GetVMA()->UnmapMemory(buffer->GetVmaAllocation());

    return true;
}

bool Buffers::StagingUpdateBuffer(const BufferPtr& buffer,
                                  const std::vector<BufferUpdate>& updates,
                                  VkPipelineStageFlagBits firstUsageStageFlag,
                                  VkPipelineStageFlagBits lastUsageStageFlag,
                                  const VulkanCommandBufferPtr& commandBuffer,
                                  const VkFence& vkExecutionFence)
{
    std::size_t totalUpdateBytes{0};
    std::vector<unsigned char> allUpdatesBytes;

    for (const auto& update : updates)
    {
        assert(buffer->GetByteSize() >= (update.updateOffset + update.dataByteSize));
        assert(buffer->GetAllocation().vkBufferUsageFlags & VK_BUFFER_USAGE_TRANSFER_DST_BIT);

        const auto currentBackPos = allUpdatesBytes.size();

        allUpdatesBytes.resize(allUpdatesBytes.size() + update.dataByteSize);

        memcpy(allUpdatesBytes.data() + currentBackPos, update.pData, update.dataByteSize);

        totalUpdateBytes += update.dataByteSize;
    }

    //
    // Create a staging buffer to hold all the data updates
    //
    const auto stagingBuffer = CreateBuffer(
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        totalUpdateBytes,
        std::format("StagingUpdateBuffer-{}" ,buffer->GetBufferId().id)
    );
    if (!stagingBuffer.has_value())
    {
        m_logger->Log(Common::LogLevel::Error,
          "Buffers: StagingUpdateBuffer: Failed to create staging buffer error: {}", (uint32_t)stagingBuffer.error());
        return false;
    }

    //
    // Map the staging buffer into memory and copy all the updates into it
    //
    BufferUpdate stagingBufferUpdate{};
    stagingBufferUpdate.updateOffset = 0;
    stagingBufferUpdate.pData = allUpdatesBytes.data();
    stagingBufferUpdate.dataByteSize = allUpdatesBytes.size();

    MappedUpdateBuffer(*stagingBuffer, {stagingBufferUpdate});

    //
    // Copy the data from the staging buffer to the destination buffer.
    // Note that this internally creates pipeline barriers for each copy.
    //
    std::size_t stagingOffset{0};

    for (const auto& update : updates)
    {
        CopyBufferData(
            *stagingBuffer,             // srcBuffer
            stagingOffset,              // srcOffset
            update.dataByteSize,     // copyByteSize
            buffer,                    // dstBuffer
            update.updateOffset,       // dstOffset
            firstUsageStageFlag,
            lastUsageStageFlag,
            commandBuffer
        );

        stagingOffset += update.dataByteSize;
    }

    //
    // Mark the staging buffer for deletion
    //
    const auto stagingBufferId = (*stagingBuffer)->GetBufferId();

    m_postExecutionOps->EnqueueFrameless(vkExecutionFence, [stagingBufferId, this](){
        DestroyBuffer(stagingBufferId);
    });

    return true;
}

bool Buffers::StagingDeleteData(const BufferPtr& buffer,
                                const std::vector<BufferDelete>& deletes,
                                VkPipelineStageFlagBits firstUsageStageFlag,
                                VkPipelineStageFlagBits lastUsageStageFlag,
                                const VulkanCommandBufferPtr& commandBuffer) const
{
    if (deletes.empty()) { return true; }

    //
    // Sort the buffer deletes by offset into the buffer to be deleted
    //
    std::set<BufferDelete, DeleteOffsetSort> offsetSortedDeletes;

    for (const auto& bufferDelete : deletes)
    {
        offsetSortedDeletes.insert(bufferDelete);
    }

    //
    // Given the sections to be deleted, create an inverse of the buffer, which
    // are the sections that we don't want to delete.
    //
    std::vector<BufferSection> savedSections;

    std::size_t workingOffset{0};

    for (const auto& bufferDelete : offsetSortedDeletes)
    {
        if (bufferDelete.deleteOffset - workingOffset > 0)
        {
            savedSections.push_back({workingOffset, bufferDelete.deleteOffset - workingOffset});
        }

        workingOffset = bufferDelete.deleteOffset + bufferDelete.deleteByteSize;
    }

    //
    // Issue copy commands to move saved sections forwards, overwriting deleted sections as needed
    //
    workingOffset = 0;

    for (const auto& savedSection : savedSections)
    {
        if (!CopyBufferData(
            buffer,
            savedSection.offset,
            savedSection.byteSize,
            buffer,
            workingOffset,
            firstUsageStageFlag,
            lastUsageStageFlag,
            commandBuffer
        )) {
            return false;
        }

        workingOffset += savedSection.byteSize;
    }

    return true;
}

bool Buffers::CopyBufferData(const BufferPtr& srcBuffer,
                             const std::size_t& srcOffset,
                             const std::size_t& copyByteSize,
                             const BufferPtr& dstBuffer,
                             const std::size_t& dstOffset,
                             const VkPipelineStageFlagBits& firstUsageStageFlag,
                             const VkPipelineStageFlagBits& lastUsageStageFlag,
                             const VulkanCommandBufferPtr& commandBuffer) const
{
    //
    // Verify preconditions
    //
    assert(srcBuffer->GetByteSize() >= (srcOffset + copyByteSize));
    assert(dstBuffer->GetByteSize() >= (dstOffset + copyByteSize));

    //
    // Add a pipeline barrier to wait until previous reads have finished with the buffer before writing to it
    //
    InsertPipelineBarrier_Buffer(
        m_vulkanObjs->GetCalls(),
        commandBuffer,
        SourceStage(lastUsageStageFlag),
        DestStage(VK_PIPELINE_STAGE_TRANSFER_BIT),
        BufferMemoryBarrier(
            dstBuffer,
            dstOffset,
            copyByteSize,
            SourceAccess(VK_ACCESS_MEMORY_READ_BIT),
            DestAccess(VK_ACCESS_TRANSFER_WRITE_BIT)
        )
    );

    //
    // Issue a command to copy the src buffer data to the dst buffer
    //
    VkBufferCopy bufferCopy{};
    bufferCopy.srcOffset = srcOffset;
    bufferCopy.dstOffset = dstOffset;
    bufferCopy.size = copyByteSize;

    m_vulkanObjs->GetCalls()->vkCmdCopyBuffer(
        commandBuffer->GetVkCommandBuffer(),
        srcBuffer->GetVkBuffer(),
        dstBuffer->GetVkBuffer(),
        1,
        &bufferCopy
    );

    //
    // Add a pipeline barrier to protect shaders from reading the buffer section until the transfer has finished
    //
    InsertPipelineBarrier_Buffer(
        m_vulkanObjs->GetCalls(),
        commandBuffer,
        SourceStage(VK_PIPELINE_STAGE_TRANSFER_BIT),
        DestStage(firstUsageStageFlag),
        BufferMemoryBarrier(
            dstBuffer,
            dstOffset,
            copyByteSize,
            SourceAccess(VK_ACCESS_TRANSFER_WRITE_BIT),
            DestAccess(VK_ACCESS_MEMORY_READ_BIT)
        )
    );

    //
    // Add a pipeline barrier to protect subsequent transfers from reading or writing to the buffer section
    // until the current transfer has finished
    //
    InsertPipelineBarrier_Buffer(
        m_vulkanObjs->GetCalls(),
        commandBuffer,
        SourceStage(VK_PIPELINE_STAGE_TRANSFER_BIT),
        DestStage(VK_PIPELINE_STAGE_TRANSFER_BIT),
        BufferMemoryBarrier(
            dstBuffer,
            dstOffset,
            copyByteSize,
            SourceAccess(VK_ACCESS_TRANSFER_WRITE_BIT),
            DestAccess(VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT)
        )
    );

    return true;
}

void Buffers::SyncMetrics()
{
    m_metrics->SetCounterValue(Renderer_Buffers_Count, m_buffers.size());

    std::size_t totalBuffersByteSize{0};

    for (const auto& bufferIt : m_buffers)
    {
        totalBuffersByteSize += bufferIt.second->GetByteSize();
    }

    m_metrics->SetCounterValue(Renderer_Buffers_ByteSize, totalBuffersByteSize);
}

}
