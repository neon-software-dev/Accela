/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_UTIL_HEIGHTMAPUTIL_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_UTIL_HEIGHTMAPUTIL_H

#include <Accela/Engine/Scene/LoadedHeightMap.h>
#include <Accela/Engine/Scene/LoadedStaticMesh.h>

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <optional>

namespace Accela::Engine
{
    /**
     * Result returned when calling QueryLoadedHeightMap for a given height map model-space point
     */
    struct ACCELA_PUBLIC HeightMapQueryResult
    {
        // The model-space height at the queried point
        float pointHeight_modelSpace{0.0f};

        // The model-space normal unit at the queried point
        glm::vec3 pointNormalUnit_modelSpace{0.0f};
    };

    /**
     * Query a loaded height map for the model-space height and normal associated with a specific model point.
     *
     * modelSpacePoint is in the model-space of the mesh - as in, [-halfMeshWidth/Height, halfMeshWidth/Height],
     * in the usual right-handed 3D coordinate system. Height-map is conceptualized as lying in the x/z plane,
     * so put the z coordinate into the vec2's y coordinate.
     *
     * Warning: The returned normal is not directly usable if you're skewing the height map at render time with
     * a non-uniform scale, unless the normal is also manipulated appropriately.
     *
     * @return Height map height/normal at the point, or std::nullopt if the point is out of bounds
     */
    [[nodiscard]] ACCELA_PUBLIC std::optional<HeightMapQueryResult> QueryLoadedHeightMap(
            const LoadedStaticMesh::Ptr& mesh,
            const LoadedHeightMap& heightMap,
            const glm::vec2& modelSpacePoint);
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_UTIL_HEIGHTMAPUTIL_H
