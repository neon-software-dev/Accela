/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERSETTINGS_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERSETTINGS_H

#include "Eye.h"

#include "Util/Rect.h"

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <optional>

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
    struct ACCELA_PUBLIC RenderSettings
    {
        //
        // Presentation
        //
        PresentMode presentMode{PresentMode::Immediate};
        PresentScaling presentScaling{PresentScaling::CenterInside};
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

        // Shadow quality level - determines shadow map texture size
        QualityLevel shadowQuality{QualityLevel::Medium};

        // Allows objects not directly in the camera's view to cast shadows onto viewed geometry. Corresponds
        // to the depth from a shadow cut center to the shadow render position. Increase as needed to allow
        // objects further away to cast shadows into view, but keep as small as possible for highest quality
        // shadows. (Only relevant for directional/cascaded light sources).
        float shadowCascadeMinRadiusDepth = 15.0f;

        // By what percentage cascading shadow map cuts should overlap so that the overlapping area can be
        // blended to create a smooth transition between cascades. Valid values: [0.0..1.0]
        float shadowCascadeOverlapRatio = 0.1f; // 10% overlap

        // Maximum distance in which shadows for objects will render. If unset, shadows will render as long
        // as the objects themselves are rendered
        std::optional<float> shadowRenderDistance;

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
        HighlightMode highlightMode{HighlightMode::Outline};
        glm::vec3 highlightColor{0,1,0};
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERSETTINGS_H
