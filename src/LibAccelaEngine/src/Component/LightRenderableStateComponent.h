/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
        Render::LightId lightId{};
        std::string sceneName;
        ComponentState state{ComponentState::New};
    };
}

#endif //LIBACCELAENGINE_SRC_COMPONENT_LIGHTRENDERABLESTATECOMPONENT_H
