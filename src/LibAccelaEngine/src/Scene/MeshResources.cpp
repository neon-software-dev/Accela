/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "MeshResources.h"

#include <Accela/Engine/IEngineAssets.h>
#include <Accela/Engine/Scene/ITextureResources.h>

#include <Accela/Platform/File/IFiles.h>

#include <Accela/Render/IRenderer.h>
#include <Accela/Render/Mesh/StaticMesh.h>

#include <Accela/Common/Thread/ResultMessage.h>

namespace Accela::Engine
{

struct MeshResultMessage : public Common::ResultMessage<Render::MeshId>
{
    MeshResultMessage()
        : Common::ResultMessage<Render::MeshId>("MeshResultMessage")
    { }
};

MeshResources::MeshResources(Common::ILogger::Ptr logger,
                             std::shared_ptr<ITextureResources> textures,
                             std::shared_ptr<Render::IRenderer> renderer,
                             std::shared_ptr<IEngineAssets> assets,
                             std::shared_ptr<Platform::IFiles> files,
                             std::shared_ptr<Common::MessageDrivenThreadPool> threadPool)
    : m_logger(std::move(logger))
    , m_textures(std::move(textures))
    , m_renderer(std::move(renderer))
    , m_assets(std::move(assets))
    , m_files(std::move(files))
    , m_threadPool(std::move(threadPool))
{

}

std::future<Render::MeshId> MeshResources::LoadStaticMesh(const std::vector<Render::MeshVertex>& vertices,
                                                          const std::vector<uint32_t>& indices,
                                                          Render::MeshUsage usage,
                                                          const std::string& tag,
                                                          ResultWhen resultWhen)
{
    auto message = std::make_shared<MeshResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& message){
        std::dynamic_pointer_cast<MeshResultMessage>(message)->SetResult(
            OnLoadStaticMesh(vertices, indices, usage, tag, resultWhen)
        );
    });

    return messageFuture;
}

std::future<Render::MeshId> MeshResources::LoadHeightMapMesh(const Render::TextureId& heightMapTextureId,
                                                             const Render::USize& heightMapDataSize,
                                                             const Render::USize& meshSize_worldSpace,
                                                             const float& displacementFactor,
                                                             Render::MeshUsage usage,
                                                             const std::string& tag,
                                                             ResultWhen resultWhen)
{
    auto message = std::make_shared<MeshResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& message){
        std::dynamic_pointer_cast<MeshResultMessage>(message)->SetResult(
            OnLoadHeightMapMesh(heightMapTextureId, heightMapDataSize, meshSize_worldSpace, displacementFactor, usage, tag, resultWhen)
        );
    });

    return messageFuture;
}


Render::MeshId MeshResources::OnLoadStaticMesh(const std::vector<Render::MeshVertex>& vertices,
                                               const std::vector<uint32_t>& indices,
                                               Render::MeshUsage usage,
                                               const std::string& tag,
                                               ResultWhen resultWhen)
{
    m_logger->Log(Common::LogLevel::Info, "MeshResources: Loading static mesh: {}", tag);

    const auto mesh = std::make_shared<Render::StaticMesh>(
        m_renderer->GetIds()->meshIds.GetId(),
        vertices,
        indices,
        tag
    );

    return LoadMesh(mesh, usage, resultWhen);
}

Render::MeshId MeshResources::OnLoadHeightMapMesh(const Render::TextureId& heightMapTextureId,
                                                  const Render::USize& heightMapDataSize,
                                                  const Render::USize& meshSize_worldSpace,
                                                  const float& displacementFactor,
                                                  Render::MeshUsage usage,
                                                  const std::string& tag,
                                                  ResultWhen resultWhen)
{
    m_logger->Log(Common::LogLevel::Info, "MeshResources: Loading height map mesh: {}", tag);

    //
    // Setup
    //
    const auto heightMapTextureOpt = m_textures->GetLoadedTextureData(heightMapTextureId);
    if (!heightMapTextureOpt)
    {
        m_logger->Log(Common::LogLevel::Error,
          "OnLoadHeightMapMesh: No such texture is registered, id: {}", heightMapTextureId.id);
        return {Render::INVALID_ID};
    }

    if (!heightMapTextureOpt->data.has_value())
    {
        m_logger->Log(Common::LogLevel::Error,
          "OnLoadHeightMapMesh: Texture has no image data, not a valid height map, id: {}", heightMapTextureId.id);
        return {Render::INVALID_ID};
    }

    //
    // Parse the texture's data to generate height map data
    //
    const auto heightMapData = GenerateHeightMapData(heightMapTextureOpt->data.value(), heightMapDataSize, meshSize_worldSpace, displacementFactor);

    //
    // Transform the height map data points into a mesh
    //
    const auto heightMapMesh = GenerateHeightMapMesh(
        m_renderer->GetIds()->meshIds.GetId(),
        *heightMapData,
        meshSize_worldSpace,
        tag
    );

    // Record the height map's data before moving on with the mesh loading process
    {
        std::lock_guard<std::mutex> meshesLock(m_heightMapDataMutex);
        m_heightMapData.insert({heightMapMesh->id, heightMapData});
    }

    return LoadMesh(heightMapMesh, usage, resultWhen);
}

Render::MeshId MeshResources::LoadMesh( const Render::Mesh::Ptr& mesh,
                                       Render::MeshUsage usage,
                                       ResultWhen resultWhen)
{
    if (m_meshes.contains(mesh->id))
    {
        m_logger->Log(Common::LogLevel::Error,
          "MeshResources::LoadMesh: Mesh already existed, id: {}", mesh->id.id);
        return Render::INVALID_ID;
    }

    //
    // Tell the renderer to create the mesh
    //
    std::future<bool> opFuture = m_renderer->CreateMesh(mesh, usage);

    if (resultWhen == ResultWhen::FullyLoaded && !opFuture.get())
    {
        // The mesh creation failed
        std::lock_guard<std::mutex> meshesLock(m_meshesMutex);
        std::lock_guard<std::mutex> heightMapDataLock(m_heightMapDataMutex);

        m_renderer->GetIds()->meshIds.ReturnId(mesh->id);
        m_meshes.erase(mesh->id);
        m_heightMapData.erase(mesh->id); // May or may not exist

        return {Render::INVALID_ID};
    }

    //
    // Record a record of the created mesh
    //
    std::lock_guard<std::mutex> meshesLock(m_meshesMutex);

    m_meshes.insert(mesh->id);

    return mesh->id;
}

void MeshResources::DestroyMesh(const Render::MeshId& meshId)
{
    if (!meshId.IsValid()) { return; }

    m_logger->Log(Common::LogLevel::Debug, "MeshResources::DestroyMesh: Destroying mesh, id: {}", meshId.id);

    std::lock_guard<std::mutex> meshesLock(m_meshesMutex);
    std::lock_guard<std::mutex> heightMapDataLock(m_heightMapDataMutex);

    if (!m_meshes.contains(meshId))
    {
        return;
    }

    m_renderer->DestroyMesh(meshId);

    m_meshes.erase(meshId);
    m_heightMapData.erase(meshId); // May or may not exist
}

void MeshResources::DestroyAll()
{
    m_logger->Log(Common::LogLevel::Info, "MeshResources::DestroyAll: Destroying all meshes");

    while (!m_meshes.empty())
    {
        DestroyMesh(*m_meshes.cbegin());
    }
}

std::optional<HeightMapData::Ptr> MeshResources::GetHeightMapData(const Render::MeshId& meshId) const
{
    std::lock_guard<std::mutex> heightMapDataLock(m_heightMapDataMutex);

    const auto it = m_heightMapData.find(meshId);
    if (it == m_heightMapData.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

}
