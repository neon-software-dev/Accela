/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_MESHRESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_MESHRESOURCES_H

#include "../ForwardDeclares.h"

#include "HeightMapData.h"
#include "RegisteredStaticMesh.h"

#include <Accela/Engine/Scene/IMeshResources.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <mutex>
#include <unordered_map>

namespace Accela::Platform
{
    class IFiles;
}

namespace Accela::Render
{
    class IRenderer;
}

namespace Accela::Engine
{
    class MeshResources : public IMeshResources
    {
        public:

            MeshResources(Common::ILogger::Ptr logger,
                          ITextureResourcesPtr textures,
                          std::shared_ptr<Render::IRenderer> renderer,
                          std::shared_ptr<Platform::IFiles> files,
                          std::shared_ptr<Common::MessageDrivenThreadPool> threadPool);

            //
            // IMeshResources
            //
            [[nodiscard]] std::future<Render::MeshId> LoadStaticMesh(
                const CustomResourceIdentifier& resource,
                const std::vector<Render::MeshVertex>& vertices,
                const std::vector<uint32_t>& indices,
                Render::MeshUsage usage,
                ResultWhen resultWhen
            ) override;

            [[nodiscard]] std::future<Render::MeshId> LoadHeightMapMesh(
                const CustomResourceIdentifier& resource,
                const Render::TextureId& heightMapTextureId,
                const Render::USize& heightMapDataSize,
                const Render::USize& meshSize_worldSpace,
                const float& displacementFactor,
                Render::MeshUsage usage,
                ResultWhen resultWhen
            ) override;

            [[nodiscard]] std::future<Render::MeshId> LoadHeightMapMesh(
                const CustomResourceIdentifier& resource,
                const Common::ImageData::Ptr& heightMapImage,
                const Render::USize& heightMapDataSize,
                const Render::USize& meshSize_worldSpace,
                const float& displacementFactor,
                Render::MeshUsage usage,
                ResultWhen resultWhen
            ) override;

            std::optional<Render::MeshId> GetMeshId(const ResourceIdentifier& resource) const override;

            void DestroyMesh(const ResourceIdentifier& resource) override;

            void DestroyAll() override;

            //
            // Internal
            //
            [[nodiscard]] std::optional<RegisteredStaticMesh::Ptr> GetStaticMeshData(const ResourceIdentifier& resource) const;
            [[nodiscard]] std::optional<HeightMapData::Ptr> GetHeightMapData(const ResourceIdentifier& resource) const;

        private:

            [[nodiscard]] Render::MeshId OnLoadStaticMesh(
                const CustomResourceIdentifier& resource,
                const std::vector<Render::MeshVertex>& vertices,
                const std::vector<uint32_t>& indices,
                Render::MeshUsage usage,
                ResultWhen resultWhen
            );

            [[nodiscard]] Render::MeshId OnLoadHeightMapMesh(
                const CustomResourceIdentifier& resource,
                const Render::TextureId& heightMapTextureId,
                const Render::USize& heightMapDataSize,
                const Render::USize& meshSize_worldSpace,
                const float& displacementFactor,
                Render::MeshUsage usage,
                ResultWhen resultWhen
            );

            [[nodiscard]] Render::MeshId OnLoadHeightMapMesh(
                const CustomResourceIdentifier& resource,
                const Common::ImageData::Ptr& heightMapImage,
                const Render::USize& heightMapDataSize,
                const Render::USize& meshSize_worldSpace,
                const float& displacementFactor,
                Render::MeshUsage usage,
                ResultWhen resultWhen
            );

            [[nodiscard]] Render::MeshId LoadMesh(
                const CustomResourceIdentifier& resource,
                const Render::Mesh::Ptr& mesh,
                Render::MeshUsage usage,
                ResultWhen resultWhen
            );

        private:

            Common::ILogger::Ptr m_logger;
            ITextureResourcesPtr m_textures;
            std::shared_ptr<Render::IRenderer> m_renderer;
            std::shared_ptr<Platform::IFiles> m_files;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;

            mutable std::mutex m_meshesMutex;
            std::unordered_map<ResourceIdentifier, Render::MeshId> m_meshes; // Ids of all loaded meshes

            mutable std::mutex m_staticMeshDataMutex;
            std::unordered_map<ResourceIdentifier, RegisteredStaticMesh::Ptr> m_staticMeshData; // Data stored for static meshes

            mutable std::mutex m_heightMapDataMutex;
            std::unordered_map<ResourceIdentifier, HeightMapData::Ptr> m_heightMapData; // Data stored for height map meshes
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_MESHRESOURCES_H
