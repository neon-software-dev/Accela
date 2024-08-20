/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_UTIL_VOLUME_H
#define LIBACCELARENDERERVK_SRC_UTIL_VOLUME_H

#include <Accela/Render/Util/Triangle.h>

#include <glm/glm.hpp>

#include <array>
#include <vector>

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

        [[nodiscard]] float Width() const noexcept
        {
            return max.x - min.x;
        }

        [[nodiscard]] float Height() const noexcept
        {
            return max.y - min.y;
        }

        [[nodiscard]] float Depth() const noexcept
        {
            return max.z - min.z;
        }

        // Returns double-sided triangles encompassing the volume's bounds
        [[nodiscard]] std::vector<Triangle> GetDebugTriangles() const
        {
            std::vector<Triangle> triangles;
            triangles.reserve(24);

            const auto b = GetBoundingPoints();

            triangles.emplace_back(b[0], b[1], b[2]);
            triangles.emplace_back(b[0], b[2], b[3]);
            triangles.emplace_back(b[0], b[2], b[1]);
            triangles.emplace_back(b[0], b[3], b[2]);
            triangles.emplace_back(b[4], b[5], b[6]);
            triangles.emplace_back(b[4], b[6], b[7]);
            triangles.emplace_back(b[4], b[6], b[5]);
            triangles.emplace_back(b[4], b[7], b[6]);
            triangles.emplace_back(b[0], b[4], b[7]);
            triangles.emplace_back(b[0], b[7], b[3]);
            triangles.emplace_back(b[0], b[7], b[4]);
            triangles.emplace_back(b[0], b[3], b[7]);
            triangles.emplace_back(b[1], b[2], b[6]);
            triangles.emplace_back(b[1], b[6], b[5]);
            triangles.emplace_back(b[1], b[6], b[2]);
            triangles.emplace_back(b[1], b[5], b[6]);
            triangles.emplace_back(b[3], b[7], b[6]);
            triangles.emplace_back(b[3], b[6], b[2]);
            triangles.emplace_back(b[3], b[6], b[7]);
            triangles.emplace_back(b[3], b[2], b[6]);
            triangles.emplace_back(b[0], b[4], b[5]);
            triangles.emplace_back(b[0], b[5], b[1]);
            triangles.emplace_back(b[0], b[5], b[4]);
            triangles.emplace_back(b[0], b[1], b[5]);

            return triangles;
        }

        glm::vec3 min; // Bottom, left, rearwards corner
        glm::vec3 max; // Top, right, forwards corner
    };
}

#endif //LIBACCELARENDERERVK_SRC_UTIL_VOLUME_H
