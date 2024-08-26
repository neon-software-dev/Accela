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

#include <Accela/Common/SharedLib.h>
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
    class ACCELA_PUBLIC RendererBase : public IRenderer
    {
        public:

            RendererBase(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics);

            bool Startup(const RenderInit& renderInit, const RenderSettings& renderSettings) override;
            void Shutdown() override;

            [[nodiscard]] Ids::Ptr GetIds() const override;

            std::future<bool> CreateTexture(const Texture& texture,
                                            const TextureView& textureView,
                                            const TextureSampler& textureSampler) override;
            std::future<bool> DestroyTexture(TextureId textureId) override;
            std::future<bool> CreateMesh(const Mesh::Ptr& mesh, MeshUsage usage) override;
            std::future<bool> DestroyMesh(MeshId meshId) override;
            std::future<bool> CreateMaterial(const Material::Ptr& material) override;
            std::future<bool> DestroyMaterial(MaterialId materialId) override;
            std::future<bool> CreateRenderTarget(RenderTargetId renderTargetId, const std::string& tag) override;
            std::future<bool> DestroyRenderTarget(RenderTargetId renderTargetId) override;
            std::future<bool> UpdateWorld(const WorldUpdate& update) override;
            std::future<bool> RenderFrame(const RenderGraph::Ptr& renderGraph) override;
            std::future<bool> SurfaceChanged() override;
            std::future<bool> ChangeRenderSettings(const RenderSettings& renderSettings) override;

        protected:

            virtual void OnIdle() = 0;

            virtual bool OnInitialize(const RenderInit& renderInit, const RenderSettings& renderSettings) = 0;
            virtual bool OnShutdown() = 0;
            virtual bool OnRenderFrame(RenderGraph::Ptr renderGraph) = 0;
            virtual void OnCreateTexture(std::promise<bool> resultPromise,
                                         const Texture& texture,
                                         const TextureView& textureView,
                                         const TextureSampler& textureSampler) = 0;
            virtual bool OnDestroyTexture(TextureId textureId) = 0;
            virtual bool OnCreateMesh(std::promise<bool> resultPromise,
                                      const Mesh::Ptr& mesh,
                                      MeshUsage meshUsage) = 0;
            virtual bool OnDestroyMesh(MeshId meshId) = 0;
            virtual bool OnCreateMaterial(std::promise<bool> resultPromise,
                                          const Material::Ptr& material) = 0;
            virtual bool OnDestroyMaterial(MaterialId materialId) = 0;
            virtual bool OnCreateRenderTarget(RenderTargetId renderTargetId, const std::string& tag) = 0;
            virtual bool OnDestroyRenderTarget(RenderTargetId renderTargetId) = 0;
            virtual bool OnWorldUpdate(const WorldUpdate& update) = 0;
            virtual bool OnSurfaceChanged() = 0;
            virtual bool OnChangeRenderSettings(const RenderSettings& renderSettings) = 0;

        private:

            /**
             * Submits a RenderTask to the thread pool for processing.
             *
             * @tparam T The specific RenderTask subclass type for the task to be submitted
             * @tparam Ret The result type that the RenderTask operation returns
             * @tparam Args The argument types the RenderTask subclass is constructed with
             * @param defaultRet Default result to return if the renderer isn't running
             * @param args Arguments to construct the RenderTask subclass from
             *
             * @return A future which will contain the result of the render task operation
             */
            template <typename T, typename Ret, typename... Args>
            std::future<Ret> Submit(Ret defaultRet, Args&&... args)
            {
                // If the renderer isn't running, return an immediate default result
                if (m_thread == nullptr)
                {
                    std::promise<Ret> immediatePromise;
                    std::future<Ret> immediateFuture = immediatePromise.get_future();
                    immediatePromise.set_value(defaultRet);
                    return immediateFuture;
                }

                // Otherwise, bundle the args into a task+message and send it to the thread
                const auto task = std::make_shared<T>(std::forward<Args>(args)...);
                const auto taskMessage = std::make_shared<RenderTaskMessage<Ret>>(task);
                auto taskFuture = taskMessage->CreateFuture();
                m_thread->PostMessage(taskMessage);
                return taskFuture;
            }

            void OnTaskMessageReceived(const Common::Message::Ptr& msg);

        protected:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            Ids::Ptr m_ids;

        private:

            std::unique_ptr<Common::MessageDrivenThreadPool> m_thread;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDER_RENDERERBASE_H
