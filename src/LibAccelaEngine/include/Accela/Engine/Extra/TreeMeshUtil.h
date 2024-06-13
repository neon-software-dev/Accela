/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_TREEMESHUTIL_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_TREEMESHUTIL_H

#include <Accela/Render/Mesh/StaticMesh.h>

#include <glm/glm.hpp>

#include <vector>
#include <array>
#include <random>
#include <string>

namespace Accela::Engine
{
    /**
     * Parameters defining how to generate tree definitions
     */
    struct TreeParams
    {
        // General
        float maturity{1.0f};                       // Specifies tree "maturity"; 0 = none, 1 = mature
                                                    //  Impacts factors such as number of and size of branches and
                                                    //  leaves (also, leaves only start to appear at maturity > 0.75)

        // Sun
        glm::vec3 sun_directionUnit{0,1,0};   // Unit vector from the tree's origin to a hypothetical sun
        float sun_strength{0.03f};                  // Strength with which the sun shines - will cause branches to tend
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
     * Parameters defining how to create tree meshes from tree definitions
     */
    struct TreeMeshParams
    {
        unsigned int numVerticesPerSegment{10};         // Number of vertices around each branch segment/loop
        float vertexAngleRandomizationPercent{0.1f};    // Variance applied to segment vertex angles to make triangles more irregular
    };

    /**
     * Utility class for generating tree definitions as well as creating meshes from tree definitions
     */
    class TreeMeshUtil
    {
        public:

            /**
             * An individual segment within a branch.
             */
            struct BranchSegment
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
            struct Leaf
            {
                glm::vec3 origin{0};            // The origin / starting point of the leaf
                glm::vec3 orientationUnit{0};   // The direction the leaf is oriented in
                float width{0.0f};                    // The width of the leaf
                float height{0.0f};                   // The height of the leaf
            };

            /**
             * A branch. Recursive structure that contains zero or more child branches. Leaf-level
             * branches contain zero or more child leaves. Defines all or part of a tree.
             */
            struct Branch
            {
                glm::vec3 origin{0.0f};            // Origin / starting point of the branch
                glm::vec3 orientationUnit{0,1,0}; // The direction the branch is oriented in
                float length{0.0f};                     // The total length of all the branch's segments

                std::vector<BranchSegment> segments;    // The segments that define the branch's geometry
                std::vector<Branch> childBranches;      // Child branches connected to this branch
                std::vector<Leaf> childLeaves;          // Child leaves connected to this branch
            };

        public:

            /**
             * @param seed seed used to initialize PRNG used to generate tree definitions
             */
            explicit TreeMeshUtil(const std::mt19937::result_type& seed = std::random_device{}());

            /**
             * Generate a new tree definitions from the provided tree parameters.
             *
             * @param params Parameters controlling tree generation
             *
             * @return The root/trunk branch of the generated tree (recursively containing the rest
             * of the tree definition within itself)
             */
            [[nodiscard]] Branch GenerateTree(const TreeParams& params);

            /**
             * Create meshes from a tree definition.
             *
             * @param params Parameters controlling mesh creation
             * @param tree The tree definition to build meshes from
             * @param tag A debug tag to associate with the tree meshes
             *
             * @return Two meshes - the first mesh contains branch geometry, the second mesh contains leaf geometry
             */
            [[nodiscard]] std::array<std::shared_ptr<Render::StaticMesh>, 2> CreateTreeMesh(
                const TreeMeshParams& params,
                const Branch& tree,
                const std::string& tag);

        private:

            //////////////////
            // Tree Generation
            //////////////////

            [[nodiscard]] Branch CreateBranch(const TreeParams& params,
                                              const glm::vec3& origin,
                                              const glm::vec3& orientationUnit,
                                              float startRadius,
                                              float branchLength,
                                              unsigned int level);

            [[nodiscard]] std::vector<BranchSegment> GenerateBranchSegments(const TreeParams& params,
                                                                            const glm::vec3& origin,
                                                                            const glm::vec3& orientationUnit,
                                                                            float startRadius,
                                                                            float branchLength,
                                                                            unsigned int level);

            void CreateBranches(Branch& parentBranch, const TreeParams& params, unsigned int level);

            // Length offset within the segment, segment index
            [[nodiscard]] std::pair<float, unsigned int> ChooseBranchSplitPoint(const TreeParams& params, const Branch& branch);

            [[nodiscard]] Leaf CreateLeaf(const TreeParams& params,
                                          const glm::vec3& origin,
                                          const glm::vec3& orientationUnit,
                                          bool rotate90);

            ////////////////
            // Mesh Creation
            ////////////////

            void AppendBranchGeometry(const TreeMeshParams& params, const Branch& branch, const std::shared_ptr<Render::StaticMesh>& mesh);

            [[nodiscard]] std::vector<Render::MeshVertex> GenerateSegmentVertices(const TreeMeshParams& params,
                                                                                  const glm::vec3& origin,
                                                                                  const glm::vec3& orientationUnit,
                                                                                  float radius,
                                                                                  float texV,
                                                                                  bool isFirstOrLastSegment);

            void AppendLeafGeometry(const Leaf& leaf, const std::shared_ptr<Render::StaticMesh>& mesh);

            /////////
            // Helper
            /////////

            [[nodiscard]] inline float Rand(float min, float max);

        private:

            std::mt19937 m_mt{0};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_TREEMESHUTIL_H
