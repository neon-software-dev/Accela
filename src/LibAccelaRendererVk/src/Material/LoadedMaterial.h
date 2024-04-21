/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_MATERIAL_LOADEDMATERIAL_H
#define LIBACCELARENDERERVK_SRC_MATERIAL_LOADEDMATERIAL_H

#include "../ForwardDeclares.h"

#include <Accela/Render/Material/Material.h>

#include <Accela/Render/Id.h>

#include <cstdint>
#include <unordered_map>
#include <string>

namespace Accela::Render
{
    struct LoadedMaterial
    {
        MaterialId id{INVALID_ID};
        Material::Type type{};

        DataBufferPtr payloadBuffer;
        std::size_t payloadByteOffset{0};
        std::size_t payloadByteSize{0};
        std::size_t payloadIndex{0};

        std::unordered_map<std::string, TextureId> textureBinds;
    };
}

#endif //LIBACCELARENDERERVK_SRC_MATERIAL_LOADEDMATERIAL_H
