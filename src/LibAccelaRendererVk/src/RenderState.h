/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERSTATE_H
#define LIBACCELARENDERERVK_SRC_RENDERSTATE_H

#include <Accela/Render/Id.h>

#include <vulkan/vulkan.h>

#include <unordered_map>

namespace Accela::Render
{
    class RenderState
    {
        std::unordered_map<TextureId, VkImageLayout> textureLayouts;
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERSTATE_H
