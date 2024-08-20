/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_PIPELINE_PIPELINECONFIG
#define LIBACCELARENDERERVK_SRC_PIPELINE_PIPELINECONFIG

#include "../Vulkan/VulkanRenderPass.h"
#include "../Renderer/RendererCommon.h"

#include <Accela/Render/Util/Rect.h>

#include <vulkan/vulkan.h>

#include <string>
#include <vector>
#include <sstream>
#include <optional>

namespace Accela::Render
{
    enum class PipelineType
    {
        Graphics,
        Compute
    };

    enum class PrimitiveTopology
    {
        TriangleList,
        TriangleFan,
        PatchList
    };

    enum class PolygonFillMode
    {
        Fill,
        Line
    };

    enum class DepthBias
    {
        Enabled,
        Disabled
    };

    struct ColorAttachment
    {
        explicit ColorAttachment(bool _enableColorBlending)
            : enableColorBlending(_enableColorBlending)
        { }

        bool enableColorBlending;
    };

    /**
     * Contains the details needed to build a graphics pipeline.
     *
     * Warning: Any changes made to this struct require a matching change in GetUniqueKey() func below.
     */
    struct GraphicsPipelineConfig
    {
        // Set this if a pipeline should have a different key than an otherwise identical config
        std::optional<std::size_t> tag{std::nullopt};

        //
        // General
        //
        uint32_t subpassIndex{0};

        //
        // Shader stage configuration
        //
        std::optional<std::string> vertShaderFileName;
        std::optional<std::string> fragShaderFileName;
        std::optional<std::string> tescShaderFileName;
        std::optional<std::string> teseShaderFileName;

        //
        // Viewport/Scissoring configuration
        //
        Viewport viewport;

        //
        // Rasterization configuration
        //
        CullFace cullFace{CullFace::Back};
        PolygonFillMode polygonFillMode{PolygonFillMode::Fill};
        DepthBias depthBias{DepthBias::Disabled};

        //
        // Tesselation configuration
        //
        uint32_t tesselationNumControlPoints{4};

        //
        // RenderPass configuration
        //
        VkRenderPass vkRenderPass{VK_NULL_HANDLE};
        bool usesDepthStencil{false};
        std::vector<ColorAttachment> colorAttachments;

        //
        // Vertex input configuration
        //
        std::vector<VkVertexInputBindingDescription> vkVertexInputBindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> vkVertexInputAttributeDescriptions;

        //
        // Pipeline layout configuration
        //
        std::optional<std::vector<VkPushConstantRange>> vkPushConstantRanges;
        std::optional<std::vector<VkDescriptorSetLayout>> vkDescriptorSetLayouts;

        //
        // Vertex assembly configuration
        //
        bool primitiveRestartEnable = false;
        PrimitiveTopology primitiveTopology = PrimitiveTopology::TriangleList;

        //
        //
        //

        [[nodiscard]] size_t GetUniqueKey() const
        {
            std::stringstream ss;

            if (tag.has_value())
            {
                ss << "[Tag]" << *tag;
            }

            ss << "[SubpassIndex]" << subpassIndex;

            if (vertShaderFileName.has_value())
            {
                ss << "[VertShader]" << *vertShaderFileName;
            }
            if (fragShaderFileName.has_value())
            {
                ss << "[FragShader]" << *fragShaderFileName;
            }
            if (tescShaderFileName.has_value())
            {
                ss << "[TescShader]" << *tescShaderFileName;
            }
            if (teseShaderFileName.has_value())
            {
                ss << "[TeseShader]" << *teseShaderFileName;
            }
            ss << "[Viewport]" << viewport.x << "," << viewport.y << "-" << viewport.w << "x" << viewport.h;
            ss << "[CullFace]" << (unsigned int)cullFace;
            ss << "[PolygonFillMode]" << (unsigned int)polygonFillMode;
            ss << "[TesselationNumControlPoints]" << tesselationNumControlPoints;
            ss << "[RenderPass]" << std::hash<VkRenderPass>{}(vkRenderPass);
            ss << "[DepthStencil]" << usesDepthStencil;

            for (const auto& colorAttachment : colorAttachments)
            {
                ss << "[ColorAttachment] " << "ColorBlending: " << colorAttachment.enableColorBlending;
            }

            for (const auto& bindingDesc : vkVertexInputBindingDescriptions)
            {
                ss << "[VIBD]" << bindingDesc.binding << "," << bindingDesc.stride << "," << bindingDesc.inputRate;
            }

            for (const auto& attribDesc : vkVertexInputAttributeDescriptions)
            {
                ss << "[VIAD]" << attribDesc.location << "," << attribDesc.binding << "," << attribDesc.format
                    << "," << attribDesc.offset;
            }

            if (vkPushConstantRanges.has_value())
            {
                for (unsigned int x = 0; x < vkPushConstantRanges->size(); ++x)
                {
                    ss << "[PUSH" << x << "] " << (*vkPushConstantRanges)[x].offset << "," <<
                        (*vkPushConstantRanges)[x].size << "," << (*vkPushConstantRanges)[x].stageFlags;
                }
            }

            if (vkDescriptorSetLayouts.has_value())
            {
                for (unsigned int x = 0; x < vkDescriptorSetLayouts->size(); ++x)
                {
                    ss << "[Layout " << x << "]" << (*vkDescriptorSetLayouts)[x];
                }
            }

            ss << "[PrimitiveRestart]" << static_cast<unsigned int>(primitiveRestartEnable);
            ss << "[PrimitiveTopology] " << static_cast<unsigned int>(primitiveTopology);

            const std::string keyStr = ss.str();
            return std::hash<std::string>{}(keyStr);
        }
    };

    /**
     * Contains the details needed to build a compute pipeline.
     *
     * Warning: Any changes made to this struct require a matching change in GetUniqueKey() func below.
     */
    struct ComputePipelineConfig
    {
        // Set this if a pipeline should have a different key than an otherwise identical config
        std::optional<std::size_t> tag{std::nullopt};

        //
        // Shader Configuration
        //
        std::string computeShaderFileName;

        //
        // Pipeline layout configuration
        //
        std::optional<std::vector<VkPushConstantRange>> vkPushConstantRanges;
        std::optional<std::vector<VkDescriptorSetLayout>> vkDescriptorSetLayouts;

        [[nodiscard]] size_t GetUniqueKey() const
        {
            std::stringstream ss;

            if (tag.has_value())
            {
                ss << "[Tag]" << *tag;
            }

            ss << "[ComputeShader]" << computeShaderFileName;

            if (vkPushConstantRanges.has_value())
            {
                for (unsigned int x = 0; x < vkPushConstantRanges->size(); ++x)
                {
                    ss << "[PUSH" << x << "] " << (*vkPushConstantRanges)[x].offset << "," <<
                       (*vkPushConstantRanges)[x].size << "," << (*vkPushConstantRanges)[x].stageFlags;
                }
            }

            if (vkDescriptorSetLayouts.has_value())
            {
                for (unsigned int x = 0; x < vkDescriptorSetLayouts->size(); ++x)
                {
                    ss << "[Layout " << x << "]" << (*vkDescriptorSetLayouts)[x];
                }
            }

            const std::string keyStr = ss.str();
            return std::hash<std::string>{}(keyStr);
        }
    };
}

#endif //LIBACCELARENDERERVK_SRC_PIPELINE_PIPELINECONFIG
