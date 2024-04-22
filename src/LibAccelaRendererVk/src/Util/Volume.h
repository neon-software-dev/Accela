/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_UTIL_VOLUME_H
#define LIBACCELARENDERERVK_SRC_UTIL_VOLUME_H

#include <glm/glm.hpp>

#include <array>

namespace Accela::Render
{
    struct Volume
    {
        static Volume EntireRange() { return {}; }

        /**
         * Defaults the bounding volume to the entire addressable space
         */
        Volume()
            : min(-FLT_MAX, -FLT_MAX, -FLT_MAX)
            , max(FLT_MAX, FLT_MAX, FLT_MAX)
        { }

        Volume(const glm::vec3& _min, const glm::vec3& _max)
            : min(_min)
            , max(_max)
        { }

        bool operator==(const Volume& other) const
        {
            return min == other.min && max == other.max;
        }

        [[nodiscard]] std::array<glm::vec3, 8> GetBoundingPoints() const noexcept
        {
            std::array<glm::vec3, 8> points{};

            points[0] = glm::vec3(min.x, min.y, min.z);
            points[1] = glm::vec3(max.x, min.y, min.z);
            points[2] = glm::vec3(max.x, min.y, max.z);
            points[3] = glm::vec3(min.x, min.y, max.z);
            points[4] = glm::vec3(min.x, max.y, min.z);
            points[5] = glm::vec3(max.x, max.y, min.z);
            points[6] = glm::vec3(max.x, max.y, max.z);
            points[7] = glm::vec3(min.x, max.y, max.z);

            return points;
        }

        [[nodiscard]] glm::vec3 GetCenterPoint() const noexcept
        {
            return (min + max) / 2.0f;
        }

        glm::vec3 min; // Bottom, left, rearwards corner
        glm::vec3 max; // Top, right, forwards corner
    };
}

#endif //LIBACCELARENDERERVK_SRC_UTIL_VOLUME_H
