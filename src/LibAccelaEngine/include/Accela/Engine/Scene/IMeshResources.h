#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMESHRESOURCES_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMESHRESOURCES_H

#include <Accela/Engine/ResourceIdentifier.h>

#include <Accela/Render/Id.h>
#include <Accela/Render/Mesh/Mesh.h>
#include <Accela/Render/Mesh/MeshVertex.h>

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
             * Load a custom static mesh resource
             *
             * @param resource Identifies the mesh resource
             * @param vertices The vertices that define the mesh
             * @param indices The indices that define the vertices to be drawn
             * @param usage Usage pattern for the mesh
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future that's signaled with the MeshId when the operation has finished
             */
            [[nodiscard]] virtual std::future<Render::MeshId> LoadStaticMesh(
                const CustomResourceIdentifier& resource,
                const std::vector<Render::MeshVertex>& vertices,
                const std::vector<uint32_t>& indices,
                Render::MeshUsage usage,
                ResultWhen resultWhen
            ) = 0;

            /**
             * Load a custom mesh resource generated from a height map texture
             *
             * @param resource Identifies the mesh resource
             * @param heightMapTextureId The textureId of the (previously loaded) height map texture
             * @param heightMapDataSize The number of data points, width x height, that should be pulled from the height map
             * @param meshSize_worldSpace The world-space size of the mesh that will be created
             * @param displacementFactor Constant multiplier against texture values to determine height displacement of vertices
             * @param usage Usage pattern for the mesh
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future that's signaled with the MeshId when the operation has finished
             */
            [[nodiscard]] virtual std::future<Render::MeshId> LoadHeightMapMesh(
                const CustomResourceIdentifier& resource,
                const Render::TextureId& heightMapTextureId,
                const Render::USize& heightMapDataSize,
                const Render::USize& meshSize_worldSpace,
                const float& displacementFactor,
                Render::MeshUsage usage,
                ResultWhen resultWhen
            ) = 0;

            /**
             * Load a custom mesh resource generated from a height map image
             *
             * @param resource Identifies the mesh resource
             * @param heightMapImage The image containing the height map data
             * @param heightMapDataSize The number of data points, width x height, that should be pulled from the height map
             * @param meshSize_worldSpace The world-space size of the mesh that will be created
             * @param displacementFactor Constant multiplier against texture values to determine height displacement of vertices
             * @param usage Usage pattern for the mesh
             * @param resultWhen At which point of the load the returned future should be signaled
             *
             * @return A future that's signaled with the MeshId when the operation has finished
             */
            [[nodiscard]] virtual std::future<Render::MeshId> LoadHeightMapMesh(
                const CustomResourceIdentifier& resource,
                const Common::ImageData::Ptr& heightMapImage,
                const Render::USize& heightMapDataSize,
                const Render::USize& meshSize_worldSpace,
                const float& displacementFactor,
                Render::MeshUsage usage,
                ResultWhen resultWhen
            ) = 0;

            /**
              * Returns the MeshId associated with a previously loaded mesh resource
              *
              * @param resource Identifies the mesh resource
              *
              * @return The loaded MeshId, or std::nullopt if no such material
              */
            virtual std::optional<Render::MeshId> GetMeshId(const ResourceIdentifier& resource) const = 0;

            /**
             * Destroy a previously loaded mesh
             *
             * @param resource The resource to be destroyed
             */
            virtual void DestroyMesh(const ResourceIdentifier& resource) = 0;

            /**
             * Destroy all previously loaded meshes
             */
            virtual void DestroyAll() = 0;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_IMESHRESOURCES_H
