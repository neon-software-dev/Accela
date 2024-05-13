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

            /**
             * Load a static mesh that can be used when rendering
             *
             * @param vertices The vertices that define the mesh
             * @param indices The indices that define the vertices to be drawn
             * @param usage Usage pattern for the mesh
             * @param tag Debug tag to associate with the mesh
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future that's signaled with the MeshId when the operation has finished
             */
            [[nodiscard]] virtual std::future<Render::MeshId> LoadStaticMesh(
                const std::vector<Render::MeshVertex>& vertices,
                const std::vector<uint32_t>& indices,
                Render::MeshUsage usage,
                const std::string& tag,
                ResultWhen resultWhen
            ) = 0;

            /**
             * Load a mesh generated from a height map texture
             *
             * @param heightMapTextureId The textureId of the (previously loaded) height map texture
             * @param heightMapDataSize The number of data points, width x height, that should be pulled from the height map
             * @param meshSize_worldSpace The world-space size of the mesh that will be created
             * @param displacementFactor Constant multiplier against texture values to determine height displacement of vertices
             * @param usage Usage pattern for the mesh
             * @param tag Debug tag to associate with the mesh
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future that's signaled with the MeshId when the operation has finished
             */
            [[nodiscard]] virtual std::future<Render::MeshId> LoadHeightMapMesh(
                const Render::TextureId& heightMapTextureId,
                const Render::USize& heightMapDataSize,
                const Render::USize& meshSize_worldSpace,
                const float& displacementFactor,
                Render::MeshUsage usage,
                const std::string& tag,
                ResultWhen resultWhen
            ) = 0;

            /**
            * Load a mesh generated from a height map image
            *
            * @param heightMapImage The image containing the height map data
            * @param heightMapDataSize The number of data points, width x height, that should be pulled from the height map
            * @param meshSize_worldSpace The world-space size of the mesh that will be created
            * @param displacementFactor Constant multiplier against texture values to determine height displacement of vertices
            * @param usage Usage pattern for the mesh
            * @param tag Debug tag to associate with the mesh
            * @param resultWhen At which point of the load the returned future should be signaled
            *
            * @return A future that's signaled with the MeshId when the operation has finished
            */
            [[nodiscard]] virtual std::future<Render::MeshId> LoadHeightMapMesh(
                const Common::ImageData::Ptr& heightMapImage,
                const Render::USize& heightMapDataSize,
                const Render::USize& meshSize_worldSpace,
                const float& displacementFactor,
                Render::MeshUsage usage,
                const std::string& tag,
                ResultWhen resultWhen
            ) = 0;

            /**
             * Destroy a previously loaded mesh
             *
             * @param meshId The MeshId of the mesh to destroy
             */
            virtual void DestroyMesh(const Render::MeshId& meshId) = 0;

            /**
             * Destroy all previously loaded meshes
             */
            virtual void DestroyAll() = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMESHRESOURCES_H
