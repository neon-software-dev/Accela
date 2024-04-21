/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#include "Renderer.h"

#include "../PostExecutionOp.h"

#include "../Buffer/CPUItemBuffer.h"
#include "../Program/ProgramDef.h"

#include "../Vulkan/VulkanDescriptorSet.h"

namespace Accela::Render
{

Renderer::Renderer(
    Common::ILogger::Ptr logger,
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
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_ids(std::move(ids))
    , m_postExecutionOps(std::move(postExecutionOps))
    , m_vulkanObjs(std::move(vulkanObjs))
    , m_programs(std::move(programs))
    , m_shaders(std::move(shaders))
    , m_pipelines(std::move(pipelines))
    , m_buffers(std::move(buffers))
    , m_materials(std::move(materials))
    , m_textures(std::move(textures))
    , m_meshes(std::move(meshes))
    , m_lights(std::move(lights))
    , m_renderables(std::move(renderables))
    , m_frameIndex(frameIndex)
{

}

bool Renderer::Initialize(const RenderSettings& renderSettings)
{
    m_descriptorSets = std::make_shared<DescriptorSets>(
        m_logger,
        m_vulkanObjs->GetCalls(),
        m_vulkanObjs->GetDevice(),
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
    );

    m_renderSettings = renderSettings;

    return true;
}

void Renderer::Destroy()
{
    m_renderSettings = {};

    if (m_descriptorSets != nullptr)
    {
        m_descriptorSets->Destroy();
        m_descriptorSets = nullptr;
    }
}

bool Renderer::OnRenderSettingsChanged(const RenderSettings& renderSettings)
{
    m_renderSettings = renderSettings;
    return true;
}

void Renderer::OnFrameSynced()
{
    m_descriptorSets->MarkCachedSetsNotInUse();
}

}
