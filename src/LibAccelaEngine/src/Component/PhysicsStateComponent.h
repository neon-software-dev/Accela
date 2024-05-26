#ifndef LIBACCELAENGINE_SRC_COMPONENT_PHYSICSSTATECOMPONENT_H
#define LIBACCELAENGINE_SRC_COMPONENT_PHYSICSSTATECOMPONENT_H

#include "ComponentState.h"

namespace Accela::Engine
{
    struct PhysicsStateComponent
    {
        ComponentState state{ComponentState::New};
    };
}

#endif //LIBACCELAENGINE_SRC_COMPONENT_PHYSICSSTATECOMPONENT_H
