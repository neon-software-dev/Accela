/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERER_RENDERERGROUP_H
#define LIBACCELARENDERERVK_SRC_RENDERER_RENDERERGROUP_H

#include "Renderer.h"

#include "../ForwardDeclares.h"

#include <Accela/Render/Ids.h>
#include <Accela/Render/RenderSettings.h>
#include <Accela/Render/Task/WorldUpdate.h>

#include <Accela/Common/Log/ILogger.h>

#include <vector>
#include <memory>
#include <cassert>

namespace Accela::Render
{
    template <class R>
    concept RendererDerived = std::is_base_of<Renderer, R>::value;

    /**
     * Wrapper class which internally maintains X instances of a given Renderer subclass, where
     * X matches the number of frames in flight. Manages creating and destroying the Renderers as
     * frames in flight settings changes. Also provides a couple of helper functions for delivering
     * messages to all Renderers in the group, such as when render settings change.
     *
     * @tparam R The concrete type of Renderer
     */
    template <RendererDerived R>
    class RendererGroup
    {
        public:

            RendererGroup(Common::ILogger::Ptr logger,
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
                          IRenderablesPtr renderables)
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
                 , m_images(std::move(images))
                 , m_textures(std::move(textures))
                 , m_meshes(std::move(meshes))
                 , m_lights(std::move(lights))
                 , m_renderables(std::move(renderables))
            { }

            bool Initialize(const RenderSettings& renderSettings);
            void Destroy();
            bool OnRenderSettingsChanged(const RenderSettings& renderSettings);

            [[nodiscard]] R& GetRendererForFrame(uint8_t frameIndex);

        private:

            bool AddNewRenderer(const RenderSettings& renderSettings);

        private:

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
            IImagesPtr m_images;
            ITexturesPtr m_textures;
            IMeshesPtr m_meshes;
            ILightsPtr m_lights;
            IRenderablesPtr m_renderables;

            std::vector<std::unique_ptr<R>> m_renderers;
    };

    template<RendererDerived R>
    bool RendererGroup<R>::Initialize(const RenderSettings& renderSettings)
    {
        m_logger->Log(Common::LogLevel::Info, "RendererGroup: Initializing for {} frames in flight", renderSettings.framesInFlight);

        assert(m_renderers.empty());

        for (uint8_t frameIndex = 0; frameIndex < renderSettings.framesInFlight; ++frameIndex)
        {
            if (!AddNewRenderer(renderSettings)) { return false; }
        }

        return true;
    }

    template<RendererDerived R>
    void RendererGroup<R>::Destroy()
    {
        m_logger->Log(Common::LogLevel::Info, "RendererGroup: Destroying");

        for (auto& renderer : m_renderers)
        {
            renderer->Destroy();
        }
        m_renderers.clear();
    }

    template<RendererDerived R>
    R& RendererGroup<R>::GetRendererForFrame(uint8_t frameIndex)
    {
        return *m_renderers[frameIndex];
    }

    template<RendererDerived R>
    bool RendererGroup<R>::OnRenderSettingsChanged(const RenderSettings& renderSettings)
    {
        m_logger->Log(Common::LogLevel::Info, "RendererGroup: Notified render settings changed");

        //
        // Update renderers with new render settings
        //
        for (auto& renderer : m_renderers)
        {
            renderer->OnRenderSettingsChanged(renderSettings);
        }

        //
        // Add or destroy renderers as needed to match the number of frames in flight
        //
        if (renderSettings.framesInFlight == m_renderers.size())
        {
            m_logger->Log(Common::LogLevel::Info, "RendererGroup: Same number of frames, ignoring");
            return true;
        }

        if (renderSettings.framesInFlight > m_renderers.size())
        {
            m_logger->Log(Common::LogLevel::Info, "RendererGroup: More frames in flight, building as needed");

            for (uint8_t frameIndex = m_renderers.size(); frameIndex < renderSettings.framesInFlight; ++frameIndex)
            {
                if (!AddNewRenderer(renderSettings)) { return false; }
            }

            return true;
        }

        if (renderSettings.framesInFlight < m_renderers.size())
        {
            m_logger->Log(Common::LogLevel::Info, "RendererGroup: Fewer frames in flight, destroying as needed");

            const uint8_t numToDestroy = m_renderers.size() - renderSettings.framesInFlight;

            for (uint8_t x = 0; x < numToDestroy; ++x)
            {
                m_renderers[m_renderers.size()-1]->Destroy();
                m_renderers.erase(m_renderers.end());
            }

            return true;
        }

        return true;
    }

    template<RendererDerived R>
    bool RendererGroup<R>::AddNewRenderer(const RenderSettings& renderSettings)
    {
        const uint8_t frameIndex = m_renderers.size();

        std::unique_ptr<R> renderer = std::make_unique<R>(
            m_logger,
            m_metrics,
            m_ids,
            m_postExecutionOps,
            m_vulkanObjs,
            m_programs,
            m_shaders,
            m_pipelines,
            m_buffers,
            m_materials,
            m_images,
            m_textures,
            m_meshes,
            m_lights,
            m_renderables,
            frameIndex
        );
        if (!renderer->Initialize(renderSettings))
        {
            m_logger->Log(Common::LogLevel::Error, "RendererGroup: Failed to initialize a renderer");
            return false;
        }

        m_renderers.push_back(std::move(renderer));
        return true;
    }
}

#endif //LIBACCELARENDERERVK_SRC_RENDERER_RENDERERGROUP_H
