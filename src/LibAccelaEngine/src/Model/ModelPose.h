#ifndef LIBACCELAENGINE_SRC_MODEL_MODELPOSE_H
#define LIBACCELAENGINE_SRC_MODEL_MODELPOSE_H

#include "RegisteredModel.h"

#include <Accela/Render/Renderable/ObjectRenderable.h>

#include <glm/glm.hpp>

#include <vector>
#include <sstream>

namespace Accela::Engine
{
    struct NodeMeshId
    {
        unsigned int nodeId{0};
        unsigned int meshIndex{0}; // Note this is the index the mesh is listed in the node, not the index into the model's mesh collection

        auto operator<=>(const NodeMeshId&) const = default;

        struct HashFunction
        {
            std::size_t operator()(const NodeMeshId& o) const
            {
                std::stringstream ss;
                ss << o.nodeId << "-" << o.meshIndex;
                return std::hash<std::string>{}(ss.str());
            }
        };
    };

    struct MeshPoseData
    {
        NodeMeshId id;
        LoadedModelMesh modelMesh;
        glm::mat4 nodeTransform{1.0f};
    };

    struct BoneMesh
    {
        // Mesh data
        MeshPoseData meshPoseData;

        // Skeleton data
        std::vector<glm::mat4> boneTransforms;
    };

    struct ModelPose
    {
        // The data of a model's basic meshes in a particular pose
        std::vector<MeshPoseData> meshPoseDatas;

        // The data of a model's skeleton-based meshes in a particular pose
        std::vector<BoneMesh> boneMeshes;
    };
}

#endif //LIBACCELAENGINE_SRC_MODEL_MODELPOSE_H
