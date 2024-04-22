/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Lights.h"

#include "../VulkanObjs.h"

#include "../Framebuffer/IFramebuffers.h"
#include "../Renderer/RendererCommon.h"

#include <format>
#include <algorithm>
#include <cassert>

namespace Accela::Render
{

static const Render::USize Shadow_Low_Quality_Size{1024, 1024};
static const Render::USize Shadow_Medium_Quality_Size{2048, 2048};
static const Render::USize Shadow_High_Quality_Size{4096, 4096};

Lights::Lights(Common::ILogger::Ptr logger,
               Common::IMetrics::Ptr metrics,
               VulkanObjsPtr vulkanObjs,
               IFramebuffersPtr framebuffers,
               Ids::Ptr ids)
    : m_logger(std::move(logger))
    , m_metrics(std::move(metrics))
    , m_vulkanObjs(std::move(vulkanObjs))
    , m_framebuffers(std::move(framebuffers))
    , m_ids(std::move(ids))
{

}

void Lights::Destroy()
{
    for (const auto& light: m_lights)
    {
        if (light.second.shadowFrameBufferId)
        {
            m_framebuffers->DestroyFramebuffer(*light.second.shadowFrameBufferId, true);
        }
    }

    m_lights.clear();
}

std::vector<LoadedLight> Lights::GetAllLights() const
{
    std::vector<LoadedLight> result;

    for (const auto& lightIt : m_lights)
    {
        result.push_back(lightIt.second);
    }

    return result;
}

std::vector<LoadedLight> Lights::GetSceneLights(const std::string& sceneName, const std::vector<ViewProjection>& viewProjections) const
{
    std::vector<LoadedLight> result;

    for (const auto& lightIt : m_lights)
    {
        // If the light isn't for the requested scene, ignore it
        if (lightIt.second.light.sceneName != sceneName)
        {
            continue;
        }

        // If the light doesn't affect the view projection space, ignore it
        if (!LightAffectsViewProjections(lightIt.second.light, viewProjections))
        {
            continue;
        }

        result.push_back(lightIt.second);
    }

    return result;
}

std::optional<LoadedLight> Lights::GetLightById(const LightId& lightId) const
{
    const auto it = m_lights.find(lightId);
    if (it != m_lights.cend())
    {
        return it->second;
    }

    return std::nullopt;
}

void Lights::ProcessUpdate(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence)
{
    ProcessAddedLights(update, commandBuffer, vkFence);
    ProcessUpdatedLights(update, commandBuffer, vkFence);
    ProcessDeletedLights(update, commandBuffer, vkFence);
}

void Lights::ProcessAddedLights(const WorldUpdate& update, const VulkanCommandBufferPtr&, VkFence)
{
    const auto renderSettings = m_vulkanObjs->GetRenderSettings();

    for (const auto& light: update.toAddLights)
    {
        if (m_lights.contains(light.lightId))
        {
            m_logger->Log(Common::LogLevel::Error,
              "Lights::ProcessAddedLights: Light already exists, ignoring, id: ", light.lightId.id);
            continue;
        }

        std::optional<FrameBufferId> shadowFramebufferId;

        // If this light casts shadows, create a framebuffer for its shadow map
        if (light.castsShadows)
        {
            const auto framebufferExpect = CreateShadowFramebuffer(light, renderSettings);
            if (!framebufferExpect)
            {
                m_logger->Log(Common::LogLevel::Error,
                  "Lights::ProcessAddedLights: Failed to create shadow framebuffer for light, id: ", light.lightId.id);
            }
            else
            {
                shadowFramebufferId = *framebufferExpect;
            }
        }

        m_lights.insert({light.lightId, LoadedLight(light, shadowFramebufferId)});
    }
}

void Lights::ProcessUpdatedLights(const WorldUpdate& update, const VulkanCommandBufferPtr&, VkFence)
{
    for (const auto& light : update.toUpdateLights)
    {
        const auto it = m_lights.find(light.lightId);
        if (it != m_lights.cend())
        {
            it->second.light = light;
            // TODO Perf: Only invalidate if light properties actually changed
            // TODO Perf: Only invalidate if something affecting shadow changed
            it->second.shadowInvalidated = true;
        }
        else
        {
            m_logger->Log(Common::LogLevel::Error,
                "Lights::ProcessUpdatedLights: Light doesn't exist, ignoring, id: ", light.lightId.id);
        }
    }
}

void Lights::ProcessDeletedLights(const WorldUpdate& update, const VulkanCommandBufferPtr&, VkFence)
{
    for (const auto& lightId : update.toDeleteLightIds)
    {
        const auto it = m_lights.find(lightId);
        if (it == m_lights.cend())
        {
            m_logger->Log(Common::LogLevel::Error,
              "Lights::ProcessDeletedLights: Light doesn't exist, ignoring, id: ", lightId.id);
            continue;
        }

        if (it->second.shadowFrameBufferId)
        {
            m_framebuffers->DestroyFramebuffer(*it->second.shadowFrameBufferId, false);
        }

        m_lights.erase(lightId);
    }
}

// TODO: Only invalidate/recreate if a setting affecting shadows changed
bool Lights::OnRenderSettingsChanged(const RenderSettings& renderSettings)
{
    bool allSuccessful = true;

    for (auto& lightIt : m_lights)
    {
        if (lightIt.second.light.castsShadows)
        {
            if (!RecreateShadowFramebuffer(lightIt.second, renderSettings))
            {
                allSuccessful = false;
            }
        }
    }

    return allSuccessful;
}

std::vector<uint8_t> GetCubeFacesAffectedByLightCone(const Light& light)
{
    // TODO Perf: Implement
    (void)light;
    return std::vector<uint8_t>{0,1,2,3,4,5};
}

void Lights::InvalidateShadowMapsByBounds(const std::vector<AABB>& boundingBoxes_worldSpace)
{
    for (auto& lightIt : m_lights)
    {
        // If the light doesn't cast shadows, then there's no shadow map to be invalidated, ignore it
        if (!lightIt.second.light.castsShadows) { continue; }

        // Check if any of the updated bounding boxes fall within any of the light's shadow maps.
        // If so, invalidate the affected shadow maps.
        for (const auto& boundingBox : boundingBoxes_worldSpace)
        {
            // Ignore bad/empty bounding boxes
            if (boundingBox.IsEmpty()) { continue; }

            // Get the list of shadow map cube faces that the light's cone touches. We only need to invalidate
            // shadow maps that the light can possibly affect.
            const auto litCubeFaces = GetCubeFacesAffectedByLightCone(lightIt.second.light);

            // For each shadow map cube face, invalidate it if the bounding box isn't trivially outside the
            // light's view projection for that face
            for (const auto& cubeFace : litCubeFaces)
            {
                const auto shadowMapViewProjection = GetShadowMapViewProjection(
                    lightIt.second.light,
                    static_cast<CubeFace>(cubeFace)
                );

                if (!shadowMapViewProjection)
                {
                    m_logger->Log(Common::LogLevel::Error,
                      "Lights::InvalidateShadowMapsByBounds: Failed to generate shadow map view projection for light: {}",
                      lightIt.first.id);
                    continue;
                }

                const bool triviallyOutside = VolumeTriviallyOutsideProjection(
                    boundingBox.GetVolume(),
                    shadowMapViewProjection->GetTransformation()
                );

                if (!triviallyOutside)
                {
                    lightIt.second.shadowInvalidated = true;
                    break;
                }
            }
        }
    }
}

void Lights::OnShadowMapSynced(const LightId& lightId)
{
    const auto it = m_lights.find(lightId);
    if (it == m_lights.cend())
    {
        m_logger->Log(Common::LogLevel::Error,
          "Lights::OnShadowMapSynced: Light doesn't exist, ignoring, id: ", lightId.id);
        return;
    }

    it->second.shadowInvalidated = false;
}

std::expected<FrameBufferId, bool> Lights::CreateShadowFramebuffer(const Light& light, const RenderSettings& renderSettings) const
{
    const auto framebufferId = m_ids->frameBufferIds.GetId();
    const auto shadowFramebufferSize = GetShadowFramebufferSize(renderSettings);
    const auto tag = std::format("Shadow-{}", light.lightId.id);

    std::vector<std::pair<TextureDefinition, std::string>> attachments;

    // Depth Attachment
    const auto texture = Texture::Empty(INVALID_ID, TextureUsage::DepthCubeAttachment, shadowFramebufferSize, 6, std::format("DepthCube-{}", tag));
    const auto textureView = TextureView::ViewAsCube(TextureView::DEFAULT, TextureView::Aspect::ASPECT_DEPTH_BIT);
    const auto textureSampler = TextureSampler(CLAMP_ADDRESS_MODE);

    attachments.emplace_back(
        TextureDefinition(texture, {textureView}, textureSampler),
        TextureView::DEFAULT
    );

    if (!m_framebuffers->CreateFramebuffer(
        framebufferId,
        m_vulkanObjs->GetShadowRenderPass(),
        attachments,
        shadowFramebufferSize,
        1,
        tag))
    {
        m_ids->frameBufferIds.ReturnId(framebufferId);
        return std::unexpected(false);
    }

    return framebufferId;
}

bool Lights::RecreateShadowFramebuffer(LoadedLight& loadedLight, const RenderSettings& renderSettings) const
{
    // Destroy any existing framebuffer
    if (loadedLight.shadowFrameBufferId)
    {
        m_framebuffers->DestroyFramebuffer(*loadedLight.shadowFrameBufferId, false);
        loadedLight.shadowFrameBufferId = std::nullopt;
    }

    // Create a new framebuffer
    const auto framebufferExpect = CreateShadowFramebuffer(loadedLight.light, renderSettings);
    if (!framebufferExpect)
    {
        m_logger->Log(Common::LogLevel::Error,
          "Lights::RecreateShadowFramebuffer: Failed to create light shadow framebuffer, id: ", loadedLight.light.lightId.id);
        return false;
    }

    loadedLight.shadowFrameBufferId = *framebufferExpect;
    loadedLight.shadowInvalidated = true;

    return true;
}

Render::USize Lights::GetShadowFramebufferSize(const RenderSettings& renderSettings)
{
    switch (renderSettings.shadowQuality)
    {
        case QualityLevel::Low: return Shadow_Low_Quality_Size;
        case QualityLevel::Medium: return Shadow_Medium_Quality_Size;
        case QualityLevel::High: return Shadow_High_Quality_Size;
    }

    assert(false);
    return Shadow_Low_Quality_Size;
}

bool Lights::LightAffectsViewProjections(const Light& light, const std::vector<ViewProjection>& viewProjections)
{
    const Sphere lightSphere(light.worldPos, GetLightMaxAffectRange(light));

    return std::ranges::any_of(viewProjections, [&](const auto& viewProjection){
        const auto boundingVolume = viewProjection.GetWorldSpaceAABB().GetVolume();

        return Intersects(lightSphere, boundingVolume);
    });
}

}
