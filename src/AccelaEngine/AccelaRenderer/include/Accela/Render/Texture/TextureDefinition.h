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

#include <Accela/Common/SharedLib.h>

#include <vector>

namespace Accela::Render
{
    struct ACCELA_PUBLIC TextureDefinition
    {
        Texture texture;
        std::vector<TextureView> textureViews;
        std::vector<TextureSampler> textureSamplers;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_TEXTURE_TEXTUREDEFINITION_H
