/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_COMPONENT_LIGHTRENDERABLESTATECOMPONENT_H
#define LIBACCELAENGINE_SRC_COMPONENT_LIGHTRENDERABLESTATECOMPONENT_H

#include "ComponentState.h"

#include <Accela/Render/Id.h>

#include <string>
#include <unordered_map>
#include <cstdint>

namespace Accela::Engine
{
    struct LightRenderableStateComponent
    {
        LightRenderableStateComponent() = default;
        explicit LightRenderableStateComponent(std::string _sceneName)
            : sceneName(std::move(_sceneName))
        { }

        ComponentState state{ComponentState::New};
        Render::LightId lightId{};
        std::string sceneName;
    };
}

#endif //LIBACCELAENGINE_SRC_COMPONENT_LIGHTRENDERABLESTATECOMPONENT_H
