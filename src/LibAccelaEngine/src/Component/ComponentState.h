#ifndef LIBACCELAENGINE_SRC_COMPONENT_COMPONENTSTATE_H
#define LIBACCELAENGINE_SRC_COMPONENT_COMPONENTSTATE_H

namespace Accela::Engine
{
    enum class ComponentState
    {
        New,
        Dirty,
        Synced
    };
}

#endif //LIBACCELAENGINE_SRC_COMPONENT_COMPONENTSTATE_H
