/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERABLE_SPRITERENDERABLE_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERABLE_SPRITERENDERABLE_H

#include "../Id.h"
#include "../Util/Rect.h"
#include "../Util/Rotation.h"

#include <glm/glm.hpp>

#include <optional>
#include <utility>
#include <string>

namespace Accela::Render
{
    /**
     * A sprite that's rendered in 2D screen space
     */
    struct SpriteRenderable
    {
        SpriteId spriteId{INVALID_ID};
        std::string sceneName;
        TextureId textureId{INVALID_ID};
        std::optional<URect> srcPixelRect{}; // Optional subset of the source texture to use. If not set, will use the entire texture.
        std::optional<FSize> dstSize{}; // Optional destination size. If not set, will match the size of the source being used.
        glm::vec3 position{0};
        glm::quat orientation{};
        glm::vec3 scale{1.0f};
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERABLE_SPRITERENDERABLE_H
