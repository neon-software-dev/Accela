/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_SPRITERENDERABLECOMPONENT_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_SPRITERENDERABLECOMPONENT_H

#include <Accela/Engine/Scene/SceneCommon.h>

#include <Accela/Render/Id.h>
#include <Accela/Render/Util/Rect.h>
#include <Accela/Render/Util/Rotation.h>

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <optional>
#include <utility>
#include <string>

namespace Accela::Engine
{
    /**
     * Allows for attaching a sprite to an entity
     */
    struct ACCELA_PUBLIC SpriteRenderableComponent
    {
        std::string sceneName = DEFAULT_SCENE;

        /**
         * The TextureId of the texture that should be applied to this sprite
         */
        Render::TextureId textureId{INVALID_ID};

        /**
         * An optional subset of the source texture's pixel size to create the sprite
         * from. The default is to use the entire source texture.
         */
        std::optional<Render::URect> srcPixelRect{};

        /**
         * An optional virtual size to render the sprite as. Defaults to the virtual
         * size of the texture being selected as the source.
         */
        std::optional<Render::FSize> dstVirtualSize{};

    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_SPRITERENDERABLECOMPONENT_H
