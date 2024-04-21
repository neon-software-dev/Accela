/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_UTIL_KDTREE_H
#define LIBACCELARENDERERVK_SRC_UTIL_KDTREE_H

#include "KDNode.h"

#include "../InternalCommon.h"

#include "../Renderer/RendererCommon.h"

#include <glm/glm.hpp>

#include <vector>
#include <functional>

namespace Accela::Render
{
    class KDTree
    {
        public:

            explicit KDTree(unsigned int maxMembersPerNode = 10);

            void AddMembers(const std::vector<KDNode::Member>& members);

            [[nodiscard]] std::vector<KDNode::Member> GetPotentiallyVisible(const Volume& volume) const;
            [[nodiscard]] std::vector<KDNode::Member> GetPotentiallyVisible(const glm::mat4& projection) const;

        private:

            struct SplitNodeResult
            {
                Volume beforeVolume;
                std::vector<KDNode::Member> beforeMembers;

                Volume afterVolume;
                std::vector<KDNode::Member> afterMembers;
            };

            using NotVisibleTest = std::function<bool(KDNode* pNode)>;

        private:

            void AddMember(const KDNode::Member& member);

            [[nodiscard]] static SplitNodeResult SplitNode(KDNode* pNode);
            [[nodiscard]] static float GetSplitPoint(KDNode* pNode);
            static void PopulateSplitMembers(KDNode* pNode, Axis axis, float splitPoint, SplitNodeResult& result);
            static void PopulateSplitVolumes(KDNode* pNode, Axis axis, float splitPoint, SplitNodeResult& result);

            [[nodiscard]] std::vector<KDNode::Member> GetPotentiallyVisible(const NotVisibleTest& notVisibleTest) const;

        private:

            unsigned int m_maxMembersPerNode;
            std::unique_ptr<KDNode> m_root;
    };
}

#endif //LIBACCELARENDERERVK_SRC_UTIL_KDTREE_H
