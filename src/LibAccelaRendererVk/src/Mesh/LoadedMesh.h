#ifndef LIBACCELARENDERERVK_SRC_MESH_LOADEDMESH_H
#define LIBACCELARENDERERVK_SRC_MESH_LOADEDMESH_H

#include "../ForwardDeclares.h"

#include "../Util/AABB.h"

#include <Accela/Render/Id.h>

#include <Accela/Render/Mesh/Mesh.h>

#include <cstdint>
#include <optional>

namespace Accela::Render
{
    struct LoadedMesh
    {
        MeshId id{INVALID_ID};
        MeshType meshType{};
        MeshUsage usage{};

        DataBufferPtr verticesBuffer;       // The buffer which contains the mesh's vertex data
        std::size_t numVertices{0};         // Number of vertices in the mesh
        std::size_t verticesByteOffset{0};  // Byte offset into verticesBuffer until the mesh's vertices start
        std::size_t verticesOffset{0};      // Vertex offset into verticesBuffer until the mesh's vertices start
        std::size_t verticesByteSize{0};    // Total byte-size of the mesh's vertex data (not of the buffer containing it)

        DataBufferPtr indicesBuffer;        // The buffer which contains the mesh's index data
        std::size_t numIndices{0};          // Number of indices in the mesh
        std::size_t indicesByteOffset{0};   // Byte offset into indicesBuffer until the mesh's indices start
        std::size_t indicesOffset{0};       // Index offset into indicesBuffer until the mesh's indices start
        std::size_t indicesByteSize{0};     // Total byte-size of the mesh's index data (not of the buffer containing it)

        std::optional<DataBufferPtr> dataBuffer;    // The buffer which contains the mesh's (optional) data payload
        std::size_t dataByteOffset{0};              // Byte offset into dataBuffer until the mesh's payload data starts
        std::size_t dataByteSize{0};                // Total byte-size of the mesh's payload data (not of the buffer containing it)

        AABB boundingBox_modelSpace{};      // The mesh's axially-aligned, model space, bounding box
    };
}

#endif //LIBACCELARENDERERVK_SRC_MESH_LOADEDMESH_H
