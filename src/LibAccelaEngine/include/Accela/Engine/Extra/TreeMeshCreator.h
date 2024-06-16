/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_TREEMESHCREATOR_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_TREEMESHCREATOR_H

#include "Tree.h"

#include <Accela/Render/Mesh/StaticMesh.h>

#include <glm/glm.hpp>

#include <vector>
#include <random>
#include <string>

namespace Accela::Engine
{
    /**
     * Parameters defining how to create tree meshes from tree definitions
     */
    struct TreeMeshParams
    {
        unsigned int numVerticesPerSegment{10};         // Number of vertices around each branch segment/loop
        float vertexAngleRandomizationPercent{0.1f};    // Variance applied to segment vertex angles to make triangles more irregular

        // TODO Perf: Also simplify leaf mesh for distant trees - simple textured dome?
    };

    struct TreeMesh
    {
        std::shared_ptr<Render::StaticMesh> branchesMesh;
        std::shared_ptr<Render::StaticMesh> leavesMesh;

        // Helper values which define where the base trunk's vertices
        // and indices are located. Mostly intended for generating physics
        // bounds from in order to add basic collision to tree meshes.
        std::size_t trunkVerticesStartIndex{0};
        std::size_t trunkVerticesCount{0};
        std::size_t trunkIndicesStartIndex{0};
        std::size_t trunkIndicesCount{0};
    };

    /**
     * Utility class for generating tree definitions as well as creating meshes from tree definitions
     */
    class TreeMeshCreator
    {
        public:

            /**
            * @param seed seed used to initialize PRNG used to generate tree definitions
            */
            explicit TreeMeshCreator(const std::mt19937::result_type& seed = std::random_device{}());

            /**
             * Create tree mesh parameters based on intended minimum view distance. Will cause
             * lower-quality meshes to be generated for objects that will be viewed from a further
             * distance.
             *
             * @param minimumViewDistance The intended minimum distance the object will be viewed from.
             */
            [[nodiscard]] static TreeMeshParams QualityBasedMeshParams(float minimumViewDistance);

            /**
             * Create meshes from a tree definition.
             *
             * @param params Parameters controlling mesh creation
             * @param tree The tree definition to build meshes from
             * @param tag A debug tag to associate with the tree meshes
             *
             * @return A TreeMesh structure containing the mesh data
             */
            [[nodiscard]] TreeMesh CreateTreeMesh(const TreeMeshParams& params, const Tree& tree, const std::string& tag);

        private:

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

            std::mt19937 m_mt;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_EXTRA_TREEMESHCREATOR_H
