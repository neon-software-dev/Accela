/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDERER_TASK_RENDERTASK_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDERER_TASK_RENDERTASK_H

#include <Accela/Common/SharedLib.h>

#include <memory>
#include <tuple>

namespace Accela::Render
{
    enum class RenderTaskType
    {
        Initialize,             // Initialize/Start-Up message
        Shutdown,               // Shut-Down message
        RenderFrame,            // Render a frame
        CreateTexture,          // Register a texture
        UpdateTexture,          // Update a textures data
        DestroyTexture,         // Destroy a texture
        CreateMesh,             // Register a mesh
        DestroyMesh,            // Destroy a mesh
        CreateMaterial,         // Register a material
        DestroyMaterial,        // Destroy a material
        CreateRenderTarget,     // Create a render target
        DestroyRenderTarget,    // Destroy a render target
        WorldUpdate,            // Update the state of the world
        SurfaceChanged,         // Handle window/surface change
        ChangeRenderSettings    // Apply new render settings
    };

    /**
     * Represents a message/task that can be sent to the Renderer
     */
    struct ACCELA_PUBLIC RenderTask
    {
        using Ptr = std::shared_ptr<RenderTask>;

        virtual ~RenderTask() = default;

        [[nodiscard]] virtual RenderTaskType GetTaskType() const noexcept = 0;
    };

    template <RenderTaskType TaskType, typename... Args>
    struct DataRenderTask : public RenderTask
    {
        explicit DataRenderTask(Args... args)
            : data(args...)
        { }

        [[nodiscard]] RenderTaskType GetTaskType() const noexcept override { return TaskType; }

        std::tuple<Args...> data;
    };
}

#endif //LIBACCELARENDERER_INCLUDE_ACCELA_RENDERER_TASK_RENDERTASK_H
