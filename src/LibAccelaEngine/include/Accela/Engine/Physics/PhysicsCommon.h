/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_PHYSICSCOMMON_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_PHYSICSCOMMON_H

namespace Accela::Engine
{
    enum class PhysicsBodyType
    {
        Static,
        Kinematic,
        Dynamic
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_PHYSICS_PHYSICSCOMMON_H
