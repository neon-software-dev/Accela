/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_CAMERA_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_CAMERA_H

#include <glm/glm.hpp>

namespace Accela::Render
{
    struct RenderCamera
    {
        glm::vec3 position{0};
        glm::vec3 lookUnit{0};
        glm::vec3 upUnit{0};
        glm::vec3 rightUnit{0};

        // Only used for 3D renders
        float fovYDegrees{45.0f};
        float aspectRatio{1.0f};
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_CAMERA_H
