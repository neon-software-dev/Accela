/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_FORWARDDECLARES_H
#define LIBACCELAENGINE_SRC_FORWARDDECLARES_H

#include <memory>

namespace Accela::Engine
{
    class Scene; using ScenePtr = std::shared_ptr<Scene>;
    class IKeyboardState; using IKeyboardStatePtr = std::shared_ptr<IKeyboardState>;
    class IWorldResources; using IWorldResourcesPtr = std::shared_ptr<IWorldResources>;
    class IWorldState; using IWorldStatePtr = std::shared_ptr<IWorldState>;
    class AudioManager; using AudioManagerPtr = std::shared_ptr<AudioManager>;
    class IPhysics; using IPhysicsPtr = std::shared_ptr<IPhysics>;
    class IPhysicsRuntime; using IPhysicsRuntimePtr = std::shared_ptr<IPhysicsRuntime>;
}

#endif //LIBACCELAENGINE_SRC_FORWARDDECLARES_H
