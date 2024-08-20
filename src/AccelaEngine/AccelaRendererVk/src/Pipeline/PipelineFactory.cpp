/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PipelineFactory.h"

#include "../VulkanObjs.h"

#include "../Vulkan/VulkanPipeline.h"
#include "../Vulkan/VulkanDevice.h"

#include <functional>

namespace Accela::Render
{

PipelineFactory::PipelineFactory(Common::ILogger::Ptr logger,
                                 VulkanObjsPtr vulkanObjs,
                                 IShadersPtr shaders)
    : m_logger(std::move(logger))
    , m_vulkanObjs(std::move(vulkanObjs))
    , m_shaders(std::move(shaders))
{

}

std::expected<VulkanPipelinePtr, bool> PipelineFactory::GetPipeline(const VulkanDevicePtr& device, const GraphicsPipelineConfig& config)
{
   return GetPipelineT(device, config);
}

std::expected<VulkanPipelinePtr, bool> PipelineFactory::GetPipeline(const VulkanDevicePtr& device, const ComputePipelineConfig& config)
{
    return GetPipelineT(device, config);
}

template<typename ConfigType>
std::expected<VulkanPipelinePtr, bool> PipelineFactory::GetPipelineT(const VulkanDevicePtr& device, const ConfigType& config)
{
    const auto pipelineKey = config.GetUniqueKey();

    // Return an existing pipeline, if one exists
    const auto pipelineIt = m_pipelines.find(pipelineKey);
    if (pipelineIt != m_pipelines.cend())
    {
        return pipelineIt->second;
    }

    // Otherwise, create a new pipeline
    m_logger->Log(Common::LogLevel::Info, "Pipelines: Creating a new pipeline for config: {}", config.GetUniqueKey());

    auto pipeline = std::make_shared<VulkanPipeline>(m_logger, m_vulkanObjs->GetCalls(), m_shaders, device);
    if (!pipeline->Create(config))
    {
        m_logger->Log(Common::LogLevel::Fatal, "GetGraphicsPipeline: Failed to create pipeline");
        return std::unexpected(false);
    }

    m_pipelines.insert({pipelineKey, pipeline});

    return pipeline;
}

void PipelineFactory::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "Pipelines: Destroying pipelines");

    while (!m_pipelines.empty())
    {
        DestroyPipeline(m_pipelines.cbegin()->first);
    }
}

void PipelineFactory::DestroyPipeline(const size_t& pipelineKey)
{
    const auto it = m_pipelines.find(pipelineKey);
    if (it == m_pipelines.cend())
    {
        return;
    }

    m_logger->Log(Common::LogLevel::Info, "Pipelines: Destroying pipeline {}", pipelineKey);

    it->second->Destroy();
    m_pipelines.erase(it);
}

}
