/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_TREE_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_TREE_H

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <vector>

namespace Accela::Engine
{
    /**
     * An individual segment within a branch.
     */
    struct ACCELA_PUBLIC BranchSegment
    {
        glm::vec3 origin{0.0f};            // The origin / starting point of the segment
        glm::vec3 orientationUnit{0,1,0}; // The direction the segment is oriented in
        float startRadius{0.0f};                // Branch radius at the start of the segment
        float endRadius{0.0f};                  // Branch radius at the end of the segment
        float length{0.0f};                     // Length of the segment
    };

    /**
     * An individual leaf
     */
    struct ACCELA_PUBLIC Leaf
    {
        glm::vec3 origin{0};           // The origin / starting point of the leaf
        glm::vec3 orientationUnit{0};  // The direction the leaf is oriented in
        float width{0.0f};                  // The width of the leaf
        float height{0.0f};                 // The height of the leaf
    };

    /**
     * A branch. Recursive structure that contains zero or more child branches. Leaf-level
     * branches contain zero or more child leaves. Defines all or part of a tree.
     */
    struct ACCELA_PUBLIC Branch
    {
        glm::vec3 origin{0.0f};            // Origin / starting point of the branch
        glm::vec3 orientationUnit{0,1,0}; // The direction the branch is oriented in
        float length{0.0f};                     // The total length of all the branch's segments

        std::vector<BranchSegment> segments;    // The segments that define the branch's geometry

        std::vector<Branch> childBranches;      // Child branches connected to this branch
        std::vector<Leaf> childLeaves;          // Child leaves connected to this branch
    };

    /**
     * The base data structure defining a tree layout
     */
    struct ACCELA_PUBLIC Tree
    {
        Branch root{};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_TREE_H
