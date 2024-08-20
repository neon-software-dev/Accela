/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_UTIL_MATH_H
#define LIBACCELAENGINE_SRC_UTIL_MATH_H

#include <Accela/Render/Util/Vector.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Accela::Engine
{
    /**
     * @return A rotation operation that represents the rotation from a start vector to a dest vector
     */
    [[nodiscard]] glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest);

    /**
     * @return if any components of the provided data are nan or inf
     */
    template <typename T>
    bool IsBad(const T& o)
    {
        const auto nan = glm::isnan(o);
        if (nan.x || nan.y || nan.z)
        {
            return true;
        }

        const auto inf = glm::isinf(o);
        if (inf.x || inf.y || inf.z)
        {
            return true;
        }

        return false;
    }

    // Maps a value X in the range of [A1...A2] into the range [B1...B2]
    template<typename T>
    inline T MapValue(const std::pair<T,T>& a, const std::pair<T, T>& b, const T& val)
    {
        return b.first + (((float)(val - a.first) / (float)(a.second - a.first)) * (b.second - b.first));
    }
}

#endif //LIBACCELAENGINE_SRC_UTIL_MATH_H
