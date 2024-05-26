#ifndef LIBACCELAENGINE_SRC_RUNSTATE_H
#define LIBACCELAENGINE_SRC_RUNSTATE_H

#include "ForwardDeclares.h"

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
                 std::shared_ptr<IWorldState> worldState);

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
        std::shared_ptr<IKeyboardState> keyboardState;
        std::shared_ptr<IWorldResources> worldResources;
        std::shared_ptr<IWorldState> worldState;
    };
}

#endif //LIBACCELAENGINE_SRC_RUNSTATE_H
