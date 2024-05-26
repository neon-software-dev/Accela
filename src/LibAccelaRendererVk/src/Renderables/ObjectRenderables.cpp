#include "ObjectRenderables.h"

#include "../Buffer/IBuffers.h"
#include "../Buffer/GPUItemBuffer.h"
#include "../Mesh/IMeshes.h"
#include "../Light/ILights.h"

namespace Accela::Render
{


ObjectRenderables::ObjectRenderables(
    Common::ILogger::Ptr logger,
    Ids::Ptr ids,
    PostExecutionOpsPtr postExecutionOps,
    IBuffersPtr buffers,
    ITexturesPtr textures,
    IMeshesPtr meshes,
    ILightsPtr lights)
    : m_logger(std::move(logger))
    , m_ids(std::move(ids))
    , m_postExecutionOps(std::move(postExecutionOps))
    , m_buffers(std::move(buffers))
    , m_textures(std::move(textures))
    , m_meshes(std::move(meshes))
    , m_lights(std::move(lights))
{

}

bool ObjectRenderables::Initialize()
{
    assert(m_objectPayloadBuffer == nullptr);

    const auto dataBuffer = GPUItemBuffer<ObjectPayload>::Create(
        m_buffers,
        m_postExecutionOps,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        16,
        "SceneObjects-Data"
    );
    if (!dataBuffer)
    {
        m_logger->Log(Common::LogLevel::Fatal, "ObjectRenderables: Failed to create payload buffer");
        return false;
    }

    m_objectPayloadBuffer = *dataBuffer;

    return true;
}

void ObjectRenderables::Destroy()
{
    if (m_objectPayloadBuffer != nullptr)
    {
        m_buffers->DestroyBuffer(m_objectPayloadBuffer->GetBuffer()->GetBufferId());
    }
}

void ObjectRenderables::ProcessUpdate(const WorldUpdate& update,
                                      const VulkanCommandBufferPtr& commandBuffer,
                                      VkFence vkFence)
{
    // World-space AABBs of objects that were added/removed/deleted that are/were part of
    // shadow passes; used for invalidating shadow maps for lights that see those areas.
    ModifiedWorldAreas modifiedShadowWorldAreas{};

    ProcessAddedObjects(update, modifiedShadowWorldAreas, commandBuffer, vkFence);
    ProcessUpdatedObjects(update, modifiedShadowWorldAreas, commandBuffer, vkFence);
    ProcessDeletedObjects(update, modifiedShadowWorldAreas, commandBuffer, vkFence);

    // Tell the lighting system about the world-space bounds of every object that was added, updated,
    // or deleted, and it in turn will invalidate the shadow maps of any lights that cover those bounds
    m_lights->InvalidateShadowMapsByBounds(modifiedShadowWorldAreas.boundingBoxes_worldSpace);
}

void ObjectRenderables::ProcessAddedObjects(const WorldUpdate& update,
                                            ModifiedWorldAreas& modifieShadowWorldAreas,
                                            const VulkanCommandBufferPtr& commandBuffer,
                                            VkFence vkFence)
{
    if (update.toAddObjectRenderables.empty()) { return; }

    //
    // Transform the objects to object payloads
    //
    std::vector<ItemUpdate<ObjectPayload>> updates;
    updates.reserve(update.toAddObjectRenderables.size());

    IdType highestId{0};

    for (const auto& object : update.toAddObjectRenderables)
    {
        if (object.objectId.id == INVALID_ID)
        {
            m_logger->Log(Common::LogLevel::Warning, "ProcessAddedObjects: An object has an invalid id, ignoring");
            continue;
        }

        updates.emplace_back(ObjectToPayload(object), object.objectId.id - 1);

        highestId = std::max(highestId, object.objectId.id);
    }

    //
    // Update the GPU data buffer
    //
    const auto executionContext = ExecutionContext::GPU(commandBuffer, vkFence);

    if (m_objectPayloadBuffer->GetSize() < highestId)
    {
        m_objectPayloadBuffer->Resize(executionContext, highestId);
    }

    if (!m_objectPayloadBuffer->Update(executionContext, updates))
    {
        m_logger->Log(Common::LogLevel::Error, "ProcessAddedObjects: Failed to update payload buffer");
        return;
    }

    //
    // Update the CPU data buffer
    //
    if (m_objects.size() < highestId)
    {
        m_objects.resize(highestId);
    }

    for (const auto& object : update.toAddObjectRenderables)
    {
        RenderableData<ObjectRenderable> objectData{};
        objectData.isValid = true;
        objectData.renderable = object;

        const auto aabbExpect = GetObjectAABB(object);
        if (aabbExpect)
        {
            objectData.boundingBox_worldSpace = *aabbExpect;

            if (object.shadowPass)
            {
                modifieShadowWorldAreas.boundingBoxes_worldSpace.push_back(objectData.boundingBox_worldSpace);
            }
        }

        m_objects[object.objectId.id - 1] = objectData;
        m_objectsRTree[object.sceneName].Insert(aabbExpect->GetVolume(), object.objectId);
    }
}

void ObjectRenderables::ProcessUpdatedObjects(const WorldUpdate& update,
                                              ModifiedWorldAreas& modifiedShadowWorldAreas,
                                              const VulkanCommandBufferPtr& commandBuffer,
                                              VkFence vkFence)
{
    if (update.toUpdateObjectRenderables.empty()) { return; }

    std::vector<ItemUpdate<ObjectPayload>> updates;
    updates.reserve(update.toUpdateObjectRenderables.size());

    for (const auto& object : update.toUpdateObjectRenderables)
    {
        if (object.objectId.id == INVALID_ID)
        {
            m_logger->Log(Common::LogLevel::Warning, "ProcessUpdatedObjects: An object has an invalid id, ignoring");
            continue;
        }

        if (object.objectId.id > m_objects.size())
        {
            m_logger->Log(Common::LogLevel::Error,
              "ProcessUpdatedObjects: No such object with id {} exists", object.objectId.id);
            continue;
        }

        updates.emplace_back(ObjectToPayload(object), object.objectId.id - 1);
    }

    const auto executionContext = ExecutionContext::GPU(commandBuffer, vkFence);

    //
    // Update the GPU data buffer
    //
    if (!m_objectPayloadBuffer->Update(executionContext, updates))
    {
        m_logger->Log(Common::LogLevel::Error, "ProcessUpdatedObjects: Failed to update payload buffer");
        return;
    }

    //
    // Update the CPU data buffer
    //
    for (const auto& toUpdateObjectRenderable : update.toUpdateObjectRenderables)
    {
        auto& existingObject = m_objects[toUpdateObjectRenderable.objectId.id - 1];

        const auto aabbExpect = GetObjectAABB(toUpdateObjectRenderable);
        if (!aabbExpect)
        {
            m_logger->Log(Common::LogLevel::Error,
              "ProcessUpdatedObjects: Failed to get AABB for object, id: {}", toUpdateObjectRenderable.objectId.id);
            continue;
        }

        RenderableData<ObjectRenderable> updatedData{};
        updatedData.isValid = true;
        updatedData.renderable = toUpdateObjectRenderable;
        updatedData.boundingBox_worldSpace = *aabbExpect;

        const bool aabbInvalidated = existingObject.boundingBox_worldSpace != updatedData.boundingBox_worldSpace;

        // If the object's AABB changed, update its spatial data in the object r-tree
        if (aabbInvalidated)
        {
            m_objectsRTree[toUpdateObjectRenderable.sceneName].Remove(
                existingObject.boundingBox_worldSpace.GetVolume(),
                toUpdateObjectRenderable.objectId
            );

            m_objectsRTree[toUpdateObjectRenderable.sceneName].Insert(
                updatedData.boundingBox_worldSpace.GetVolume(),
                toUpdateObjectRenderable.objectId
            );

            if (existingObject.renderable.shadowPass)
            {
                modifiedShadowWorldAreas.boundingBoxes_worldSpace.push_back(existingObject.boundingBox_worldSpace);
            }

            if (updatedData.renderable.shadowPass)
            {
                modifiedShadowWorldAreas.boundingBoxes_worldSpace.push_back(updatedData.boundingBox_worldSpace);
            }
        }

        // Update the object's CPU data
        existingObject = updatedData;
    }
}

void ObjectRenderables::ProcessDeletedObjects(const WorldUpdate& update,
                                              ModifiedWorldAreas& modifiedShadowWorldAreas,
                                              const VulkanCommandBufferPtr&,
                                              VkFence)
{
    if (update.toDeleteObjectIds.empty()) { return; }

    for (const auto& toDeleteId : update.toDeleteObjectIds)
    {
        if (toDeleteId.id == INVALID_ID)
        {
            m_logger->Log(Common::LogLevel::Warning, "ProcessDeletedObjects: An object has an invalid id, ignoring");
            continue;
        }

        if (toDeleteId.id > m_objects.size())
        {
            m_logger->Log(Common::LogLevel::Error,
              "ProcessDeletedObjects: No such object with id {} exists", toDeleteId.id);
            continue;
        }

        auto& renderableObject = m_objects[toDeleteId.id - 1];

        if (!renderableObject.boundingBox_worldSpace.IsEmpty())
        {
            if (renderableObject.renderable.shadowPass)
            {
                modifiedShadowWorldAreas.boundingBoxes_worldSpace.push_back(renderableObject.boundingBox_worldSpace);
            }
        }

        renderableObject.isValid = false;

        m_objectsRTree[renderableObject.renderable.sceneName].Remove(
            renderableObject.boundingBox_worldSpace.GetVolume(),
            toDeleteId.id
        );

        m_ids->objectIds.ReturnId(toDeleteId);
    }
}

ObjectPayload ObjectRenderables::ObjectToPayload(const ObjectRenderable& object)
{
    ObjectPayload payload{};
    payload.modelTransform = object.modelTransform;

    return payload;
}

std::expected<AABB, bool> ObjectRenderables::GetObjectAABB(const ObjectRenderable& object) const
{
    const auto meshOpt = m_meshes->GetLoadedMesh(object.meshId);
    if (!meshOpt)
    {
        return std::unexpected(false);
    }

    auto objectModelSpaceAABB = meshOpt->boundingBox_modelSpace;
    const auto originalObjectModelSpaceVolume = objectModelSpaceAABB.GetVolume();

    //
    // If the mesh has bone transforms, expand the bounds of the mesh's AABB by the effect
    // the transforms apply, so that the AABB fully covers the mesh's vertex positions after
    // bone transforms are applied
    //
    if (object.boneTransforms)
    {
        for (const auto& boneTransform : *object.boneTransforms)
        {
            objectModelSpaceAABB.AddPoints({
               boneTransform * glm::vec4(originalObjectModelSpaceVolume.min, 1),
               boneTransform * glm::vec4(originalObjectModelSpaceVolume.max, 1)
            });
        }
    }

    //
    // Convert the object model space AABB to world space by transforming its points by
    // the object's transform
    //
    const auto objectWorldSpaceAABB = AABB(std::vector<glm::vec3>{
        object.modelTransform * glm::vec4(objectModelSpaceAABB.GetVolume().min, 1),
        object.modelTransform * glm::vec4(objectModelSpaceAABB.GetVolume().max, 1)
    });

    return objectWorldSpaceAABB;
}

const ObjectsRTree& ObjectRenderables::GetDataRTree(const std::string& sceneName) const noexcept
{
    return m_objectsRTree.at(sceneName);
}

[[nodiscard]] std::vector<ObjectRenderable> ObjectRenderables::GetVisibleObjects(const std::string& sceneName,
                                                                                 const Volume& volume) const
{
    //
    // Query the objects r-tree for the ids of the objects within the specified volume of space
    //
    const auto sceneRenderableIdsInVolume = m_objectsRTree.at(sceneName).FetchMatching(volume);

    //
    // Transform the list of volume objects ids to renderables by accessing the objects list
    //
    std::vector<ObjectRenderable> visibleRenderables;
    visibleRenderables.reserve(sceneRenderableIdsInVolume.size());

    for (const auto& id : sceneRenderableIdsInVolume)
    {
        const auto& renderableObject = m_objects[id.id - 1];

        // Filter out renderables that have been deleted / are invalid
        if (!renderableObject.isValid)
        {
            continue;
        }

        visibleRenderables.push_back(renderableObject.renderable);
    }

    return visibleRenderables;
}

}
