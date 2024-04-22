/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_SRC_PHYSICS_REACTPHYSICS3D_H
#define LIBACCELAENGINE_SRC_PHYSICS_REACTPHYSICS3D_H

//
// For linux we compile with all warnings on, and we're currently
// building reactphysics3d locally, so wrap its include in pragmas
// to ignore warnings that come from code we don't own
//

#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wtype-limits"
#endif

    #include <reactphysics3d/reactphysics3d.h>

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

#endif //LIBACCELAENGINE_SRC_PHYSICS_REACTPHYSICS3D_H
