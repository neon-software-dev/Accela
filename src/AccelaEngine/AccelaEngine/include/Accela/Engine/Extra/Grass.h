/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_GRASS_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_GRASS_H

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <vector>

namespace Accela::Engine
{
    struct ACCELA_PUBLIC GrassTuft
    {
        glm::vec3 origin{0};            // The origin / starting point of the tuft
        glm::vec3 orientationUnit{0};   // The direction the tuft is oriented in

        float width{0.0f};                    // The width of the grass in the tuft
        float height{0.0f};                   // The height of the grass in the tuft
    };

    struct ACCELA_PUBLIC GrassClump
    {
        std::vector<GrassTuft> tufts;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_GRASS_H
