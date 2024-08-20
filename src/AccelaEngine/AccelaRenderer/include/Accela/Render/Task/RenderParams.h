/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TASK_RENDERPARAMS_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TASK_RENDERPARAMS_H

#include "../RenderCamera.h"
#include "../Light.h"
#include "../Id.h"
#include "../Eye.h"

#include "../Util/Triangle.h"

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <vector>
#include <optional>
#include <unordered_set>

namespace Accela::Render
{
    /**
     * Parameters which apply to a scene render
     */
    struct ACCELA_PUBLIC RenderParams
    {
        RenderCamera worldRenderCamera{}; // The world camera to use
        RenderCamera spriteRenderCamera{}; // The sprite camera to use

        float ambientLightIntensity{0.1f}; // Intensity of ambient world light [0..1]
        glm::vec3 ambientLightColor{1.0f}; // Color of ambient world light

        std::optional<TextureId> skyBoxTextureId; // SkyBox to be rendered
        std::optional<glm::mat4> skyBoxViewTransform; // Transformation to apply to skybox

        std::unordered_set<ObjectId> highlightedObjects; // Objects which to add highlight effect to
        std::vector<Triangle> debugTriangles; // Additional raw debug triangle list to render
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TASK_RENDERPARAMS_H
