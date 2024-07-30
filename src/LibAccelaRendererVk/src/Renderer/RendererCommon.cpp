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

static constexpr float PERSPECTIVE_CLIP_NEAR = 0.1f;

static const Render::USize Shadow_Low_Quality_Size{1024, 1024};
static const Render::USize Shadow_Medium_Quality_Size{2048, 2048};
static const Render::USize Shadow_High_Quality_Size{4096, 4096};

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

GlobalPayload GetGlobalPayload(const RenderParams& renderParams, const RenderSettings& renderSettings, const unsigned int& numLights)
{
    GlobalPayload globalPayload{};
    globalPayload.surfaceTransform = glm::mat4(1); // TODO ANDROID: PASS IN
    globalPayload.numLights = numLights;
    globalPayload.ambientLightIntensity = renderParams.ambientLightIntensity;
    globalPayload.ambientLightColor = renderParams.ambientLightColor;
    globalPayload.shadowCascadeOverlap = renderSettings.shadowCascadeOverlapRatio;

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

Render::USize GetShadowFramebufferSize(const RenderSettings& renderSettings)
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

std::expected<ViewProjection, bool> GetPointShadowMapViewProjectionNonFaced(const RenderSettings& renderSettings,
                                                                            const LoadedLight& loadedLight)
{
    //
    // View - The light looking from its position towards its configured direction
    //
    const auto upUnit = This({0,1,0})
        .ButIfParallelWith(loadedLight.light.lightProperties.directionUnit)
        .Then({0,0,1});

    const auto view = glm::lookAt(
        loadedLight.light.worldPos,
        loadedLight.light.worldPos + loadedLight.light.lightProperties.directionUnit,
        upUnit
    );

    //
    // Projection
    //
    const auto projection = GetPointShadowMapProjectionTransform(
        renderSettings,
        loadedLight,
        loadedLight.light.lightProperties.areaOfEffect
    );
    if (!projection)
    {
        return std::unexpected(false);
    }

    return ViewProjection(view, *projection);
}

std::expected<ViewProjection, bool> GetPointShadowMapViewProjectionFaced(const RenderSettings& renderSettings,
                                                                         const LoadedLight& loadedLight,
                                                                         const CubeFace& cubeFace)
{
    const auto viewTransform = GetPointShadowMapViewTransformFaced(loadedLight, cubeFace);
    const auto projectionTransform = GetPointShadowMapProjectionTransform(renderSettings, loadedLight, 90.0f);

    if (!viewTransform) { return std::unexpected(false); }
    if (!projectionTransform) { return std::unexpected(false); }

    return ViewProjection{*viewTransform, *projectionTransform};
}

std::expected<glm::mat4, bool> GetPointShadowMapViewTransformFaced(const LoadedLight& loadedLight, const CubeFace& cubeFace)
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

std::expected<Projection::Ptr, bool> GetPointShadowMapProjectionTransform(const RenderSettings& renderSettings,
                                                                          const LoadedLight& loadedLight,
                                                                          float fovYDegrees)
{
    const float lightMaxAffectRange = GetLightMaxAffectRange(renderSettings, loadedLight.light);

    return FrustumProjection::From(
        fovYDegrees,
        1.0f,
        PERSPECTIVE_CLIP_NEAR,
        lightMaxAffectRange
    );
}

[[nodiscard]] std::expected<DirectionalShadowRender, bool> GetDirectionalShadowMapViewProjection(
    const RenderSettings& renderSettings,
    const IVulkanContextPtr& context,
    const LoadedLight& loadedLight,
    const RenderCamera& viewCamera,
    CascadeCut cascadeCut)
{
    const auto shadowFramebufferSize = GetShadowFramebufferSize(renderSettings);

    //
    // Fetch the various view projections used for the camera - will be a single VP in desktop mode, and left/right
    // eye VPs in headset mode
    //
    std::vector<ViewProjection> eyeViewProjections;

    // VR Mode
    if (renderSettings.presentToHeadset)
    {
        auto viewProjection = GetCameraViewProjection(renderSettings, context, viewCamera, Eye::Left);
        assert(viewProjection.has_value());
        if (viewProjection) { eyeViewProjections.push_back(*viewProjection); }

        viewProjection = GetCameraViewProjection(renderSettings, context, viewCamera, Eye::Right);
        assert(viewProjection.has_value());
        if (viewProjection) { eyeViewProjections.push_back(*viewProjection); }
    }
    // Desktop Mode
    else
    {
        const auto viewProjection = GetCameraViewProjection(renderSettings, context, viewCamera);
        assert(viewProjection.has_value());
        if (viewProjection) { eyeViewProjections.push_back(*viewProjection); }
    }

    // Cut each eye's view projection down to the cascade-specific sub-frustrum that was requested
    for (auto& viewProjection : eyeViewProjections)
    {
        (void)viewProjection.projectionTransform->SetNearPlaneDistance(glm::max(PERSPECTIVE_CLIP_NEAR, cascadeCut.start));
        (void)viewProjection.projectionTransform->SetFarPlaneDistance(cascadeCut.end);
    }

    //
    // Get the world-space bounding points which bound all the cut view areas
    //
    std::vector<glm::vec3> viewBounds_worldSpace;

    for (const auto& viewProjection : eyeViewProjections)
    {
        std::ranges::copy(
            viewProjection.GetWorldSpaceBoundingPoints(),
            std::back_inserter(viewBounds_worldSpace)
        );
    }

    //
    // Determine the world-space center point of the area being viewed
    //
    const glm::vec3 viewBoundsCenter_worldSpace = GetCenterPoint(viewBounds_worldSpace);

    //
    // Determine the spherical radius of the area being viewed
    //
    float viewBoundsRadius_worldSpace{0.0f};

    for (const auto& point : viewBounds_worldSpace)
    {
        const auto radius = glm::distance(viewBoundsCenter_worldSpace, point);
        viewBoundsRadius_worldSpace = std::max(viewBoundsRadius_worldSpace, radius);
    }

    const float viewBoundsDiameter_worldSpace = viewBoundsRadius_worldSpace * 2.0f;

    // Determine depth radius of the projection, which is viewBoundsRadius_worldSpace but additionally,
    // optional, pulled back further to a specified minimum radius
    const auto viewDepthRadius_worldSpace = std::max(viewBoundsRadius_worldSpace, renderSettings.shadowCascadeMinRadiusDepth);

    //
    // Determine the texel-snapped world-space center of the shadow render
    //
    const auto shadowRenderLookUnit_worldSpace = loadedLight.light.lightProperties.directionUnit;

    const auto shadowRenderUpUnit_worldSpace =
        This({0,1,0})
            .ButIfParallelWith(shadowRenderLookUnit_worldSpace)
            .Then({0,0,1});

    // Warning: Assumes/requires square-faced render area, which using the spherical-based approach fulfills
    const auto texelsPerUnit = (float)shadowFramebufferSize.w / viewBoundsDiameter_worldSpace;

    auto shadowRenderPosition_worldSpace =
        viewBoundsCenter_worldSpace + (-loadedLight.light.lightProperties.directionUnit * viewDepthRadius_worldSpace);

    // Temporary view matrix used for snapping shadow render position to texel space; orients a position for
    // a shadow render, and scales it to texel-space scale
    auto texelShadowRenderView = glm::lookAt(
        {0,0,0},
        loadedLight.light.lightProperties.directionUnit,
        shadowRenderUpUnit_worldSpace
    );
    texelShadowRenderView = glm::scale(texelShadowRenderView, glm::vec3{texelsPerUnit});

    // Transform the render center to texel space, round it off to an even texel, then transform back to world space
    shadowRenderPosition_worldSpace = texelShadowRenderView * glm::vec4(shadowRenderPosition_worldSpace, 1.0f);
    shadowRenderPosition_worldSpace = glm::floor(shadowRenderPosition_worldSpace);
    shadowRenderPosition_worldSpace = glm::inverse(texelShadowRenderView) * glm::vec4(shadowRenderPosition_worldSpace, 1.0f);

    //
    // Create the render space view transform
    //
    auto shadowRenderView = glm::lookAt(
        shadowRenderPosition_worldSpace,
        viewBoundsCenter_worldSpace,
        shadowRenderUpUnit_worldSpace
    );

    //
    // Create the render space projection transform
    //
    float orthoWidth = viewBoundsDiameter_worldSpace;
    float orthoHeight = viewBoundsDiameter_worldSpace;
    float orthoNear = 0.0f;
    float orthoFar = viewBoundsRadius_worldSpace + viewDepthRadius_worldSpace;

    const float halfOrthoWidth = orthoWidth / 2.0f;
    const float halfOrthoHeight = orthoHeight / 2.0f;

    glm::vec3 nearMin(-halfOrthoWidth, -halfOrthoHeight, -orthoNear);
    glm::vec3 nearMax(halfOrthoWidth, halfOrthoHeight, -orthoNear);
    glm::vec3 farMin(-halfOrthoWidth, -halfOrthoHeight, -orthoFar);
    glm::vec3 farMax(halfOrthoWidth, halfOrthoHeight, -orthoFar);

    const auto renderProjection = OrthoProjection::From(nearMin, nearMax, farMin, farMax);
    assert(renderProjection.has_value());

    return DirectionalShadowRender(
        shadowRenderPosition_worldSpace,
        cascadeCut,
        ViewProjection(shadowRenderView, *renderProjection)
    );
}

std::expected<std::vector<DirectionalShadowRender>, bool> GetDirectionalShadowMapViewProjections(
    const RenderSettings& renderSettings,
    const IVulkanContextPtr& context,
    const LoadedLight& loadedLight,
    const RenderCamera& viewCamera)
{
    const auto cascadeCuts = GetDirectionalShadowCascadeCuts(renderSettings);

    std::vector<DirectionalShadowRender> shadowRenders;

    for (const auto& cascadeCut : cascadeCuts)
    {
        const auto shadowRender = GetDirectionalShadowMapViewProjection(renderSettings, context, loadedLight, viewCamera, cascadeCut);
        if (!shadowRender)
        {
            return std::unexpected(false);
        }

        shadowRenders.push_back(*shadowRender);
    }

    return shadowRenders;
}

std::vector<CascadeCut> GetDirectionalShadowCascadeCuts(const RenderSettings& renderSettings)
{
    //
    // Determine the distance at which we'll render object shadows. This distance is the minimum
    // of: ObjectRenderDistance, MaxRenderDistance, and, if set, ShadowRenderDistance
    //
    float shadowRenderDistance = std::min(renderSettings.objectRenderDistance, renderSettings.maxRenderDistance);

    if (renderSettings.shadowRenderDistance)
    {
        shadowRenderDistance = std::min(shadowRenderDistance, *renderSettings.shadowRenderDistance);
    }

    //
    // Determine cut percentages that define the cascade cuts
    //
    const float cascadeSplitLambda = 0.95f;
    const float nearClip = PERSPECTIVE_CLIP_NEAR;
    const float farClip = shadowRenderDistance;
    const float clipRange = farClip - nearClip;
    const float minZ = nearClip;
    const float maxZ = nearClip + clipRange;
    const float range = maxZ - minZ;
    const float ratio = maxZ / minZ;

    // Determine percentages along the view frustum to create splits at using
    // a logarithmic practical split scheme
    std::array<float, Shadow_Cascade_Count> cutPercentages{0.0f};

    for (uint32_t x = 0; x < Shadow_Cascade_Count; ++x)
    {
        const float p = ((float)x + 1.0f) / static_cast<float>(Shadow_Cascade_Count);
        const float log = minZ * std::pow(ratio, p);
        const float uniform = minZ + (range * p);
        const float d = (cascadeSplitLambda * (log - uniform)) + uniform;
        cutPercentages[x] = (d - nearClip) / clipRange;
    }

    //
    // Transform cut percentages into CascadeCuts
    //
    std::vector<CascadeCut> cuts;

    float lastCutEnd = minZ;

    for (uint32_t x = 0; x < Shadow_Cascade_Count; ++x)
    {
        float cutStart = lastCutEnd;

        // Move the start of cuts forwards to create an overlap between cuts, so that
        // we can smoothly blend between cuts rather than having a hard edge
        if (x > 0)
        {
            const CascadeCut& prevCut = cuts[x - 1];
            const float prevCutRange = prevCut.end - prevCut.start;
            const float prevCutOverlapAmount = prevCutRange * renderSettings.shadowCascadeOverlapRatio;
            cutStart -= prevCutOverlapAmount;
        }

        const float cutEnd = clipRange * cutPercentages[x];

        cuts.emplace_back(cutStart, cutEnd);
        lastCutEnd = cutEnd;
    }

    assert(cuts.size() == Shadow_Cascade_Count);

    return cuts;
}

}
