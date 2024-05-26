#ifndef LIBACCELARENDERERVK_SRC_PIPELINE_IPIPELINEFACTORY
#define LIBACCELARENDERERVK_SRC_PIPELINE_IPIPELINEFACTORY

#include "PipelineConfig.h"

#include "../ForwardDeclares.h"

#include <expected>

namespace Accela::Render
{
    /**
     * Returns pipelines from a pipeline config. Re-uses pipelines that were previously
     * created for a specific config.
     */
    class IPipelineFactory
    {
        public:

            virtual ~IPipelineFactory() = default;

            /**
             * @return The pipeline, or nullptr on pipeline creation error
             */
            [[nodiscard]] virtual std::expected<VulkanPipelinePtr, bool> GetPipeline(const VulkanDevicePtr& device,
                                                                                    const PipelineConfig& config) = 0;

            virtual void DestroyPipeline(const std::size_t& pipelineKey) = 0;

            /**
             * Destroy and clear all pipelines that are being tracked
             */
            virtual void Destroy() = 0;
    };
}

#endif //LIBACCELARENDERERVK_SRC_PIPELINE_IPIPELINEFACTORY
