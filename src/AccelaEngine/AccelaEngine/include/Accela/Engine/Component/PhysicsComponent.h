/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_PHYSICSCOMPONENT_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_COMPONENT_PHYSICSCOMPONENT_H

#include <Accela/Engine/Physics/PhysicsCommon.h>

#include <Accela/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <array>
#include <vector>

namespace Accela::Engine
{
    /**
     * Attaches to an entity to give it physics properties.
     */
    class ACCELA_PUBLIC PhysicsComponent
    {
        public:

            /**
             * Create a static physics body - Has infinite mass, no velocity
             */
            [[nodiscard]] static PhysicsComponent StaticBody(const PhysicsSceneName& scene, const std::vector<PhysicsShape>& _shapes)
            {
                return {scene, RigidBodyType::Static, _shapes};
            }

            /**
             * Create a kinematic physics body - Has infinite mass, velocity can be changed
             */
            [[nodiscard]] static PhysicsComponent KinematicBody(const PhysicsSceneName& scene, const std::vector<PhysicsShape>& _shapes)
            {
                return {scene, RigidBodyType::Kinematic, _shapes};
            }

            /**
             * Create a dynamic physics body - Has mass, has velocity
             */
            [[nodiscard]] static PhysicsComponent DynamicBody(const PhysicsSceneName& scene, const std::vector<PhysicsShape>& _shapes, float _mass)
            {
                return {scene, RigidBodyType::Dynamic, _shapes, _mass};
            }

        private:

            PhysicsComponent(PhysicsSceneName _scene,
                             RigidBodyType _bodyType,
                             std::vector<PhysicsShape> _shapes,
                             const float& _mass = 0.0f,
                             const glm::vec3& _linearVelocity = glm::vec3(0))
                : scene(std::move(_scene))
                , bodyType(_bodyType)
                , shapes(std::move(_shapes))
                , mass(_mass)
                , linearVelocity(_linearVelocity)
            { }

        public:

            PhysicsSceneName scene;

            RigidBodyType bodyType{RigidBodyType::Dynamic};

            std::vector<PhysicsShape> shapes;

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
