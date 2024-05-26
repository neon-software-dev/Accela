#ifndef LIBACCELARENDERER_INCLUDE_ACCELA_RENDERER_TASK_RENDERTASK_H
#define LIBACCELARENDERER_INCLUDE_ACCELA_RENDERER_TASK_RENDERTASK_H

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
        DestroyTexture,         // Destroy a texture
        CreateMesh,             // Register a mesh
        DestroyMesh,            // Destroy a mesh
        CreateMaterial,         // Register a material
        DestroyMaterial,        // Destroy a material
        CreateFrameBuffer,      // Create a framebuffer
        DestroyFrameBuffer,     // Destroy a framebuffer
        WorldUpdate,            // Update the state of the world
        SurfaceChanged,         // Handle window/surface change
        ChangeRenderSettings    // Apply new render settings
    };

    /**
     * Represents a message/task that can be sent to the Renderer
     */
    struct RenderTask
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
