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
            Model,
            Terrain
        };

        explicit RenderableStateComponent(Type _type)
            : type(_type)
        { }

        Type type;
        std::unordered_map<std::size_t, Render::RenderableId> renderableIds;
        std::string sceneName;
        ComponentState state{ComponentState::New};
    };
}

#endif //LIBACCELAENGINE_SRC_COMPONENT_RENDERABLESTATECOMPONENT_H
