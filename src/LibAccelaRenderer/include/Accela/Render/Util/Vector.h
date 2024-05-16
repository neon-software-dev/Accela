/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_VECTOR_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_VECTOR_H

#include <glm/glm.hpp>

namespace Accela::Render
{
    /**
     * Answers whether two *unit* vectors are (sufficiently close to) parallel.
     *
     * Warning: You will get the wrong answers if you don't pass in unit vectors.
     *
     * @param a Unit vector a
     * @param b Unit vector b
     *
     * @return Whether the unit vectors are within .01% of parallel
     */
    static inline bool AreUnitVectorsParallel(const glm::vec3& a, const glm::vec3& b)
    {
        return glm::abs(glm::dot(a, b)) > .9999f;
    }

    /**
     * If two vectors are parallel, will return an alternate vector, otherwise will return
     * the original query vector.
     *
     * - If queryVec and constantVec are not parallel, will return queryVec.
     * - If queryVec and constantVec are parallel, and both point in the same direction, will return alternateQueryVec.
     * - If queryVec and constantVec are parallel, and both point in opposite directions, will return -alternateQueryVec.
     *
     * @param queryVec The vector that can be swapped with an alternative if parallel
     * @param constantVec The vector to test queryVec against
     * @param alternateQueryVec An alternate vector to be used if queryVec and constantVec are parallel
     *
     * @return queryVec if not parallel with constantVec, otherwise positive or negative alternateQueryVec, depending
     * on whether queryVec and constantVec are aligned in the same direction or not.
     */
    [[nodiscard]] static glm::vec3 EnsureNotParallel(const glm::vec3& queryVec,
                                                     const glm::vec3& constantVec,
                                                     const glm::vec3& alternateQueryVec)
    {
        // If the vectors aren't parallel then we're good, return the query vector
        if (!AreUnitVectorsParallel(glm::normalize(constantVec), glm::normalize(queryVec)))
        {
            return queryVec;
        }

        // Determine if the query vector is looking straight at the constant vector, or straight away from it
        const bool sameDirection =
            ((constantVec.x >= 0.0f) == (queryVec.x >= 0.0f)) &&
            ((constantVec.y >= 0.0f) == (queryVec.y >= 0.0f)) &&
            ((constantVec.z >= 0.0f) == (queryVec.z >= 0.0f));

        // If aligned with the constant vector, return the alternate query vec
        if (sameDirection)  { return alternateQueryVec; }

        // If aligned away from the constant vec, return the reverse of the alternate query vec
        else                { return -alternateQueryVec; }
    }

    /**
     * Helper struct for making clear statements such as: const auto v = This(x).ButIfParallelWith(y).Then(z)
     */
    struct IfParallel
    {
        IfParallel(const glm::vec3& _query, const glm::vec3& _constant)
            : query(_query)
            , constant(_constant)
        { }

        [[nodiscard]] glm::vec3 Then(const glm::vec3& alt)
        {
            return EnsureNotParallel(query, constant, alt);
        }

        private:

            glm::vec3 query;
            glm::vec3 constant;
    };

    /**
     * Helper struct for making clear statements such as: const auto v = This(x).ButIfParallelWith(y).Then(z)
     */
    struct This
    {
        explicit This(const glm::vec3& _query)
            : query(_query)
        { }

        [[nodiscard]] IfParallel ButIfParallelWith(const glm::vec3& constant)
        {
            return IfParallel(query, constant);
        }

        private:

            glm::vec3 query;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_UTIL_VECTOR_H
