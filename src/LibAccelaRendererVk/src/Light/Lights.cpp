/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Lights.h"

#include "../VulkanObjs.h"

#include "../Framebuffer/IFramebuffers.h"
#include "../Renderer/RendererCommon.h"

#include "../Vulkan/VulkanPhysicalDevice.h"

#include <format>
#include <algorithm>
#include <cassert>

namespace Accela::Render
{

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
        if (!LightAffectsViewProjections(lightIt.second, viewProjections))
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
                return;
            }
            else
            {
                shadowFramebufferId = *framebufferExpect;
            }
        }

        auto loadedLight = LoadedLight(light, shadowFramebufferId);

        const auto shadowRenders = DetermineLightShadowRenders(loadedLight, RenderCamera{});
        if (!shadowRenders)
        {
            m_logger->Log(Common::LogLevel::Error,
              "Lights::ProcessAddedLights: Failed to determine shadow renders for light, id: ", light.lightId.id);
            return;
        }

        loadedLight.shadowRenders = *shadowRenders;

        m_lights.insert({light.lightId, loadedLight});
    }
}

std::expected<std::vector<ShadowRender>, bool> Lights::DetermineLightShadowRenders(const LoadedLight& loadedLight, const RenderCamera& renderCamera)
{
    std::vector<ShadowRender> shadowRenders;

    if (!loadedLight.light.castsShadows)
    {
        return shadowRenders;
    }

    switch (loadedLight.shadowMapType)
    {
        case ShadowMapType::Cascaded:
        {
            const auto directionalShadowRenders = *GetDirectionalShadowMapViewProjections(
                m_vulkanObjs->GetRenderSettings(),
                m_vulkanObjs->GetContext(),
                loadedLight,
                renderCamera
            );

            for (const auto& directionalShadowRender : directionalShadowRenders)
            {
                shadowRenders.push_back(ShadowRender{
                    .worldPos = directionalShadowRender.render_worldPosition,
                    .viewProjection = directionalShadowRender.viewProjection,
                    .cascadeIndex = shadowRenders.size(),
                    .cut = directionalShadowRender.cut.AsVec2()
                });
            }
        }
        break;
        case ShadowMapType::Cube:
        {
            for (unsigned int cubeFaceIndex = 0; cubeFaceIndex < 6; ++cubeFaceIndex)
            {
                const auto viewProjection = GetPointShadowMapViewProjectionFaced(m_vulkanObjs->GetRenderSettings(), loadedLight, static_cast<CubeFace>(cubeFaceIndex));
                if (!viewProjection)
                {
                    m_logger->Log(Common::LogLevel::Error, "Lights::DetermineLightShadowRenders: Failed to get point face shadow render");
                    return std::unexpected(false);
                }

                shadowRenders.push_back(ShadowRender{
                    .worldPos = loadedLight.light.worldPos,
                    .viewProjection = *viewProjection,
                    .cascadeIndex = std::nullopt,
                    .cut = std::nullopt
                });
            }
        }
        break;
    }

    return shadowRenders;
}

void Lights::ProcessUpdatedLights(const WorldUpdate& update, const VulkanCommandBufferPtr&, VkFence)
{
    for (const auto& light : update.toUpdateLights)
    {
        const auto it = m_lights.find(light.lightId);
        if (it != m_lights.cend())
        {
            const bool shadowMapTypeChanged = it->second.shadowMapType != GetShadowMapType(light);

            it->second.light = light;
            it->second.shadowMapType = GetShadowMapType(light);

            // TODO Perf: Only invalidate if light properties actually changed
            // TODO Perf: Only invalidate if something affecting shadow changed
            it->second.shadowInvalidated = true;

            // If the light's shadow map type changed recreate its framebuffer for the new type
            if (shadowMapTypeChanged)
            {
                if (!RecreateShadowFramebuffer(it->second, m_vulkanObjs->GetRenderSettings()))
                {
                    m_logger->Log(Common::LogLevel::Error,
                        "Lights::ProcessUpdatedLights: Failed to recreate light framebuffer");
                }
            }
        }
        else
        {
            m_logger->Log(Common::LogLevel::Warning,
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

// TODO: Only invalidate/recreate if a setting affecting shadows changed. (Note: Still
//  recreate if max render distance render setting changes)
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

            switch (lightIt.second.shadowMapType)
            {
                case ShadowMapType::Cascaded:
                    InvalidateShadowMapsByBounds_Cascaded(lightIt.second, boundingBox.GetVolume());
                break;
                case ShadowMapType::Cube:
                    InvalidateShadowMapsByBounds_Cube(lightIt.second, boundingBox.GetVolume());
                break;
            }
        }
    }
}

void Lights::InvalidateShadowMapsByBounds_Cascaded(LoadedLight& loadedLight, const Volume& volume_worldSpace)
{
    const bool volumeTriviallyOutsideAllShadowRenders = std::ranges::all_of(loadedLight.shadowRenders, [&](const auto& shadowRender){
        return VolumeTriviallyOutsideProjection(
            volume_worldSpace,
            shadowRender.viewProjection.GetTransformation()
        );
    });

    if (!volumeTriviallyOutsideAllShadowRenders)
    {
        loadedLight.shadowInvalidated = true;
    }
}

void Lights::InvalidateShadowMapsByBounds_Cube(LoadedLight& loadedLight, const Volume& volume_worldSpace)
{
    // Get the list of shadow map cube faces that the light's cone touches. We only need to evaluate
    // shadow renders that the light can possibly affect.
    const auto litCubeFaces = GetCubeFacesAffectedByLightCone(loadedLight.light);

    const bool volumeTriviallyOutsideAllLitFaces = std::ranges::all_of(litCubeFaces, [&](const auto& litCubeFace){
        return VolumeTriviallyOutsideProjection(
            volume_worldSpace,
            loadedLight.shadowRenders.at(litCubeFace).viewProjection.GetTransformation()
        );
    });

    if (!volumeTriviallyOutsideAllLitFaces)
    {
        loadedLight.shadowInvalidated = true;
    }
}

void Lights::UpdateShadowMapsForCamera(const RenderCamera& renderCamera)
{
    for (auto& lightIt : m_lights)
    {
        switch (lightIt.second.shadowMapType)
        {
            case ShadowMapType::Cascaded:
            {
                // If the camera hasn't changed since the last time the light's shadow renders were done, then we don't
                // need to invalidate them
                const bool cameraIsTheSame =
                    lightIt.second.shadowRenderCamera &&
                    (renderCamera == *lightIt.second.shadowRenderCamera);

                if (cameraIsTheSame)
                {
                    continue;
                }

                // Otherwise, as cascaded shadows depend on camera properties, update and invalidated the shadow renders
                const auto shadowRenders = DetermineLightShadowRenders(lightIt.second, renderCamera);
                if (!shadowRenders)
                {
                    m_logger->Log(Common::LogLevel::Error,
                      "Lights::UpdateShadowMapsForCamera: Failed to update shadow map for light: {}", lightIt.first.id);
                }

                lightIt.second.shadowRenders = *shadowRenders;
                lightIt.second.shadowRenderCamera = renderCamera;
                lightIt.second.shadowInvalidated = true;
            }
            break;
            case ShadowMapType::Cube:
            {
                // No-op - cubic shadow maps aren't affected by camera position
            }
            break;
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

    std::vector<std::pair<ImageDefinition, std::string>> attachments;

    Image image{};
    ImageView imageView{};

    ImageSampler imageSampler{
        .name = ImageSampler::DEFAULT(),
        .vkMagFilter = VK_FILTER_LINEAR,
        .vkMinFilter = VK_FILTER_LINEAR,
        .vkSamplerAddressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .vkSamplerAddressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .vkSamplerMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR
    };

    VulkanRenderPassPtr renderPass{};

    switch (GetShadowMapType(light))
    {
        case ShadowMapType::Cascaded:
        {
            const auto numLayers = Shadow_Cascade_Count;

            image = Image{
                .tag = std::format("ShadowCascaded-{}", tag),
                .vkImageType = VK_IMAGE_TYPE_2D,
                .vkFormat = m_vulkanObjs->GetPhysicalDevice()->GetDepthBufferFormat(),
                .vkImageTiling = VK_IMAGE_TILING_OPTIMAL,
                .vkImageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .size = shadowFramebufferSize,
                .numLayers = numLayers,
                .vmaAllocationCreateFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
            };

            imageView = ImageView{
                .name = ImageView::DEFAULT(),
                .vkImageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
                .vkImageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseLayer = 0,
                .layerCount = numLayers
            };

            renderPass = m_vulkanObjs->GetShadowCascadedRenderPass();
        }
        break;
        case ShadowMapType::Cube:
        {
            image = Image{
                .tag = std::format("ShadowCube-{}", tag),
                .vkImageType = VK_IMAGE_TYPE_2D,
                .vkFormat = m_vulkanObjs->GetPhysicalDevice()->GetDepthBufferFormat(),
                .vkImageTiling = VK_IMAGE_TILING_OPTIMAL,
                .vkImageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                .size = shadowFramebufferSize,
                .numLayers = 6,
                .cubeCompatible = true,
                .vmaAllocationCreateFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
            };

            imageView = ImageView{
                .name = ImageView::DEFAULT(),
                .vkImageViewType = VK_IMAGE_VIEW_TYPE_CUBE,
                .vkImageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
                .baseLayer = 0,
                .layerCount = 6
            };

            renderPass = m_vulkanObjs->GetShadowCubeRenderPass();
        }
        break;
    }

    attachments.emplace_back(
        ImageDefinition(image, {imageView}, {imageSampler}),
        ImageView::DEFAULT()
    );

    if (!m_framebuffers->CreateFramebuffer(
        framebufferId,
        renderPass,
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

bool Lights::LightAffectsViewProjections(const LoadedLight& loadedLight, const std::vector<ViewProjection>& viewProjections) const
{
    const Sphere lightSphere(loadedLight.light.worldPos, GetLightMaxAffectRange(m_vulkanObjs->GetRenderSettings(), loadedLight.light));

    return std::ranges::any_of(viewProjections, [&](const auto& viewProjection){
        const auto boundingVolume = viewProjection.GetWorldSpaceAABB().GetVolume();

        return Intersects(lightSphere, boundingVolume);
    });
}

}
