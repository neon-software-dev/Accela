/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_TEXTURE_REGISTEREDTEXTURE_H
#define LIBACCELAENGINE_SRC_TEXTURE_REGISTEREDTEXTURE_H

#include <Accela/Render/Texture/Texture.h>

namespace Accela::Engine
{
    struct RegisteredTexture
    {
        explicit RegisteredTexture(Render::Texture _texture)
            : texture(std::move(_texture))
        { }

        Render::Texture texture;
    };
}

#endif //LIBACCELAENGINE_SRC_TEXTURE_REGISTEREDTEXTURE_H
