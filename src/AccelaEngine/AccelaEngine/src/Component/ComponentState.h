/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
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
