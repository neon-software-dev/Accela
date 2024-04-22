/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Materials.h"
#include "ObjectMaterialPayload.h"

#include "../PostExecutionOp.h"

#include "../Buffer/IBuffers.h"
#include "../Buffer/CPUDataBuffer.h"
#include "../Buffer/GPUDataBuffer.h"

#include "../Util/VulkanFuncs.h"

#include <format>
#include <cstring>
#include <vector>
#include <cassert>

namespace Accela::Render
{


Materials::Materials(
    Common::ILogger::Ptr logger,
    VulkanObjsPtr vulkanObjs,
    PostExecutionOpsPtr postExecutionOps,
    Ids::Ptr ids,
    ITexturesPtr textures,
    IBuffersPtr buffers)
    : m_logger(std::move(logger))
    , m_vulkanObjs(std::move(vulkanObjs))
    , m_postExecutionOps(std::move(postExecutionOps))
    , m_ids(std::move(ids))
    , m_textures(std::move(textures))
    , m_buffers(std::move(buffers))
{

}

bool Materials::Initialize(VulkanCommandPoolPtr transferCommandPool,
                           VkQueue vkTransferQueue)
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
}

bool Materials::CreateMaterial(const Material::Ptr& material)
{
    if (m_materials.find(material->materialId) != m_materials.cend())
    {
        m_logger->Log(Common::LogLevel::Warning, "Materials: CreateMaterial: Material with id {} already exists", material->materialId.id);
        return true;
    }

    const auto bufferExpect = EnsureMaterialBuffer(material->type);
    if (!bufferExpect)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Materials::CreateMaterial: Failed to ensure immutable buffer for material type");
        return false;
    }

    //
    // Convert the material to a render material
    //
    const RenderMaterial renderMaterial = ToRenderMaterial(material);

    //
    // Record a record of the material and start a transfer of its data to the GPU
    //
    LoadedMaterial loadedMaterial{};
    loadedMaterial.id = material->materialId;
    loadedMaterial.type = material->type;
    loadedMaterial.payloadBuffer = *bufferExpect;
    loadedMaterial.payloadByteOffset = (*bufferExpect)->GetDataByteSize();
    loadedMaterial.payloadByteSize = renderMaterial.payloadBytes.size();
    loadedMaterial.payloadIndex = (*bufferExpect)->GetDataByteSize() / renderMaterial.payloadBytes.size();
    loadedMaterial.textureBinds = renderMaterial.textureBinds;

    m_materials.insert({material->materialId, loadedMaterial});

    VulkanFuncs vulkanFuncs(m_logger, m_vulkanObjs);

    // Mark the material as loading
    m_materialsLoading.insert(loadedMaterial.id);

    BufferAppend bufferAppend{};
    bufferAppend.pData = renderMaterial.payloadBytes.data();
    bufferAppend.dataByteSize = renderMaterial.payloadBytes.size();

    return vulkanFuncs.QueueSubmit(
        std::format("LoadMaterial-{}", loadedMaterial.id.id),
        m_postExecutionOps,
        m_vkTransferQueue,
        m_transferCommandPool,
        [=,this](const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence) {
            (*bufferExpect)->PushBack(ExecutionContext::GPU(commandBuffer, vkFence), bufferAppend);

            // When the transfer has finished, handle post load tasks
            m_postExecutionOps->Enqueue(vkFence, [=,this](){ OnMaterialLoadFinished(loadedMaterial); });
        }
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

bool Materials::UpdateMaterial(const Material::Ptr& material)
{
    const auto it = m_materials.find(material->materialId);
    if (it == m_materials.cend())
    {
        m_logger->Log(Common::LogLevel::Error, "Materials: UpdateMaterial: No such material: {}", material->materialId.id);
        return false;
    }

    const auto loadedMaterial = it->second;
    const auto renderMaterial = ToRenderMaterial(material);

    VulkanFuncs vulkanFuncs(m_logger, m_vulkanObjs);

    // Mark the material as loading
    m_materialsLoading.insert(loadedMaterial.id);

    return vulkanFuncs.QueueSubmit(
        std::format("UpdateMaterial-{}", loadedMaterial.id.id),
        m_postExecutionOps,
        m_vkTransferQueue,
        m_transferCommandPool,
        [=,this](const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence) {
            if (!UpdateMaterial(ExecutionContext::GPU(commandBuffer, vkFence), loadedMaterial, renderMaterial))
            {
                m_logger->Log(Common::LogLevel::Error, "Materials: Failed to upload payload data to GPU for material {}", loadedMaterial.id.id);
                return;
            }

            // When the transfer has finished, handle post load tasks
            m_postExecutionOps->Enqueue(vkFence, [=,this](){ OnMaterialLoadFinished(loadedMaterial); });
        }
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
          "Materials: UpdateMaterial: Material payload byte size change currently not supported, for material: ", loadedMaterial.id.id);
        return false;
    }

    if (!loadedMaterial.payloadBuffer->Update(executionContext, {payloadBufferUpdate}))
    {
        m_logger->Log(Common::LogLevel::Error, "Materials: Failed to update payload data for material {}", loadedMaterial.id.id);
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

    ////

    // If a material's data transfer is still happening, need to wait until the transfer has finished before
    // destroying the material's Vulkan objects. Mark the material as to be deleted and bail out.
    if (m_materialsLoading.contains(materialId) && !destroyImmediately)
    {
        m_logger->Log(Common::LogLevel::Debug, "Materials: Postponing destroy of material: {}", materialId.id);
        m_materialsToDestroy.insert(materialId);
        return;
    }

    if (destroyImmediately)
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

void Materials::OnMaterialLoadFinished(const LoadedMaterial& loadedMaterial)
{
    m_logger->Log(Common::LogLevel::Debug, "Materials: Material load finished for material: {}", loadedMaterial.id.id);

    m_materialsLoading.erase(loadedMaterial.id);

    if (m_materialsToDestroy.contains(loadedMaterial.id))
    {
        m_logger->Log(Common::LogLevel::Debug, "Meshes: Loaded mesh should be destroyed: {}", loadedMaterial.id.id);

        m_materialsToDestroy.erase(loadedMaterial.id);

        // Can immediately destroy since PostExecutionOps will only run the load fence's
        // queued work once both it is done and all frames have since finished their work.
        DestroyMaterial(loadedMaterial.id, true);
    }
}

void Materials::DestroyMaterialObjects(const LoadedMaterial& loadedMaterial)
{
    m_logger->Log(Common::LogLevel::Debug, "Materials: Destroying material objects: {}", loadedMaterial.id.id);

    // TODO: Support destroying materials

    // TODO: Also remember to return materail id afterwards
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
    materialPayload.opacity = material->properties.opacity;
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

}
