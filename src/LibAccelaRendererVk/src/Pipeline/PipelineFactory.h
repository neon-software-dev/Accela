/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_PIPELINE_PIPELINEFACTORY
#define LIBACCELARENDERERVK_SRC_PIPELINE_PIPELINEFACTORY

#include "IPipelineFactory.h"

#include "../ForwardDeclares.h"

#include <Accela/Common/Log/ILogger.h>
#include <unordered_map>

namespace Accela::Render
{
    class PipelineFactory : public IPipelineFactory
    {
        public:

            PipelineFactory(Common::ILogger::Ptr logger,
                            VulkanObjsPtr vulkanObjs,
                            IShadersPtr shaders);

            //
            // IPipelineFactory
            //
            [[nodiscard]] std::expected<VulkanPipelinePtr, bool> GetPipeline(const VulkanDevicePtr& device,
                                                                             const GraphicsPipelineConfig& config) override;

            [[nodiscard]] std::expected<VulkanPipelinePtr, bool> GetPipeline(const VulkanDevicePtr& device,
                                                                             const ComputePipelineConfig& config) override;

            void DestroyPipeline(const std::size_t& pipelineKey) override;

            void Destroy() override;

        private:

            template <typename ConfigType>
            [[nodiscard]] std::expected<VulkanPipelinePtr, bool> GetPipelineT(const VulkanDevicePtr& device, const ConfigType& config);

        private:

            Common::ILogger::Ptr m_logger;
            VulkanObjsPtr m_vulkanObjs;
            IShadersPtr m_shaders;

            // Config Hash -> Pipeline
            std::unordered_map<size_t, VulkanPipelinePtr> m_pipelines;
    };
}

#endif //LIBACCELARENDERERVK_SRC_PIPELINE_PIPELINEFACTORY
