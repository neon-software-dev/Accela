/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_LIGHT_LOADEDLIGHT_H
#define LIBACCELARENDERERVK_SRC_LIGHT_LOADEDLIGHT_H

#include "../Util/ViewProjection.h"

#include <Accela/Render/Id.h>
#include <Accela/Render/RenderCamera.h>

#include <Accela/Render/Light.h>

#include <glm/glm.hpp>

#include <optional>
#include <array>
#include <algorithm>
#include <vector>

namespace Accela::Render
{
    enum class ShadowMapType
    {
        Cascaded,   // Multi-viewed, cascaded, shadow map
        Cube        // Multi-viewed, cubic, shadow map
    };

    struct ShadowRender
    {
        // The world position the shadow render is taken from
        glm::vec3 worldPos{0};

        // The ViewProjection associated with the shadow render
        ViewProjection viewProjection;

        // Directional/Cascaded-specific
        std::optional<uint32_t> cascadeIndex; // [0..Shadow_Cascade_Count)
        std::optional<glm::vec2> cut; // Shadow-space z-axis start/end cut distances
    };

    [[nodiscard]] static ShadowMapType GetShadowMapType(const Light& light)
    {
        switch (light.lightProperties.type)
        {
            case LightType::Point: return ShadowMapType::Cube;
            case LightType::Directional: return ShadowMapType::Cascaded;
        }

        assert(false);
        return ShadowMapType::Cube;
    }

    struct LoadedLight
    {
        LoadedLight(Light _light, std::optional<FrameBufferId> _framebufferId)
            : light(std::move(_light))
            , shadowFrameBufferId(_framebufferId)
        {
            shadowMapType = GetShadowMapType(light);
        }

        // The light's properties as provided by the client
        Light light;

        // If true, the light's shadow renders are out of date and need to be rendered
        bool shadowInvalidated{true};

        // Whether the light uses cascaded or cubic shadow maps
        ShadowMapType shadowMapType;

        // Framebuffer which binds shadow render(s) for the light
        std::optional<FrameBufferId> shadowFrameBufferId;

        // Details of each shadow render which is associated with the light
        std::vector<ShadowRender> shadowRenders;

        // The camera that was associated with the light's latest shadow renders.
        // Only relevant for / used by directional shadow maps.
        std::optional<RenderCamera> shadowRenderCamera;
    };
}

#endif //LIBACCELARENDERERVK_SRC_LIGHT_LOADEDLIGHT_H
