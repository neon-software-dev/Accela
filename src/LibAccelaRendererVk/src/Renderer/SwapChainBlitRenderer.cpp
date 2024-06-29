/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SwapChainBlitRenderer.h"

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

#include <glm/gtc/matrix_transform.hpp>

#include <format>
#include <vector>

namespace Accela::Render
{

SwapChainBlitRenderer::SwapChainBlitRenderer(Common::ILogger::Ptr logger,
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

bool SwapChainBlitRenderer::Initialize(const RenderSettings& renderSettings)
{
    if (!Renderer::Initialize(renderSettings))
    {
        return false;
    }

    m_logger->Log(Common::LogLevel::Info, "SwapChainBlitRenderer: Initializing, for frame {}", m_frameIndex);

    //
    // Create a DescriptorSet for rendering
    //
    const auto programDef = m_programs->GetProgramDef("SwapChainBlit");
    if (programDef == nullptr)
    {
        m_logger->Log(Common::LogLevel::Error, "SwapChainBlitRenderer: Failed to find SwapChainBlit program");
        return false;
    }

    const auto descriptorSet = m_descriptorSets->AllocateDescriptorSet(
        programDef->GetDescriptorSetLayouts()[0],
        std::format("SwapChainBlitRenderer-{}", m_frameIndex)
    );
    if (!descriptorSet)
    {
        m_logger->Log(Common::LogLevel::Error,"SwapChainBlitRenderer: Failed to allocate descriptor set for binding 0");
        return false;
    }

    //
    // Update state
    //
    m_programDef = programDef;
    m_descriptorSet = *descriptorSet;

    return true;
}

void SwapChainBlitRenderer::Destroy()
{
    m_logger->Log(Common::LogLevel::Info, "SwapChainBlitRenderer: Destroying for frame {}", m_frameIndex);

    if (m_meshId != MeshId(INVALID_ID))
    {
        m_meshes->DestroyMesh(m_meshId, true);
        m_meshId = MeshId(INVALID_ID);
    }

    if (m_descriptorSet != nullptr)
    {
        m_descriptorSets->FreeDescriptorSet(m_descriptorSet);
        m_descriptorSet = nullptr;
    }

    if (m_pipelineHash)
    {
        m_pipelines->DestroyPipeline(*m_pipelineHash);
        m_pipelineHash = std::nullopt;
    }

    m_programDef = nullptr;
    m_renderSettings = {};
    m_renderSize = {};
    m_targetSize = {};

    return Renderer::Destroy();
}

void SwapChainBlitRenderer::Render(const VulkanCommandBufferPtr& commandBuffer,
                                   const VulkanRenderPassPtr& renderPass,
                                   const VulkanFramebufferPtr& swapChainFramebuffer,
                                   const LoadedTexture& renderTexture,
                                   const LoadedTexture& screenTexture)
{
    //
    // Update our mesh to blit the render into the swap chain framebuffer
    //
    if (!ConfigureMeshFor(m_renderSettings, *swapChainFramebuffer->GetSize()))
    {
        return;
    }

    //
    // Obtain required objects/data
    //
    const auto loadedMesh = m_meshes->GetLoadedMesh(m_meshId);
    if (!loadedMesh)
    {
        m_logger->Log(Common::LogLevel::Error, "SwapChainBlitRenderer: Failed to retrieve mesh");
        return;
    }

    const auto vkDescriptorSet = m_descriptorSet->GetVkDescriptorSet();
    const auto vkVerticesBuffer = loadedMesh->verticesBuffer->GetBuffer()->GetVkBuffer();
    const auto vkIndicesBuffer = loadedMesh->indicesBuffer->GetBuffer()->GetVkBuffer();

    const auto renderSamplerBindingDetails = m_programDef->GetBindingDetailsByName("i_renderSampler");
    if (!renderSamplerBindingDetails)
    {
        m_logger->Log(Common::LogLevel::Error, "SwapChainBlitRenderer: Failed to retrieve render sampler binding details");
        return;
    }

    const auto screenSamplerBindingDetails = m_programDef->GetBindingDetailsByName("i_screenSampler");
    if (!screenSamplerBindingDetails)
    {
        m_logger->Log(Common::LogLevel::Error, "SwapChainBlitRenderer: Failed to retrieve screen sampler binding details");
        return;
    }

    const auto viewport = Viewport(0, 0, swapChainFramebuffer->GetSize()->w, swapChainFramebuffer->GetSize()->h);

    const auto pipeline = GetGraphicsPipeline(
        m_logger,
        m_vulkanObjs,
        m_shaders,
        m_pipelines,
        m_programDef,
        renderPass,
        0,
        viewport,
        CullFace::Back,
        PolygonFillMode::Fill,
        DepthBias::Disabled,
        PushConstantRange::None(),
        m_frameIndex,
        m_pipelineHash
    );
    if (!pipeline)
    {
        m_logger->Log(Common::LogLevel::Error, "SwapChainBlitRenderer: Failed to retrieve pipeline");
        return;
    }

    m_pipelineHash = (*pipeline)->GetConfigHash();

    //
    // Render work
    //

    m_descriptorSet->WriteCombinedSamplerBind(
        (*renderSamplerBindingDetails),
        renderTexture.vkImageViews.at(TextureView::DEFAULT),
        renderTexture.vkSamplers.at(TextureSampler::DEFAULT)
    );

    m_descriptorSet->WriteCombinedSamplerBind(
        (*screenSamplerBindingDetails),
        screenTexture.vkImageViews.at(TextureView::DEFAULT),
        screenTexture.vkSamplers.at(TextureSampler::DEFAULT)
    );

    commandBuffer->CmdBindPipeline(*pipeline);
    commandBuffer->CmdBindDescriptorSets(*pipeline, 0, {vkDescriptorSet});
    commandBuffer->CmdBindVertexBuffers(0, 1, {vkVerticesBuffer}, {VkDeviceSize{0}});
    commandBuffer->CmdBindIndexBuffer(vkIndicesBuffer, 0, VK_INDEX_TYPE_UINT32);
    commandBuffer->CmdDrawIndexed(loadedMesh->numIndices, 1, 0, 0, 0);
}

bool SwapChainBlitRenderer::ConfigureMeshFor(const RenderSettings& renderSettings, const USize& targetSize)
{
    if ((m_renderSize && m_targetSize && m_presentScaling) &&
        (*m_renderSize == renderSettings.resolution) &&
        (*m_targetSize == targetSize) &&
        (*m_presentScaling == renderSettings.presentScaling))
    {
        // No change in render resolution or target resolution, nothing to update
        return true;
    }

    //
    // Calculate blit mesh vertices
    //

    // ScreenRect, in screen coordinates, of the rect to use to blit the render to the swap chain image
    const ScreenRect blitRect = CalculateBlitRect(m_renderSettings, targetSize);

    // Create an orthographic projectionFrustum to transform screen coordinates to clip coordinates
    const glm::mat4 projection = glm::ortho(0.0f, (float)targetSize.w, (float)targetSize.h, 0.0f, 0.0f, 1.0f);

    glm::vec3 topLeft = projection * glm::vec4(blitRect.x, blitRect.y, 0, 1);
    glm::vec3 topRight = projection * glm::vec4(blitRect.x + blitRect.w, blitRect.y, 0, 1);
    glm::vec3 bottomRight = projection * glm::vec4(blitRect.x + blitRect.w, blitRect.y + blitRect.h, 0, 1);
    glm::vec3 bottomLeft = projection * glm::vec4(blitRect.x, blitRect.y + blitRect.h, 0, 1);

    // Adjust the clip coordinates to match vulkan clip space, which has y-axis inverted
    topLeft.y = -topLeft.y;
    topRight.y = -topRight.y;
    bottomRight.y = -bottomRight.y;
    bottomLeft.y = -bottomLeft.y;

    //
    // Create the blit mesh's data
    //

    auto meshId = m_meshId;
    if (!meshId.IsValid())
    {
        meshId = m_ids->meshIds.GetId();
    }

    const auto mesh = std::make_shared<StaticMesh>(
        meshId,
        std::vector<MeshVertex>{
            {
                {{topLeft.x, topLeft.y, 0}, {0, 0, 0}, {0, 0}},
                {{topRight.x, topRight.y, 0}, {0, 0, 0}, {1, 0}},
                {{bottomRight.x, bottomRight.y, 0}, {0, 0, 0}, {1, 1}},
                {{bottomLeft.x, bottomLeft.y, 0}, {0, 0, 0}, {0, 1}}
            }
        },
        std::vector<uint32_t>{ 0, 2, 1, 0, 3, 2 },
        std::format("SwapChainMesh-{}", m_frameIndex)
    );

    //
    // Create or update the blit mesh
    //
    if (m_meshId.IsValid())
    {
        if (!m_meshes->UpdateMesh(mesh, std::promise<bool>{}))
        {
            m_logger->Log(Common::LogLevel::Error, "SwapChainBlitRenderer: Failed to update mesh: {}", m_meshId.id);
            return false;
        }
    }
    else
    {
        if (!m_meshes->LoadMesh(mesh, MeshUsage::Dynamic, std::promise<bool>{}))
        {
            m_logger->Log(Common::LogLevel::Error, "SwapChainBlitRenderer: Failed to create mesh");
            m_ids->meshIds.ReturnId(mesh->id);
            return false;
        }
    }

    //
    // Update state
    //
    m_meshId = meshId;
    m_presentScaling = renderSettings.presentScaling;
    m_renderSize = renderSettings.resolution;
    m_targetSize = targetSize;

    return true;
}

}
