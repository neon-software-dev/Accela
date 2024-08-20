/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_GRASSMESHCREATOR_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_GRASSMESHCREATOR_H

#include "Grass.h"

#include <Accela/Render/Mesh/StaticMesh.h>

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <vector>
#include <random>
#include <string>

namespace Accela::Engine
{
    struct ACCELA_PUBLIC GrassMeshParams
    {

    };

    struct ACCELA_PUBLIC GrassMesh
    {
        std::shared_ptr<Render::StaticMesh> mesh;
    };

    class ACCELA_PUBLIC GrassMeshCreator
    {
        public:

            explicit GrassMeshCreator(const std::mt19937::result_type& seed = std::random_device{}());

            [[nodiscard]] static GrassMeshParams QualityBasedMeshParams(float minimumViewDistance);

            [[nodiscard]] static GrassMesh CreateGrassMesh(const GrassMeshParams& params, const GrassClump& clump, const std::string& tag);

        private:

            static void AppendTuftGeometry(const GrassTuft& tuft,
                                           const std::shared_ptr<Render::StaticMesh>& mesh);

            static void AppendGrassGeometry(const glm::vec3& origin,
                                            const glm::vec3& orientationUnit,
                                            float tuftRotationDegrees,
                                            float width,
                                            float height,
                                            const std::shared_ptr<Render::StaticMesh>& mesh);

            [[nodiscard]] inline float Rand(float min, float max);

        private:

            std::mt19937 m_mt;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_GRASSMESHCREATOR_H
