#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MESH_BONEMESH_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MESH_BONEMESH_H

#include "Mesh.h"
#include "BoneMeshVertex.h"

namespace Accela::Render
{
    /**
     * A mesh with a bone-based skeleton
     */
    struct BoneMesh : public Mesh
    {
        BoneMesh(MeshId meshId,
                 std::vector<BoneMeshVertex> _vertices,
                 std::vector<uint32_t> _indices,
                 uint32_t _numBones,
                 std::string _tag)
            : Mesh(MeshType::Bone, meshId, std::move(_tag))
            , vertices(std::move(_vertices))
            , indices(std::move(_indices))
            , numBones(_numBones)
        { }

        std::vector<BoneMeshVertex> vertices;
        std::vector<uint32_t> indices;
        uint32_t numBones;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_MESH_BONEMESH_H
