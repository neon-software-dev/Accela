/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef TESTDESKTOPAPP_CUBEMESH_H
#define TESTDESKTOPAPP_CUBEMESH_H

#include <Accela/Render/Mesh/MeshVertex.h>

#include <vector>

namespace Accela
{
    static const std::vector<Render::MeshVertex> CubeVertices = {
        // Back
        Render::MeshVertex(glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(0,0,-1), glm::vec2(1,1)),
        Render::MeshVertex(glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(0,0,-1), glm::vec2(0,1)),
        Render::MeshVertex(glm::vec3(0.5f,  0.5f, -0.5f), glm::vec3(0,0,-1), glm::vec2(0,0)),
        Render::MeshVertex(glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(0,0,-1), glm::vec2(1,0)),

        // Front
        Render::MeshVertex(glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0,0,1), glm::vec2(1,1)),
        Render::MeshVertex(glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0,0,1), glm::vec2(0,1)),
        Render::MeshVertex(glm::vec3(-0.5f,  0.5f, 0.5f), glm::vec3(0,0,1), glm::vec2(0,0)),
        Render::MeshVertex(glm::vec3(0.5f,  0.5f, 0.5f), glm::vec3(0,0,1), glm::vec2(1,0)),

        // Left
        Render::MeshVertex(glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(-1,0,0), glm::vec2(1,1)),
        Render::MeshVertex(glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec3(-1,0,0), glm::vec2(0,1)),
        Render::MeshVertex(glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(-1,0,0), glm::vec2(0,0)),
        Render::MeshVertex(glm::vec3(-0.5f,  0.5f, 0.5f), glm::vec3(-1,0,0), glm::vec2(1,0)),

        // Right
        Render::MeshVertex(glm::vec3(0.5f, -0.5f, -0.5f), glm::vec3(1,0,0), glm::vec2(1,1)),
        Render::MeshVertex(glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(1,0,0), glm::vec2(0,1)),
        Render::MeshVertex(glm::vec3(0.5f,  0.5f, 0.5f), glm::vec3(1,0,0), glm::vec2(0,0)),
        Render::MeshVertex(glm::vec3(0.5f,  0.5f, -0.5f), glm::vec3(1,0,0), glm::vec2(1,0)),

        // Top
        Render::MeshVertex(glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec3(0,1,0), glm::vec2(0,0)),
        Render::MeshVertex(glm::vec3(0.5f, 0.5f, -0.5f), glm::vec3(0,1,0), glm::vec2(1,0)),
        Render::MeshVertex(glm::vec3(0.5f,  0.5f, 0.5f), glm::vec3(0,1,0), glm::vec2(1,1)),
        Render::MeshVertex(glm::vec3(-0.5f,  0.5f, 0.5f), glm::vec3(0,1,0), glm::vec2(0,1)),

        // Bottom
        Render::MeshVertex(glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec3(0,-1,0), glm::vec2(0,0)),
        Render::MeshVertex(glm::vec3(0.5f, -0.5f, 0.5f), glm::vec3(0,-1,0), glm::vec2(1,0)),
        Render::MeshVertex(glm::vec3(0.5f,  -0.5f, -0.5f), glm::vec3(0,-1,0), glm::vec2(1,1)),
        Render::MeshVertex(glm::vec3(-0.5f,  -0.5f, -0.5f), glm::vec3(0,-1,0), glm::vec2(0,1))
    };

    static const std::vector<unsigned int> CubeIndices = {
        0, 2, 1, 0, 3, 2,           // Back
        4, 6, 5, 4, 7, 6,           // Front
        8, 10, 9, 8, 11, 10,        // Left
        12, 14, 13, 12, 15, 14,     // Right
        16, 18, 17, 16, 19, 18,     // Top
        20, 22, 21, 20, 23, 22      // Bottom
    };
}

#endif //TESTDESKTOPAPP_CUBEMESH_H
