/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PostProcessingRenderer.h"

#include "../Program/IPrograms.h"
#include "../Mesh/IMeshes.h"
#include "../Pipeline/IPipelineFactory.h"
#include "../Buffer/DataBuffer.h"
#include "../Pipeline/PipelineUtil.h"

#include "../Vulkan/VulkanCommandBuffer.h"
#include "../Vulkan/VulkanPipeline.h"
#include "../Vulkan/VulkanDescriptorSet.h"
#include "../Vulkan/VulkanFramebuffer.h"

#include <Accela/Render/IVulkanCalls.h>
#include <Accela/Render/RenderLogic.h>

#include <Accela/Render/Mesh/StaticMesh.h>
#include <Accela/Render/Texture/TextureView.h>

#include <format>
#include <vector>

namespace Accela::Render
{

PostProcessingRenderer::PostProcessingRenderer(Common::ILogger::Ptr logger,
                                               Common::IMetrics::Ptr metrics,
                                               Ids::Ptr ids,
                                               PostExecutionOpsPtr postExecutionOps,
                                               VulkanObjsPtr vulkanObjs,
                                               IProgramsPtr programs,
                                               IShadersPtr shaders,
                                               IPipelineFactoryPtr pipelines,
                                               IBuffersPtr buffers,
                                               IMaterialsPtr materials,
                                               ITexturesPtr textures,
                                               IMeshesPtr meshes,
                                               ILightsPtr lights,
                                               IRenderablesPtr renderables,
                                               uint8_t frameIndex)
    : Renderer(std::move(logger),
               std::move(metrics),
               std::move(ids),
               std::move(postExecutionOps),
               std::move(vulkanObjs),
               std::move(programs),
               std::move(shaders),
               std::move(pipelines),
               std::move(buffers),
               std::move(materials),
               std::move(textures),
               std::move(meshes),
               std::move(lights),
               std::move(renderables),
               frameIndex)
{

}

bool PostProcessingRenderer::Initialize(const RenderSettings& renderSettings)
{
    if (!Renderer::Initialize(renderSettings))
    {
        return false;
    }

    m_logger->Log(Common::LogLevel::Info, "PostProcessingRenderer: Initializing, for frame {}", m_frameIndex);

    return true;
}

void PostProcessingRenderer::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "PostProcessingRenderer: Destroying for frame {}", m_frameIndex);

    m_renderSettings = {};

    return Renderer::Destroy();
}

void PostProcessingRenderer::Render(const VulkanCommandBufferPtr& commandBuffer,
                                    const LoadedTexture& inputTexture,
                                    const LoadedTexture& outputTexture,
                                    const PostProcessEffect& effect)
{
    //
    // Setup
    //
    const auto programDef = m_programs->GetProgramDef(effect.programName);
    if (programDef == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "PostProcessingRenderer: No such program exists: {}", effect.programName);
        return;
    }

    const auto descriptorSet = m_descriptorSets->CachedAllocateDescriptorSet(
        programDef->GetDescriptorSetLayouts()[0],
        std::format("{}-{}-{}", "PostProcess", effect.tag, m_frameIndex)
    );
    if (!descriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error,"PostProcessingRenderer: Failed to allocate descriptor set");
        return;
    }

    //
    // Fetch Pipeline
    //
    std::vector<PushConstantRange> pushConstantRanges;

    pushConstantRanges = {
        {VK_SHADER_STAGE_COMPUTE_BIT, 0, (uint32_t)effect.pushPayload.size()}
    };

    const auto pipeline = GetComputePipeline(
        m_logger,
        m_vulkanObjs,
        m_shaders,
        m_pipelines,
        programDef,
        pushConstantRanges,
        m_frameIndex
    );
    if (!pipeline)
    {
        m_logger->Log(Common::LogLevel::Error, "PostProcessingRenderer: Failed to retrieve pipeline");
        return;
    }

    //
    // Render work
    //

    //
    // Bind Descriptor Set 0
    //

    // Bind Input Image
    {
        const auto samplerBindingDetails = programDef->GetBindingDetailsByName("i_inputImage");
        if (!samplerBindingDetails)
        {
            m_logger->Log(Common::LogLevel::Error, "PostProcessingRenderer: Failed to retrieve input sampler binding details");
            return;
        }

        (*descriptorSet)->WriteCombinedSamplerBind(
            (*samplerBindingDetails),
            inputTexture.vkImageViews.at(TextureView::DEFAULT),
            inputTexture.vkSamplers.at(effect.inputSamplerName)
        );
    }

    // Bind Output Image
    {
        const auto samplerBindingDetails = programDef->GetBindingDetailsByName("i_outputImage");
        if (!samplerBindingDetails)
        {
            m_logger->Log(Common::LogLevel::Error, "PostProcessingRenderer: Failed to retrieve output sampler binding details");
            return;
        }

        (*descriptorSet)->WriteCombinedSamplerBind(
            (*samplerBindingDetails),
            outputTexture.vkImageViews.at(TextureView::DEFAULT),
            outputTexture.vkSamplers.at(TextureSampler::DEFAULT)
        );
    }

    // Calculate work ground sizes by fitting the local work group sizes into
    // the render resolution
    const auto workGroupSize = CalculateWorkGroupSize();

    // Bind Push Constants
    commandBuffer->CmdPushConstants(
        *pipeline,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        effect.pushPayload.size(),
        effect.pushPayload.data()
    );

    // Issue compute dispatch
    const auto vkGlobalDescriptorSet = (*descriptorSet)->GetVkDescriptorSet();

    commandBuffer->CmdBindPipeline(*pipeline);
    commandBuffer->CmdBindDescriptorSets(*pipeline, 0, {vkGlobalDescriptorSet});
    commandBuffer->CmdDispatch(workGroupSize.first, workGroupSize.second, POST_PROCESS_LOCAL_SIZE_Z);
}

std::pair<uint32_t, uint32_t> PostProcessingRenderer::CalculateWorkGroupSize() const
{
    std::optional<unsigned int> workGroupSizeX;
    std::optional<unsigned int> workGroupSizeY;

    // Handle work cleanly divisible work group sizes with no fractional part
    if (m_renderSettings.resolution.w % POST_PROCESS_LOCAL_SIZE_X == 0)
    {
        workGroupSizeX = m_renderSettings.resolution.w / POST_PROCESS_LOCAL_SIZE_X;
    }
    if (m_renderSettings.resolution.h % POST_PROCESS_LOCAL_SIZE_Y == 0)
    {
        workGroupSizeY = m_renderSettings.resolution.h / POST_PROCESS_LOCAL_SIZE_Y;
    }

    // Handle non-cleanly divisible work by rounding up
    if (!workGroupSizeX)
    {
        workGroupSizeX = (unsigned int)((float)m_renderSettings.resolution.w / (float)POST_PROCESS_LOCAL_SIZE_X) + 1;
    }
    if (!workGroupSizeY)
    {
        workGroupSizeY = (unsigned int)((float)m_renderSettings.resolution.h / (float)POST_PROCESS_LOCAL_SIZE_Y) + 1;
    }

    return std::make_pair(*workGroupSizeX, *workGroupSizeY);
}

}
