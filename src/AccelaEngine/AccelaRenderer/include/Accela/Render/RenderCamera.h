/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_CAMERA_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_CAMERA_H

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/constants.hpp>

namespace Accela::Render
{
    struct ACCELA_PUBLIC RenderCamera
    {
        glm::vec3 position{0,0,0};
        glm::vec3 lookUnit{0,0,-1};
        glm::vec3 upUnit{0,1,0};
        glm::vec3 rightUnit{1,0,0};

        // Only used for 3D renders
        float fovYDegrees{45.0f};
        float aspectRatio{1.0f};

        bool operator==(const RenderCamera& other) const
        {
            if (!glm::all(glm::epsilonEqual(position, other.position, glm::epsilon<float>()))) { return false; }
            if (!glm::all(glm::epsilonEqual(lookUnit, other.lookUnit, glm::epsilon<float>()))) { return false; }
            if (!glm::all(glm::epsilonEqual(upUnit, other.upUnit, glm::epsilon<float>()))) { return false; }
            if (!glm::all(glm::epsilonEqual(rightUnit, other.rightUnit, glm::epsilon<float>()))) { return false; }
            if (!glm::epsilonEqual(fovYDegrees, other.fovYDegrees, glm::epsilon<float>())) { return false; }
            if (!glm::epsilonEqual(aspectRatio, other.aspectRatio, glm::epsilon<float>())) { return false; }
            return true;
        }
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_CAMERA_H
