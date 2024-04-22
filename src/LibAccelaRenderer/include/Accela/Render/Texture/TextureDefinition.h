/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTUREDEFINITION_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTUREDEFINITION_H

#include "Texture.h"
#include "TextureView.h"
#include "TextureSampler.h"

#include <vector>

namespace Accela::Render
{
    /**
     * Fully defines a texture for the renderer to render - The texture's data,
     * one or more views onto that data, and how to sample the data.
     */
    struct TextureDefinition
    {
        TextureDefinition(Texture _texture,
                          std::vector<TextureView> _textureViews,
                          TextureSampler _textureSampler)
            : texture(std::move(_texture))
            , textureViews(std::move(_textureViews))
            , textureSampler(std::move(_textureSampler))
        { }

        Texture texture;
        std::vector<TextureView> textureViews;
        TextureSampler textureSampler;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTUREDEFINITION_H
