#ifndef LIBACCELARENDERERVK_SRC_LIGHT_LOADEDLIGHT_H
#define LIBACCELARENDERERVK_SRC_LIGHT_LOADEDLIGHT_H

#include <Accela/Render/Id.h>

#include <Accela/Render/Light.h>

#include <optional>
#include <array>
#include <algorithm>

namespace Accela::Render
{
    enum class ShadowMapType
    {
        /**
         * The light's shadow map can be performed with a single shadow map pass (the
         * light's cone is <= 90 degrees)
         */
        Single,

        /**
         * The light's shadow map has to be done as a cubic shadow map
         */
        Cube
    };

    [[nodiscard]] static ShadowMapType GetShadowMapType(const Light& light)
    {
        return light.lightProperties.coneFovDegrees <= 90.0f ? ShadowMapType::Single : ShadowMapType::Cube;
    }

    struct LoadedLight
    {
        LoadedLight(Light _light, std::optional<FrameBufferId> _framebufferId)
            : light(std::move(_light))
            , shadowFrameBufferId(_framebufferId)
        {
            shadowMapType = GetShadowMapType(light);

            // If the light has a shadow framebuffer then default its shadow maps to invalidated
            // so that they'll all be rendered/synced
            shadowInvalidated = shadowFrameBufferId.has_value();
        }

        Light light;
        ShadowMapType shadowMapType;
        std::optional<FrameBufferId> shadowFrameBufferId;
        bool shadowInvalidated;
    };
}

#endif //LIBACCELARENDERERVK_SRC_LIGHT_LOADEDLIGHT_H
