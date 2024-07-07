/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERSETTINGS_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERSETTINGS_H

#include "Eye.h"

#include "Util/Rect.h"

#include <glm/glm.hpp>

namespace Accela::Render
{
    enum class PresentMode
    {
        Immediate,
        VSync
    };

    enum class PresentScaling
    {
        CenterCrop,
        CenterInside
    };

    enum class QualityLevel
    {
        Low,
        Medium,
        High
    };

    enum class TextureAnisotropy
    {
        None,
        Low,
        Maximum
    };

    enum class HighlightMode
    {
        Fill,
        Outline
    };

    /**
     * Parameters which control rendering
     */
    struct RenderSettings
    {
        //
        // Presentation
        //
        PresentMode presentMode{PresentMode::Immediate};
        PresentScaling presentScaling{PresentScaling::CenterInside};
        bool presentToHeadset{false};
        glm::vec3 presentClearColor{0.1f, 0.1f, 0.1f};
        Eye presentEye{Eye::Left}; // Eye render presented to the window (only relevant when in VR mode)

        //
        // General
        //
        uint8_t framesInFlight{3};
        // Note: This is render resolution, which is different from window resolution and virtual resolution
        USize resolution{1920, 1080};
        float maxRenderDistance{1000.0f};
        float globalViewScale{1.0f};

        //
        // Shadows
        //
        QualityLevel shadowQuality{QualityLevel::Medium};

        //
        // Textures
        //

        // Warning: Changing this at runtime does NOT retroactively recreate pre-existing texture samplers
        TextureAnisotropy textureAnisotropy{TextureAnisotropy::Low};

        //
        // Objects
        //

        // Max distance objects will be rendered at
        float objectRenderDistance{200.0f};

        // Whether to render objects at all (for debugging purposes)
        bool renderObjects{true};

        // Whether to render objects in wireframe
        bool objectsWireframe{false};

        //
        // Lighting
        //
        bool hdr{true};
        float exposure{1.0f};

        //
        // Post-Processing
        //
        float gamma{2.2f};
        bool fxaa{true};
        HighlightMode highlightMode{HighlightMode::Fill};
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERSETTINGS_H
