#ifndef LIBACCELARENDERERVK_SRC_UTIL_AABB_H
#define LIBACCELARENDERERVK_SRC_UTIL_AABB_H

#include "Volume.h"

#include <glm/glm.hpp>

#include <vector>

namespace Accela::Render
{
    class AABB
    {
        public:

            AABB();
            explicit AABB(const Volume& volume);
            explicit AABB(const std::vector<glm::vec3>& points);

            bool operator==(const AABB& other) const
            {
                return m_volume == other.m_volume;
            }

            void AddPoints(const std::vector<glm::vec3>& points);
            void AddVolume(const Volume& volume);

            [[nodiscard]] bool IsEmpty() const noexcept;
            [[nodiscard]] Volume GetVolume() const noexcept;

        private:

            Volume m_volume;
    };
}

#endif //LIBACCELARENDERERVK_SRC_UTIL_AABB_H
