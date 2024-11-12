/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_RUNSTATE_H
#define LIBACCELAENGINE_SRC_RUNSTATE_H

#include "ForwardDeclares.h"

#include <Accela/Platform/IPlatform.h>
#include <Accela/Platform/Event/IKeyboardState.h>
#include <Accela/Platform/Event/IMouseState.h>

#include <Accela/Render/RenderSettings.h>

#include <memory>
#include <chrono>
#include <future>
#include <utility>
#include <cstdint>

namespace Accela::Engine
{
    struct RunState
    {
        using Ptr = std::shared_ptr<RunState>;

        RunState(std::shared_ptr<Scene> _initialScene,
                 std::shared_ptr<IWorldResources> worldResources,
                 std::shared_ptr<IWorldState> worldState,
                 std::shared_ptr<Platform::IPlatform> platform,
                 AudioManagerPtr audioManager,
                 MediaManagerPtr mediaManager);

        //
        // Execution State
        //
        const unsigned int timeStep = 10; // MS
        const unsigned int maxProducedTimePerLoop = 50; // MS

        bool keepRunning{true};

        std::uintmax_t tickIndex{0};

        std::chrono::high_resolution_clock::time_point lastTimeSync{std::chrono::high_resolution_clock::now()};
        double accumulatedTime{0.0};
        std::future<bool> previousFrameRenderedFuture;

        //
        // Engine State
        //
        std::shared_ptr<Scene> scene;
        Platform::IKeyboardState::CPtr keyboardState;
        Platform::IMouseState::CPtr mouseState;
        std::shared_ptr<IWorldResources> worldResources;
        std::shared_ptr<IWorldState> worldState;
        AudioManagerPtr audioManager;
        MediaManagerPtr mediaManager;
    };
}

#endif //LIBACCELAENGINE_SRC_RUNSTATE_H
