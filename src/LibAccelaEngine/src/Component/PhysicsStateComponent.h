/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
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
