/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Util/HeightMapUtil.h>

#include "Math.h"

#include <utility>
#include <array>
#include <algorithm>

namespace Accela::Engine
{

static inline float GetHeightMapValue(const LoadedStaticMesh::Ptr& mesh,
                                      const LoadedHeightMap& heightMap,
                                      unsigned int colIndex,
                                      unsigned int rowIndex)
{
    const auto inverseRowIndex = heightMap.dataHeight - rowIndex - 1;
    return mesh->vertices[colIndex + (inverseRowIndex * heightMap.dataWidth)].position.y;
}

static glm::vec2 ModelPointToDataPoint(const LoadedHeightMap& heightMap, const glm::vec2& modelSpacePoint)
{
    const auto halfModelWidth = heightMap.worldWidth / 2.0f;
    const auto halfModelHeight = heightMap.worldHeight / 2.0f;

    const float x = MapValue({-halfModelWidth, halfModelWidth}, {0.0f, (float)heightMap.dataWidth - 1.0f}, modelSpacePoint.x);
    const float y = MapValue({-halfModelHeight, halfModelHeight}, {0.0f, (float)heightMap.dataHeight - 1.0f}, modelSpacePoint.y);

    return {x, y};
}

std::pair<unsigned int, unsigned> GetDataTopLeftColRow(const glm::vec2& dataPosition)
{
    return {(unsigned int)dataPosition.x, (unsigned int)dataPosition.y};
}

glm::vec2 DataRowColToModelPoint(const LoadedHeightMap& heightMap, const std::pair<unsigned int, unsigned>& rowCol)
{
    const auto halfModelWidth = heightMap.worldWidth / 2.0f;
    const auto halfModelHeight = heightMap.worldHeight / 2.0f;

    const float x = MapValue({0.0f, (float)heightMap.dataWidth - 1.0f}, {-halfModelWidth, halfModelWidth}, (float)rowCol.first);
    const float y = MapValue({0.0f, (float)heightMap.dataHeight - 1.0f}, {-halfModelHeight, halfModelHeight}, (float)rowCol.second);

    return {x, y};
}

static std::array<glm::vec3, 3> GetClosestTrianglePoints(const LoadedStaticMesh::Ptr& mesh,
                                                         const LoadedHeightMap& heightMap,
                                                         const glm::vec2& modelSpacePoint)
{
    std::array<glm::vec3, 3> points{};

    const auto dataSpacePoint = ModelPointToDataPoint(heightMap, modelSpacePoint);

    const std::pair<unsigned int, unsigned> topLeftDataPoint = GetDataTopLeftColRow(dataSpacePoint);
    const std::pair<unsigned int, unsigned> topRightDataPoint = {topLeftDataPoint.first + 1, topLeftDataPoint.second};
    const std::pair<unsigned int, unsigned> bottomLeftDataPoint = {topLeftDataPoint.first, topLeftDataPoint.second + 1};
    const std::pair<unsigned int, unsigned> bottomRightDataPoint = {topLeftDataPoint.first + 1, topLeftDataPoint.second + 1};

    const float dx = dataSpacePoint.x - (float)topLeftDataPoint.first;
    const float dy = dataSpacePoint.y - (float)topLeftDataPoint.second;

    std::array<std::pair<unsigned int, unsigned>, 3> triDataPoints{};
    triDataPoints[0] = topLeftDataPoint;
    triDataPoints[1] = bottomRightDataPoint;

    const bool isLowerTriangle = dy > dx;

    if (isLowerTriangle)
    {
        triDataPoints = {
            topLeftDataPoint,
            bottomLeftDataPoint,
            bottomRightDataPoint
        };
    }
    else
    {
        triDataPoints = {
            topLeftDataPoint,
            bottomRightDataPoint,
            topRightDataPoint
        };
    }

    for (unsigned int x = 0; x < 3; ++x)
    {
        const auto& dataPoint = triDataPoints[x];

        const auto modelPoint = DataRowColToModelPoint(heightMap, dataPoint);

        points[x] = {
            modelPoint.x,
            GetHeightMapValue(mesh, heightMap, dataPoint.first, dataPoint.second),
            modelPoint.y
        };
    }

    return points;
}

/**
 * Query a height map for the model-space height and normal at a specific model point.
 *
 * Warning: The returned normal is probably not applicable if you're skewing the height map
 * at render time with a non-uniform scale unless the normal is also manipulated appropriately.
 *
 * @return Height map height/normal at the point, or std::nullopt if the point is out of bounds
 */
std::optional<HeightMapQueryResult> QueryLoadedHeightMap(const LoadedStaticMesh::Ptr& mesh,
                                                         const LoadedHeightMap& heightMap,
                                                         const glm::vec2& modelSpacePoint)
{
    const auto halfModelWidth = heightMap.worldWidth / 2.0f;
    const auto halfModelHeight = heightMap.worldHeight / 2.0f;

    if (modelSpacePoint.x < -halfModelWidth || modelSpacePoint.x > halfModelWidth)
    {
        return std::nullopt;
    }

    if (modelSpacePoint.y < -halfModelHeight || modelSpacePoint.y > halfModelHeight)
    {
        return std::nullopt;
    }

    const auto closestPoints = GetClosestTrianglePoints(mesh, heightMap, modelSpacePoint);

    const auto e1 = closestPoints[1] - closestPoints[0];
    const auto e2 = closestPoints[2] - closestPoints[0];

    const auto triNormalUnit = glm::normalize(glm::cross(e1, e2));

    const auto rayOrigin = glm::vec3(modelSpacePoint.x, 0.0f, modelSpacePoint.y);
    const auto rayDirUnit = glm::vec3(0,1,0);

    // Solve equation for intersection between ray and plane
    const float d = glm::dot(closestPoints[0], triNormalUnit);
    const float dn = glm::dot(rayDirUnit, triNormalUnit);

    // No intersection (shouldn't ever be the case)
    if (dn < 0)
    {
        return std::nullopt;
    }

    const float height = (d - (glm::dot(rayOrigin, triNormalUnit))) / dn;

    return HeightMapQueryResult{
        .pointHeight_modelSpace = height,
        .pointNormalUnit_modelSpace = triNormalUnit
    };
}

}
