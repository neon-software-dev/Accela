/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELARENDERERVK_SRC_LIGHT_LIGHTS_H
#define LIBACCELARENDERERVK_SRC_LIGHT_LIGHTS_H

#include "ILights.h"

#include <Accela/Render/Ids.h>

#include <Accela/Common/Log/ILogger.h>
#include <Accela/Common/Metrics/IMetrics.h>

#include <expected>
#include <unordered_map>

namespace Accela::Render
{
    class Lights : public ILights
    {
        public:

            Lights(Common::ILogger::Ptr logger,
                   Common::IMetrics::Ptr metrics,
                   VulkanObjsPtr vulkanObjs,
                   IFramebuffersPtr framebuffers,
                   Ids::Ptr ids);

            //
            // ILights
            //
            void Destroy() override;
            [[nodiscard]] std::vector<LoadedLight> GetAllLights() const override;
            [[nodiscard]] std::vector<LoadedLight> GetSceneLights(const std::string& sceneName, const std::vector<ViewProjection>& viewProjections) const override;
            [[nodiscard]] std::optional<LoadedLight> GetLightById(const LightId& lightId) const override;
            void ProcessUpdate(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence) override;
            [[nodiscard]] bool OnRenderSettingsChanged(const RenderSettings& renderSettings) override;
            void InvalidateShadowMapsByBounds(const std::vector<AABB>& boundingBoxes_worldSpace) override;
            void OnShadowMapSynced(const LightId& lightId) override;

        private:

            void ProcessAddedLights(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence);
            void ProcessUpdatedLights(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence);
            void ProcessDeletedLights(const WorldUpdate& update, const VulkanCommandBufferPtr& commandBuffer, VkFence vkFence);

            [[nodiscard]] std::expected<FrameBufferId, bool> CreateShadowFramebuffer(const Light& light, const RenderSettings& renderSettings) const;
            [[nodiscard]] bool RecreateShadowFramebuffer(LoadedLight& loadedLight, const RenderSettings& renderSettings) const;
            [[nodiscard]] static Render::USize GetShadowFramebufferSize(const RenderSettings& renderSettings);

            [[nodiscard]] static inline bool LightAffectsViewProjections(const Light& light, const std::vector<ViewProjection>& viewProjections);

            [[nodiscard]] bool IsVolumeTriviallyOutsideLight_Single(const Light& light, const Volume& volume_worldSpace);
            [[nodiscard]] bool IsVolumeTriviallyOutsideLight_Cube(const Light& light, const Volume& volume_worldSpace);

        private:

            Common::ILogger::Ptr m_logger;
            Common::IMetrics::Ptr m_metrics;
            VulkanObjsPtr m_vulkanObjs;
            IFramebuffersPtr m_framebuffers;
            Ids::Ptr m_ids;

            // TODO: K-D Tree of light volumes for efficient fetching by volume
            std::unordered_map<LightId, LoadedLight> m_lights;
    };
}

#endif //LIBACCELARENDERERVK_SRC_LIGHT_LIGHTS_H
