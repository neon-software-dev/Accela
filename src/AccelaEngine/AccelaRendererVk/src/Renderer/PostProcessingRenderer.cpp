/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PostProcessingRenderer.h"

#include "../PostExecutionOp.h"

#include "../Program/IPrograms.h"
#include "../Mesh/IMeshes.h"
#include "../Pipeline/IPipelineFactory.h"
#include "../Buffer/CPUDataBuffer.h"
#include "../Pipeline/PipelineUtil.h"

#include "../Vulkan/VulkanCommandBuffer.h"
#include "../Vulkan/VulkanPipeline.h"
#include "../Vulkan/VulkanDescriptorSet.h"
#include "../Vulkan/VulkanFramebuffer.h"

#include <Accela/Render/IVulkanCalls.h>
#include <Accela/Render/RenderLogic.h>

#include <Accela/Render/Mesh/StaticMesh.h>

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
                                               IImagesPtr images,
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
               std::move(images),
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
                                    const LoadedImage& inputImage,
                                    const LoadedImage& outputImage,
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
    const std::vector<PushConstantRange> pushConstantRanges = {
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
    // Bind Input Samplers
    //
    auto inputSamplers = effect.additionalSamplers;
    inputSamplers.emplace_back("i_inputImage", inputImage, VK_IMAGE_ASPECT_COLOR_BIT, effect.inputImageView, effect.inputImageSampler);

    for (const auto& inputSampler : inputSamplers)
    {
        const auto& bindingName = std::get<0>(inputSampler);
        const auto& texture = std::get<1>(inputSampler);
        const auto& textureViewName = std::get<3>(inputSampler);
        const auto& textureSamplerName = std::get<4>(inputSampler);

        const auto samplerBindingDetails = programDef->GetBindingDetailsByName(bindingName);
        if (!samplerBindingDetails)
        {
            m_logger->Log(Common::LogLevel::Error,
              "PostProcessingRenderer: Failed to retrieve input sampler binding details: {}", bindingName);
            return;
        }

        (*descriptorSet)->WriteCombinedSamplerBind(
            (*samplerBindingDetails),
            texture.vkImageViews.at(textureViewName),
            texture.vkSamplers.at(textureSamplerName)
        );
    }

    //
    // Bind Input Buffers
    //
    unsigned int inputBufferIndex = 0;

    for (const auto& bufferPayload : effect.bufferPayloads)
    {
        const auto bufferBindingDetails = programDef->GetBindingDetailsByName(bufferPayload.first);
        if (!bufferBindingDetails)
        {
            m_logger->Log(Common::LogLevel::Error,
              "PostProcessingRenderer: Failed to retrieve input buffer binding details: {}", bufferPayload.first);
            return;
        }

        const auto inputBuffer = CPUDataBuffer::Create(
            m_buffers,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            bufferPayload.second.size(),
            std::format("PostProcessInput-{}-{}", effect.tag, inputBufferIndex)
        );

        BufferUpdate bufferUpdate{};
        bufferUpdate.pData = bufferPayload.second.data();
        bufferUpdate.updateOffset = 0;
        bufferUpdate.dataByteSize = bufferPayload.second.size();

        (*inputBuffer)->Update(ExecutionContext::CPU(), {bufferUpdate});

        (*descriptorSet)->WriteBufferBind(
            bufferBindingDetails,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            (*inputBuffer)->GetBuffer()->GetVkBuffer(),
            0,
            0
        );

        m_postExecutionOps->Enqueue_Current(BufferDeleteOp(m_buffers, (*inputBuffer)->GetBuffer()->GetBufferId()));

        inputBufferIndex++;
    }

    //
    // Bind Output RenderTexture
    //
    {
        const auto samplerBindingDetails = programDef->GetBindingDetailsByName("i_outputImage");
        if (!samplerBindingDetails)
        {
            m_logger->Log(Common::LogLevel::Error, "PostProcessingRenderer: Failed to retrieve output sampler binding details");
            return;
        }

        (*descriptorSet)->WriteCombinedSamplerBind(
            (*samplerBindingDetails),
            outputImage.vkImageViews.at(ImageView::DEFAULT()),
            outputImage.vkSamplers.at(ImageSampler::DEFAULT())
        );
    }

    // Calculate work group sizes by fitting the local work group sizes into
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

    // Handle cleanly divisible work group sizes with no fractional part
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
