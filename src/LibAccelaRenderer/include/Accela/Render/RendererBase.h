/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERERBASE_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERERBASE_H

#include "IRenderer.h"
#include "Task/RenderTask.h"
#include "Task/RenderTaskMessage.h"

#include <Accela/Common/ImageData.h>
#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>
#include <Accela/Common/Thread/MessageDrivenThreadPool.h>

#include <thread>
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <variant>

namespace Accela::Render
{
    /**
     * IRenderer implementation which spins up a render thread and dispatches tasks to it via
     * a thread-safe queue. Calls into OnXXX(..) methods on the render thread for each message
     * that it receives.
     */
    class RendererBase : public IRenderer
    {
        public:

            RendererBase(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics);

            bool Startup(const RenderSettings& renderSettings, const std::vector<ShaderSpec>& shaders) override;
            void Shutdown() override;

            [[nodiscard]] Ids::Ptr GetIds() const override;

            std::future<bool> CreateTexture(const Texture& texture,
                                            const TextureView& textureView,
                                            const TextureSampler& textureSampler,
                                            bool generateMipMaps) override;
            std::future<bool> DestroyTexture(TextureId textureId) override;
            std::future<bool> CreateMesh(const Mesh::Ptr& mesh, MeshUsage usage) override;
            std::future<bool> DestroyMesh(MeshId meshId) override;
            std::future<bool> CreateMaterial(const Material::Ptr& material) override;
            std::future<bool> DestroyMaterial(MaterialId materialId) override;
            std::future<bool> CreateFrameBuffer(FrameBufferId frameBufferId, const std::vector<TextureId>& attachmentTextures) override;
            std::future<bool> DestroyFrameBuffer(FrameBufferId frameBufferId) override;
            std::future<bool> UpdateWorld(const WorldUpdate& update) override;
            std::future<bool> RenderFrame(const RenderGraph::Ptr& renderGraph) override;
            std::future<bool> SurfaceChanged() override;
            std::future<bool> ChangeRenderSettings(const RenderSettings& renderSettings) override;

        protected:

            virtual void OnIdle() = 0;

            virtual bool OnInitialize(const RenderSettings& renderSettings, const std::vector<ShaderSpec>& shaders) = 0;
            virtual bool OnShutdown() = 0;
            virtual bool OnRenderFrame(RenderGraph::Ptr renderGraph) = 0;
            virtual void OnCreateTexture(std::promise<bool> resultPromise,
                                         const Texture& texture,
                                         const TextureView& textureView,
                                         const TextureSampler& textureSampler,
                                         bool generateMipMaps) = 0;
            virtual bool OnDestroyTexture(TextureId textureId) = 0;
            virtual bool OnCreateMesh(std::promise<bool> resultPromise,
                                      const Mesh::Ptr& mesh,
                                      MeshUsage meshUsage) = 0;
            virtual bool OnDestroyMesh(MeshId meshId) = 0;
            virtual bool OnCreateMaterial(std::promise<bool> resultPromise,
                                          const Material::Ptr& material) = 0;
            virtual bool OnDestroyMaterial(MaterialId materialId) = 0;
            virtual bool OnCreateFrameBuffer(FrameBufferId frameBufferId, const std::vector<TextureId>& attachmentTextures) = 0;
            virtual bool OnDestroyFrameBuffer(FrameBufferId frameBufferId) = 0;
            virtual bool OnWorldUpdate(const WorldUpdate& update) = 0;
            virtual bool OnSurfaceChanged() = 0;
            virtual bool OnChangeRenderSettings(const RenderSettings& renderSettings) = 0;

        private:

            template <typename T, typename... Args>
            std::future<bool> Submit(Args&&... args)
            {
                // If the renderer isn't running, return an immediate false result
                if (m_thread == nullptr)
                {
                    std::promise<bool> immediatePromise;
                    std::future<bool> immediateFuture = immediatePromise.get_future();
                    immediatePromise.set_value(false);
                    return immediateFuture;
                }

                // Otherwise, bundle the args into a task+message and send it to the thread
                const auto task = std::make_shared<T>(std::forward<Args>(args)...);
                const auto taskMessage = std::make_shared<RenderTaskMessage>(task);
                auto taskFuture = taskMessage->CreateFuture();
                m_thread->PostMessage(taskMessage);
                return taskFuture;
            }

            void OnTaskMessageReceived(const RenderTaskMessage::Ptr& msg);

        protected:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            Ids::Ptr m_ids;

        private:

            std::unique_ptr<Common::MessageDrivenThreadPool> m_thread;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERERBASE_H
