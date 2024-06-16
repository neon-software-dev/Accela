/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Extra/TreeMeshCreator.h>

#include "../Util/Math.h"

#include <glm/gtc/quaternion.hpp>

#include <numbers>
#include <format>
#include <queue>

namespace Accela::Engine
{


TreeMeshParams TreeMeshCreator::QualityBasedMeshParams(float minimumViewDistance)
{
    TreeMeshParams treeMeshParams{};

    //
    // Tweak vertices per segment depending on view distance
    //

    // Draw branches as cubes rather than cylindrical if viewed from further than 4 meters.
    if (minimumViewDistance >= 4.0f) { treeMeshParams.numVerticesPerSegment = 4; }
    else { treeMeshParams.numVerticesPerSegment = 10; }

    return treeMeshParams;
}

TreeMeshCreator::TreeMeshCreator(const std::mt19937::result_type& seed)
    : m_mt(seed)
{

}

TreeMesh TreeMeshCreator::CreateTreeMesh(const TreeMeshParams& params, const Tree& tree, const std::string& tag)
{
    TreeMesh treeMesh{};
    treeMesh.branchesMesh = std::make_shared<Render::StaticMesh>(Render::INVALID_ID, std::format("Branches-{}", tag));
    treeMesh.leavesMesh = std::make_shared<Render::StaticMesh>(Render::INVALID_ID, std::format("Leaves-{}", tag));

    //
    // DFS iterate over the tree and append geometry for all branches and leaves we encounter
    //
    std::queue<Branch> toProcess;
    toProcess.push(tree.root);

    bool isTrunkBranch = true;

    while (!toProcess.empty())
    {
        const auto branch = toProcess.front();
        toProcess.pop();

        // Append geometry for the branch
        AppendBranchGeometry(params, branch, treeMesh.branchesMesh);

        for (const auto& childLeaf : branch.childLeaves)
        {
            AppendLeafGeometry(childLeaf, treeMesh.leavesMesh);
        }

        // Push the branch's children into the queue for processing
        for (const auto& childBranch : branch.childBranches)
        {
            toProcess.push(childBranch);
        }

        // Fill out special output variables to hold data about the trunk's vertices/indices
        if (isTrunkBranch)
        {
            treeMesh.trunkVerticesStartIndex = 0;
            treeMesh.trunkVerticesCount = treeMesh.branchesMesh->vertices.size();
            treeMesh.trunkIndicesStartIndex = 0;
            treeMesh.trunkIndicesCount = treeMesh.branchesMesh->indices.size();
        }

        isTrunkBranch = false;
    }

    return treeMesh;
}

void TreeMeshCreator::AppendBranchGeometry(const TreeMeshParams& params, const Branch& branch, const std::shared_ptr<Render::StaticMesh>& mesh)
{
    if (branch.segments.empty()) { return; }

    // GenerateSegmentVertices appends an additional vertex the same as the starting vertex (but with
    // a different uv) to close out each segment loop, so there's always truly one more vertex per segment.
    const unsigned int trueSegmentNumVertices = params.numVerticesPerSegment + 1;

    //
    // Create Branch Vertices
    //
    const std::size_t branchRootVerticesStartIndex = mesh->vertices.size();

    // Special-case create the initial/root segment vertices
    const auto& firstSegment = branch.segments.at(0);

    const auto rootVertices = GenerateSegmentVertices(
        params,
        firstSegment.origin,
        firstSegment.orientationUnit,
        firstSegment.startRadius,
        0.0f,
        true
    );

    std::ranges::copy(rootVertices, std::back_inserter(mesh->vertices));

    // Create vertices for the end/back of each branch segment
    const std::size_t branchSegmentVerticesStartIndex = mesh->vertices.size();

    for (unsigned int segmentIndex = 0; segmentIndex < branch.segments.size(); ++segmentIndex)
    {
        const auto& segment = branch.segments[segmentIndex];

        const bool isFirstOrLastSegment = segmentIndex == 0 || segmentIndex == branch.segments.size() - 1;

        const float texV = (float)(segmentIndex + 1) / (float)branch.segments.size();

        const auto segmentVertices = GenerateSegmentVertices(
            params,
            segment.origin + (segment.orientationUnit * segment.length),
            segment.orientationUnit,
            segment.endRadius,
            texV,
            isFirstOrLastSegment
        );

        std::ranges::copy(segmentVertices, std::back_inserter(mesh->vertices));
    }

    //
    // Create Branch Indices
    //
    for (unsigned int segmentIndex = 0; segmentIndex < branch.segments.size(); ++segmentIndex)
    {
        const std::size_t vertexDataOffset = branchSegmentVerticesStartIndex + (segmentIndex * trueSegmentNumVertices);

        for (unsigned int vertexIndex = 0; vertexIndex < params.numVerticesPerSegment; ++vertexIndex)
        {
            // Special case handle indices for triangles which link downwards into the special case
            // initial/root segment vertices we added above
            if (segmentIndex == 0)
            {
                mesh->indices.push_back(vertexDataOffset + vertexIndex);
                mesh->indices.push_back(vertexDataOffset + vertexIndex + 1);
                mesh->indices.push_back(branchRootVerticesStartIndex + vertexIndex);

                mesh->indices.push_back(branchRootVerticesStartIndex + vertexIndex);
                mesh->indices.push_back(vertexDataOffset + vertexIndex + 1);
                mesh->indices.push_back(branchRootVerticesStartIndex + vertexIndex + 1);
            }
            else
            {
                mesh->indices.push_back(vertexDataOffset + vertexIndex);
                mesh->indices.push_back(vertexDataOffset + vertexIndex + 1);
                mesh->indices.push_back(vertexDataOffset + vertexIndex - trueSegmentNumVertices);

                mesh->indices.push_back(vertexDataOffset + vertexIndex - trueSegmentNumVertices);
                mesh->indices.push_back(vertexDataOffset + vertexIndex + 1);
                mesh->indices.push_back(vertexDataOffset + vertexIndex - trueSegmentNumVertices + 1);
            }
        }
    }
}

std::vector<Render::MeshVertex> TreeMeshCreator::GenerateSegmentVertices(const TreeMeshParams& params,
                                                                         const glm::vec3& origin,
                                                                         const glm::vec3& orientationUnit,
                                                                         float radius,
                                                                         float texV,
                                                                         bool isFirstOrLastSegment)
{
    const unsigned int trueSegmentNumVertices = params.numVerticesPerSegment + 1;

    std::vector<Render::MeshVertex> results;
    results.reserve(trueSegmentNumVertices);

    for (unsigned int vertexIndex = 0; vertexIndex < params.numVerticesPerSegment; ++vertexIndex)
    {
        float vertexAngleRads = ((2.0f * (float)std::numbers::pi) / (float)params.numVerticesPerSegment) * (float)vertexIndex;

        // Randomize the angle a bit to make triangles between segments more irregular. Don't do this on
        // the first or last segment so that branches that continue an existing branch line up correctly.
        if (!isFirstOrLastSegment)
        {
            vertexAngleRads += Rand(-params.vertexAngleRandomizationPercent, params.vertexAngleRandomizationPercent);
        }

        const float posX = glm::cos(vertexAngleRads);
        const float posZ = glm::sin(vertexAngleRads);

        const auto rotation = RotationBetweenVectors({0,1,0}, orientationUnit);

        const auto vertexPosition = (rotation * glm::vec3(posX * radius, 0, posZ * radius)) + origin;
        const auto vertexNormal = glm::normalize(rotation * glm::vec3(posX, 0, posZ));
        const auto vertexUv = glm::vec2((float)vertexIndex / (float)params.numVerticesPerSegment, texV);

        results.emplace_back(vertexPosition, vertexNormal, vertexUv);
    }

    // Duplicate the first vertex to close the loop with flush UVs
    auto finalVertex = results.at(0);
    finalVertex.uv = {1.0f, texV};
    results.push_back(finalVertex);

    return results;
}

void TreeMeshCreator::AppendLeafGeometry(const Leaf& leaf, const std::shared_ptr<Render::StaticMesh>& mesh)
{
    const std::size_t vertexDataStartPosition = mesh->vertices.size();

    const float halfLeafWidth = leaf.width / 2.0f;
    const float halfLeafLength = leaf.height / 2.0f;

    auto positions = std::vector<glm::vec3> {
        {-halfLeafWidth, halfLeafLength, 0},
        {-halfLeafWidth, -halfLeafLength,0},
        {halfLeafWidth, -halfLeafLength,0},
        {halfLeafWidth, halfLeafLength,0}
    };

    const auto rotation = RotationBetweenVectors({0,1,0}, leaf.orientationUnit);

    for (auto& pos : positions)
    {
        pos = rotation * pos;
        pos += leaf.origin;
        pos += (leaf.orientationUnit * halfLeafLength);
    }

    const auto normal = glm::normalize(rotation * glm::vec3(0,1,0));
    std::vector<glm::vec3> normals = { normal, normal, normal, normal };

    std::vector<glm::vec2> uvs = {
        {0, 0},
        {0, 1},
        {1, 1},
        {1, 0},
    };

    // Create leaf vertices
    for (unsigned int x = 0; x < positions.size(); ++x)
    {
        mesh->vertices.emplace_back(positions[x], normals[x], uvs[x]);
    }

    // Create leaf indices
    mesh->indices.push_back(vertexDataStartPosition);
    mesh->indices.push_back(vertexDataStartPosition + 1);
    mesh->indices.push_back(vertexDataStartPosition + 2);

    mesh->indices.push_back(vertexDataStartPosition);
    mesh->indices.push_back(vertexDataStartPosition + 2);
    mesh->indices.push_back(vertexDataStartPosition + 3);
}

float TreeMeshCreator::Rand(float min, float max)
{
    return std::uniform_real_distribution<float>(min, max)(m_mt);
}

}
