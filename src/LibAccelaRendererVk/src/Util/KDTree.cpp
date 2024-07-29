/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "KDTree.h"
#include "GeometryUtil.h"

#include <queue>

namespace Accela::Render
{

KDTree::KDTree(unsigned int maxMembersPerNode)
    : m_maxMembersPerNode(maxMembersPerNode)
    , m_root(std::make_unique<KDNode>(Volume::EntireRange(), 0))
{

}

void KDTree::AddMembers(const std::vector<KDNode::Member>& members)
{
    for (const auto& member : members)
    {
        AddMember(member);
    }
}

void KDTree::AddMember(const KDNode::Member& member)
{
    //
    // Add the member to all the leaf nodes in the tree which contain the member's volume
    //
    std::queue<KDNode*> toProcess;
    toProcess.push(m_root.get());

    std::queue<KDNode*> modifiedNodes;

    while (!toProcess.empty())
    {
        KDNode* pNode = toProcess.front();
        toProcess.pop();

        if (!pNode->ContainsVolume(member.boundingVolume))
        {
            continue;
        }

        if (pNode->IsLeafNode())
        {
            pNode->AddMembers({member});
            modifiedNodes.push(pNode);
        }
        else
        {
            const auto pBefore = pNode->GetBefore();
            if (pBefore != nullptr) { toProcess.push(pBefore); }

            const auto pAfter = pNode->GetAfter();
            if (pAfter != nullptr) { toProcess.push(pAfter); }
        }
    }

    //
    // Process each modified node to split it down into sub-nodes, as needed
    //
    while (!modifiedNodes.empty())
    {
        KDNode* pNode = modifiedNodes.front();
        modifiedNodes.pop();

        // If the node has <= the max amount of members, it doesn't need to be split further
        if (pNode->GetMembers().size() <= m_maxMembersPerNode)
        {
            continue;
        }

        // Otherwise, split the node into two, and add the two new nodes into
        // the queue for further splitting, as needed
        const auto splitResult = SplitNode(pNode);

        // If we tried to split the node and ended up with a new node that has all
        // the same members (as in, the members all overlap the split point), give
        // up on splitting this node
        if (splitResult.beforeMembers.size() == pNode->GetMembers().size() ||
            splitResult.afterMembers.size() == pNode->GetMembers().size())
        {
            continue;
        }

        pNode->ConvertToParent(splitResult.beforeVolume, splitResult.beforeMembers, splitResult.afterVolume, splitResult.afterMembers);

        modifiedNodes.push(pNode->GetBefore());
        modifiedNodes.push(pNode->GetAfter());
    }
}

KDTree::SplitNodeResult KDTree::SplitNode(KDNode *pNode)
{
    SplitNodeResult result{};

    const float splitPoint = GetSplitPoint(pNode);

    PopulateSplitMembers(pNode, pNode->GetAxis(), splitPoint, result);
    PopulateSplitVolumes(pNode, pNode->GetAxis(), splitPoint, result);

    return result;
}

float KDTree::GetSplitPoint(KDNode *pNode)
{
    return pNode->GetMembersAxisAverage();
}

void KDTree::PopulateSplitMembers(KDNode *pNode, Axis axis, float splitPoint, KDTree::SplitNodeResult& result)
{
    const auto& nodeMembers = pNode->GetMembers();

    for (const auto& member : nodeMembers)
    {
        float minVal = 0.0f;
        float maxVal = 0.0f;

        switch (axis)
        {
            case Axis::X:
            {
                minVal = member.boundingVolume.min.x;
                maxVal = member.boundingVolume.max.x;
            }
            break;
            case Axis::Y:
            {
                minVal = member.boundingVolume.min.y;
                maxVal = member.boundingVolume.max.y;
            }
            break;
            case Axis::Z:
            {
                minVal = member.boundingVolume.min.z;
                maxVal = member.boundingVolume.max.z;
            }
            break;
        }

        // Note that members which overlap the split point are added to both
        // the before and the after list

        if (minVal <= splitPoint)
        {
            result.beforeMembers.push_back(member);
        }

        if (maxVal >= splitPoint)
        {
            result.afterMembers.push_back(member);
        }
    }
}

void KDTree::PopulateSplitVolumes(KDNode *pNode, Axis axis, float splitPoint, KDTree::SplitNodeResult& result)
{
    result.beforeVolume = pNode->GetBoundingVolume();
    result.afterVolume = pNode->GetBoundingVolume();

    switch (axis)
    {
        case Axis::X:
        {
            result.beforeVolume.max.x = splitPoint;
            result.afterVolume.min.x = splitPoint;
        }
        break;
        case Axis::Y:
        {
            result.beforeVolume.max.y = splitPoint;
            result.afterVolume.min.y = splitPoint;
        }
        break;
        case Axis::Z:
        {
            result.beforeVolume.max.z = splitPoint;
            result.afterVolume.min.z = splitPoint;
        }
        break;
    }
}

std::vector<KDNode::Member> KDTree::GetPotentiallyVisible(const Volume& volume) const
{
    return GetPotentiallyVisible([&](KDNode* pNode){
        return !Intersects(pNode->GetBoundingVolume(), volume);
    });
}

std::vector<KDNode::Member> KDTree::GetPotentiallyVisible(const glm::mat4& projection) const
{
    return GetPotentiallyVisible([&](KDNode* pNode){
        return VolumeTriviallyOutsideProjection(pNode->GetBoundingVolume(), projection);
    });
}

std::vector<KDNode::Member> KDTree::GetPotentiallyVisible(const NotVisibleTest& notVisibleTest) const
{
    std::vector<KDNode::Member> members;

    std::queue<KDNode*> toProcess;
    toProcess.push(m_root.get());

    while (!toProcess.empty())
    {
        KDNode *pNode = toProcess.front();
        toProcess.pop();

        if (notVisibleTest(pNode))
        {
            continue;
        }
        else if (pNode->IsLeafNode())
        {
            for (const auto& member : pNode->GetMembers())
            {
                members.push_back(member);
            }
        }
        else
        {
            const auto pBefore = pNode->GetBefore();
            if (pBefore != nullptr) { toProcess.push(pBefore); }

            const auto pAfter = pNode->GetAfter();
            if (pAfter != nullptr) { toProcess.push(pAfter); }
        }
    }

    return members;
}

}
