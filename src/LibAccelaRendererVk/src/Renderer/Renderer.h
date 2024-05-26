#ifndef LIBACCELARENDERERVK_SRC_RENDERER_RENDERER_H
#define LIBACCELARENDERERVK_SRC_RENDERER_RENDERER_H

#include "../ForwardDeclares.h"
#include "../VulkanObjs.h"

#include "../Util/DescriptorSets.h"

#include <Accela/Render/Ids.h>
#include <Accela/Render/RenderSettings.h>
#include <Accela/Render/Task/WorldUpdate.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <queue>
#include <optional>

namespace Accela::Render
{
    class Renderer
    {
        public:

            Renderer(Common::ILogger::Ptr logger,
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
                     uint8_t frameIndex);

            virtual ~Renderer() = default;

            virtual bool Initialize(const RenderSettings& renderSettings);
            virtual void Destroy();
            virtual bool OnRenderSettingsChanged(const RenderSettings& renderSettings);

            void OnFrameSynced();

        protected:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            Ids::Ptr m_ids;
            PostExecutionOpsPtr m_postExecutionOps;
            VulkanObjsPtr m_vulkanObjs;
            IProgramsPtr m_programs;
            IShadersPtr m_shaders;
            IPipelineFactoryPtr m_pipelines;
            IBuffersPtr m_buffers;
            IMaterialsPtr m_materials;
            ITexturesPtr m_textures;
            IMeshesPtr m_meshes;
            ILightsPtr m_lights;
            IRenderablesPtr m_renderables;
            uint8_t m_frameIndex;

            DescriptorSetsPtr m_descriptorSets;
            RenderSettings m_renderSettings{};
    };
}

#endif //LIBACCELARENDERERVK_SRC_RENDERER_RENDERER_H
