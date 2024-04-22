/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_LIGHT_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_LIGHT_H

#include <Accela/Render/Id.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <variant>

namespace Accela::Render
{
    constexpr uint32_t Max_Light_Count = 16;

    // Warning - Can't change the order of these values without syncing shaders to the changed values
    enum class AttenuationMode
    {
        None,
        Linear,
        Exponential
    };

    enum class LightType
    {
        Point
    };

    struct LightBaseProperties
    {
        AttenuationMode attenuationMode{AttenuationMode::Exponential};

        glm::vec3 diffuseColor{0};
        glm::vec3 diffuseIntensity{0};

        glm::vec3 specularColor{0};
        glm::vec3 specularIntensity{0};
    };

    struct LightProperties_PointLight
    {

    };

    struct LightProperties_SpotLight
    {
        glm::vec3 directionUnit{0,0,-1};
        float coneFovDegrees{45.0f}; // [0.0 .. 180.0] degree size of the directional cone
    };

    using LightTypeProperties = std::variant<
        LightProperties_PointLight,
        LightProperties_SpotLight
    >;

    struct LightProperties
    {
        LightProperties(const LightBaseProperties& _base, const LightTypeProperties& _type)
            : base(_base)
            , type(_type)
        { }

        LightBaseProperties base;
        LightTypeProperties type;
    };

    /**
     * Defines a light that the renderer can include in the rendered world
     */
    struct Light
    {
        Light(LightId _lightId,
              std::string _sceneName,
              const glm::vec3& _worldPos,
              bool _castsShadows,
              const LightProperties& _lightProperties)
            : lightId(_lightId)
            , sceneName(std::move(_sceneName))
            , worldPos(_worldPos)
            , castsShadows(_castsShadows)
            , lightProperties(_lightProperties)
        {
            if (std::holds_alternative<Render::LightProperties_PointLight>(lightProperties.type))
            {
                lightType = Render::LightType::Point;
            }
        }

        LightId lightId;
        std::string sceneName;
        glm::vec3 worldPos;
        bool castsShadows;
        LightProperties lightProperties;

        LightType lightType{Render::LightType::Point};
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_LIGHT_H
