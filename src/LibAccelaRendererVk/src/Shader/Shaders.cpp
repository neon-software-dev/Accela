/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Shaders.h"
#include "VulkanObjs.h"

#include "Vulkan/VulkanShaderModule.h"

namespace Accela::Render
{

Shaders::Shaders(Common::ILogger::Ptr logger, VulkanObjsPtr vulkanObjs)
    : m_logger(std::move(logger))
    , m_vulkanObjs(std::move(vulkanObjs))
{

}

bool Shaders::LoadShader(const ShaderSpec& shaderSpec)
{
    m_logger->Log(Common::LogLevel::Info, "Shaders: Loading shader: {}", shaderSpec.shaderName);

    const auto shaderIt = m_loadedShaders.find(shaderSpec.shaderName);
    if (shaderIt != m_loadedShaders.end())
    {
        m_logger->Log(Common::LogLevel::Warning, "Shaders: Shader was already loaded");
        return true;
    }

    auto shaderModule = std::make_shared<VulkanShaderModule>(m_logger, m_vulkanObjs->GetCalls(), m_vulkanObjs->GetDevice());
    if (!shaderModule->Create(shaderSpec))
    {
        m_logger->Log(Common::LogLevel::Error, "Shaders: Failed to create shader: {}", shaderSpec.shaderName);
        return false;
    }

    m_loadedShaders.insert({shaderSpec.shaderName, shaderModule});

    return true;
}

std::optional<VulkanShaderModulePtr> Shaders::GetShaderModule(const std::string& shaderFileName) const
{
    const auto shaderIt = m_loadedShaders.find(shaderFileName);
    if (shaderIt == m_loadedShaders.end())
    {
        return std::nullopt;
    }

    return shaderIt->second;
}

void Shaders::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "Shaders: Destroying all shaders");

    for (const auto& shaderIt : m_loadedShaders)
    {
        m_logger->Log(Common::LogLevel::Info, "Shaders: Destroying shader: {}", shaderIt.first);
        shaderIt.second->Destroy();
    }

    m_loadedShaders.clear();
}

}
