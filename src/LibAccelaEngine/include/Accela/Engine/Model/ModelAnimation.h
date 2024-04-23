/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELANIMATION_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELANIMATION_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace Accela::Engine
{
    struct PositionKeyFrame
    {
        PositionKeyFrame(const glm::vec3& _position, const double& _animationTime)
            : position(_position)
            , animationTime(_animationTime)
        { }

        glm::vec3 position;
        double animationTime{0};
    };

    struct RotationKeyFrame
    {
        RotationKeyFrame(const glm::quat& _rotation, const double& _animationTime)
            : rotation(_rotation)
            , animationTime(_animationTime)
        { }

        glm::quat rotation;
        double animationTime{0};
    };

    struct ScaleKeyFrame
    {
        ScaleKeyFrame(const glm::vec3& _scale, const double& _animationTime)
            : scale(_scale)
            , animationTime(_animationTime)
        { }

        glm::vec3 scale;
        double animationTime;
    };

    struct NodeKeyFrames
    {
        std::vector<PositionKeyFrame> positionKeyFrames{};
        std::vector<RotationKeyFrame> rotationKeyFrames{};
        std::vector<ScaleKeyFrame> scaleKeyFrames{};
    };

    /**
     * Defines the node-based keyframe data for a model animation
     */
    struct ModelAnimation
    {
        std::string animationName;
        double animationDurationTicks{0};
        double animationTicksPerSecond{0};

        // Node name -> bone key frames
        std::unordered_map<std::string, NodeKeyFrames> nodeKeyFrameMap;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_MODEL_MODELANIMATION_H
