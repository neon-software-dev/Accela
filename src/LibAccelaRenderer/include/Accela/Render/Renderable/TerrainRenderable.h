/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERABLE_TERRAINRENDERABLE_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERABLE_TERRAINRENDERABLE_H

#include "../Id.h"
#include "../Util/Rect.h"

#include <glm/glm.hpp>

#include <string>
#include <optional>
#include <vector>

namespace Accela::Render
{
    /**
     * Terrain that's rendered in 3D world space. Dynamically generated at
     * render time via tesselation shaders.
     */
    struct TerrainRenderable
    {
        TerrainId terrainId{INVALID_ID};
        std::string sceneName;
        MaterialId materialId{INVALID_ID};
        Render::USize size{0, 0};
        glm::mat4 modelTransform{1.0f};

        TextureId heightMapTextureId{INVALID_ID};
        float tesselationLevel{0.0f}; // Controls how much tesselation to perform
        float displacementFactor{0.0f}; // Factor multiplied against height map texture values
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERABLE_TERRAINRENDERABLE_H
