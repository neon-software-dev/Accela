/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_VULKAN_VULKANCOMMON_H
#define LIBACCELARENDERERVK_SRC_VULKAN_VULKANCOMMON_H

#include <Accela/Common/Build.h>

#include <vulkan/vulkan.h>

#include <spirv_reflect.h>

#include <tuple>
#include <optional>

namespace Accela::Render
{
    static const uint32_t VULKAN_API_VERSION = VK_API_VERSION_1_3;

    // {Variant, Major, Minor, Patch}
    using VulkanVersionCode = std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>;

    static constexpr uint32_t ToVulkanApiInt(const VulkanVersionCode& code)
    {
        return VK_MAKE_API_VERSION(std::get<0>(code), std::get<1>(code), std::get<2>(code), std::get<3>(code));
    }

    SUPPRESS_IS_NOT_USED static std::optional<VkShaderStageFlags> SpvToVkShaderStageFlags(const SpvReflectShaderStageFlagBits& spvFlags)
    {
        switch (spvFlags)
        {
            case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
                return VK_SHADER_STAGE_VERTEX_BIT;
            case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
                return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
                return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
                return VK_SHADER_STAGE_GEOMETRY_BIT;
            case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
                return VK_SHADER_STAGE_FRAGMENT_BIT;
            case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
                return VK_SHADER_STAGE_COMPUTE_BIT;
            case SPV_REFLECT_SHADER_STAGE_TASK_BIT_NV:
                return VK_SHADER_STAGE_TASK_BIT_NV;
            case SPV_REFLECT_SHADER_STAGE_MESH_BIT_NV:
                return VK_SHADER_STAGE_MESH_BIT_NV;
            case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR:
                return VK_SHADER_STAGE_RAYGEN_BIT_NV;
            case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR:
                return VK_SHADER_STAGE_ANY_HIT_BIT_NV;
            case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:
                return VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
            case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR:
                return VK_SHADER_STAGE_MISS_BIT_NV;
            case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR:
                return VK_SHADER_STAGE_INTERSECTION_BIT_NV;
            case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR:
                return VK_SHADER_STAGE_CALLABLE_BIT_NV;
            default:
                return std::nullopt;
        }
    }

    SUPPRESS_IS_NOT_USED static std::optional<VkDescriptorType> SpvToVkDescriptorType(const SpvReflectDescriptorType& spvType)
    {
        switch (spvType)
        {
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
                return VK_DESCRIPTOR_TYPE_SAMPLER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
            default:
                return std::nullopt;
        }
    }
}

#endif //LIBACCELARENDERERVK_SRC_VULKAN_VULKANCOMMON_H
