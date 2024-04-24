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
#include <Accela/Render/Task/RenderParams.h>
#include <Accela/Render/Light.h>

#include <glm/glm.hpp>

#include <expected>

namespace Accela::Render
{
    static const uint32_t Offscreen_GPassSubpass_Index = 0;
    static const uint32_t Offscreen_LightingSubpass_Index = 1;
    static const uint32_t Offscreen_ForwardSubpass_Index = 2;

    enum class RenderType
    {
        Gpass,
        Shadow
    };

    enum class CullFace
    {
        Front,
        Back
    };

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

    struct LightPayload
    {
        alignas(4) uint32_t shadowMapType{0};
        alignas(16) glm::mat4 lightTransform{1}; // Not used by all light types
        alignas(16) glm::vec3 worldPos{0};
        alignas(4) int32_t shadowMapIndex{-1};
        alignas(4) float maxAffectRange{0.0f};

        alignas(4) uint32_t attenuationMode{(uint32_t)AttenuationMode::Exponential};
        alignas(16) glm::vec3 diffuseColor{1};
        alignas(16) glm::vec3 diffuseIntensity{0};
        alignas(16) glm::vec3 specularColor{1};
        alignas(16) glm::vec3 specularIntensity{0};
        alignas(16) glm::vec3 directionUnit{0,0,-1};
        alignas(4) float coneFovDegrees{45.0f};
    };

    /////

    [[nodiscard]] float GetLightMaxAffectRange(const Light& light);

    /**
     * Generates a GlobalPayload given the current render settings and params
     */
    [[nodiscard]] GlobalPayload GetGlobalPayload(const RenderParams& renderParams, const unsigned int& numLights);
    [[nodiscard]] ViewProjectionPayload GetViewProjectionPayload(const ViewProjection& viewProjection);

    ////

    [[nodiscard]] std::expected<ViewProjection, bool> GetCameraViewProjection(const IVulkanContextPtr& context,
                                                                              const RenderCamera& camera,
                                                                              const std::optional<Eye>& eye = std::nullopt);

    [[nodiscard]] glm::mat4 GetCameraViewTransform(const IVulkanContextPtr& context,
                                                   const RenderCamera& camera,
                                                   const std::optional<Eye>& eye);

    [[nodiscard]] std::expected<Projection::Ptr, bool> GetCameraProjectionTransform(const IVulkanContextPtr& context,
                                                                                    const RenderCamera& camera,
                                                                                    const std::optional<Eye>& eye);

    [[nodiscard]] std::expected<ViewProjection, bool> GetShadowMapViewProjection(const LoadedLight& light);
    [[nodiscard]] std::expected<glm::mat4, bool> GetShadowMapViewTransform(const LoadedLight& light);

    [[nodiscard]] std::expected<ViewProjection, bool> GetShadowMapCubeViewProjection(const LoadedLight& light, const CubeFace& cubeFace);
    [[nodiscard]] glm::mat4 GetShadowMapCubeViewTransform(const LoadedLight& light, const CubeFace& cubeFace);

    [[nodiscard]] std::expected<Projection::Ptr, bool> GetShadowMapProjectionTransform(const LoadedLight& light);
}

#endif //LIBACCELARENDERERVK_SRC_RENDERER_RENDERERCOMMON_H
