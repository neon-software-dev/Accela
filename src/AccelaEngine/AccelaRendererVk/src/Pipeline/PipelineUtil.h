/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_PIPELINE_PIPELINEUTIL
#define LIBACCELARENDERERVK_SRC_PIPELINE_PIPELINEUTIL

#include "PushConstant.h"
#include "PipelineConfig.h"

#include "../ForwardDeclares.h"

#include "../Renderer/RendererCommon.h"

#include <Accela/Render/Util/Rect.h>

#include <Accela/Common/Log/ILogger.h>

#include <expected>
#include <optional>
#include <cstdint>
#include <vector>

namespace Accela::Render
{
    [[nodiscard]] std::expected<VulkanPipelinePtr, bool> GetGraphicsPipeline(
        const Common::ILogger::Ptr& logger,
        const VulkanObjsPtr& vulkanObjs,
        const IShadersPtr& shaders,
        const IPipelineFactoryPtr& pipelines,
        const ProgramDefPtr& programDef,
        const VulkanRenderPassPtr& renderPass,
        const uint32_t& subpassIndex,
        const Viewport& viewport,
        const CullFace& cullFace = CullFace::Back,
        const PolygonFillMode& polygonFillMode = PolygonFillMode::Fill,
        const DepthBias& depthBias = DepthBias::Disabled,
        const std::optional<std::vector<PushConstantRange>>& pushConstantRanges = std::nullopt,
        const std::optional<std::size_t>& tag = std::nullopt,
        const std::optional<std::size_t>& oldPipelineHash = std::nullopt
    );

    [[nodiscard]] std::expected<VulkanPipelinePtr, bool> GetComputePipeline(
        const Common::ILogger::Ptr& logger,
        const VulkanObjsPtr& vulkanObjs,
        const IShadersPtr& shaders,
        const IPipelineFactoryPtr& pipelines,
        const ProgramDefPtr& programDef,
        const std::optional<std::vector<PushConstantRange>>& pushConstantRanges = std::nullopt,
        const std::optional<std::size_t>& tag = std::nullopt,
        const std::optional<std::size_t>& oldPipelineHash = std::nullopt
    );
}

#endif //LIBACCELARENDERERVK_SRC_PIPELINE_PIPELINEUTIL
