/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Extra/StandardGrassGenerator.h>
#include <Accela/Engine/Util/HeightMapUtil.h>

#include "../Util/Math.h"

namespace Accela::Engine
{

StandardGrassGenerator::StandardGrassGenerator(const unsigned long& seed)
    : m_mt(seed)
{

}

GrassClump StandardGrassGenerator::GenerateGrassClump(const StandardGrassParams& params)
{
    GrassClump clump{};

    const auto grassCount =
        (unsigned int)(std::round(Rand(0.0f, 1.0f) * (float)(params.grass_tuftMaxCount - params.grass_tuftMinCount)) + (float)params.grass_tuftMinCount);

    for (unsigned int x = 0; x < grassCount; ++x)
    {
        const auto tuftOriginDirection = glm::normalize(glm::vec3{
            Rand(-1.0, 1.0), 0.0f, Rand(-1.0, 1.0)
        });
        const auto tuftOriginRadius = Rand(-params.distributionRadius, params.distributionRadius);
        const auto tuftOrigin = tuftOriginDirection * tuftOriginRadius;

        const auto tuft = CreateGrassTuft(
            params,
            tuftOrigin,
            {0,1,0},
            params.grass_size
        );

        clump.tufts.push_back(tuft);
    }

    return clump;
}

GrassClump StandardGrassGenerator::GenerateGrassClump(const StandardGrassParams& params,
                                                      const glm::vec2& modelSpacePosition,
                                                      const LoadedStaticMesh::Ptr& mesh,
                                                      const LoadedHeightMap& heightMap)
{
    GrassClump clump{};

    const auto grassCount =
        (unsigned int)(std::round(Rand(0.0f, 1.0f) * (float)(params.grass_tuftMaxCount - params.grass_tuftMinCount)) + (float)params.grass_tuftMinCount);

    for (unsigned int x = 0; x < grassCount; ++x)
    {
        const auto tuftOriginDirection = glm::normalize(glm::vec3{
            Rand(-1.0, 1.0), 0.0f, Rand(-1.0, 1.0)
        });
        const auto tuftOriginRadius = Rand(-params.distributionRadius, params.distributionRadius);
        auto tuftOrigin =
            (tuftOriginDirection * tuftOriginRadius) +
            glm::vec3(modelSpacePosition.x, 0.0f, modelSpacePosition.y);

        if (tuftOrigin.x < -heightMap.worldWidth / 2.0f || tuftOrigin.x >= heightMap.worldWidth / 2.0f ||
            tuftOrigin.z < -heightMap.worldHeight / 2.0f || tuftOrigin.z >= heightMap.worldHeight / 2.0f)
        {
            continue;
        }

        const auto heightMapQuery = QueryLoadedHeightMap(mesh, heightMap, {tuftOrigin.x, tuftOrigin.z});
        if (!heightMapQuery)
        {
            continue;
        }

        tuftOrigin.y = heightMapQuery->pointHeight_modelSpace;

        const auto tuft = CreateGrassTuft(
            params,
            tuftOrigin,
            heightMapQuery->pointNormalUnit_modelSpace,
            params.grass_size
        );

        clump.tufts.push_back(tuft);
    }

    return clump;
}

GrassTuft StandardGrassGenerator::CreateGrassTuft(const StandardGrassParams& params,
                                                  const glm::vec3& origin,
                                                  const glm::vec3& orientationUnit,
                                                  float size)
{
    GrassTuft tuft{};
    tuft.origin = origin;
    tuft.orientationUnit = orientationUnit;
    tuft.width = size * (1.0f + Rand(-params.grass_sizeVariance, params.grass_sizeVariance));
    tuft.height = size * (1.0f + Rand(-params.grass_sizeVariance, params.grass_sizeVariance));

    return tuft;
}

float StandardGrassGenerator::Rand(float min, float max)
{
    return std::uniform_real_distribution<float>(min, max)(m_mt);
}

}

