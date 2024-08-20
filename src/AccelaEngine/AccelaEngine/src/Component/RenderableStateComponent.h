/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_COMPONENT_RENDERABLESTATECOMPONENT_H
#define LIBACCELAENGINE_SRC_COMPONENT_RENDERABLESTATECOMPONENT_H

#include "ComponentState.h"

#include <Accela/Render/Id.h>

#include <string>
#include <unordered_map>
#include <cstdint>

namespace Accela::Engine
{
    struct RenderableStateComponent
    {
        enum class Type
        {
            Sprite,
            Object,
            Model
        };

        explicit RenderableStateComponent(Type _type, std::string _sceneName)
            : type(_type)
            , sceneName(std::move(_sceneName))
        { }

        ComponentState state{ComponentState::New};
        Type type;
        std::string sceneName;

        std::unordered_map<std::size_t, Render::RenderableId> renderableIds;
    };
}

#endif //LIBACCELAENGINE_SRC_COMPONENT_RENDERABLESTATECOMPONENT_H
