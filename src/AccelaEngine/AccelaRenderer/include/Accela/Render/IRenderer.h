/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDERER_IRENDERER_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDERER_IRENDERER_H

#include "Ids.h"
#include "RenderInit.h"
#include "RenderSettings.h"
#include "Shader/ShaderSpec.h"
#include "Texture/Texture.h"
#include "Texture/TextureView.h"
#include "Texture/TextureSampler.h"
#include "Graph/RenderGraph.h"
#include "Task/WorldUpdate.h"
#include "Mesh/Mesh.h"
#include "Material/Material.h"

#include <Accela/Common/SharedLib.h>
#include <Accela/Common/ImageData.h>

#include <memory>
#include <future>
#include <vector>
#include <string>

namespace Accela::Render
{
    /**
     * Main external interface for users to interact with the Renderer system.
     *
     * Most methods are asynchronous and return a future that's signaled when the render
     * thread has finished processing the message.
     */
    class ACCELA_PUBLIC IRenderer
    {
        public:

            using Ptr = std::shared_ptr<IRenderer>;

        public:

            virtual ~IRenderer() = default;

            /**
             * Blocking call to start the renderer with the provided initial render settings and shaders.
             *
             * @param renderInit Renderer initialization settings
             * @param renderSettings The initial render settings to be used
             *
             * @return Whether or not startup was successful
             */
            virtual bool Startup(const RenderInit& renderInit, const RenderSettings& renderSettings) = 0;

            /**
             * Stops the render thread and cleans up resources
             */
            virtual void Shutdown() = 0;

            // Synchronous
            [[nodiscard]] virtual Ids::Ptr GetIds() const = 0;
            [[nodiscard]] virtual std::optional<ObjectId> GetTopObjectAtRenderPoint(const glm::vec2& renderPoint) const = 0;

            // Asynchronous
            virtual std::future<bool> CreateTexture(const Texture& texture,
                                                    const TextureView& textureView,
                                                    const TextureSampler& textureSampler) = 0;
            virtual std::future<bool> UpdateTexture(const TextureId& textureId,
                                                    const Common::ImageData::Ptr& imageData) = 0;
            virtual std::future<bool> DestroyTexture(TextureId textureId) = 0;
            virtual std::future<bool> CreateMesh(const Mesh::Ptr& mesh, MeshUsage usage) = 0;
            virtual std::future<bool> DestroyMesh(MeshId meshId) = 0;
            virtual std::future<bool> CreateMaterial(const Material::Ptr& material) = 0;
            virtual std::future<bool> DestroyMaterial(MaterialId materialId) = 0;
            virtual std::future<bool> CreateRenderTarget(RenderTargetId renderTargetId, const std::string& tag) = 0;
            virtual std::future<bool> DestroyRenderTarget(RenderTargetId renderTargetId) = 0;
            virtual std::future<bool> UpdateWorld(const WorldUpdate& update) = 0;
            virtual std::future<bool> RenderFrame(const RenderGraph::Ptr& renderGraph) = 0;
            virtual std::future<bool> SurfaceChanged() = 0;
            virtual std::future<bool> ChangeRenderSettings(const RenderSettings& renderSettings) = 0;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDERER_IRENDERER_H
