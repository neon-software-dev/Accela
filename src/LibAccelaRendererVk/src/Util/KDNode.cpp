/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#include "KDNode.h"
#include "SpaceTests.h"

#include <cassert>

namespace Accela::Render
{

bool KDNode::AxisComparator::operator()(const KDNode::Member& m1, const KDNode::Member& m2) const
{
    switch (m_axis)
    {
        case Axis::X:return m1.boundingVolume.min.x < m2.boundingVolume.min.x;
        case Axis::Y: return m1.boundingVolume.min.y < m2.boundingVolume.min.y;
        case Axis::Z: return m1.boundingVolume.min.z < m2.boundingVolume.min.z;
    }

    assert(false);
    return false;
}

inline Axis DepthToAxis(const unsigned int& depth)
{
    return static_cast<Axis>(depth % 3);
}

inline float UpdateAverage(float avg, std::size_t n, float new_number) {
    return avg + (new_number - avg) / (static_cast<float>(n) + 1.0f);
}

KDNode::KDNode(const Volume& _boundingVolume, unsigned int _depth, const std::vector<Member>& _members)
    : m_boundingVolume(_boundingVolume)
    , m_depth(_depth)
    , m_axis(DepthToAxis(m_depth))
    , m_members(AxisComparator(m_axis))
{
    AddMembers(_members);
}

float KDNode::GetAxisValue(const KDNode::Member& member) const noexcept
{
    switch (m_axis)
    {
        case Axis::X: return member.boundingVolume.min.x;
        case Axis::Y: return member.boundingVolume.min.y;
        case Axis::Z: return member.boundingVolume.min.z;
    }

    assert(false);
    return 0.0f;
}

void KDNode::AddMembers(const std::vector<Member>& members)
{
    for (const auto& member : members)
    {
        m_memberAxisAverage = UpdateAverage(
            m_memberAxisAverage,
            m_members.size(),
            GetAxisValue(member)
        );

        m_members.insert(member);
    }
}

void KDNode::ConvertToParent(const Volume& beforeVolume,
                             const std::vector<Member>& beforeMembers,
                             const Volume& afterVolume,
                             const std::vector<Member>& afterMembers)
{
    assert(IsLeafNode());

    m_members.clear();
    m_pBefore = std::make_unique<KDNode>(beforeVolume, m_depth + 1, beforeMembers);
    m_pAfter = std::make_unique<KDNode>(afterVolume, m_depth + 1, afterMembers);
}

Volume KDNode::GetBoundingVolume() const noexcept
{
    return m_boundingVolume;
}

unsigned int KDNode::GetDepth() const noexcept
{
    return m_depth;
}

Axis KDNode::GetAxis() const noexcept
{
    return m_axis;
}

KDNode* KDNode::GetBefore() const noexcept
{
    return m_pBefore.get();
}

KDNode* KDNode::GetAfter() const noexcept
{
    return m_pAfter.get();
}

bool KDNode::ContainsVolume(const Volume& volume) const noexcept
{
    return Intersects(m_boundingVolume, volume);
}

bool KDNode::IsLeafNode() const noexcept
{
    return m_pBefore == nullptr;
}

const std::multiset<KDNode::Member, KDNode::AxisComparator>& KDNode::GetMembers() const noexcept
{
    return m_members;
}

float KDNode::GetMembersAxisAverage() const noexcept
{
    return m_memberAxisAverage;
}

}
