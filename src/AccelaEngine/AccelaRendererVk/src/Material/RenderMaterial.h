/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_MATERIAL_RENDERMATERIAL_H
#define LIBACCELARENDERERVK_SRC_MATERIAL_RENDERMATERIAL_H

#include <Accela/Render/Id.h>

#include <vector>
#include <unordered_map>
#include <string>

namespace Accela::Render
{
    struct RenderMaterial
    {
        std::vector<unsigned char> payloadBytes;
        std::unordered_map<std::string, TextureId> textureBinds;
    };
}

#endif //LIBACCELARENDERERVK_SRC_MATERIAL_RENDERMATERIAL_H
