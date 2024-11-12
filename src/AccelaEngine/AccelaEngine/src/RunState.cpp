/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RunState.h"

namespace Accela::Engine
{

RunState::RunState(std::shared_ptr<Scene> _initialScene,
                   std::shared_ptr<IWorldResources> _worldResources,
                   std::shared_ptr<IWorldState> _worldState,
                   std::shared_ptr<Platform::IPlatform> _platform,
                   AudioManagerPtr _audioManager,
                   MediaManagerPtr _mediaManager)
    : scene(std::move(_initialScene))
    , keyboardState(_platform->GetEvents()->GetKeyboardState())
    , mouseState(_platform->GetEvents()->GetMouseState())
    , worldResources(std::move(_worldResources))
    , worldState(std::move(_worldState))
    , audioManager(std::move(_audioManager))
    , mediaManager(std::move(_mediaManager))
{

}

}
