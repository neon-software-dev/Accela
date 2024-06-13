/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "MeshResources.h"

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
                             ITextureResourcesPtr textures,
                             std::shared_ptr<Render::IRenderer> renderer,
                             std::shared_ptr<Platform::IFiles> files,
                             std::shared_ptr<Common::MessageDrivenThreadPool> threadPool)
    : m_logger(std::move(logger))
    , m_textures(std::move(textures))
    , m_renderer(std::move(renderer))
    , m_files(std::move(files))
    , m_threadPool(std::move(threadPool))
{

}

std::future<Render::MeshId> MeshResources::LoadStaticMesh(const CustomResourceIdentifier& resource,
                                                          const std::vector<Render::MeshVertex>& vertices,
                                                          const std::vector<uint32_t>& indices,
                                                          Render::MeshUsage usage,
                                                          ResultWhen resultWhen)
{
    auto message = std::make_shared<MeshResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<MeshResultMessage>(_message)->SetResult(
            OnLoadStaticMesh(resource, vertices, indices, usage, resultWhen)
        );
    });

    return messageFuture;
}

std::future<Render::MeshId> MeshResources::LoadHeightMapMesh(const CustomResourceIdentifier& resource,
                                                             const Render::TextureId& heightMapTextureId,
                                                             const Render::USize& heightMapDataSize,
                                                             const Render::USize& meshSize_worldSpace,
                                                             const float& displacementFactor,
                                                             Render::MeshUsage usage,
                                                             ResultWhen resultWhen)
{
    auto message = std::make_shared<MeshResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=,this](const Common::Message::Ptr& _message){
        std::dynamic_pointer_cast<MeshResultMessage>(_message)->SetResult(
            OnLoadHeightMapMesh(resource, heightMapTextureId, heightMapDataSize, meshSize_worldSpace, displacementFactor, usage, resultWhen)
        );
    });

    return messageFuture;
}

std::future<Render::MeshId> MeshResources::LoadHeightMapMesh(const CustomResourceIdentifier& resource,
                                                             const Common::ImageData::Ptr& heightMapImage,
                                                             const Render::USize& heightMapDataSize,
                                                             const Render::USize& meshSize_worldSpace,
                                                             const float& displacementFactor,
                                                             Render::MeshUsage usage,
                                                             ResultWhen resultWhen)
{
    auto message = std::make_shared<MeshResultMessage>();
    auto messageFuture = message->CreateFuture();

    m_threadPool->PostMessage(message, [=, this](const Common::Message::Ptr& _message) {
        std::dynamic_pointer_cast<MeshResultMessage>(_message)->SetResult(
            OnLoadHeightMapMesh(resource, heightMapImage, heightMapDataSize, meshSize_worldSpace, displacementFactor, usage, resultWhen)
        );
    });

    return messageFuture;
}

Render::MeshId MeshResources::OnLoadStaticMesh(const CustomResourceIdentifier& resource,
                                               const std::vector<Render::MeshVertex>& vertices,
                                               const std::vector<uint32_t>& indices,
                                               Render::MeshUsage usage,
                                               ResultWhen resultWhen)
{
    m_logger->Log(Common::LogLevel::Info, "MeshResources: Loading static mesh resource: {}", resource.GetUniqueName());

    const auto mesh = std::make_shared<Render::StaticMesh>(
        m_renderer->GetIds()->meshIds.GetId(),
        vertices,
        indices,
        resource.GetUniqueName()
    );

    // Record the mesh's data before moving on with the mesh loading process
    {
        std::lock_guard<std::mutex> dataLock(m_staticMeshDataMutex);
        m_staticMeshData.insert({resource, std::make_shared<RegisteredStaticMesh>(vertices, indices)});
    }

    return LoadMesh(resource, mesh, usage, resultWhen);
}

Render::MeshId MeshResources::OnLoadHeightMapMesh(const CustomResourceIdentifier& resource,
                                                  const Render::TextureId& heightMapTextureId,
                                                  const Render::USize& heightMapDataSize,
                                                  const Render::USize& meshSize_worldSpace,
                                                  const float& displacementFactor,
                                                  Render::MeshUsage usage,
                                                  ResultWhen resultWhen)
{
    m_logger->Log(Common::LogLevel::Info, "MeshResources: Loading height map mesh resource: {}", resource.GetUniqueName());

    //
    // Fetch the texture's data
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
    // Load the height map mesh from the texture's image data
    //
    return OnLoadHeightMapMesh(
        resource,
        heightMapTextureOpt->data.value(),
        heightMapDataSize,
        meshSize_worldSpace,
        displacementFactor,
        usage,
        resultWhen
    );
}

Render::MeshId MeshResources::OnLoadHeightMapMesh(const CustomResourceIdentifier& resource,
                                                  const Common::ImageData::Ptr& heightMapImage,
                                                  const Render::USize& heightMapDataSize,
                                                  const Render::USize& meshSize_worldSpace,
                                                  const float& displacementFactor,
                                                  Render::MeshUsage usage,
                                                  ResultWhen resultWhen)
{
    //
    // Parse the image data to generate height map data
    //
    const auto heightMapData = GenerateHeightMapData(heightMapImage, heightMapDataSize, meshSize_worldSpace, displacementFactor);

    //
    // Transform the height map data points into a mesh
    //
    const auto heightMapMesh = GenerateHeightMapMesh(
        m_renderer->GetIds()->meshIds.GetId(),
        *heightMapData,
        meshSize_worldSpace,
        resource.GetUniqueName()
    );

    // Record the height map's data before moving on with the mesh loading process
    {
        std::lock_guard<std::mutex> meshesLock(m_heightMapDataMutex);
        m_heightMapData.insert({resource, heightMapData});
    }

    return LoadMesh(resource, heightMapMesh, usage, resultWhen);
}

Render::MeshId MeshResources::LoadMesh(const CustomResourceIdentifier& resource,
                                       const Render::Mesh::Ptr& mesh,
                                       Render::MeshUsage usage,
                                       ResultWhen resultWhen)
{
    const auto meshId = GetMeshId(resource);
    if (meshId)
    {
        m_logger->Log(Common::LogLevel::Warning,
          "MeshResources::LoadMesh: Mesh was already loaded, ignoring: {}", resource.GetUniqueName());
        return *meshId;
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

        m_meshes.erase(resource);
        m_heightMapData.erase(resource); // May or may not exist

        return {Render::INVALID_ID};
    }

    //
    // Record a record of the created mesh
    //
    std::lock_guard<std::mutex> meshesLock(m_meshesMutex);

    m_meshes.insert({resource, mesh->id});

    return mesh->id;
}

std::optional<Render::MeshId> MeshResources::GetMeshId(const ResourceIdentifier& resource) const
{
    std::lock_guard<std::mutex> meshesLock(m_meshesMutex);

    const auto it = m_meshes.find(resource);
    if (it == m_meshes.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

std::optional<HeightMapData::Ptr> MeshResources::GetHeightMapData(const ResourceIdentifier& resource) const
{
    std::lock_guard<std::mutex> heightMapDataLock(m_heightMapDataMutex);

    const auto it = m_heightMapData.find(resource);
    if (it == m_heightMapData.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

void MeshResources::DestroyMesh(const ResourceIdentifier& resource)
{
    m_logger->Log(Common::LogLevel::Info, "MeshResources: Destroying mesh resource: {}", resource.GetUniqueName());

    std::lock_guard<std::mutex> meshesLock(m_meshesMutex);
    std::lock_guard<std::mutex> staticMeshDataLock(m_staticMeshDataMutex);
    std::lock_guard<std::mutex> heightMapDataLock(m_heightMapDataMutex);

    const auto it = m_meshes.find(resource);
    if (it == m_meshes.cend())
    {
        return;
    }

    m_renderer->DestroyMesh(it->second);

    m_meshes.erase(resource);
    m_staticMeshData.erase(resource);
    m_heightMapData.erase(resource); // May or may not exist
}

void MeshResources::DestroyAll()
{
    m_logger->Log(Common::LogLevel::Info, "MeshResources: Destroying all mesh resources");

    while (!m_meshes.empty())
    {
        DestroyMesh(m_meshes.cbegin()->first);
    }
}

std::optional<RegisteredStaticMesh::Ptr> MeshResources::GetStaticMeshData(const ResourceIdentifier& resource) const
{
    std::lock_guard<std::mutex> dataLock(m_staticMeshDataMutex);

    const auto it = m_staticMeshData.find(resource);
    if (it == m_staticMeshData.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

}
