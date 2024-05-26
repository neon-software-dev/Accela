/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_PIPELINE_PUSHCONSTANT
#define LIBACCELARENDERERVK_SRC_PIPELINE_PUSHCONSTANT

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

namespace Accela::Render
{
    struct PushConstantRange
    {
        PushConstantRange(VkShaderStageFlags _vkShaderStageFlagBits, uint32_t _offset, uint32_t _size)
            : vkShaderStageFlagBits(_vkShaderStageFlagBits)
            , offset(_offset)
            , size(_size)
        {}

        static std::optional<std::vector<PushConstantRange>> None() { return std::nullopt; }

        VkShaderStageFlags vkShaderStageFlagBits;
        uint32_t offset;
        uint32_t size;
    };
}

// VkPushConstantRange

#endif // LIBACCELARENDERERVK_SRC_PIPELINE_PUSHCONSTANT