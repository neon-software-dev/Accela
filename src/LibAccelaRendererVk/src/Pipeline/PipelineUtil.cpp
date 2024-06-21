/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PipelineUtil.h"
#include "IPipelineFactory.h"

#include "../VulkanObjs.h"

#include "../Vulkan/VulkanShaderModule.h"
#include "../Vulkan/VulkanPhysicalDevice.h"
#include "../Shader/IShaders.h"
#include "../Program/ProgramDef.h"
#include "../Util/VulkanFuncs.h"

#include <Accela/Render/Util/Rect.h>

namespace Accela::Render
{

std::expected<VulkanPipelinePtr, bool> GetGraphicsPipeline(
    const Common::ILogger::Ptr& logger,
    const VulkanObjsPtr& vulkanObjs,
    const IShadersPtr& shaders,
    const IPipelineFactoryPtr& pipelines,
    const ProgramDefPtr& programDef,
    const VulkanRenderPassPtr& renderPass,
    const uint32_t& subpassIndex,
    const Viewport& viewport,
    const CullFace& cullFace,
    const PolygonFillMode& polygonFillMode,
    const DepthBias& depthBias,
    const std::optional<std::vector<PushConstantRange>>& pushConstantRanges,
    const std::optional<std::size_t>& tag,
    const std::optional<std::size_t>& oldPipelineHash)
{
    auto vulkanFuncs = VulkanFuncs(logger, vulkanObjs);

    const auto subpasses = renderPass->GetSubpasses();
    if (subpasses.size() < subpassIndex)
    {
        logger->Log(Common::LogLevel::Error, "GetGraphicsPipeline: Invalid subpass index");
        return std::unexpected(false);
    }

    const auto& subpass = subpasses[subpassIndex];

    //
    // General configuration
    //
    GraphicsPipelineConfig pipelineConfig{};
    pipelineConfig.subpassIndex = subpassIndex;
    pipelineConfig.viewport = viewport;
    pipelineConfig.vkRenderPass = renderPass->GetVkRenderPass();
    pipelineConfig.usesDepthStencil = renderPass->HasDepthAttachment();

    const auto renderPassAttachments = renderPass->GetAttachments();

    for (const auto attachmentRef : subpass.colorAttachmentRefs)
    {
        const auto attachmentIndex = attachmentRef.attachment;

        if (attachmentIndex >= renderPassAttachments.size())
        {
            logger->Log(Common::LogLevel::Error, "GetGraphicsPipeline: Color attachment ref index out of bounds");
            return std::unexpected(false);
        }

        const auto attachment = renderPassAttachments[attachmentIndex];

        const VkFormatProperties vkFormatProperties = vulkanFuncs.GetVkFormatProperties(attachment.description.format);

        const bool supportsColorBlending = vkFormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;

        pipelineConfig.colorAttachments.emplace_back(supportsColorBlending);
    }

    if (tag.has_value())
    {
        pipelineConfig.tag = *tag;
    }

    //
    // Rasterization configuration
    //
    pipelineConfig.cullFace = cullFace;
    pipelineConfig.polygonFillMode = polygonFillMode;
    pipelineConfig.depthBias = depthBias;

    // If the device doesn't support non-solid fill mode, override the setting to fill
    if (!vulkanObjs->GetPhysicalDevice()->GetPhysicalDeviceFeatures().fillModeNonSolid)
    {
        pipelineConfig.polygonFillMode = PolygonFillMode::Fill;
    }

    //
    // Shader configuration
    //
    const auto programShaderNames = programDef->GetShaderNames();
    for (const auto& shaderName : programShaderNames)
    {
        const auto shaderModuleOpt = shaders->GetShaderModule(shaderName);
        if (!shaderModuleOpt)
        {
            logger->Log(Common::LogLevel::Error, "GetGraphicsPipeline: Failed to find shader: {}", shaderName);
            return std::unexpected(false);
        }

        switch ((*shaderModuleOpt)->GetShaderSpec()->shaderType)
        {
            case ShaderType::Vertex:
                pipelineConfig.vertShaderFileName = shaderName;
            break;
            case ShaderType::Fragment:
                pipelineConfig.fragShaderFileName = shaderName;
            break;
            case ShaderType::TESC:
                pipelineConfig.tescShaderFileName = shaderName;
            break;
            case ShaderType::TESE:
                pipelineConfig.teseShaderFileName = shaderName;
            break;
            case ShaderType::Compute:
            {
                logger->Log(Common::LogLevel::Error, "GetGraphicsPipeline: Compute shader provided: {}", shaderName);
                return std::unexpected(false);
            }
        }
    }

    //
    // Pipeline Bindings
    //
    if (!programDef->GetVertexInputBindingDescription().has_value())
    {
        logger->Log(Common::LogLevel::Error, "GetGraphicsPipeline: Program doesn't have vertex input binding description: {}", programDef->GetProgramName());
        return std::unexpected(false);
    }

    pipelineConfig.vkVertexInputBindingDescriptions.push_back(*programDef->GetVertexInputBindingDescription());

    //
    // Vertex Input Attributes
    //
    const auto vertexInputAttributes = programDef->GetVertexInputAttributeDescriptions();
    for (const auto& vertexInputAttribute : vertexInputAttributes)
    {
        pipelineConfig.vkVertexInputAttributeDescriptions.push_back(vertexInputAttribute);
    }

    //
    // Pipeline layout configuration
    //
    if (pushConstantRanges)
    {
        std::vector<VkPushConstantRange> vkPushConstantRanges;

        for (const auto& pushConstantRange : *pushConstantRanges)
        {
            VkPushConstantRange vkPushConstantRange{};
            vkPushConstantRange.stageFlags = pushConstantRange.vkShaderStageFlagBits;
            vkPushConstantRange.offset = pushConstantRange.offset;
            vkPushConstantRange.size = pushConstantRange.size;

            vkPushConstantRanges.push_back(vkPushConstantRange);
        }

        pipelineConfig.vkPushConstantRanges = vkPushConstantRanges;
    }

    //
    // If using tesselation, switch to patch list topology
    //
    if (pipelineConfig.tescShaderFileName.has_value() || pipelineConfig.teseShaderFileName.has_value())
    {
        pipelineConfig.primitiveTopology = PrimitiveTopology::PatchList;
    }

    const auto programVkDescriptorSetLayouts = programDef->GetVkDescriptorSetLayouts();
    pipelineConfig.vkDescriptorSetLayouts = programVkDescriptorSetLayouts;

    //
    // Delete the old pipeline, if different
    //
    // TODO: Verify that this is no longer needed and add metrics around number of created pipelines
    (void)oldPipelineHash;
    /*if (oldPipelineHash.has_value() && pipelineConfig.GetUniqueKey() != oldPipelineHash.value())
    {
        pipelines->DestroyPipeline(*oldPipelineHash);
    }*/

    //
    // Create/Get the pipeline
    //
    auto pipeline = pipelines->GetPipeline(vulkanObjs->GetDevice(), pipelineConfig);
    if (pipeline == nullptr)
    {
        logger->Log(Common::LogLevel::Error, "GetGraphicsPipeline: Failed to create or retrieve rendering pipeline");
        return std::unexpected(false);
    }

    return pipeline;
}

std::expected<VulkanPipelinePtr, bool> GetComputePipeline(
    const Common::ILogger::Ptr& logger,
    const VulkanObjsPtr& vulkanObjs,
    const IShadersPtr& shaders,
    const IPipelineFactoryPtr& pipelines,
    const ProgramDefPtr& programDef,
    const std::optional<std::vector<PushConstantRange>>& pushConstantRanges,
    const std::optional<std::size_t>& tag,
    const std::optional<std::size_t>& oldPipelineHash)
{
    auto vulkanFuncs = VulkanFuncs(logger, vulkanObjs);

    //
    // General configuration
    //
    ComputePipelineConfig pipelineConfig{};

    if (tag.has_value())
    {
        pipelineConfig.tag = *tag;
    }

    //
    // Shader configuration
    //
    const auto programShaderNames = programDef->GetShaderNames();
    if (programShaderNames.size() != 1)
    {
        logger->Log(Common::LogLevel::Error,
            "GetComputePipeline: Compute program requires exactly 1 shader: {}", programDef->GetProgramName());
        return std::unexpected(false);
    }

    const auto& shaderName = programShaderNames.at(0);

    const auto shaderModuleOpt = shaders->GetShaderModule(shaderName);
    if (!shaderModuleOpt)
    {
        logger->Log(Common::LogLevel::Error, "GetComputePipeline: Failed to find shader: {}", shaderName);
        return std::unexpected(false);
    }

    if ((*shaderModuleOpt)->GetShaderSpec()->shaderType != ShaderType::Compute)
    {
        logger->Log(Common::LogLevel::Error, "GetComputePipeline: Program has a non-compute shader: {}", shaderName);
        return std::unexpected(false);
    }

    pipelineConfig.computeShaderFileName = shaderName;

    //
    // Pipeline layout configuration
    //
    if (pushConstantRanges)
    {
        std::vector<VkPushConstantRange> vkPushConstantRanges;

        for (const auto& pushConstantRange : *pushConstantRanges)
        {
            VkPushConstantRange vkPushConstantRange{};
            vkPushConstantRange.stageFlags = pushConstantRange.vkShaderStageFlagBits;
            vkPushConstantRange.offset = pushConstantRange.offset;
            vkPushConstantRange.size = pushConstantRange.size;

            vkPushConstantRanges.push_back(vkPushConstantRange);
        }

        pipelineConfig.vkPushConstantRanges = vkPushConstantRanges;
    }

    //
    // Pipeline layout configuration
    //
    const auto programVkDescriptorSetLayouts = programDef->GetVkDescriptorSetLayouts();
    pipelineConfig.vkDescriptorSetLayouts = programVkDescriptorSetLayouts;

    //
    // Delete the old pipeline, if different
    //
    // TODO: Verify that this is no longer needed and add metrics around number of created pipelines
    (void)oldPipelineHash;
    /*if (oldPipelineHash.has_value() && pipelineConfig.GetUniqueKey() != oldPipelineHash.value())
    {
        pipelines->DestroyPipeline(*oldPipelineHash);
    }*/

    //
    // Create/Get the pipeline
    //
    auto pipeline = pipelines->GetPipeline(vulkanObjs->GetDevice(), pipelineConfig);
    if (pipeline == nullptr)
    {
        logger->Log(Common::LogLevel::Error, "GetComputePipeline: Failed to create or retrieve rendering pipeline");
        return std::unexpected(false);
    }

    return pipeline;
}

}
