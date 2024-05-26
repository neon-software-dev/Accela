/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RunState.h"
#include "KeyboardState.h"

namespace Accela::Engine
{

RunState::RunState(std::shared_ptr<Scene> _initialScene,
                   std::shared_ptr<IWorldResources> _worldResources,
                   std::shared_ptr<IWorldState> _worldState)
    : scene(std::move(_initialScene))
    , keyboardState(std::make_shared<KeyboardState>())
    , worldResources(std::move(_worldResources))
    , worldState(std::move(_worldState))
{

}

}
