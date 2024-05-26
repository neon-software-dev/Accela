#ifndef LIBACCELARENDERERVK_SRC_UTIL_KDNODE_H
#define LIBACCELARENDERERVK_SRC_UTIL_KDNODE_H

#include "Volume.h"
#include "AABB.h"

#include "../InternalCommon.h"

#include <Accela/Render/Id.h>

#include <memory>
#include <set>
#include <vector>

namespace Accela::Render
{
    class KDNode
    {
        public:

            struct Member
            {
                Member(IdType _id, const Volume& _boundingVolume)
                    : id(_id)
                    , boundingVolume(_boundingVolume)
                { }

                IdType id;
                Volume boundingVolume;
            };

            struct AxisComparator
            {
                    explicit AxisComparator(const Axis& axis)
                        : m_axis(axis)
                    { }

                    [[nodiscard]] bool operator()(const Member& m1, const Member& m2) const;

                private:

                    Axis m_axis;
            };

        public:

            KDNode(const Volume& _boundingVolume, unsigned int _depth, const std::vector<Member>& _members = {});

            void AddMembers(const std::vector<Member>& members);
            void ConvertToParent(const Volume& beforeVolume,
                                 const std::vector<Member>& beforeMembers,
                                 const Volume& afterVolume,
                                 const std::vector<Member>& afterMembers);

            [[nodiscard]] Volume GetBoundingVolume() const noexcept;
            [[nodiscard]] unsigned int GetDepth() const noexcept;
            [[nodiscard]] Axis GetAxis() const noexcept;
            [[nodiscard]] KDNode* GetBefore() const noexcept;
            [[nodiscard]] KDNode* GetAfter() const noexcept;
            [[nodiscard]] bool ContainsVolume(const Volume& volume) const noexcept;
            [[nodiscard]] bool IsLeafNode() const noexcept;
            [[nodiscard]] const std::multiset<Member, AxisComparator>& GetMembers() const noexcept;
            [[nodiscard]] float GetMembersAxisAverage() const noexcept;

        private:

            [[nodiscard]] inline float GetAxisValue(const Member& member) const noexcept;

        private:

            Volume m_boundingVolume{};
            unsigned int m_depth{0};
            Axis m_axis;

            // Child Nodes
            std::unique_ptr<KDNode> m_pBefore;
            std::unique_ptr<KDNode> m_pAfter;

            // Leaf Members
            std::multiset<Member, AxisComparator> m_members;
            float m_memberAxisAverage{0.0f};
    };
}

#endif //LIBACCELARENDERERVK_SRC_UTIL_KDNODE_H
