/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_PHYSICS_PHYSXWRAPPER_H
#define LIBACCELAENGINE_SRC_PHYSICS_PHYSXWRAPPER_H

//
// Wrapper around PhysX includes so that our strict warning
// settings don't apply to code we don't own
//

#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wredundant-decls"
#endif

#include <PxPhysicsAPI.h>

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif


#endif //LIBACCELAENGINE_SRC_PHYSICS_PHYSXWRAPPER_H
