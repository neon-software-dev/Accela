/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_SRC_SCENE_SCENESTATE_H
#define LIBACCELAENGINE_SRC_SCENE_SCENESTATE_H

#include <Accela/Engine/Camera2D.h>
#include <Accela/Engine/Camera3D.h>

#include <Accela/Render/Id.h>

#include <glm/glm.hpp>

#include <optional>

namespace Accela::Engine
{
    struct SceneState
    {
        Camera3D::Ptr worldCamera = std::make_shared<Camera3D>();
        Camera2D::Ptr spriteCamera = std::make_shared<Camera2D>();

        float ambientLightIntensity{0.1f};
        glm::vec3 ambientLightColor{1.0f};

        std::optional<Render::TextureId> skyBoxTextureId;
    };
}

#endif //LIBACCELAENGINE_SRC_SCENE_SCENESTATE_H
