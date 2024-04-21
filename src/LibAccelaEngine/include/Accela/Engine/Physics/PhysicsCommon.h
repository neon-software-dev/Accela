/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_PHYSICSCOMMON_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_PHYSICSCOMMON_H

namespace Accela::Engine
{
    enum class PhysicsBodyType
    {
        Static,     // infinite mass, zero velocity, position manual
        Kinematic,  // infinite mass, velocity manual, position determined by physics
        Dynamic     // non-zero mass, velocity and position determined by physics
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_PHYSICSCOMMON_H
