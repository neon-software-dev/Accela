#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_UNITS_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_UNITS_H

#include <glm/glm.hpp>

#include <cmath>
#include <numbers>

namespace Accela::Render
{
    struct Degrees
    {
        explicit Degrees(float _value)
            : value(std::fmod(_value, 360.0f))
        { }

        float value;
    };

    struct Radians
    {
        explicit Radians(float _value)
            : value(std::fmod(_value, 2.0f * std::numbers::pi_v<float>))
        { }

        float value;
    };

    struct WorldPosition
    {
        explicit WorldPosition(const glm::vec3& _value)
            : value(_value)
        { }

        glm::vec3 value;
    };

    struct LocalPosition
    {
        explicit LocalPosition(const glm::vec3& _value)
            : value(_value)
        { }

        glm::vec3 value;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_UNITS_H
