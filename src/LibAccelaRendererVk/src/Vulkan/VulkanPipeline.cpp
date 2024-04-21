/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#include "VulkanPipeline.h"
#include "VulkanDevice.h"
#include "VulkanShaderModule.h"

#include "../Shader/IShaders.h"

#include <Accela/Render/IVulkanCalls.h>

namespace Accela::Render
{

VulkanPipeline::VulkanPipeline(Common::ILogger::Ptr logger, IVulkanCallsPtr vk, IShadersPtr shaders, VulkanDevicePtr device)
    : m_logger(std::move(logger))
    , m_vk(std::move(vk))
    , m_shaders(std::move(shaders))
    , m_device(std::move(device))
{

}

bool VulkanPipeline::Create(const PipelineConfig& config)
{
    //
    // Configure shader stages
    //
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    if (config.vertShaderFileName.has_value())
    {
        const auto shaderModuleOpt = m_shaders->GetShaderModule(*config.vertShaderFileName);
        if (!shaderModuleOpt)
        {
            m_logger->Log(Common::LogLevel::Error,
                "Pipeline creation failure: Failed to find vert shader: {}", *config.vertShaderFileName);
            return false;
        }

        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStageInfo.module = (*shaderModuleOpt)->GetVkShaderModule();
        shaderStageInfo.pName = "main";

        shaderStages.push_back(shaderStageInfo);
    }

    if (config.fragShaderFileName.has_value())
    {
        const auto shaderModuleOpt = m_shaders->GetShaderModule(*config.fragShaderFileName);
        if (!shaderModuleOpt)
        {
            m_logger->Log(Common::LogLevel::Error,
              "Pipeline creation failure: Failed to find frag shader: {}", *config.fragShaderFileName);
            return false;
        }

        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStageInfo.module = (*shaderModuleOpt)->GetVkShaderModule();
        shaderStageInfo.pName = "main";

        shaderStages.push_back(shaderStageInfo);
    }

    if (config.tescShaderFileName.has_value())
    {
        const auto shaderModuleOpt = m_shaders->GetShaderModule(*config.tescShaderFileName);
        if (!shaderModuleOpt)
        {
            m_logger->Log(Common::LogLevel::Error,
                "Pipeline creation failure: Failed to find tesc shader: {}", *config.tescShaderFileName);
            return false;
        }

        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        shaderStageInfo.module = (*shaderModuleOpt)->GetVkShaderModule();
        shaderStageInfo.pName = "main";

        shaderStages.push_back(shaderStageInfo);
    }

    if (config.teseShaderFileName.has_value())
    {
        const auto shaderModuleOpt = m_shaders->GetShaderModule(*config.teseShaderFileName);
        if (!shaderModuleOpt)
        {
            m_logger->Log(Common::LogLevel::Error,
                "Pipeline creation failure: Failed to find tese shader: {}", *config.teseShaderFileName);
            return false;
        }

        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        shaderStageInfo.module = (*shaderModuleOpt)->GetVkShaderModule();
        shaderStageInfo.pName = "main";

        shaderStages.push_back(shaderStageInfo);
    }

    //
    // Configure vertex assembly stage
    //
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

    if (config.primitiveRestartEnable)
    {
        inputAssembly.primitiveRestartEnable = VK_TRUE;
    }
    else
    {
        inputAssembly.primitiveRestartEnable = VK_FALSE;
    }

    switch (config.primitiveTopology)
    {
        case PrimitiveTopology::TriangleList:
            inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
        case PrimitiveTopology::TriangleFan:
            inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        break;
        case PrimitiveTopology::PatchList:
            inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        break;
    }

    //
    // Configure viewport/scissoring state
    //
    VkViewport viewport{};
    viewport.x = (float)config.viewport.x;
    viewport.y = (float)config.viewport.y;
    viewport.width = (float)config.viewport.w;
    viewport.height = (float)config.viewport.h;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { (int32_t)config.viewport.x, (int32_t)config.viewport.y };
    scissor.extent = {config.viewport.w, config.viewport.h };

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    //
    // Configure rasterizer stage
    //
    VkCullModeFlags vkCullModeFlags{};

    switch (config.cullFace)
    {
        case CullFace::Front:
            vkCullModeFlags = VK_CULL_MODE_FRONT_BIT;
        break;
        case CullFace::Back:
            vkCullModeFlags = VK_CULL_MODE_BACK_BIT;
        break;
    }

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vkCullModeFlags;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_TRUE;
    rasterizer.depthBiasConstantFactor = 4.0f; // Optional (from https://blogs.igalia.com/itoral/2017/10/02/working-with-lights-and-shadows-part-iii-rendering-the-shadows/)
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 1.5f; // Optional

    switch (config.polygonFillMode)
    {
        case PolygonFillMode::Fill: rasterizer.polygonMode = VK_POLYGON_MODE_FILL; break;
        case PolygonFillMode::Line: rasterizer.polygonMode = VK_POLYGON_MODE_LINE; break;
    }

    //
    // Configure multisampling
    //
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    //
    // Configure color blending
    //
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;

    std::ranges::transform(config.colorAttachments, std::back_inserter(colorBlendAttachments),
       [](const auto& colorAttachment){
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = colorAttachment.enableColorBlending;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        return colorBlendAttachment;
    });

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = colorBlendAttachments.size();
    colorBlending.pAttachments = colorBlendAttachments.data();
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    //
    // Configure tesselation
    //
    const bool doesTesselation = config.tescShaderFileName.has_value() || config.teseShaderFileName.has_value();

    VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo{};
    tessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellationStateCreateInfo.pNext = nullptr;
    tessellationStateCreateInfo.flags = 0;
    tessellationStateCreateInfo.patchControlPoints = config.tesselationNumControlPoints;

    //
    // Configure pipeline layout - vertex inputs, push constants, descriptor sets
    //
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // Vertex binding description
    if (!config.vkVertexInputBindingDescriptions.empty())
    {
        vertexInputInfo.vertexBindingDescriptionCount = config.vkVertexInputBindingDescriptions.size();
        vertexInputInfo.pVertexBindingDescriptions = config.vkVertexInputBindingDescriptions.data();
    }
    else
    {
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
    }

    // Vertex attribute descriptions
    if (!config.vkVertexInputAttributeDescriptions.empty())
    {
        vertexInputInfo.vertexAttributeDescriptionCount = config.vkVertexInputAttributeDescriptions.size();
        vertexInputInfo.pVertexAttributeDescriptions = config.vkVertexInputAttributeDescriptions.data();
    }
    else
    {
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr; // Optional

    // Push constants
    if (config.vkPushConstantRanges.has_value())
    {
        pipelineLayoutInfo.pushConstantRangeCount = config.vkPushConstantRanges->size();
        pipelineLayoutInfo.pPushConstantRanges = config.vkPushConstantRanges->data();
    }
    else
    {
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
    }

    // Descriptor sets
    if (config.vkDescriptorSetLayouts.has_value())
    {
        pipelineLayoutInfo.setLayoutCount = config.vkDescriptorSetLayouts->size();
        pipelineLayoutInfo.pSetLayouts = config.vkDescriptorSetLayouts->data();
    }

    auto result = m_vk->vkCreatePipelineLayout(m_device->GetVkDevice(), &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Pipeline creation failure: vkCreatePipelineLayout failed, result code: {}", (uint32_t)result);
        return false;
    }

    //
    // Depth buffer configuration
    //
    VkPipelineDepthStencilStateCreateInfo depthStencil{};

    if (config.usesDepthStencil)
    {
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL; // Note that for skybox it was changed from OP_LESS to OP_LESS_OR_EQUAL
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};
    }

    //
    // Create the pipeline
    //
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = m_vkPipelineLayout;
    pipelineInfo.renderPass = config.vkRenderPass;
    pipelineInfo.subpass = config.subpassIndex;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (config.usesDepthStencil)
    {
        pipelineInfo.pDepthStencilState = &depthStencil;
    }

    if (doesTesselation)
    {
        pipelineInfo.pTessellationState = &tessellationStateCreateInfo;
    }

    result = m_vk->vkCreateGraphicsPipelines(m_device->GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_vkPipeline);
    if (result != VK_SUCCESS)
    {
        m_logger->Log(Common::LogLevel::Error,
          "vkCreateGraphicsPipelines call failure, result code: {}", (uint32_t)result);
        return false;
    }

    m_config = config;

    return true;
}

void VulkanPipeline::Destroy()
{
    if (m_vkPipeline != VK_NULL_HANDLE)
    {
        m_vk->vkDestroyPipeline(m_device->GetVkDevice(), m_vkPipeline, nullptr);
        m_vkPipeline = VK_NULL_HANDLE;
    }

    if (m_vkPipelineLayout != VK_NULL_HANDLE)
    {
        m_vk->vkDestroyPipelineLayout(m_device->GetVkDevice(), m_vkPipelineLayout, nullptr);
        m_vkPipelineLayout = VK_NULL_HANDLE;
    }
}

}
