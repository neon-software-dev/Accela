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

bool RendererBase::Startup(const RenderSettings& renderSettings, const std::vector<ShaderSpec>& shaders)
{
    m_logger->Log(Common::LogLevel::Info, "RendererBase: Starting");

    m_thread = std::make_unique<Common::MessageDrivenThreadPool>(
        "Renderer",
        1,
        [this](const Common::Message::Ptr& message){
            OnTaskMessageReceived(std::static_pointer_cast<RenderTaskMessage>(message));
        },
        [this](){
            OnIdle();
        }
    );

    if (!Submit<RenderTask_Initialize>(renderSettings, shaders).get())
    {
        Shutdown();
        return false;
    }

    return true;
}

void RendererBase::Shutdown()
{
    m_logger->Log(Common::LogLevel::Info, "RendererBase: Shutting down");

    // Message the renderer to tell it to stop rendering and clean up resources
    Submit<RenderTask_Shutdown>().get();

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
                                              const TextureSampler& textureSampler,
                                              bool generateMipMaps)
{
    return Submit<RenderTask_CreateTexture>(texture, textureView, textureSampler, generateMipMaps);
}

std::future<bool> RendererBase::DestroyTexture(TextureId textureId)
{
    return Submit<RenderTask_DestroyTexture>(textureId);
}

std::future<bool> RendererBase::CreateMesh(const Mesh::Ptr& mesh, MeshUsage usage)
{
    return Submit<RenderTask_CreateMesh>(mesh, usage);
}

std::future<bool> RendererBase::DestroyMesh(MeshId meshId)
{
    return Submit<RenderTask_DestroyMesh>(meshId);
}

std::future<bool> RendererBase::CreateMaterial(const Material::Ptr& material)
{
    return Submit<RenderTask_CreateMaterial>(material);
}

std::future<bool> RendererBase::DestroyMaterial(MaterialId materialId)
{
    return Submit<RenderTask_DestroyMaterial>(materialId);
}

std::future<bool> RendererBase::CreateFrameBuffer(FrameBufferId frameBufferId, const std::vector<TextureId>& attachmentTextures)
{
    return Submit<RenderTask_CreateFrameBuffer>(frameBufferId, attachmentTextures);
}

std::future<bool> RendererBase::DestroyFrameBuffer(FrameBufferId frameBufferId)
{
    return Submit<RenderTask_DestroyFrameBuffer>(frameBufferId);
}

std::future<bool> RendererBase::UpdateWorld(const WorldUpdate& update)
{
    return Submit<RenderTask_WorldUpdate>(update);
}

std::future<bool> RendererBase::RenderFrame(const RenderGraph::Ptr& renderGraph)
{
    return Submit<RenderTask_RenderFrame>(renderGraph);
}

std::future<bool> RendererBase::SurfaceChanged()
{
    return Submit<RenderTask_SurfaceChanged>();
}

std::future<bool> RendererBase::ChangeRenderSettings(const RenderSettings& renderSettings)
{
    return Submit<RenderTask_ChangeRenderSettings>(renderSettings);
}

// Fulfills a render task message's promise by setting the message's result to the value returned
// by applying the data arguments within the message to the provided func, as arguments.
template <typename TaskType, typename Func>
void FulfillDirect(const RenderTaskMessage::Ptr& msg, Func func)
{
    msg->SetResult(
        std::apply(func, std::static_pointer_cast<TaskType>(msg->GetTask())->data)
    );
}

// Similar to FulfillDirect, but does not set the message's result directly and instead
// relies on passing the message's promise to the func, where the func itself is responsible
// for eventually manually setting the promise's result.
template <typename TaskType, typename Func>
void FulfillManual(const RenderTaskMessage::Ptr& msg, Func func)
{
    std::apply(func, std::tuple_cat(
        std::make_tuple(msg->StealPromise()),
        std::static_pointer_cast<TaskType>(msg->GetTask())->data)
    );
}

void RendererBase::OnTaskMessageReceived(const RenderTaskMessage::Ptr& msg)
{
    const auto taskType = msg->GetTask()->GetTaskType();

    switch (taskType)
    {
        case RenderTaskType::Initialize:
            FulfillDirect<RenderTask_Initialize>(msg, std::bind_front(&RendererBase::OnInitialize, this));
        break;
        case RenderTaskType::Shutdown:
            FulfillDirect<RenderTask_Shutdown>(msg, std::bind_front(&RendererBase::OnShutdown, this));
        break;
        case RenderTaskType::RenderFrame:
            FulfillDirect<RenderTask_RenderFrame>(msg, std::bind_front(&RendererBase::OnRenderFrame, this));
        break;
        case RenderTaskType::CreateTexture:
            FulfillManual<RenderTask_CreateTexture>(msg, std::bind_front(&RendererBase::OnCreateTexture, this));
        break;
        case RenderTaskType::DestroyTexture:
            FulfillDirect<RenderTask_DestroyTexture>(msg, std::bind_front(&RendererBase::OnDestroyTexture, this));
        break;
        case RenderTaskType::CreateMesh:
            FulfillManual<RenderTask_CreateMesh>(msg, std::bind_front(&RendererBase::OnCreateMesh, this));
        break;
        case RenderTaskType::DestroyMesh:
            FulfillDirect<RenderTask_DestroyMesh>(msg, std::bind_front(&RendererBase::OnDestroyMesh, this));
        break;
        case RenderTaskType::CreateMaterial:
            FulfillDirect<RenderTask_CreateMaterial>(msg, std::bind_front(&RendererBase::OnCreateMaterial, this));
        break;
        case RenderTaskType::DestroyMaterial:
            FulfillDirect<RenderTask_DestroyMaterial>(msg, std::bind_front(&RendererBase::OnDestroyMaterial, this));
        break;
        case RenderTaskType::CreateFrameBuffer:
            FulfillDirect<RenderTask_CreateFrameBuffer>(msg, std::bind_front(&RendererBase::OnCreateFrameBuffer, this));
        break;
        case RenderTaskType::DestroyFrameBuffer:
            FulfillDirect<RenderTask_DestroyFrameBuffer>(msg, std::bind_front(&RendererBase::OnDestroyFrameBuffer, this));
        break;
        case RenderTaskType::WorldUpdate:
            FulfillDirect<RenderTask_WorldUpdate>(msg, std::bind_front(&RendererBase::OnWorldUpdate, this));
        break;
        case RenderTaskType::SurfaceChanged:
            FulfillDirect<RenderTask_SurfaceChanged>(msg, std::bind_front(&RendererBase::OnSurfaceChanged, this));
        break;
        case RenderTaskType::ChangeRenderSettings:
            FulfillDirect<RenderTask_ChangeRenderSettings>(msg, std::bind_front(&RendererBase::OnChangeRenderSettings, this));
        break;
    }
}

}
