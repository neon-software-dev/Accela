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
    /**
     * Attaches to an entity to give it physics properties.
     */
    class PhysicsComponent
    {
        public:

            /**
             * Create a static physics body - Has infinite mass, no velocity
             */
            [[nodiscard]] static PhysicsComponent StaticBody(const PhysicsShape& shape, const PhysicsMaterial& material)
            {
                return {RigidBodyType::Static, shape, material};
            }

            /**
             * Create a kinematic physics body - Has infinite mass, velocity can be changed
             */
            [[nodiscard]] static PhysicsComponent KinematicBody(const PhysicsShape& shape, const PhysicsMaterial& material)
            {
                return {RigidBodyType::Kinematic, shape, material};
            }

            /**
             * Create a dynamic physics body - Has mass, has velocity
             */
            [[nodiscard]] static PhysicsComponent DynamicBody(const PhysicsShape& shape, const PhysicsMaterial& material, float mass)
            {
                return {RigidBodyType::Dynamic,shape, material, mass};
            }

        private:

            PhysicsComponent(RigidBodyType _bodyType,
                             const PhysicsShape& _shape,
                             const PhysicsMaterial& _material,
                             const float& _mass = 0.0f,
                             const glm::vec3& _linearVelocity = glm::vec3(0))
                : bodyType(_bodyType)
                , shape(_shape)
                , material(_material)
                , mass(_mass)
                , linearVelocity(_linearVelocity)
            { }

        public:

            RigidBodyType bodyType{RigidBodyType::Dynamic};

            PhysicsShape shape;
            PhysicsMaterial material;

            float mass{0.0f};

            //
            // Dynamic body properties
            //
            glm::vec3 linearVelocity{0.0f};
            std::array<bool, 3> axisMotionAllowed{true, true, true};
            float linearDamping{0.0f};
            float angularDamping{0.0f};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_PHYSICSCOMPONENT_H
