/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_LIGHT_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_LIGHT_H

#include <Accela/Render/Id.h>

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <variant>

namespace Accela::Render
{
    // Maximum active lights in a scene
    constexpr uint32_t Max_Light_Count = 16;

    // Warning - Can't change the order of these values without syncing shaders to the changed values
    enum class AttenuationMode
    {
        None,
        Linear,
        Exponential
    };

    // Warning - Can't change the order of these values without syncing shaders to the changed values
    enum class LightType
    {
        Directional,
        Spotlight,
        Point
    };

    struct ACCELA_PUBLIC LightProperties
    {
        LightType type{LightType::Point};
        AttenuationMode attenuationMode{AttenuationMode::Exponential};

        glm::vec3 diffuseColor{0};
        glm::vec3 diffuseIntensity{0};
        glm::vec3 specularColor{0};
        glm::vec3 specularIntensity{0};

        /**
         * The world-space unit vector which describes the direction the light is pointed.
         * For an omni-directional light, the value doesn't matter.
         */
        glm::vec3 directionUnit{0,0,-1};

        /**
         * Value to specify in which way the emitted light is restricted. Means something
         * different for each light type.
         *
         * [Point Lights]
         * Represents the degree width of the cone of light that the light emits, pointing in
         * the light's direction. Should be set to 360.0 for an omni-directional light, and valid
         * values are [0.0..360.0].
         *
         * [Spot Lights]
         * Represents the degree width of the cone of light that the light emits, pointing in
         * the light's direction. Should ideally be set to 90.0 or lower for decent shadow quality,
         * and valid values are [0.0..180.0].
         *
         * [Directional Lights]
         * Represents the world-space light plane disk radius of the emitted light. Should be set
         * to the special case value of 0.0f to represent no limitation of area of effect.
         * Any non-(sufficiently)zero value represents a disk radius from which to emit light from
         * the light's plane.
         */
        float areaOfEffect{360.0f};
    };

    /**
     * Defines a light that the renderer can include in the rendered world
     */
    struct ACCELA_PUBLIC Light
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

        }

        LightId lightId;
        std::string sceneName;
        glm::vec3 worldPos;
        bool castsShadows;

        LightProperties lightProperties;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_LIGHT_H
