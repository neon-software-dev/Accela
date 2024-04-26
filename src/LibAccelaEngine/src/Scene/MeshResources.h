/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_MESHRESOURCES_H
#define LIBACCELAENGINE_SRC_SCENE_MESHRESOURCES_H

#include "HeightMapData.h"

#include <Accela/Engine/Scene/IMeshResources.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <string>

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
    class IEngineAssets;
    class ITextureResources;

    class MeshResources : public IMeshResources
    {
        public:

            //
            // IMeshResources
            //

            MeshResources(Common::ILogger::Ptr logger,
                          std::shared_ptr<ITextureResources> textures,
                          std::shared_ptr<Render::IRenderer> renderer,
                          std::shared_ptr<IEngineAssets> assets,
                          std::shared_ptr<Platform::IFiles> files,
                          std::shared_ptr<Common::MessageDrivenThreadPool> threadPool);

            [[nodiscard]] std::future<Render::MeshId> LoadStaticMesh(
                const std::vector<Render::MeshVertex>& vertices,
                const std::vector<uint32_t>& indices,
                Render::MeshUsage usage,
                const std::string& tag,
                ResultWhen resultWhen
            ) override;

            [[nodiscard]] std::future<Render::MeshId> LoadHeightMapMesh(
                const Render::TextureId& heightMapTextureId,
                const Render::USize& heightMapDataSize,
                const Render::USize& meshSize_worldSpace,
                const float& displacementFactor,
                Render::MeshUsage usage,
                const std::string& tag,
                ResultWhen resultWhen
            ) override;

            void DestroyMesh(const Render::MeshId& meshId) override;

            void DestroyAll() override;

            //
            // Other
            //

            [[nodiscard]] std::optional<HeightMapData::Ptr> GetHeightMapData(const Render::MeshId& meshId) const;

        private:

            [[nodiscard]] Render::MeshId OnLoadStaticMesh(
                const std::vector<Render::MeshVertex>& vertices,
                const std::vector<uint32_t>& indices,
                Render::MeshUsage usage,
                const std::string& tag,
                ResultWhen resultWhen
            );

            [[nodiscard]] Render::MeshId OnLoadHeightMapMesh(
                const Render::TextureId& heightMapTextureId,
                const Render::USize& heightMapDataSize,
                const Render::USize& meshSize_worldSpace,
                const float& displacementFactor,
                Render::MeshUsage usage,
                const std::string& tag,
                ResultWhen resultWhen
            );

        private:

            [[nodiscard]] Render::MeshId LoadMesh(
                const Render::Mesh::Ptr& mesh,
                Render::MeshUsage usage,
                ResultWhen resultWhen
            );

        private:

            Common::ILogger::Ptr m_logger;
            std::shared_ptr<ITextureResources> m_textures;
            std::shared_ptr<Render::IRenderer> m_renderer;
            std::shared_ptr<IEngineAssets> m_assets;
            std::shared_ptr<Platform::IFiles> m_files;
            std::shared_ptr<Common::MessageDrivenThreadPool> m_threadPool;

            mutable std::mutex m_meshesMutex;
            std::unordered_set<Render::MeshId> m_meshes; // Ids of loaded meshes

            mutable std::mutex m_heightMapDataMutex;
            std::unordered_map<Render::MeshId, HeightMapData::Ptr> m_heightMapData; // Extra data stored for height map meshes
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_MESHRESOURCES_H
