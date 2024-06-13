/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Extra/TreeMeshUtil.h>

#include "../Util/Math.h"

#include <glm/gtc/quaternion.hpp>

#include <numbers>
#include <format>
#include <queue>

namespace Accela::Engine
{

TreeMeshUtil::TreeMeshUtil(const std::mt19937::result_type& seed)
    : m_mt(seed)
{

}

TreeMeshUtil::Branch TreeMeshUtil::GenerateTree(const TreeParams& params)
{
    // Create the root/trunk branch
    auto trunk = CreateBranch(
        params,
        {0,0,0},
        {0,1,0},
        params.trunk_baseRadius * (params.maturity / 2.0f),
        params.trunk_baseLength * params.maturity,
        0
    );

    // Recursively create sub-branches as needed
    CreateBranches(trunk, params, 1);

    return trunk;
}

TreeMeshUtil::Branch TreeMeshUtil::CreateBranch(const TreeParams& params,
                                                const glm::vec3& origin,
                                                const glm::vec3& orientationUnit,
                                                float startRadius,
                                                float branchLength,
                                                unsigned int level)
{
    Branch branch{};
    branch.origin = origin;
    branch.orientationUnit = orientationUnit;
    branch.segments = GenerateBranchSegments(params, origin, orientationUnit, startRadius, branchLength, level);

    // Manually compute the branch's total length from its segments, as GenerateBranchSegments adds some random
    // variance to each segment's length, so the branch's final length might not be equal to branchLength
    for (const auto& segment : branch.segments)
    {
        branch.length += segment.length;
    }

    return branch;
}

std::vector<TreeMeshUtil::BranchSegment> TreeMeshUtil::GenerateBranchSegments(const TreeParams& params,
                                                                              const glm::vec3& origin,
                                                                              const glm::vec3& orientationUnit,
                                                                              float startRadius,
                                                                              float branchLength,
                                                                              unsigned int level)
{
    std::vector<BranchSegment> segments(params.branch_numSegments);

    const bool isTrunkBranch = level == 0;
    const float segmentLength = branchLength / params.branch_numSegments;
    const float taperFactor = 1.0f - ((1.0f - params.branch_taperPercent) / segments.size());

    glm::vec3 segmentOrigin = origin;
    glm::vec3 segmentOrientationUnit = orientationUnit;
    float segmentStartRadius = startRadius;

    // Generate the branch's segments
    for (unsigned int segmentIndex = 0; segmentIndex < params.branch_numSegments; ++segmentIndex)
    {
        const bool isFirstSegment = segmentIndex == 0;

        const float radiusVarianceFactor = 1.0f + Rand(-params.segment_radiusVariance, params.segment_radiusVariance);
        const float lengthVarianceFactor = 1.0f + Rand(-params.segment_lengthVariance, params.segment_lengthVariance);

        float trunkFlareFactor = 1.0f;
        if (isTrunkBranch && isFirstSegment)
        {
            trunkFlareFactor = params.trunk_flarePercent;
        }

        //
        // Set this segment's parameters
        //

        auto& segment = segments[segmentIndex];
        segment.origin = segmentOrigin;
        segment.orientationUnit = segmentOrientationUnit;
        segment.length = segmentLength * lengthVarianceFactor;
        segment.startRadius = segmentStartRadius * trunkFlareFactor;
        segment.endRadius = segment.startRadius * taperFactor * radiusVarianceFactor;

        //
        // Update the parameters for values that the next segment will be built from
        //

        // Move the origin forward to the next segment's starting position
        segmentOrigin += (segmentOrientationUnit * segmentLength);

        // Set the next segment to use the same orientation, although this will likely be modified further below
        segmentOrientationUnit = segment.orientationUnit;

        // The next segment's start radius is this segment's end radius
        segmentStartRadius = segment.endRadius;

        //
        // Manipulate the parameters that the next segment will be built from
        //

        // Rotation to get to the next segment's orientation
        glm::quat segmentRotation = RotationBetweenVectors({0,1,0}, segmentOrientationUnit);

        // Apply a gnarliness factor to rotate the next segment's orientation differently than this one's
        const float gnarliness = params.maturity * (params.branch_gnarliness + params.branch_gnarliness1_R / segment.startRadius);
        const auto xGnarliness = glm::angleAxis(Rand(-gnarliness, gnarliness), glm::vec3(1,0,0));
        const auto yGnarliness = glm::angleAxis(Rand(-gnarliness, gnarliness), glm::vec3(0,1,0));
        const auto zGnarliness = glm::angleAxis(Rand(-gnarliness, gnarliness), glm::vec3(0,0,1));

        segmentRotation = xGnarliness * yGnarliness * zGnarliness * segmentRotation;
        segmentOrientationUnit = glm::normalize(segmentRotation * glm::vec3(0,1,0));

        //
        // Add a sun seeking-force to rotate the next segment's orientation towards the sun
        //
        const glm::quat rotationBetweenSegmentAndSun = RotationBetweenVectors(segmentOrientationUnit, params.sun_directionUnit);
        // Mix between no additional rotation and rotation needed for the segment to reach the sun
        const glm::quat sunForceRotation = glm::mix(glm::identity<glm::quat>(), rotationBetweenSegmentAndSun, params.sun_strength);

        segmentRotation = sunForceRotation * segmentRotation;
        segmentOrientationUnit = glm::normalize(segmentRotation * glm::vec3(0,1,0));
    }

    //
    // Additional manipulations now that all segments have been created
    //

    // Scale the radius of each segment downwards by the tree's maturity level
    const float maturityRadiusFactor = std::pow(params.maturity, 2.0f);

    for (unsigned int segmentIndex = 0; segmentIndex < segments.size(); ++segmentIndex)
    {
        const bool isFirstSegment = segmentIndex == 0;

        auto& segment = segments[segmentIndex];

        // Don't adjust start radius down for the first segment in a non-trunk branch,
        // as some branches continue the trunk branch, and we want their starting radius
        // to match the ending radius of that trunk branch
        if (!isTrunkBranch && !isFirstSegment)
        {
            segment.startRadius *= maturityRadiusFactor;
        }

        segment.endRadius *= maturityRadiusFactor;
    }

    return segments;
}

void TreeMeshUtil::CreateBranches(Branch& parentBranch, const TreeParams& params, unsigned int level)
{
    // Bail out if we've hit max recursion depth
    if (level > params.branch_numLevels)
    {
        return;
    }

    const bool isLeafLevel = level == params.branch_numLevels;

    // Determine how many children to create off of the parent branch
    const auto minChildren = isLeafLevel ? params.branch_minLeafChildren : params.branch_minBranchChildren;
    const auto maxChildren = isLeafLevel ? params.branch_maxLeafChildren : params.branch_maxBranchChildren;
    const auto numChildren = std::round(Rand(0.0f, 1.0f) * (maxChildren - minChildren)) + minChildren;

    // Create children (whether branches or leaves)
    for (unsigned int childIndex = 0; childIndex < numChildren; ++childIndex)
    {
        const bool isLastChild = childIndex == numChildren - 1;

        // Parameters which define the child
        glm::vec3 childOrigin{0.0f};
        glm::vec3 childOrientationUnit{0.0f};
        float childStartRadius{0.0f};

        // Force the last child of a branch to always sprout directly from the last segment of the parent branch (this
        // allows the trunk to grow longer as more branch levels are added, and lets a leaf sprout directly outwards
        // from each leaf-level branch).
        if (isLastChild)
        {
            const auto& lastSegment = parentBranch.segments.at(parentBranch.segments.size() - 1);

            childOrigin = lastSegment.origin + (lastSegment.orientationUnit * lastSegment.length);
            childOrientationUnit = lastSegment.orientationUnit;
            childStartRadius = lastSegment.endRadius;
        }
        // Otherwise, the child can sprout from wherever on the parent branch is allowed
        else
        {
            //
            // Determine where and in which parent segment to split a child off from
            //
            const auto split = ChooseBranchSplitPoint(params, parentBranch);
            const auto splitSegmentOffset = split.first;
            const auto splitSegmentIndex = split.second;
            const auto& splitSegment = parentBranch.segments.at(splitSegmentIndex);
            const auto splitSegmentRotation = RotationBetweenVectors({0,1,0}, splitSegment.orientationUnit);

            //
            // Set the child's origin to be the split point we just calculated
            //
            childOrigin = splitSegment.origin + (splitSegment.orientationUnit * splitSegmentOffset);

            //
            // Set the child's orientation to be oriented differently from the parent segment's orientation.
            //

            // Rotate the child branch "outwards" from its parent. By default, the child branch is oriented
            // in the same direction as the parent segment. We want to "swing" that orientation some amount
            // away towards the opposite of the parent segment's orientation. (Note that there's an infinite
            // number of ways to do this). The sweep angle parameter defines the maximum deflection away from
            // the parent orientation that's allowed.

            // Enforce a max allowed value of pi for the sweep angle parameter
            const float sweepAngle = std::min(params.branch_sweepAngle, (float)std::numbers::pi);

            // Factor used to mix between fully parallel and fully anti-parallel with the parent orientation
            float sweepAngleFactor = Rand(0.0f, sweepAngle / std::numbers::pi);

            // Enforce a minimum sweep factor of .2 (~12 degrees) to prevent child branches from being too aligned
            // with their parent
            sweepAngleFactor = std::max(0.2f, sweepAngleFactor);

            // Rotation that would keep the child orientation the same as the parent segment's
            glm::quat parentRot = glm::identity<glm::quat>();
            // Rotation that would make the child orientation completely opposite the parent segment's
            glm::quat antiParentRot = RotationBetweenVectors(splitSegment.orientationUnit, -splitSegment.orientationUnit);
            // Mix between the two extremes
            const glm::quat rotationOutwardsFromParent = glm::mix(parentRot, antiParentRot, sweepAngleFactor);

            // Now that we've swept the child away from its parent, rotate it some random amount around the axis of
            // its parent. Note that RotationBetweenVectors above, when given anti-parallel vectors, will always choose
            // the same arbitrary axis for the rotation, so this step is needed to actually distribute the branches
            // randomly around the parent branch axis rather than all in a line.
            const float axisRot = Rand(0.0f, 2.0f * std::numbers::pi);
            const glm::quat rotationAroundParent = glm::angleAxis(axisRot, splitSegment.orientationUnit);

            const auto childRotation = rotationAroundParent * rotationOutwardsFromParent * splitSegmentRotation;
            childOrientationUnit = glm::normalize(childRotation * glm::vec3(0,1,0));

            //
            // Set the child's start radius
            //
            const float childRadiusFactor = Rand(params.branch_minChildRadiusPercent, params.branch_maxChildRadiusPercent);
            childStartRadius = splitSegment.endRadius * childRadiusFactor;
        }

        // If we're on the leaf level, create a leaf
        if (isLeafLevel)
        {
            parentBranch.childLeaves.push_back(CreateLeaf(params, childOrigin, childOrientationUnit, false));

            if (params.leaf_style_double)
            {
                parentBranch.childLeaves.push_back(CreateLeaf(params, childOrigin, childOrientationUnit, true));
            }
        }
        // Otherwise, create a child branch
        else
        {
            unsigned int branchIndex = parentBranch.childBranches.size();

            const float childBranchLengthFactor = Rand(params.branch_minChildLengthPercent, params.branch_maxChildLengthPercent);
            float childBranchLength = parentBranch.length * childBranchLengthFactor;
            childBranchLength *= std::min(1.0f, childBranchLength * params.maturity);

            parentBranch.childBranches.push_back(CreateBranch(
                params,
                childOrigin,
                childOrientationUnit,
                childStartRadius,
                childBranchLength,
                level
            ));

            CreateBranches(parentBranch.childBranches.at(branchIndex), params,level + 1);
        }
    }
}

std::pair<float, unsigned int> TreeMeshUtil::ChooseBranchSplitPoint(const TreeParams& params, const TreeMeshUtil::Branch& branch)
{
    // Determine the length along the branch to split a child off
    const float splitFactor = Rand(params.branch_splitStartPercent, params.branch_splitEndPercent);
    const float splitPoint = branch.length * splitFactor;

    // Traverse through the branch's segments to find the segment which contains the split point
    float traversedLength = 0.0f;

    for (unsigned int segmentIndex = 0; segmentIndex < branch.segments.size(); ++segmentIndex)
    {
        const auto& segment = branch.segments[segmentIndex];

        traversedLength += segment.length;

        if (traversedLength >= splitPoint)
        {
            return std::make_pair(segment.length - (traversedLength - splitPoint), segmentIndex);
        }
    }

    // Shouldn't ever be the case
    assert(false);
    return std::pair<float, unsigned int>(0.0f, 0);
}

TreeMeshUtil::Leaf TreeMeshUtil::CreateLeaf(const TreeParams& params,
                                            const glm::vec3& origin,
                                            const glm::vec3& orientationUnit,
                                            bool rotate90)
{
    Leaf leaf{};
    leaf.origin = origin;

    const glm::quat globalRotation = RotationBetweenVectors({0,1,0}, orientationUnit);

    const glm::quat localRotation = glm::angleAxis(
        rotate90 ? (float)(std::numbers::pi / 2.0f) : 0.0f,
        glm::vec3(0,1,0)
    );

    leaf.orientationUnit = (globalRotation * localRotation) * orientationUnit;

    float leafWidth = params.leaf_width;
    leafWidth *= (1.0f + Rand(-params.leaf_sizeVariance, params.leaf_sizeVariance));
    leafWidth = std::max(0.0f, leafWidth * (params.maturity - 0.75f) * 4.0f);

    leaf.width = leafWidth;
    leaf.height = (1.5f * leafWidth);

    return leaf;
}

float TreeMeshUtil::Rand(float min, float max)
{
    return std::uniform_real_distribution<float>(min, max)(m_mt);
}

//

std::array<std::shared_ptr<Render::StaticMesh>, 2> TreeMeshUtil::CreateTreeMesh(const TreeMeshParams& params,
                                                                                const Branch& tree,
                                                                                const std::string& tag)
{
    const auto branchesMesh = std::make_shared<Render::StaticMesh>(Render::INVALID_ID, std::format("Branches-{}", tag));
    const auto leavesMesh = std::make_shared<Render::StaticMesh>(Render::INVALID_ID, std::format("Leaves-{}", tag));

    //
    // DFS iterate over the tree and append geometry for all branches and leaves we encounter
    //
    std::queue<Branch> toProcess;
    toProcess.push(tree);

    while (!toProcess.empty())
    {
        const auto branch = toProcess.front();
        toProcess.pop();

        // Append geometry for the branch
        AppendBranchGeometry(params, branch, branchesMesh);

        for (const auto& childLeaf : branch.childLeaves)
        {
            AppendLeafGeometry(childLeaf, leavesMesh);
        }

        // Push the branch's children into the queue for processing
        for (const auto& childBranch : branch.childBranches)
        {
            toProcess.push(childBranch);
        }
    }

    return {branchesMesh, leavesMesh};
}

void TreeMeshUtil::AppendBranchGeometry(const TreeMeshParams& params, const Branch& branch, const std::shared_ptr<Render::StaticMesh>& mesh)
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

std::vector<Render::MeshVertex> TreeMeshUtil::GenerateSegmentVertices(const TreeMeshParams& params,
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
        float vertexAngleRads = ((2.0f * std::numbers::pi) / params.numVerticesPerSegment) * vertexIndex;

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

void TreeMeshUtil::AppendLeafGeometry(const Leaf& leaf, const std::shared_ptr<Render::StaticMesh>& mesh)
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

    const auto normal = rotation * glm::vec3(0,1,0);
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

}
