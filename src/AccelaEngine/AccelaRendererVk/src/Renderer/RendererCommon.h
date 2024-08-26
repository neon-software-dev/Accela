/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_RENDERER_RENDERERCOMMON_H
#define LIBACCELARENDERERVK_SRC_RENDERER_RENDERERCOMMON_H

#include "ForwardDeclares.h"

#include "../InternalCommon.h"

#include "../Light/LoadedLight.h"
#include "../Util/Projection.h"
#include "../Util/ViewProjection.h"

#include <Accela/Render/RenderSettings.h>
#include <Accela/Render/IOpenXR.h>
#include <Accela/Render/Light.h>
#include <Accela/Render/Task/RenderParams.h>

#include <glm/glm.hpp>

#include <expected>
#include <vector>

namespace Accela::Render
{
    enum class RenderType
    {
        GpassDeferred,
        GpassForward,
        Shadow
    };

    enum class CullFace
    {
        None,
        Front,
        Back
    };

    // Number of directional light cascading shadow maps
    constexpr uint32_t Shadow_Cascade_Count = 4;

    // Maximum number of shadow renders a light can have (cubic shadows have 6)
    constexpr uint32_t Max_Shadow_Render_Count = 6;

    //
    // Vulkan-aligned shader input payload data types
    //

    struct ObjectDrawPayload
    {
        alignas(4) uint32_t dataIndex{0};
        alignas(4) uint32_t materialIndex{0};
    };

    struct SpriteDrawPayload
    {
        alignas(4) uint32_t dataIndex{0};
    };

    struct GlobalPayload
    {
        // General
        alignas(16) glm::mat4 surfaceTransform{1};

        // Lighting
        alignas(4) uint32_t numLights{0};
        alignas(4) float ambientLightIntensity{0.0f};
        alignas(16) glm::vec3 ambientLightColor{1.0f};
        alignas(4) float shadowCascadeOverlap{0.0f};
    };

    struct ViewProjectionPayload
    {
        alignas(16) glm::mat4 viewTransform{1};
        alignas(16) glm::mat4 projectionTransform{1};
    };

    struct SpritePayload
    {
        alignas(16) glm::mat4 modelTransform{1};
        alignas(8) glm::vec2 uvTranslation{0,0};
        alignas(8) glm::vec2 uvSize{0,0};
    };

    struct ObjectPayload
    {
        alignas(16) glm::mat4 modelTransform{1};
    };

    struct TerrainPayload
    {
        alignas(16) glm::mat4 modelTransform{1};
        alignas(4) float tesselationLevel{1.0f};
        alignas(4) float displacementFactor{1.0f};
    };

    struct ShadowMapPayload
    {
        alignas(16) glm::vec3 worldPos{0};
        alignas(16) glm::mat4 transform{1};
        alignas(8) glm::vec2 cut{0};
        alignas(4) uint32_t cascadeIndex{0};
    };

    struct LightPayload
    {
        alignas(16) glm::vec3 worldPos{0};
        alignas(4) float maxAffectRange{0.0f};

        alignas(4) uint32_t attenuationMode{(uint32_t)AttenuationMode::Exponential};
        alignas(16) glm::vec3 diffuseColor{1};
        alignas(16) glm::vec3 diffuseIntensity{0};
        alignas(16) glm::vec3 specularColor{1};
        alignas(16) glm::vec3 specularIntensity{0};
        alignas(16) glm::vec3 directionUnit{0,0,-1};
        alignas(4) float areaOfEffect{45.0f};

        alignas(4) uint32_t shadowMapType{0};
        alignas(4) int32_t shadowMapIndex{-1};
        alignas(16) ShadowMapPayload shadowMaps[Max_Shadow_Render_Count];
    };

    /////

    [[nodiscard]] float GetLightMaxAffectRange(const RenderSettings& renderSettings, const Light& light);

    /**
     * Generates a GlobalPayload given the current render settings and params
     */
    [[nodiscard]] GlobalPayload GetGlobalPayload(const RenderParams& renderParams,
                                                 const RenderSettings& renderSettings,
                                                 const unsigned int& numLights);
    [[nodiscard]] ViewProjectionPayload GetViewProjectionPayload(const ViewProjection& viewProjection);

    ////

    [[nodiscard]] std::expected<ViewProjection, bool> GetCameraViewProjection(const RenderSettings& renderSettings,
                                                                              const IOpenXR::Ptr& openXR,
                                                                              const RenderCamera& camera,
                                                                              const std::optional<Eye>& eye = std::nullopt);

    [[nodiscard]] glm::mat4 GetCameraViewTransform(const IOpenXR::Ptr& openXR,
                                                   const RenderCamera& camera,
                                                   const std::optional<Eye>& eye);

    [[nodiscard]] std::expected<Projection::Ptr, bool> GetCameraProjectionTransform(const RenderSettings& renderSettings,
                                                                                    const IOpenXR::Ptr& openXR,
                                                                                    const RenderCamera& camera,
                                                                                    const std::optional<Eye>& eye);

    //
    // Light General
    //
    [[nodiscard]] Render::USize GetShadowFramebufferSize(const RenderSettings& renderSettings);

    //
    // Point Lights
    //
    [[nodiscard]] std::expected<ViewProjection, bool> GetPointShadowMapViewProjectionNonFaced(
        const RenderSettings& renderSettings,
        const LoadedLight& loadedLight);

    [[nodiscard]] std::expected<ViewProjection, bool> GetPointShadowMapViewProjectionFaced(
        const RenderSettings& renderSettings,
        const LoadedLight& loadedLight,
        const CubeFace& cubeFace);

    [[nodiscard]] std::expected<glm::mat4, bool> GetPointShadowMapViewTransformFaced(
        const LoadedLight& loadedLight,
        const CubeFace& cubeFace);

    [[nodiscard]] std::expected<Projection::Ptr, bool> GetPointShadowMapProjectionTransform(
        const RenderSettings& renderSettings,
        const LoadedLight& loadedLight,
        float fovYDegrees);

    //
    // Directional Lights
    //
    struct CascadeCut
    {
        CascadeCut(float _start, float _end)
            : start(_start)
            , end(_end)
        { }

        [[nodiscard]] glm::vec2 AsVec2() const noexcept { return {start, end}; }

        float start;
        float end;
    };

    struct DirectionalShadowRender
    {
        DirectionalShadowRender(const glm::vec3& _render_worldPosition, CascadeCut _cut, ViewProjection _viewProjection)
            : render_worldPosition(_render_worldPosition)
            , cut(_cut)
            , viewProjection(std::move(_viewProjection))
        { }

        glm::vec3 render_worldPosition; // The world position the shadow is being rendered from
        CascadeCut cut;
        ViewProjection viewProjection; // The view-projection for the shadow render
    };

    // TODO: Move cut cubes forward so no part of it is behind the viewer's plane? (Note: can't make it
    //  non-square or else texel snapping won't work)

    [[nodiscard]] std::expected<std::vector<DirectionalShadowRender>, bool> GetDirectionalShadowMapViewProjections(
        const VulkanObjsPtr& vulkanObjs,
        const IOpenXR::Ptr& openXR,
        const LoadedLight& loadedLight,
        const RenderCamera& viewCamera);

    [[nodiscard]] std::vector<CascadeCut> GetDirectionalShadowCascadeCuts(const RenderSettings& renderSettings);
}

#endif //LIBACCELARENDERERVK_SRC_RENDERER_RENDERERCOMMON_H
