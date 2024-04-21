/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#include "VulkanShaderModule.h"
#include "VulkanDevice.h"
#include "VulkanDebug.h"

#include <Accela/Render/IVulkanCalls.h>

#include <vector>

namespace Accela::Render
{

VulkanShaderModule::VulkanShaderModule(Common::ILogger::Ptr logger, IVulkanCallsPtr vk, VulkanDevicePtr device)
    : m_logger(std::move(logger))
    , m_vk(std::move(vk))
    , m_device(std::move(device))
{

}

bool VulkanShaderModule::Create(const ShaderSpec& shaderSpec)
{
    //
    // Use SPIRV-Reflect to parse the shader source and compile details about
    // what inputs, descriptor sets, etc., the shader requires
    //
    const auto reflectResult = spvReflectCreateShaderModule(shaderSpec.shaderSource.size(), shaderSpec.shaderSource.data(), &m_reflectInfo);
    if (reflectResult != SPV_REFLECT_RESULT_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error,"VulkanShaderModule: SPIRV parsing failed, unable to create shader module");
        return false;
    }

    //
    // Create the Vulkan shader module from the shader source
    //
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderSpec.shaderSource.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderSpec.shaderSource.data());

    const auto result = m_vk->vkCreateShaderModule(m_device->GetVkDevice(), &createInfo, nullptr, &m_vkShaderModule);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error, "vkCreateShaderModule call failure, result code: {}", (uint32_t)result);
        spvReflectDestroyShaderModule(&m_reflectInfo);
        return false;
    }

    SetDebugName(m_vk, m_device, VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)m_vkShaderModule, shaderSpec.shaderName);

    m_shaderSpec = shaderSpec;

    return true;
}

void VulkanShaderModule::Destroy()
{
    if (m_vkShaderModule == VK_NULL_HANDLE)
    {
        return;
    }

    RemoveDebugName(m_vk, m_device, VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)m_vkShaderModule);

    spvReflectDestroyShaderModule(&m_reflectInfo);
    m_reflectInfo = {};

    m_vk->vkDestroyShaderModule(m_device->GetVkDevice(), m_vkShaderModule, nullptr);
    m_vkShaderModule = VK_NULL_HANDLE;

    m_shaderSpec = std::nullopt;
}

}
