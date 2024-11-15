/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Entity/EnginePerfMonitorEntity.h>

#include <format>

namespace Accela::Engine
{

EnginePerfMonitorEntity::UPtr EnginePerfMonitorEntity::Create(
    IEngineRuntime::Ptr engine,
    SceneEvents::Ptr sceneEvents,
    PackageResourceIdentifier fontResource,
    std::uint8_t fontSize,
    std::string sceneName,
    const glm::vec3& position,
    uint32_t refreshInterval)
{
    auto perfEntity = std::make_unique<EnginePerfMonitorEntity>(
        ConstructTag{},
        std::move(engine),
        std::move(sceneEvents),
        std::move(fontResource),
        std::move(fontSize),
        std::move(sceneName),
        position,
        refreshInterval
    );

    return perfEntity;
}

EnginePerfMonitorEntity::EnginePerfMonitorEntity(
    ConstructTag,
    IEngineRuntime::Ptr engine,
    SceneEvents::Ptr sceneEvents,
    PackageResourceIdentifier fontResource,
    std::uint8_t fontSize,
    std::string sceneName,
    const glm::vec3& position,
    uint32_t refreshInterval)
    : SceneEntity(std::move(engine), std::move(sceneName), std::move(sceneEvents))
    , m_fontResource(std::move(fontResource))
    , m_fontSize(fontSize)
    , m_position(position)
    , m_refreshInterval(refreshInterval)
{
    if (!m_engine->GetWorldResources()->Fonts()->IsFontLoaded(m_fontResource, m_fontSize))
    {
        m_engine->GetWorldResources()->Fonts()->LoadFont(m_fontResource, m_fontSize).get();
    }

    CreateEntities();
}

EnginePerfMonitorEntity::~EnginePerfMonitorEntity()
{
    DestroyInternal();
}

void EnginePerfMonitorEntity::CreateEntities()
{
    const auto textProperties = Platform::TextProperties{
        m_fontResource.GetResourceName(),
        m_fontSize,
        0,
        Platform::Color::Red(),
        Platform::Color(255,255,255,50)
    };

    uint32_t currentYPos = 0;

    currentYPos += CreateEntity(
        Common::MetricType::Double,
        "Engine: Simulation Step Time: ",
        "Engine_SimulationStep_Time",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Double,
        "Engine: Scene Simulation Step Time: ",
        "Engine_SceneSimulationStep_Time",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Double,
        "Engine: Renderer Sync System Time: ",
        "Engine_RendererSyncSystem_Time",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Double,
        "Engine: Physics Sync System Time: ",
        "Engine_PhysicsSyncSystem_Time",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Double,
        "Renderer: Frame Render Total Time: ",
        "Renderer_FrameRenderTotal_Time",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Double,
        "Renderer: Frame Render Work Time: ",
        "Renderer_FrameRenderWork_Time",
        textProperties,
        currentYPos
    );
    /*currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Texture Count: ",
        "Renderer_Textures_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Images Count: ",
        "Renderer_Images_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Images Loading Count: ",
        "Renderer_Images_Loading_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Images ToDestroy Count: ",
        "Renderer_Images_ToDestroy_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Meshes Count: ",
        "Renderer_Meshes_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Meshes Loading Count: ",
        "Renderer_Meshes_Loading_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Meshes ToDestroy Count: ",
        "Renderer_Meshes_ToDestroy_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Meshes ByteSize: ",
        "Renderer_Meshes_ByteSize",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Materials Count: ",
        "Renderer_Materials_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Materials Loading Count: ",
        "Renderer_Materials_Loading_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Materials ToDestroy Count: ",
        "Renderer_Materials_ToDestroy_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Materials ByteSize: ",
        "Renderer_Materials_ByteSize",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Buffers Count: ",
        "Renderer_Buffers_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Buffers ByteSize: ",
        "Renderer_Buffers_ByteSize",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Lights Count: ",
        "Renderer_Scene_Lights_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Shadow Map Count: ",
        "Renderer_Scene_Shadow_Map_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Opaque Objects Rendered: ",
        "Renderer_Object_Opaque_Objects_Rendered_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Opaque Render Batch Count: ",
        "Renderer_Object_Opaque_RenderBatch_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Opaque Draw Calls Count: ",
        "Renderer_Object_Opaque_DrawCalls_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Transparent Draw Calls Count: ",
        "Renderer_Object_Transparent_DrawCalls_Count",
        textProperties,
        currentYPos
    );*/
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Total Memory Usage: ",
        "Renderer_Memory_Usage",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Total Memory Available: ",
        "Renderer_Memory_Available",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Physics: Scene Count: ",
        "Engine_Physics_Scene_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Physics: Static Rigid Body Count: ",
        "Engine_Physics_Static_Rigid_Bodies_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Physics: Dynamic Rigid Body Count: ",
        "Engine_Physics_Dynamic_Rigid_Bodies_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "VIDEO_PACKET_QUEUE_COUNT: ",
        "VIDEO_PACKET_QUEUE_COUNT",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "AUDIO_PACKET_QUEUE_COUNT: ",
        "AUDIO_PACKET_QUEUE_COUNT",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "SUBTITLE_PACKET_QUEUE_COUNT: ",
        "SUBTITLE_PACKET_QUEUE_COUNT",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "VIDEO_QUEUE_COUNT: ",
        "DECODER_QUEUE_COUNT_VideoDecoder",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "AUDIO_QUEUE_COUNT: ",
        "DECODER_QUEUE_COUNT_AudioDecoder",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "DROPPED_FRAME_COUNT: ",
        "DROPPED_FRAME_COUNT",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Double,
        "AUDIO_SYNC_DIFF: ",
        "AUDIO_SYNC_DIFF",
        textProperties,
        currentYPos
    );

    (void)currentYPos;
}

uint32_t EnginePerfMonitorEntity::CreateEntity(
    Common::MetricType metricType,
    const std::string& description,
    const std::string& metricName,
    const Platform::TextProperties& textProperties,
    const uint32_t& yOffset)
{
    auto textEntity = ScreenTextEntity::Create(m_engine, {}, m_sceneName);
    textEntity->SetText(description);
    textEntity->SetPosition(m_position + glm::vec3(0,yOffset,0));
    textEntity->SetTextProperties(textProperties);

    const auto renderedTextHeight = m_engine->GetWorldState()->RenderSizeToVirtualSize(*textEntity->GetRenderedTextSize()).h;

    m_entities.emplace_back(metricType, metricName, description, std::move(textEntity));

    return renderedTextHeight;
}

void EnginePerfMonitorEntity::OnSimulationStep(unsigned int)
{
    m_stepCounter++;
    if (m_stepCounter != m_refreshInterval) { return; }
    m_stepCounter = 0;

    for (auto& metricEntity : m_entities)
    {
        switch (metricEntity.metricType)
        {
            case Common::MetricType::Counter:
            {
                const auto metricValue = m_engine->GetMetrics()->GetCounterValue(metricEntity.metricName);
                if (!metricValue) { continue; }
                metricEntity.entity->SetText(metricEntity.description + std::format("{}", *metricValue));
            }
            break;
            case Common::MetricType::Double:
            {
                const auto metricValue = m_engine->GetMetrics()->GetDoubleValue(metricEntity.metricName);
                if (!metricValue) { continue; }
                metricEntity.entity->SetText(metricEntity.description + std::format("{:.3f}", *metricValue));
            }
            break;
        }
    }
}

void EnginePerfMonitorEntity::Destroy()
{
    DestroyInternal();
}

void EnginePerfMonitorEntity::DestroyInternal()
{
    for (auto& entity : m_entities)
    {
        entity.entity->Destroy();
    }
    m_entities.clear();

    m_stepCounter = 0;
}

}
