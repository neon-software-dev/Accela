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
        explicit PhysicsShape(const PhysicsMaterial& _material,
                              const BoundsVariant& _bounds,
                              const ShapeUsage& _usage = ShapeUsage::Simulation,
                              const glm::vec3& _localScale = glm::vec3(1),
                              const glm::vec3& _localTransform = glm::vec3(0),
                              const glm::quat& _localOrientation = glm::identity<glm::quat>())
            : material(_material)
            , bounds(_bounds)
            , usage(_usage)
            , localScale(_localScale)
            , localTransform(_localTransform)
            , localOrientation(_localOrientation)
        { }

        /** The material applied to the shape */
        PhysicsMaterial material;

        /** Model-space bounds defining the shape */
        BoundsVariant bounds;

        /**
        * Whether the shape is part of the physics simulation or a trigger shape.
        *
        * Note: If set to Trigger, the shape will not take part in the physics simulation and will only be used
        * as a trigger shape.
        */
        ShapeUsage usage;

        /** Additional local scale applied to the shape's bounds */
        glm::vec3 localScale;

        /** Additional local transform applied to the shape's bounds, relative to the entity's model
         * space (defaults to none) */
        glm::vec3 localTransform;

        /** Additional local orientation applied to the shape's bounds, relative to the entity's model
         * space (defaults to none) */
        glm::quat localOrientation;
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
        using TriggerOther = std::variant<EntityId, PlayerControllerName>;

        PhysicsTriggerEvent(PhysicsSceneName _scene, Type _type, EntityId _triggeredEntityId, TriggerOther _triggerOther)
            : scene(std::move(_scene))
            , type(_type)
            , triggeredEntityId(_triggeredEntityId)
            , triggerOther(std::move(_triggerOther))
        { }

        /** The scene the event is for */
        PhysicsSceneName scene;

        /** The trigger event type - touch found or lost */
        Type type{Type::TouchFound};

        /** EntityId of the entity that was triggered */
        EntityId triggeredEntityId{};

        /** What triggered the trigger entity */
        TriggerOther triggerOther{};
    };

    struct PhysicsSceneParams
    {
        glm::vec3 gravity{0.0f, -9.81f, 0.0f};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_PHYSICSCOMMON_H
