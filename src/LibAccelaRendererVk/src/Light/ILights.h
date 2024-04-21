/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef LIBACCELARENDERERVK_SRC_LIGHT_ILIGHTS_H
#define LIBACCELARENDERERVK_SRC_LIGHT_ILIGHTS_H

#include "LoadedLight.h"

#include "../ForwardDeclares.h"

#include "../Util/AABB.h"
#include "../Util/ViewProjection.h"

#include <Accela/Render/Light.h>

#include <Accela/Render/Task/WorldUpdate.h>
#include <Accela/Render/RenderSettings.h>

#include <vulkan/vulkan.h>

#include <optional>
#include <vector>

namespace Accela::Render
{
    class ILights
    {
        public:

            using Ptr = std::shared_ptr<ILights>;

        public:

            virtual ~ILights() = default;

            virtual void Destroy() = 0;

            [[nodiscard]] virtual std::vector<LoadedLight> GetAllLights() const = 0;
            [[nodiscard]] virtual std::vector<LoadedLight> GetSceneLights(const std::string& sceneName, const std::vector<ViewProjection>& viewProjections) const = 0;
            [[nodiscard]] virtual std::optional<LoadedLight> GetLightById(const LightId& lightId) const = 0;

            virtual void ProcessUpdate(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence) = 0;

            [[nodiscard]] virtual bool OnRenderSettingsChanged(const RenderSettings& renderSettings) = 0;

            /**
             * Invalidates the shadow maps for any lights which cover the specified areas.
             *
             * @param boundingBoxes_worldSpace The world-space bounding boxes of areas to be invalidated
             */
            virtual void InvalidateShadowMapsByBounds(const std::vector<AABB>& boundingBoxes_worldSpace) = 0;

            /**
             * Marks a shadow map for the specified light as in-sync (not invalidated)
             *
             * @param lightId The id of the light who's shadow map is synced
             */
            virtual void OnShadowMapSynced(const LightId& lightId) = 0;
    };
}

#endif //LIBACCELARENDERERVK_SRC_LIGHT_ILIGHTS_H
