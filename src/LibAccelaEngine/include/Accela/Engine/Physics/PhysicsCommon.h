/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_PHYSICSCOMMON_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_PHYSICSCOMMON_H

namespace Accela::Engine
{
    enum class RigidBodyType
    {
        Static,
        Kinematic,
        Dynamic
    };

    struct PhysicsMaterial
    {
        float staticFriction{1.0f};
        float dynamicFriction{1.0f};
        float restitution{0.1f};

        auto operator<=>(const PhysicsMaterial&) const = default;
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_PHYSICSCOMMON_H
