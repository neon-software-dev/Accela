/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RendererCommon.h"

#include "Util/FrustumProjection.h"
#include "Util/OrthoProjection.h"

#include <Accela/Render/IVulkanContext.h>
#include <Accela/Render/Util/Vector.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cassert>

namespace Accela::Render
{

constexpr float PERSPECTIVE_CLIP_NEAR = 0.1f;

float GetLightMaxAffectRange(const RenderSettings& renderSettings, const Light& light)
{
    switch (light.lightProperties.attenuationMode)
    {
        case AttenuationMode::None:
            // Range is however much range we normally render objects at
            return renderSettings.maxRenderDistance;
        case AttenuationMode::Linear:
            // c1 / d with c1 = 10.0
            // attenuation is 1% at d = 1000
            return 1000.0f;
        case AttenuationMode::Exponential:
            // 1.0 / (c1 + c2*d^2) with c1 = 1.0, c2 = 0.1
            // attenuation is 1% at d = 31.46
            return 31.46f;
    }

    assert(false);
    return renderSettings.maxRenderDistance;
}

GlobalPayload GetGlobalPayload(const RenderParams& renderParams, const unsigned int& numLights)
{
    GlobalPayload globalPayload{};
    globalPayload.surfaceTransform = glm::mat4(1); // TODO ANDROID: PASS IN
    globalPayload.numLights = numLights;
    globalPayload.ambientLightIntensity = renderParams.ambientLightIntensity;
    globalPayload.ambientLightColor = renderParams.ambientLightColor;

    return globalPayload;
}

ViewProjectionPayload GetViewProjectionPayload(const ViewProjection& viewProjection)
{
    ViewProjectionPayload viewProjectionPayload{};
    viewProjectionPayload.viewTransform = viewProjection.viewTransform;
    viewProjectionPayload.projectionTransform = viewProjection.projectionTransform->GetProjectionMatrix();

    return viewProjectionPayload;
}

std::expected<ViewProjection, bool> GetCameraViewProjection(const RenderSettings& renderSettings,
                                                            const IVulkanContextPtr& context,
                                                            const RenderCamera& camera,
                                                            const std::optional<Eye>& eye)
{
    auto viewTransform = GetCameraViewTransform(context, camera, eye);
    viewTransform = glm::scale(viewTransform, glm::vec3(renderSettings.globalViewScale));

    const auto projectionTransform = GetCameraProjectionTransform(renderSettings, context, camera, eye);

    if (!projectionTransform)
    {
        return std::unexpected(false);
    }

    return ViewProjection{viewTransform, *projectionTransform};
}

glm::mat4 GetCameraViewTransform(const IVulkanContextPtr& context,
                                 const RenderCamera& camera,
                                 const std::optional<Eye>& eye)
{
    const auto lookUnit = camera.lookUnit;

    const auto upUnit =
        This(camera.upUnit)
        .ButIfParallelWith(camera.lookUnit)
        .Then({0,0,1});

    glm::mat4 viewTransform = glm::lookAt(
        camera.position,
        camera.position + lookUnit,
        upUnit
    );

    //
    // If we're rendering for a specific eye we need to adjust the view transform by the headset's/eye's position
    //
    if (eye)
    {
        const auto headsetPoseOpt = context->VR_GetHeadsetPose();
        if (headsetPoseOpt)
        {
            viewTransform = glm::inverse(headsetPoseOpt.value() * context->VR_GetEyeToHeadTransform(*eye)) * viewTransform;
        }
    }

    return viewTransform;
}

std::expected<Projection::Ptr, bool> GetCameraProjectionTransform(const RenderSettings& renderSettings,
                                                                  const IVulkanContextPtr& context,
                                                                  const RenderCamera& camera,
                                                                  const std::optional<Eye>& eye)
{
    if (eye)
    {
        //
        // FrustumProjection for the projectionFrustum given to us by the VR system
        //
        float leftTanHalfAngle{0.0f};
        float rightTanHalfAngle{0.0f};
        float topTanHalfAngle{0.0f};
        float bottomTanHalfAngle{0.0f};

        context->VR_GetEyeProjectionRaw(*eye, leftTanHalfAngle, rightTanHalfAngle, topTanHalfAngle, bottomTanHalfAngle);

        return FrustumProjection::FromTanHalfAngles(
            leftTanHalfAngle,
            rightTanHalfAngle,
            topTanHalfAngle,
            bottomTanHalfAngle,
            PERSPECTIVE_CLIP_NEAR,
            renderSettings.maxRenderDistance
        );
    }
    else
    {
        //
        // FrustumProjection for the projectionFrustum for the current render camera
        //
        return FrustumProjection::From(
            camera,
            PERSPECTIVE_CLIP_NEAR,
            renderSettings.maxRenderDistance
        );
    }
}

std::expected<ViewProjection, bool> GetShadowMapViewProjection(const RenderSettings& renderSettings, const LoadedLight& loadedLight)
{
    const auto viewTransform = GetShadowMapViewTransform(loadedLight);
    const auto projectionTransform = GetShadowMapProjectionTransform(renderSettings, loadedLight);

    if (!viewTransform) { return std::unexpected(false); }
    if (!projectionTransform) { return std::unexpected(false); }

    return ViewProjection{*viewTransform, *projectionTransform};
}

std::expected<glm::mat4, bool> GetShadowMapViewTransform(const LoadedLight& loadedLight)
{
    const auto upUnit =
        This({0,1,0})
        .ButIfParallelWith(loadedLight.light.lightProperties.directionUnit)
        .Then({0,0,1});

    return glm::lookAt(
        loadedLight.light.worldPos,
        loadedLight.light.worldPos + loadedLight.light.lightProperties.directionUnit,
        upUnit
    );
}

std::expected<ViewProjection, bool> GetShadowMapCubeViewProjection(const RenderSettings& renderSettings,
                                                                   const LoadedLight& loadedLight,
                                                                   const CubeFace& cubeFace)
{
    const auto viewTransform = GetShadowMapCubeViewTransform(loadedLight, cubeFace);
    const auto projectionTransform = GetShadowMapProjectionTransform(renderSettings, loadedLight);

    if (!projectionTransform) { return std::unexpected(false); }

    return ViewProjection{viewTransform, *projectionTransform};
}

glm::mat4 GetShadowMapCubeViewTransform(const LoadedLight& loadedLight, const CubeFace& cubeFace)
{
    glm::vec3 lookUnit(0);

    switch (cubeFace)
    {
        case CubeFace::Right:       lookUnit = {1,0,0}; break;
        case CubeFace::Left:        lookUnit = {-1,0,0}; break;
        case CubeFace::Up:          lookUnit = {0,1,0}; break;
        case CubeFace::Down:        lookUnit = {0,-1,0}; break;
        // Note that we're reversing z-axis to match OpenGl/Vulkan's left-handed cubemap coordinate system
        case CubeFace::Back:        lookUnit = {0,0,-1}; break;
        case CubeFace::Forward:     lookUnit = {0,0,1}; break;
    }

    const auto upUnit =
        This({0,1,0})
        .ButIfParallelWith(lookUnit)
        .Then({0,0,1});

    return glm::lookAt(
        loadedLight.light.worldPos,
        loadedLight.light.worldPos + lookUnit,
        upUnit
    );
}

std::expected<Projection::Ptr, bool> GetShadowMapProjectionTransform(const RenderSettings& renderSettings, const LoadedLight& loadedLight)
{
    const float lightMaxAffectRange = GetLightMaxAffectRange(renderSettings, loadedLight.light);

    switch (loadedLight.light.lightProperties.projection)
    {
        case LightProjection::Perspective:
        {
            const float projectionFov =
                loadedLight.shadowMapType == ShadowMapType::Cube ? 90.0f :
                loadedLight.light.lightProperties.coneFovDegrees;

            return FrustumProjection::From(
                projectionFov,
                1.0f,
                PERSPECTIVE_CLIP_NEAR,
                lightMaxAffectRange
            );
        }
        case LightProjection::Orthographic:
            return OrthoProjection::From(
                lightMaxAffectRange,
                lightMaxAffectRange,
                PERSPECTIVE_CLIP_NEAR,
                lightMaxAffectRange
            );
    }

    assert(false);
    return std::unexpected(false);
}

}
