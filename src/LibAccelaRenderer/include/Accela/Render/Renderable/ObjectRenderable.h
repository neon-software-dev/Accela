/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERABLE_OBJECTRENDERABLE_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERABLE_OBJECTRENDERABLE_H

#include "../Id.h"

#include <glm/glm.hpp>

#include <string>
#include <optional>
#include <vector>

namespace Accela::Render
{
    /**
     * An object that's rendered in the 3D world
     */
    struct ObjectRenderable
    {
        ObjectId objectId{INVALID_ID};
        std::string sceneName;
        MeshId meshId{INVALID_ID};
        MaterialId materialId{INVALID_ID};
        glm::mat4 modelTransform{1.0f};
        bool shadowPass{true};
        std::optional<std::vector<glm::mat4>> boneTransforms;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERABLE_OBJECTRENDERABLE_H
