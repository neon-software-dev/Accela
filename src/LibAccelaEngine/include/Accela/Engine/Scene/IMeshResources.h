/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMESHRESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMESHRESOURCES_H

#include <Accela/Engine/Common.h>

#include <Accela/Render/Id.h>
#include <Accela/Render/Mesh/Mesh.h>
#include <Accela/Render/Mesh/MeshVertex.h>
#include <Accela/Render/Util/Rect.h>

#include <expected>
#include <string>
#include <memory>
#include <future>
#include <vector>
#include <optional>

namespace Accela::Engine
{
    /**
     * Encapsulates mesh resource operations
     */
    class IMeshResources
    {
        public:

            using Ptr = std::shared_ptr<IMeshResources>;

        public:

            virtual ~IMeshResources() = default;

            [[nodiscard]] virtual std::future<Render::MeshId> LoadStaticMesh(
                const std::vector<Render::MeshVertex>& vertices,
                const std::vector<uint32_t>& indices,
                Render::MeshUsage usage,
                const std::string& tag,
                ResultWhen resultWhen
            ) = 0;

            [[nodiscard]] virtual std::future<Render::MeshId> LoadHeightMapMesh(
                const Render::TextureId& heightMapTextureId,
                const Render::USize& heightMapDataSize,
                const Render::USize& meshSize_worldSpace,
                const float& displacementFactor,
                Render::MeshUsage usage,
                const std::string& tag,
                ResultWhen resultWhen
            ) = 0;

            virtual void DestroyMesh(const Render::MeshId& meshId) = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMESHRESOURCES_H
