/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_PHYSICSCOMMON_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_PHYSICSCOMMON_H

#include <Accela/Engine/Common.h>
#include <Accela/Engine/Bounds/Bounds.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <variant>
#include <string>

namespace Accela::Engine
{
    enum class RigidBodyType
    {
        /** Infinite mass, manually controlled */
        Static,

        /** Specific mass, manually controlled */
        Kinematic,

        /** Specific mass, physics controlled */
        Dynamic
    };

    enum class ShapeUsage
    {
        /** The shape will take part in normal physics simulation */
        Simulation,

        /** The shape will be used as a trigger and not take part in the physics simulation */
        Trigger
    };

    /**
     * Defines the material a shape in the physics simulation uses
     */
    struct PhysicsMaterial
    {
        float staticFriction{1.0f};
        float dynamicFriction{1.0f};
        float restitution{0.1f};

        auto operator<=>(const PhysicsMaterial&) const = default;
    };

    /**
     * Describes the shape of something within the physics simulation
     */
    struct PhysicsShape
    {
        explicit PhysicsShape(const BoundsVariant& _bounds,
                              const glm::vec3& _localTransform = glm::vec3(0),
                              const glm::quat& _localOrientation = glm::identity<glm::quat>())
            : bounds(_bounds)
            , localTransform(_localTransform)
            , localOrientation(_localOrientation)
        { }

        /** Model-space bounds defining the shape */
        BoundsVariant bounds;

        /** Additional local transform of the bounds relative to entity's model space (defaults to model-space origin)*/
        glm::vec3 localTransform;

        /** Additional local orientation of the bounds relative to the entity's model space (defaults to none) */
        glm::quat localOrientation;

        /**
         * Whether the shape is part of the physics simulation or a trigger shape.
         *
         * Note: If set to Trigger, the shape will not take part in the physics simulation and will only be used
         * as a trigger shape. This means it should probably be only attached to a static or kinematic body which
         * is manually controlled.
         */
        ShapeUsage usage{ShapeUsage::Simulation};
    };

    /**
     * Represents a physics trigger event - when an entity with a physics shape with usage=Trigger is
     * touched by another entity or a player.
     */
    struct PhysicsTriggerEvent
    {
        enum class Type
        {
            /** The trigger was touched by something */
            TouchFound,

            /** Something touching the trigger is no longer touching it */
            TouchLost
        };

        /** Describes what (was) touching the trigger - either an entity or a player */
        using TriggerOther = std::variant<EntityId, std::string>;

        PhysicsTriggerEvent(Type _type, EntityId _triggeredEntityId, TriggerOther _triggerOther)
            : type(_type)
            , triggeredEntityId(_triggeredEntityId)
            , triggerOther(std::move(_triggerOther))
        { }

        /** The trigger event type - touch found or lost */
        Type type{Type::TouchFound};

        /** EntityId of the entity that was triggered */
        EntityId triggeredEntityId{};

        /** What triggered the trigger entity */
        TriggerOther triggerOther{};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_PHYSICSCOMMON_H
