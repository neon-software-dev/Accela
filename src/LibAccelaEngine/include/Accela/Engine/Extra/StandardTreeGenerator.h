/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_STANDARDTREEGENERATOR_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_STANDARDTREEGENERATOR_H

#include <Accela/Engine/Extra/Tree.h>

#include <glm/glm.hpp>

#include <numbers>
#include <random>

namespace Accela::Engine
{
    /**
     * Parameters defining how to generate tree definitions
     */
    struct StandardTreeParams
    {
        // General
        float maturity{1.0f};                       // Specifies tree "maturity"; 0 = none, 1 = mature
        //  Impacts factors such as number of and size of branches and
        //  leaves (also, leaves only start to appear at maturity > 0.75)

        // Sun
        glm::vec3 sun_directionUnit{0,1,0};   // Unit vector from the tree's origin to a hypothetical sun
        float sun_strength{0.02f};                  // Strength with which the sun shines - will cause branches to tend
        //  to orient themselves towards the sun

        // Geometry
        float trunk_baseLength{4.0f};               // Length of the initial trunk branch (note that additional branch levels
        //  will cause more branches to be appended to the trunk and it will end up taller)
        float trunk_baseRadius{0.5f};               // Radius of the initial trunk branch
        float trunk_flarePercent{1.3f};             // Extra outwards flare applied to the root trunk segment

        // Branches
        unsigned int branch_numLevels{4};           // Levels of recursive branches
        unsigned int branch_numSegments{10};        // Number of segments/loops per branch
        float branch_taperPercent{0.4f};            // Ratio of branch end radius to branch start radius
        float branch_splitStartPercent{0.6f};       // Percentage through a branch's length after which branch children can be added
        float branch_splitEndPercent{0.9f};         // Percentage through a branch's length after which branch children can not be added
        unsigned int branch_minBranchChildren{3};   // Minimum number of branch children a branch can have
        unsigned int branch_maxBranchChildren{5};   // Maximum number of branch children a branch can have
        unsigned int branch_minLeafChildren{5};     // Minimum number of leaf children a branch can have
        unsigned int branch_maxLeafChildren{7};     // Maximum number of leaf children a branch can have
        float branch_minChildRadiusPercent{0.6f};   // Minimum ratio of child branch radius to parent branch radius
        float branch_maxChildRadiusPercent{0.9f};   // Maximum ratio of child branch radius to parent branch radius
        float branch_minChildLengthPercent{0.5f};   // Minimum ratio of child branch length to parent branch length
        float branch_maxChildLengthPercent{0.9f};   // Maximum ratio of child branch length to parent branch length
        float branch_sweepAngle{std::numbers::pi / 2.0f}; // Angle (radians) at which branches can sweep away from their parent (max of pi)
        float branch_gnarliness{0.05f};             // Amplitude of random angle added to each branch section's orientation
        float branch_gnarliness1_R{0.01f};          // Same as above, but inversely proportional to branch radius

        // Segments
        float segment_lengthVariance{0.1f};         // Variance applied to the length of each segment
        float segment_radiusVariance{0.1f};         // Variance applied to the radius of each segment

        // Leaves
        float leaf_width{2.0f};                     // Width of leaves (length is 1.5 times width)
        bool leaf_style_double{true};               // Whether to double leaves at 90 degree angles
        float leaf_sizeVariance{0.1f};              // Variance applied to the size of leaves
    };

    /**
     * Helper class which generates a standard/normal Tree definition from various parameters
     */
    class StandardTreeGenerator
    {
        public:

            /**
            * @param seed seed used to initialize PRNG used to generate tree definitions
            */
            explicit StandardTreeGenerator(const std::mt19937::result_type& seed = std::random_device{}());

            /**
             * Generate a new tree definitions from the provided tree parameters.
             *
             * @param params Parameters controlling tree generation
             *
             * @return The root/trunk branch of the generated tree (recursively containing the rest
             * of the tree definition within itself)
             */
            [[nodiscard]] Tree GenerateTree(const StandardTreeParams& params);

        private:

            [[nodiscard]] Branch CreateBranch(const StandardTreeParams& params,
                                              const glm::vec3& origin,
                                              const glm::vec3& orientationUnit,
                                              float startRadius,
                                              float branchLength,
                                              unsigned int level);

            [[nodiscard]] std::vector<BranchSegment> GenerateBranchSegments(const StandardTreeParams& params,
                                                                            const glm::vec3& origin,
                                                                            const glm::vec3& orientationUnit,
                                                                            float startRadius,
                                                                            float branchLength,
                                                                            unsigned int level);

            void CreateBranches(Branch& parentBranch, const StandardTreeParams& params, unsigned int level);

            // Length offset within the segment, segment index
            [[nodiscard]] std::pair<float, unsigned int> ChooseBranchSplitPoint(const StandardTreeParams& params, const Branch& branch);

            [[nodiscard]] Leaf CreateLeaf(const StandardTreeParams& params,
                                          const glm::vec3& origin,
                                          const glm::vec3& orientationUnit,
                                          bool rotate90);

            /////////
            // Helper
            /////////

            [[nodiscard]] inline float Rand(float min, float max);

        private:

            std::mt19937 m_mt;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_STANDARDTREEGENERATOR_H
