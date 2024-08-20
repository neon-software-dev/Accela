/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef TESTDESKTOPAPP_SPHEREMESH_H
#define TESTDESKTOPAPP_SPHEREMESH_H

#include <Accela/Render/Mesh/MeshVertex.h>

#include <vector>
#include <numbers>

namespace Accela
{
    static std::vector<Render::MeshVertex> CreateSphereMeshVertices(float sideLength) {
        float radius = sideLength / 2.0f;

        std::vector<Render::MeshVertex> result;

        unsigned int sectorCount = 20;
        unsigned int stackCount = 20;

        float x, y, z, xy;                              // vertex position
        float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
        float s, t;                                     // vertex texCoord

        float sectorStep = 2.0f * (float)std::numbers::pi / (float)sectorCount;
        float stackStep = (float)std::numbers::pi / (float)stackCount;
        float sectorAngle, stackAngle;

        for (unsigned int i = 0; i <= stackCount; ++i)
        {
            stackAngle = (float)std::numbers::pi / 2.0f - (float)i * stackStep;        // starting from pi/2 to -pi/2
            xy = radius * cosf(stackAngle);             // r * cos(u)
            z = radius * sinf(stackAngle);              // r * sin(u)

            // add (sectorCount+1) vertices per stack
            // the first and last vertices have same position and normal, but different tex coords
            for (unsigned int j = 0; j <= sectorCount; ++j)
            {
                sectorAngle = (float)j * sectorStep;           // starting from 0 to 2pi

                // vertex position (x, y, z)
                x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
                y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)

                // normalized vertex normal (nx, ny, nz)
                nx = x * lengthInv;
                ny = y * lengthInv;
                nz = z * lengthInv;

                // vertex tex coord (s, t) range between [0, 1]
                s = (float)j / (float)sectorCount;
                t = (float)i / (float)stackCount;

                result.emplace_back(glm::vec3(x, y, z), glm::vec3(nx, ny, nz), glm::vec2(s, t));
            }
        }

        return result;
    }

    static std::vector<uint32_t> CreateSphereMeshIndices()
    {
        unsigned int sectorCount = 20;
        unsigned int stackCount = 20;

        std::vector<uint32_t> result;

        int k1, k2;
        for (unsigned int i = 0; i < stackCount; ++i)
        {
            k1 = i * (sectorCount + 1);     // beginning of current stack
            k2 = k1 + sectorCount + 1;      // beginning of next stack

            for (unsigned int j = 0; j < sectorCount; ++j, ++k1, ++k2)
            {
                if (i != 0)
                {
                    result.push_back(k1 + 1);
                    result.push_back(k1);
                    result.push_back(k2);
                }

                if (i != (stackCount-1))
                {
                    result.push_back(k2 + 1);
                    result.push_back(k1 + 1);
                    result.push_back(k2);
                }
            }
        }

        return result;
    }
}

#endif //TESTDESKTOPAPP_SPHEREMESH_H
