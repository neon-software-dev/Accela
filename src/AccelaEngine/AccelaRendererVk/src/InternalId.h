/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_INTERNALID_H
#define LIBACCELARENDERERVK_SRC_INTERNALID_H

#include <Accela/Render/Id.h>

namespace Accela::Render
{
    DEFINE_ID_TYPE(BufferId)
    DEFINE_ID_TYPE(ImageId)
}

DEFINE_ID_HASH(Accela::Render::BufferId)
DEFINE_ID_HASH(Accela::Render::ImageId)

#endif //LIBACCELARENDERERVK_SRC_INTERNALID_H
