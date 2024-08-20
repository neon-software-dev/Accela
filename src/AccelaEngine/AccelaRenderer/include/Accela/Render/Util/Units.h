/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_UNITS_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_UNITS_H

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <cmath>
#include <numbers>

namespace Accela::Render
{
    struct ACCELA_PUBLIC Degrees
    {
        explicit Degrees(float _value)
            : value(std::fmod(_value, 360.0f))
        { }

        float value;
    };

    struct ACCELA_PUBLIC Radians
    {
        explicit Radians(float _value)
            : value(std::fmod(_value, 2.0f * std::numbers::pi_v<float>))
        { }

        float value;
    };

    struct ACCELA_PUBLIC WorldPosition
    {
        explicit WorldPosition(const glm::vec3& _value)
            : value(_value)
        { }

        glm::vec3 value;
    };

    struct ACCELA_PUBLIC LocalPosition
    {
        explicit LocalPosition(const glm::vec3& _value)
            : value(_value)
        { }

        glm::vec3 value;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_UNITS_H
