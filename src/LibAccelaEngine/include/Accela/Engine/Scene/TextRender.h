/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_TEXTRENDER_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_TEXTRENDER_H

#include <Accela/Render/Id.h>

#include <cstdint>

namespace Accela::Engine
{
    /**
     * The results of a text render operation
     */
    struct TextRender
    {
        Render::TextureId textureId{Render::INVALID_ID};
        uint32_t textPixelWidth{0};
        uint32_t textPixelHeight{0};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_SCENE_TEXTRENDER_H
