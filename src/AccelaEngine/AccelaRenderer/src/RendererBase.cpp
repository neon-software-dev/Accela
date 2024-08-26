/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Render/RendererBase.h>
#include "Task/RenderTasks.h"

namespace Accela::Render
{

RendererBase::RendererBase(Common::ILogger::Ptr logger, Common::IMetrics::Ptr metrics)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_ids(std::make_shared<Ids>())
{

}

bool RendererBase::Startup(const RenderInit& renderInit, const RenderSettings& renderSettings)
{
    m_logger->Log(Common::LogLevel::Info, "RendererBase: Starting");

    m_thread = std::make_unique<Common::MessageDrivenThreadPool>(
        "Renderer",
        1,
        [this](const Common::Message::Ptr& message)
        {
            if (message->GetTypeIdentifier() == RenderTaskMessage<bool>::TYPE)
            {
                OnTaskMessageReceived(message);
            }
        },
        [this]()
        {
            OnIdle();
        }
    );

    if (!Submit<RenderTask_Initialize, bool>(false, renderInit, renderSettings).get())
    {
        Shutdown();
        return false;
    }

    return true;
}

void RendererBase::Shutdown()
{
    m_logger->Log(Common::LogLevel::Info, "RendererBase: Shutting down");

    // Message the renderer to tell it to stop rendering and clean up resources, wait for the result
    Submit<RenderTask_Shutdown, bool>(false).get();

    // Stop the render thread. (Note that destructing the thread object stops and joins the thread)
    m_thread = nullptr;

    // Release all ids that were previously allocated for rendering
    m_ids->Reset();
}

Ids::Ptr RendererBase::GetIds() const
{
    return m_ids;
}

std::future<bool> RendererBase::CreateTexture(const Texture& texture,
                                              const TextureView& textureView,
                                              const TextureSampler& textureSampler)
{
    return Submit<RenderTask_CreateTexture, bool>(false, texture, textureView, textureSampler);
}

std::future<bool> RendererBase::DestroyTexture(TextureId textureId)
{
    return Submit<RenderTask_DestroyTexture, bool>(false, textureId);
}

std::future<bool> RendererBase::CreateMesh(const Mesh::Ptr& mesh, MeshUsage usage)
{
    return Submit<RenderTask_CreateMesh, bool>(false, mesh, usage);
}

std::future<bool> RendererBase::DestroyMesh(MeshId meshId)
{
    return Submit<RenderTask_DestroyMesh, bool>(false, meshId);
}

std::future<bool> RendererBase::CreateMaterial(const Material::Ptr& material)
{
    return Submit<RenderTask_CreateMaterial, bool>(false, material);
}

std::future<bool> RendererBase::DestroyMaterial(MaterialId materialId)
{
    return Submit<RenderTask_DestroyMaterial, bool>(false, materialId);
}

std::future<bool> RendererBase::CreateRenderTarget(RenderTargetId renderTargetId, const std::string& tag)
{
    return Submit<RenderTask_CreateRenderTarget, bool>(false, renderTargetId, tag);
}

std::future<bool> RendererBase::DestroyRenderTarget(RenderTargetId renderTargetId)
{
    return Submit<RenderTask_DestroyRenderTarget, bool>(false, renderTargetId);
}

std::future<bool> RendererBase::UpdateWorld(const WorldUpdate& update)
{
    return Submit<RenderTask_WorldUpdate, bool>(false, update);
}

std::future<bool> RendererBase::RenderFrame(const RenderGraph::Ptr& renderGraph)
{
    return Submit<RenderTask_RenderFrame, bool>(false, renderGraph);
}

std::future<bool> RendererBase::SurfaceChanged()
{
    return Submit<RenderTask_SurfaceChanged, bool>(false);
}

std::future<bool> RendererBase::ChangeRenderSettings(const RenderSettings& renderSettings)
{
    return Submit<RenderTask_ChangeRenderSettings, bool>(false, renderSettings);
}

// Fulfills a render task message's promise by setting the message's result to the value returned
// by applying the data arguments within the message to the provided func, as arguments.
template <typename TaskType, typename Ret, typename Func>
void FulfillDirect(const Common::Message::Ptr& msg, Func func)
{
    const auto specificRenderTaskMessage = std::dynamic_pointer_cast<RenderTaskMessage<Ret>>(msg);

    specificRenderTaskMessage->SetResult(
        std::apply(func, std::static_pointer_cast<TaskType>(specificRenderTaskMessage->GetTask())->data)
    );
}

// Similar to FulfillDirect, but does not set the message's result directly and instead
// relies on passing the message's promise to the func, where the func itself is responsible
// for eventually manually setting the promise's result.
template <typename TaskType, typename Ret, typename Func>
void FulfillManual(const Common::Message::Ptr& msg, Func func)
{
    const auto specificRenderTaskMessage = std::dynamic_pointer_cast<RenderTaskMessage<Ret>>(msg);

    std::apply(func, std::tuple_cat(
        std::make_tuple(specificRenderTaskMessage->StealPromise()),
        std::static_pointer_cast<TaskType>(specificRenderTaskMessage->GetTask())->data)
    );
}

void RendererBase::OnTaskMessageReceived(const Common::Message::Ptr& msg)
{
    const auto taskType = std::dynamic_pointer_cast<RenderTaskMessageBase>(msg)->GetTask()->GetTaskType();

    switch (taskType)
    {
        case RenderTaskType::Initialize:
            FulfillDirect<RenderTask_Initialize, bool>(msg, std::bind_front(&RendererBase::OnInitialize, this));
        break;
        case RenderTaskType::Shutdown:
            FulfillDirect<RenderTask_Shutdown, bool>(msg, std::bind_front(&RendererBase::OnShutdown, this));
        break;
        case RenderTaskType::RenderFrame:
            FulfillDirect<RenderTask_RenderFrame, bool>(msg, std::bind_front(&RendererBase::OnRenderFrame, this));
        break;
        case RenderTaskType::CreateTexture:
            FulfillManual<RenderTask_CreateTexture, bool>(msg, std::bind_front(&RendererBase::OnCreateTexture, this));
        break;
        case RenderTaskType::DestroyTexture:
            FulfillDirect<RenderTask_DestroyTexture, bool>(msg, std::bind_front(&RendererBase::OnDestroyTexture, this));
        break;
        case RenderTaskType::CreateMesh:
            FulfillManual<RenderTask_CreateMesh, bool>(msg, std::bind_front(&RendererBase::OnCreateMesh, this));
        break;
        case RenderTaskType::DestroyMesh:
            FulfillDirect<RenderTask_DestroyMesh, bool>(msg, std::bind_front(&RendererBase::OnDestroyMesh, this));
        break;
        case RenderTaskType::CreateMaterial:
            FulfillManual<RenderTask_CreateMaterial, bool>(msg, std::bind_front(&RendererBase::OnCreateMaterial, this));
        break;
        case RenderTaskType::DestroyMaterial:
            FulfillDirect<RenderTask_DestroyMaterial, bool>(msg, std::bind_front(&RendererBase::OnDestroyMaterial, this));
        break;
        case RenderTaskType::CreateRenderTarget:
            FulfillDirect<RenderTask_CreateRenderTarget, bool>(msg, std::bind_front(&RendererBase::OnCreateRenderTarget, this));
        break;
        case RenderTaskType::DestroyRenderTarget:
            FulfillDirect<RenderTask_DestroyRenderTarget, bool>(msg, std::bind_front(&RendererBase::OnDestroyRenderTarget, this));
        break;
        case RenderTaskType::WorldUpdate:
            FulfillDirect<RenderTask_WorldUpdate, bool>(msg, std::bind_front(&RendererBase::OnWorldUpdate, this));
        break;
        case RenderTaskType::SurfaceChanged:
            FulfillDirect<RenderTask_SurfaceChanged, bool>(msg, std::bind_front(&RendererBase::OnSurfaceChanged, this));
        break;
        case RenderTaskType::ChangeRenderSettings:
            FulfillDirect<RenderTask_ChangeRenderSettings, bool>(msg, std::bind_front(&RendererBase::OnChangeRenderSettings, this));
        break;
    }
}

}
