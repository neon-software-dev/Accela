/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Accela/Engine/Entity/EnginePerfMonitorEntity.h>

#include <format>

namespace Accela::Engine
{

static constexpr uint8_t Font_Size = 28;

EnginePerfMonitorEntity::UPtr EnginePerfMonitorEntity::Create(
    IEngineRuntime::Ptr engine,
    SceneEvents::Ptr sceneEvents,
    std::string fontName,
    std::string sceneName,
    const glm::vec3& position,
    uint32_t refreshInterval)
{
    auto perfEntity = std::make_unique<EnginePerfMonitorEntity>(
        ConstructTag{},
        std::move(engine),
        std::move(sceneEvents),
        std::move(fontName),
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
    std::string fontName,
    std::string sceneName,
    const glm::vec3& position,
    uint32_t refreshInterval)
    : SceneEntity(std::move(engine), std::move(sceneName), std::move(sceneEvents))
    , m_fontName(std::move(fontName))
    , m_position(position)
    , m_refreshInterval(refreshInterval)
{
    if (!m_engine->GetWorldResources()->IsFontLoaded(m_fontName, Font_Size))
    {
        (void)m_engine->GetWorldResources()->LoadFontBlocking(m_fontName, Font_Size);
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
        m_fontName,
        Font_Size,
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

    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Texture Count: ",
        "Renderer_Textures_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Texture Loading Count: ",
        "Renderer_Textures_Loading_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Texture ToDestroy Count: ",
        "Renderer_Textures_ToDestroy_Count",
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
        "Renderer: Meshes ByteSize: ",
        "Renderer_Meshes_ByteSize",
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
        "Renderer: Objects Rendered: ",
        "Renderer_Object_Objects_Rendered_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Render Batch Count: ",
        "Renderer_Object_RenderBatch_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Renderer: Draw Calls Count: ",
        "Renderer_Object_DrawCalls_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Physics: Physics Rigid Body Count: ",
        "Engine_Physics_Rigid_Bodies_Count",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Double,
        "Player Distance Above Ground: ",
        "PLAYER_ABOVE_GROUND",
        textProperties,
        currentYPos
    );
    currentYPos += CreateEntity(
        Common::MetricType::Counter,
        "Player Location State: ",
        "PLAYER_STATE",
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

    const auto renderedTextHeight = textEntity->GetRenderedTextSize()->h;

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
                metricEntity.entity->SetText(metricEntity.description + std::format("{}", *metricValue));
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
