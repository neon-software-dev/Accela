#include "Renderables.h"

namespace Accela::Render
{

Renderables::Renderables(
    Common::ILogger::Ptr logger,
    Ids::Ptr ids,
    PostExecutionOpsPtr postExecutionOps,
    ITexturesPtr textures,
    IBuffersPtr buffers,
    IMeshesPtr meshes,
    ILightsPtr lights)
    : m_logger(std::move(logger))
    , m_ids(std::move(ids))
    , m_postExecutionOps(std::move(postExecutionOps))
    , m_textures(std::move(textures))
    , m_buffers(std::move(buffers))
    , m_meshes(std::move(meshes))
    , m_lights(std::move(lights))
    , m_sprites(m_logger, m_ids, m_postExecutionOps, m_buffers, m_textures)
    , m_objects(m_logger, m_ids, m_postExecutionOps, m_buffers, m_textures, m_meshes, m_lights)
    , m_terrain(m_logger, m_ids, m_postExecutionOps, m_buffers, m_textures)
{

}

bool Renderables::Initialize()
{
    m_logger->Log(Common::LogLevel::Info, "Renderables: Initializing");

    if (!m_sprites.Initialize()) { return false; }
    if (!m_objects.Initialize()) { return false; }
    if (!m_terrain.Initialize()) { return false; }

    return true;
}

void Renderables::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "Renderables: Destroying");

    m_terrain.Destroy();
    m_objects.Destroy();
    m_sprites.Destroy();
}

void Renderables::ProcessUpdate(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence)
{
    m_sprites.ProcessUpdate(update, commandBuffer, vkFence);
    m_objects.ProcessUpdate(update, commandBuffer, vkFence);
    m_terrain.ProcessUpdate(update, commandBuffer, vkFence);
}

}
