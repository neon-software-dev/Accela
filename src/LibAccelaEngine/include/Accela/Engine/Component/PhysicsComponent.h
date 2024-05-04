/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_PHYSICSCOMPONENT_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_PHYSICSCOMPONENT_H

#include <Accela/Engine/Physics/PhysicsCommon.h>

#include <glm/glm.hpp>

#include <array>

namespace Accela::Engine
{
    static constexpr float DEFAULT_FRICTION_STATIC_COEFFICIENT = 1.0f;
    static constexpr float DEFAULT_FRICTION_DYNAMIC_COEFFICIENT = 1.0f;
    static constexpr float DEFAULT_FRICTION_RESTITUTION_COEFFICIENT = 0.1f;

    struct PhysicsMaterial
    {
        float staticFriction{DEFAULT_FRICTION_STATIC_COEFFICIENT};
        float dynamicFriction{DEFAULT_FRICTION_DYNAMIC_COEFFICIENT};
        float restitution{DEFAULT_FRICTION_RESTITUTION_COEFFICIENT};

        auto operator<=>(const PhysicsMaterial&) const = default;
    };

    /**
     * Attaches to an entity to give it physics properties. Note that physics system requires
     * a BoundsComponent to also be present, for the entity to partake in physics.
     *
     * TODO: Combine PhysicsComponent and BoundsComponent together?
     */
    class PhysicsComponent
    {
        public:

            PhysicsComponent() = default;

            /**
             * Create a static physics body - Has infinite mass, no velocity
             */
            [[nodiscard]] static PhysicsComponent StaticBody()
            {
                return {PhysicsBodyType::Static, 0.0f, glm::vec3(0.0f)};
            }

            /**
             * Create a kinematic physics body - Has infinite mass, velocity can be changed
             */
            [[nodiscard]] static PhysicsComponent KinematicBody()
            {
                return {PhysicsBodyType::Kinematic, 0.0f, glm::vec3(0.0f)};
            }

            /**
             * Create a dynamic physics body - Has mass, has velocity
             */
            [[nodiscard]] static PhysicsComponent DynamicBody(float mass)
            {
                return {PhysicsBodyType::Dynamic, mass, glm::vec3(0.0f)};
            }

            bool operator==(const PhysicsComponent&) const = default;

        private:

            PhysicsComponent(PhysicsBodyType _bodyType,
                             const float& _mass,
                             const glm::vec3& _linearVelocity)
                : bodyType(_bodyType)
                , mass(_mass)
                , linearVelocity(_linearVelocity)
            { }

        public:

            PhysicsBodyType bodyType{PhysicsBodyType::Dynamic};
            PhysicsMaterial material{};

            float mass{0.0f};
            glm::vec3 linearVelocity{0.0f};

            /** Whether the object is allowed to rotate in x/y/z axes */
            std::array<bool, 3> axisMotionAllowed{true, true, true};

            float linearDamping{0.0f};
            float angularDamping{0.0f};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_PHYSICSCOMPONENT_H
