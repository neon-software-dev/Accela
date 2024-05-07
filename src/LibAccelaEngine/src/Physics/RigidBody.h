/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_RIGIDBODY_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_RIGIDBODY_H

#include <Accela/Engine/Physics/PhysicsCommon.h>
#include <Accela/Engine/Bounds/Bounds.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <variant>

namespace Accela::Engine
{
    struct RigidBodyStaticData
    {
        // no-op
    };

    struct RigidBodyDynamicData
    {
        glm::vec3 linearVelocity{0.0f};
        float linearDamping{0.0f};
        float angularDamping{0.0f};
        std::array<bool, 3> axisMotionAllowed{true, true, true};
    };

    struct RigidBodyData
    {
        RigidBodyData(const RigidBodyType& _type,
                      const std::variant<RigidBodyStaticData, RigidBodyDynamicData>& _subData)
            : type(_type)
            , subData(_subData)
        { }

        RigidBodyType type{};

        float mass{0.0f};

        std::variant<RigidBodyStaticData, RigidBodyDynamicData> subData;
    };

    struct MaterialData
    {
        float staticFriction{1.0f};
        float dynamicFriction{1.0f};
        float restitution{0.1f};
    };

    struct ShapeData
    {
        explicit ShapeData(const BoundsVariant& _bounds,
                           const MaterialData& _material,
                           const glm::vec3& _scale = glm::vec3{1.0f},
                           const glm::vec3& _localTransform = glm::vec3(0),
                           const glm::quat& _localOrientation = glm::identity<glm::quat>())
            : bounds(_bounds)
            , material(_material)
            , scale(_scale)
            , localTransform(_localTransform)
            , localOrientation(_localOrientation)
        { }

        // Model-space shape bounds
        BoundsVariant bounds;

        // Material the shape uses
        MaterialData material;

        // Overall scale to apply to the defined shape
        glm::vec3 scale;

        // Local transform of the shape's bounds relative to the body's model space
        glm::vec3 localTransform;

        // Local orientation of the shape's bounds relative to the body's model space
        glm::quat localOrientation;
    };

    struct RigidActorData
    {
        explicit RigidActorData(const ShapeData& _shape,
                                const glm::vec3& _position = glm::vec3(0),
                                const glm::quat& _orientation = {glm::identity<glm::quat>()})
            : shape(_shape)
            , position(_position)
            , orientation(_orientation)
        { }

        ShapeData shape;
        glm::vec3 position;
        glm::quat orientation;
    };

    struct RigidBody
    {
        RigidBody(const RigidActorData& _actor, const RigidBodyData _body)
            : actor(_actor)
            , body(_body)
        { }

        RigidActorData actor;
        RigidBodyData body;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_RIGIDBODY_H
