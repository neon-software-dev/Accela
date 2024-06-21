/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Extra/GrassMeshCreator.h>

#include "../Util/Math.h"

#include <format>

namespace Accela::Engine
{

GrassMeshCreator::GrassMeshCreator(const unsigned long& seed)
    : m_mt(seed)
{

}

GrassMeshParams GrassMeshCreator::QualityBasedMeshParams(float minimumViewDistance)
{
    (void)minimumViewDistance;
    return {};
}

GrassMesh GrassMeshCreator::CreateGrassMesh(const GrassMeshParams& params, const GrassClump& clump, const std::string& tag)
{
    (void)params;

    GrassMesh grassMesh{};
    grassMesh.mesh = std::make_shared<Render::StaticMesh>(Render::MeshId::Invalid(), std::format("Grass-{}", tag));

    for (const auto& tuft : clump.tufts)
    {
        AppendTuftGeometry(tuft, grassMesh.mesh);
    }

    return grassMesh;
}

void GrassMeshCreator::AppendTuftGeometry(const GrassTuft& tuft, const std::shared_ptr<Render::StaticMesh>& mesh)
{
    float tuftRotationDegrees = 0.0f;

    for (unsigned int x = 0; x < 3; ++x)
    {
        AppendGrassGeometry(tuft.origin, tuft.orientationUnit, tuftRotationDegrees, tuft.width, tuft.height, mesh);
        tuftRotationDegrees += 45.0f;
    }
}

void GrassMeshCreator::AppendGrassGeometry(const glm::vec3& origin,
                                           const glm::vec3& orientationUnit,
                                           float tuftRotationDegrees,
                                           float width,
                                           float height,
                                           const std::shared_ptr<Render::StaticMesh>& mesh)
{
    const std::size_t vertexDataStartPosition = mesh->vertices.size();

    const float halfGrassWidth = width / 2.0f;
    const float halfGrassLength = height / 2.0f;

    auto positions = std::vector<glm::vec3> {
        {-halfGrassWidth, halfGrassLength, 0},
        {-halfGrassWidth, -halfGrassLength,0},
        {halfGrassWidth, -halfGrassLength,0},
        {halfGrassWidth, halfGrassLength,0}
    };

    const auto globalRotation = RotationBetweenVectors({0,1,0}, orientationUnit);
    const auto tuftRotation = glm::angleAxis(glm::radians(tuftRotationDegrees), orientationUnit);

    const auto rotation = tuftRotation * globalRotation;

    for (auto& pos : positions)
    {
        pos = (rotation * pos);
        pos += origin;

        // Move bottom of grass forward to the origin point
        pos += (orientationUnit * halfGrassLength);
    }

    const auto normal = glm::normalize(rotation * glm::vec3(0,1,0));
    std::vector<glm::vec3> normals = { normal, normal, normal, normal };

    std::vector<glm::vec2> uvs = {
        {0, 0},
        {0, 1},
        {1, 1},
        {1, 0},
    };

    // Create grass vertices
    for (unsigned int x = 0; x < positions.size(); ++x)
    {
        mesh->vertices.emplace_back(positions[x], normals[x], uvs[x]);
    }

    // Grass indices - front side
    mesh->indices.push_back(vertexDataStartPosition);
    mesh->indices.push_back(vertexDataStartPosition + 1);
    mesh->indices.push_back(vertexDataStartPosition + 2);

    mesh->indices.push_back(vertexDataStartPosition);
    mesh->indices.push_back(vertexDataStartPosition + 2);
    mesh->indices.push_back(vertexDataStartPosition + 3);

    // Grass indices - back side
    //
    // Define a back side, which is the same vertices as the front in an
    // opposite winding order, in order to have grass cast shadows -
    // shadow casting is done with front face culling and requires a
    // back side to exist for shadows to be cast
    mesh->indices.push_back(vertexDataStartPosition);
    mesh->indices.push_back(vertexDataStartPosition + 2);
    mesh->indices.push_back(vertexDataStartPosition + 1);

    mesh->indices.push_back(vertexDataStartPosition);
    mesh->indices.push_back(vertexDataStartPosition + 3);
    mesh->indices.push_back(vertexDataStartPosition + 2);
}

float GrassMeshCreator::Rand(float min, float max)
{
    return std::uniform_real_distribution<float>(min, max)(m_mt);
}

}

