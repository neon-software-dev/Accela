/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_PRESENTCONFIG_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_PRESENTCONFIG_H

#include <glm/glm.hpp>

namespace Accela::Render
{
    /**
     * Properties for how to present a scene on the display
     */
    struct PresentConfig
    {
        // The color the window should be cleared to before a texture is blitted
        // to it for display. Note that this is different than the clear color that's
        // used when scenes are rendered.
        glm::vec4 clearColor{0,0,0,1};
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_PRESENTCONFIG_H
