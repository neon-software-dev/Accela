/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Materials.h"
#include "ObjectMaterialPayload.h"

#include "../PostExecutionOp.h"
#include "../Metrics.h"

#include "../Buffer/IBuffers.h"
#include "../Buffer/CPUDataBuffer.h"
#include "../Buffer/GPUDataBuffer.h"

#include "../Util/VulkanFuncs.h"
#include "../Util/Futures.h"

#include <format>
#include <cstring>
#include <vector>
#include <cassert>

namespace Accela::Render
{


Materials::Materials(
    Common::ILogger::Ptr logger,
    Common::IMetrics::Ptr metrics,
    VulkanObjsPtr vulkanObjs,
    PostExecutionOpsPtr postExecutionOps,
    Ids::Ptr ids,
    ITexturesPtr textures,
    IBuffersPtr buffers)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_vulkanObjs(std::move(vulkanObjs))
    , m_postExecutionOps(std::move(postExecutionOps))
    , m_ids(std::move(ids))
    , m_textures(std::move(textures))
    , m_buffers(std::move(buffers))
{

}

bool Materials::Initialize(VulkanCommandPoolPtr transferCommandPool, VkQueue vkTransferQueue)
{
    m_logger->Log(Common::LogLevel::Info, "Materials: Initializing");

    m_transferCommandPool = transferCommandPool;
    m_vkTransferQueue = vkTransferQueue;

    return true;
}

void Materials::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "Materials: Destroying");

    // Destroy materials
    while (!m_materials.empty())
    {
        DestroyMaterial(m_materials.cbegin()->first, true);
    }

    // Destroy material buffers
    for (const auto& bufferIt : m_materialBuffers)
    {
        m_buffers->DestroyBuffer(bufferIt.second->GetBuffer()->GetBufferId());
    }
    m_materialBuffers.clear();

    m_materialsLoading.clear();
    m_materialsToDestroy.clear();

    SyncMetrics();
}

bool Materials::CreateMaterial(const Material::Ptr& material, std::promise<bool> resultPromise)
{
    if (m_materials.find(material->materialId) != m_materials.cend())
    {
        m_logger->Log(Common::LogLevel::Error, "Materials: CreateMaterial: Material with id {} already exists", material->materialId.id);
        return ErrorResult(resultPromise);
    }

    const auto bufferExpect = EnsureMaterialBuffer(material->type);
    if (!bufferExpect)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Materials::CreateMaterial: Failed to ensure immutable buffer for material type");
        return ErrorResult(resultPromise);
    }

    //
    // Convert the material to a render material
    //
    const RenderMaterial renderMaterial = ToRenderMaterial(material);

    //
    // Record a record of the material and start a transfer of its data to the GPU
    //
    LoadedMaterial loadedMaterial{};
    loadedMaterial.material = material;
    loadedMaterial.payloadBuffer = *bufferExpect;
    loadedMaterial.payloadByteOffset = (*bufferExpect)->GetDataByteSize();
    loadedMaterial.payloadByteSize = renderMaterial.payloadBytes.size();
    loadedMaterial.payloadIndex = (*bufferExpect)->GetDataByteSize() / renderMaterial.payloadBytes.size();
    loadedMaterial.textureBinds = renderMaterial.textureBinds;

    // Create a record of the material
    m_materials.insert({material->materialId, loadedMaterial});

    SyncMetrics();

    VulkanFuncs vulkanFuncs(m_logger, m_vulkanObjs);

    return vulkanFuncs.QueueSubmit<bool>(
        std::format("CreateMaterial-{}", loadedMaterial.material->materialId.id),
        m_postExecutionOps,
        m_vkTransferQueue,
        m_transferCommandPool,
        [=,this](const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence) {
            // Mark the material as loading
            m_materialsLoading.insert(loadedMaterial.material->materialId);

            SyncMetrics();

            // Append the material to the material's buffer
            BufferAppend bufferAppend{};
            bufferAppend.pData = renderMaterial.payloadBytes.data();
            bufferAppend.dataByteSize = renderMaterial.payloadBytes.size();

            return (*bufferExpect)->PushBack(ExecutionContext::GPU(commandBuffer, vkFence), bufferAppend);
        },
        [=,this](bool commandsSuccessful){
            return OnMaterialTransferFinished(commandsSuccessful, loadedMaterial, true);
        },
        std::move(resultPromise),
        EnqueueType::Frameless
    );
}

std::expected<DataBufferPtr, bool> Materials::EnsureMaterialBuffer(const Material::Type& materialType)
{
    //
    // Return the buffer for the material type, if it exists
    //
    const auto it = m_materialBuffers.find(materialType);
    if (it != m_materialBuffers.cend())
    {
        return it->second;
    }

    //
    // Otherwise, create an immutable buffer for the material type
    //
    const auto buffer = GPUDataBuffer::Create(
        m_buffers,
        m_postExecutionOps,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        1024,
        std::format("GPUMaterialData-{}", static_cast<unsigned int>(materialType))
    );
    if (!buffer)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Materials::EnsureMaterialBuffer: Failed to create buffer for material type {}", static_cast<unsigned int>(materialType));
        return std::unexpected(false);
    }

    m_materialBuffers[materialType] = *buffer;

    return *buffer;
}

bool Materials::UpdateMaterial(const Material::Ptr& material, std::promise<bool> resultPromise)
{
    const auto it = m_materials.find(material->materialId);
    if (it == m_materials.cend())
    {
        m_logger->Log(Common::LogLevel::Error, "Materials: UpdateMaterial: No such material: {}", material->materialId.id);
        return ErrorResult(resultPromise);
    }

    const auto loadedMaterial = it->second;
    const auto renderMaterial = ToRenderMaterial(material);

    VulkanFuncs vulkanFuncs(m_logger, m_vulkanObjs);

    return vulkanFuncs.QueueSubmit<bool>(
        std::format("UpdateMaterial-{}", loadedMaterial.material->materialId.id),
        m_postExecutionOps,
        m_vkTransferQueue,
        m_transferCommandPool,
        [=,this](const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence) {
            // Mark the material as loading
            m_materialsLoading.insert(loadedMaterial.material->materialId);

            SyncMetrics();

            if (!UpdateMaterial(ExecutionContext::GPU(commandBuffer, vkFence), loadedMaterial, renderMaterial))
            {
                m_logger->Log(Common::LogLevel::Error, "Materials: Failed to upload payload data to GPU for material {}", loadedMaterial.material->materialId.id);
                return false;
            }

            return true;
        },
        [=,this](bool commandsSuccessful){
            return OnMaterialTransferFinished(commandsSuccessful, loadedMaterial, false);
        },
        std::move(resultPromise),
        EnqueueType::Frameless
    );
}

bool Materials::UpdateMaterial(const ExecutionContext& executionContext,
                               const LoadedMaterial& loadedMaterial,
                               const RenderMaterial& newMaterialData)
{
    BufferUpdate payloadBufferUpdate{};
    payloadBufferUpdate.pData = newMaterialData.payloadBytes.data();
    payloadBufferUpdate.dataByteSize = newMaterialData.payloadBytes.size();
    payloadBufferUpdate.updateOffset = loadedMaterial.payloadByteOffset;

    if (payloadBufferUpdate.dataByteSize != loadedMaterial.payloadByteSize)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Materials: UpdateMaterial: Material payload byte size change currently not supported, for material: ", loadedMaterial.material->materialId.id);
        return false;
    }

    if (!loadedMaterial.payloadBuffer->Update(executionContext, {payloadBufferUpdate}))
    {
        m_logger->Log(Common::LogLevel::Error, "Materials: Failed to update payload data for material {}", loadedMaterial.material->materialId.id);
        return false;
    }

    return true;
}

std::optional<LoadedMaterial> Materials::GetLoadedMaterial(MaterialId materialId) const
{
    const auto it = m_materials.find(materialId);
    if (it == m_materials.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

std::optional<DataBufferPtr> Materials::GetMaterialBufferForType(const Material::Type& materialType) const
{
    const auto it = m_materialBuffers.find(materialType);
    if (it == m_materialBuffers.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

void Materials::DestroyMaterial(MaterialId materialId, bool destroyImmediately)
{
    const auto it = m_materials.find(materialId);
    if (it == m_materials.cend())
    {
        m_logger->Log(Common::LogLevel::Warning,
          "Materials: Asked to destroy material which doesn't exist: {}", materialId.id);
        return;
    }

    const auto loadedMaterial = it->second;

    // Whether destroying the material's objects immediately or not below, erase our knowledge
    // of the material; no future render work is allowed to use it
    m_materials.erase(it);
    m_materialsToDestroy.erase(materialId);

    SyncMetrics();

    // If a material's data transfer is still happening, need to wait until the transfer has finished before
    // destroying the material's Vulkan objects. Mark the material as to be deleted and bail out.
    if (m_materialsLoading.contains(materialId) && !destroyImmediately)
    {
        m_logger->Log(Common::LogLevel::Debug, "Materials: Postponing destroy of material: {}", materialId.id);
        m_materialsToDestroy.insert(materialId);
        return;
    }
    else if (destroyImmediately)
    {
        m_logger->Log(Common::LogLevel::Debug, "Materials: Destroying material immediately: {}", materialId.id);
        DestroyMaterialObjects(loadedMaterial);
    }
    else
    {
        m_logger->Log(Common::LogLevel::Debug, "Materials: Enqueueing material destroy: {}", materialId.id);
        m_postExecutionOps->Enqueue_Current([=,this]() { DestroyMaterialObjects(loadedMaterial); });
    }
}

bool Materials::OnMaterialTransferFinished(bool transfersSuccessful, const LoadedMaterial& loadedMaterial, bool initialDataTransfer)
{
    const auto materialId = loadedMaterial.material->materialId;

    m_logger->Log(Common::LogLevel::Debug, "Materials: Material data transfer finished for material: {}", materialId.id);

    // Mark the material as no longer loading
    m_materialsLoading.erase(materialId);

    // Now that the transfer is finished, we want to destroy the material in two cases:
    // 1) While the transfer was happening, we received a call to destroy the material
    // 2) The transfer was an initial data transfer, which failed
    //
    // Note that for update transfers, we're (currently) allowing the material to still
    // exist, even though updating its data failed.
    if (m_materialsToDestroy.contains(materialId) || (initialDataTransfer && !transfersSuccessful))
    {
        m_logger->Log(Common::LogLevel::Debug,
          "Materials::OnMaterialTransferFinished: Material should be destroyed: {}", materialId.id);

        // Erase our records of the material
        m_materials.erase(materialId);
        m_materialsToDestroy.erase(materialId);

        // Enqueue material object destruction
        m_postExecutionOps->Enqueue_Current([=, this]() { DestroyMaterialObjects(loadedMaterial); });

        SyncMetrics();
        return false;
    }

    SyncMetrics();
    return true;
}

void Materials::DestroyMaterialObjects(const LoadedMaterial& loadedMaterial)
{
    m_logger->Log(Common::LogLevel::Debug, "Materials: Destroying material objects: {}", loadedMaterial.material->materialId.id);

    // TODO: Support destroying materials

    // TODO: Also remember to return materialId afterwards
}

RenderMaterial Materials::ToRenderMaterial(const Material::Ptr& material)
{
    switch (material->type)
    {
        case Material::Type::Object: return ObjectMaterialToRenderMaterial(std::dynamic_pointer_cast<ObjectMaterial>(material));
    }

    assert(false);
    return {};
}

RenderMaterial Materials::ObjectMaterialToRenderMaterial(const ObjectMaterial::Ptr& material)
{
    RenderMaterial renderMaterial{};

    //
    // Payload
    //
    ObjectMaterialPayload materialPayload{};
    materialPayload.isAffectedByLighting = material->properties.isAffectedByLighting;
    materialPayload.ambientColor = material->properties.ambientColor;
    materialPayload.diffuseColor = material->properties.diffuseColor;
    materialPayload.specularColor = material->properties.specularColor;
    materialPayload.alphaMode = static_cast<uint32_t>(material->properties.alphaMode);
    materialPayload.alphaCutoff = material->properties.alphaCutoff;
    materialPayload.shininess = material->properties.shininess;

    materialPayload.hasAmbientTexture = material->properties.ambientTextureBind != TextureId{INVALID_ID};
    materialPayload.ambientTextureBlendFactor = material->properties.ambientTextureBlendFactor;
    materialPayload.ambientTextureOp = static_cast<uint32_t>(material->properties.ambientTextureOp);

    materialPayload.hasDiffuseTexture = material->properties.diffuseTextureBind != TextureId{INVALID_ID};
    materialPayload.diffuseTextureBlendFactor = material->properties.diffuseTextureBlendFactor;
    materialPayload.diffuseTextureOp = static_cast<uint32_t>(material->properties.diffuseTextureOp);

    materialPayload.hasSpecularTexture = material->properties.specularTextureBind != TextureId{INVALID_ID};
    materialPayload.specularTextureBlendFactor = material->properties.specularTextureBlendFactor;
    materialPayload.specularTextureOp = static_cast<uint32_t>(material->properties.specularTextureOp);

    materialPayload.hasNormalTexture = material->properties.normalTextureBind != TextureId{INVALID_ID};

    renderMaterial.payloadBytes.resize(sizeof(ObjectMaterialPayload));
    memcpy(renderMaterial.payloadBytes.data(), &materialPayload, sizeof(ObjectMaterialPayload));

    //
    // Texture Binds
    //
    renderMaterial.textureBinds["i_ambientSampler"] = material->properties.ambientTextureBind;
    renderMaterial.textureBinds["i_diffuseSampler"] = material->properties.diffuseTextureBind;
    renderMaterial.textureBinds["i_specularSampler"] = material->properties.specularTextureBind;
    renderMaterial.textureBinds["i_normalSampler"] = material->properties.normalTextureBind;

    return renderMaterial;
}

void Materials::SyncMetrics()
{
    m_metrics->SetCounterValue(Renderer_Materials_Count, m_materials.size());
    m_metrics->SetCounterValue(Renderer_Materials_Loading_Count, m_materialsLoading.size());
    m_metrics->SetCounterValue(Renderer_Materials_ToDestroy_Count, m_materialsToDestroy.size());

    std::size_t totalByteSize = 0;

    for (const auto& it : m_materials)
    {
        totalByteSize += it.second.payloadByteSize;
    }

    m_metrics->SetCounterValue(Renderer_Materials_ByteSize, totalByteSize);
}

}
