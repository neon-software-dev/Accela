/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef TESTDESKTOPAPP_INVERTEDCYLINDERMESH_H
#define TESTDESKTOPAPP_INVERTEDCYLINDERMESH_H

#include <Accela/Render/Mesh/MeshVertex.h>

#include <vector>
#include <cassert>

namespace Accela
{
    static std::vector<Render::MeshVertex> CreateInvertedCylinderVertices(float totalLength,
                                                                          float radius,
                                                                          unsigned int pointsPerLoop,
                                                                          unsigned int numLoops)
    {
        assert(numLoops > 1);
        assert(pointsPerLoop > 2);
        assert(totalLength > 0.0f);

        std::vector<Render::MeshVertex> vertices;
        vertices.reserve(pointsPerLoop * numLoops);

        const float angleBetweenPoints = 360.0f / (float)pointsPerLoop;
        const float distanceBetweenLoops = totalLength / (float)(numLoops - 1);

        for (unsigned int loopIndex = 0; loopIndex < numLoops; ++loopIndex)
        {
            const float loopDistance = distanceBetweenLoops * (float)loopIndex;

            for (unsigned int pointIndex = 0; pointIndex < pointsPerLoop; ++pointIndex)
            {
                const float pointX = glm::cos(glm::radians((float)pointIndex * angleBetweenPoints));
                const float pointY = glm::sin(glm::radians((float)pointIndex * angleBetweenPoints));

                glm::vec3 position(pointX, pointY, 0);
                position *= radius;
                position.z = -loopDistance;

                glm::vec3 normal = -position;
                normal.z = 0;
                normal = glm::normalize(normal);

                vertices.emplace_back(
                    position,
                    normal,
                    glm::vec2(0,0),
                    glm::vec3(0,0,0)
                );
            }
        }

        return vertices;
    }

    static std::vector<unsigned int> CreateInvertedCylinderIndices(unsigned int pointsPerLoop,
                                                                   unsigned int numLoops)
    {
        std::vector<unsigned int> indices;

        std::vector<unsigned int> baseIndices;

        for (unsigned int pointIndex = 0; pointIndex < pointsPerLoop; ++pointIndex)
        {
            unsigned int lastPointAdjust = 0;
            if (pointIndex == pointsPerLoop - 1)
            {
                lastPointAdjust = pointsPerLoop;
            }

            baseIndices.push_back(pointIndex);
            baseIndices.push_back(pointIndex + 1 - lastPointAdjust);
            baseIndices.push_back(pointIndex + 1 + pointsPerLoop - lastPointAdjust);

            baseIndices.push_back(pointIndex);
            baseIndices.push_back(pointIndex + 1 + pointsPerLoop - lastPointAdjust);
            baseIndices.push_back(pointIndex + pointsPerLoop);
        }

        for (unsigned int loopIndex = 0; loopIndex < numLoops - 1; ++loopIndex)
        {
            const unsigned int loopAdjust = loopIndex * pointsPerLoop;

            for (const auto& baseIndex : baseIndices)
            {
                indices.push_back(baseIndex + loopAdjust);
            }
        }

        return indices;
    };
}

#endif //TESTDESKTOPAPP_INVERTEDCYLINDERMESH_H
