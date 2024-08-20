/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_STANDARDGRASSGENERATOR_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_STANDARDGRASSGENERATOR_H

#include "Grass.h"

#include <Accela/Engine/Scene/LoadedHeightMap.h>
#include <Accela/Engine/Scene/LoadedStaticMesh.h>

#include <Accela/Common/SharedLib.h>

#include <random>
#include <array>

namespace Accela::Engine
{
    struct ACCELA_PUBLIC StandardGrassParams
    {
        float distributionRadius{3.0f};

        unsigned int grass_tuftMinCount{5};
        unsigned int grass_tuftMaxCount{10};
        float grass_width{1.0f};
        float grass_height{1.0f};
        float grass_sizeVariance{0.5f};
    };

    class ACCELA_PUBLIC StandardGrassGenerator
    {
        public:

            explicit StandardGrassGenerator(const std::mt19937::result_type& seed = std::random_device{}());

            /**
             * Generate a grass clump given some parameters
             */
            [[nodiscard]] GrassClump GenerateGrassClump(const StandardGrassParams& params);

            /**
             * Generate a grass clump given some parameters, at a specific position within a height
             * map mesh. Will orient each individual tuft of grass to be properly placed and oriented
             * for the height map / terrain underneath it. The output doesn't require model-space
             * translation, as each tuft's origin is set to the output model-space position.
             */
            [[nodiscard]] GrassClump GenerateGrassClump(const StandardGrassParams& params,
                                                        const glm::vec2& modelSpacePosition,
                                                        const LoadedStaticMesh::Ptr& mesh,
                                                        const LoadedHeightMap& heightMap);

        private:

            [[nodiscard]] GrassTuft CreateGrassTuft(const StandardGrassParams& params,
                                                    const glm::vec3& origin,
                                                    const glm::vec3& orientationUnit,
                                                    float width,
                                                    float height);

            [[nodiscard]] inline float Rand(float min, float max);

        private:

            std::mt19937 m_mt;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_STANDARDGRASSGENERATOR_H
