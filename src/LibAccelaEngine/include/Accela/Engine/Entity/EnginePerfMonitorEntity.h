/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_ENGINEPERFMONITORENTITY_H
#define LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_ENGINEPERFMONITORENTITY_H

#include "SceneEntity.h"
#include "ScreenTextEntity.h"

#include <Accela/Engine/IEngineRuntime.h>
#include <Accela/Engine/Scene/SceneEvents.h>

#include <glm/glm.hpp>

#include <vector>
#include <string>

namespace Accela::Engine
{
    /**
     * Helper Entity which displays engine and renderer metrics
     */
    class EnginePerfMonitorEntity : public SceneEntity
    {
        public:

            using Ptr = std::shared_ptr<EnginePerfMonitorEntity>;
            using UPtr = std::unique_ptr<EnginePerfMonitorEntity>;

        private:

            struct ConstructTag{};

        public:

            static EnginePerfMonitorEntity::UPtr Create(
                IEngineRuntime::Ptr engine,
                SceneEvents::Ptr sceneEvents,
                std::string sceneName = DEFAULT_SCENE,
                const glm::vec3& position = {0,0,0},
                uint32_t refreshInterval = 20);

            EnginePerfMonitorEntity(ConstructTag,
                                    IEngineRuntime::Ptr engine,
                                    SceneEvents::Ptr sceneEvents,
                                    std::string sceneName,
                                    const glm::vec3& position,
                                    uint32_t refreshInterval);

            ~EnginePerfMonitorEntity() override;

            EnginePerfMonitorEntity(const EnginePerfMonitorEntity&) = delete;
            EnginePerfMonitorEntity& operator=(const EnginePerfMonitorEntity&) = delete;

            void OnSimulationStep(const IEngineRuntime::Ptr& engine, unsigned int timeStep) override;

            void Destroy() override;

        private:

            struct MetricEntity
            {
                MetricEntity(Common::MetricType _metricType,
                             std::string _metricName,
                             std::string _description,
                             ScreenTextEntity::Ptr _entity)
                    : metricType(_metricType)
                    , metricName(std::move(_metricName))
                    , description(std::move(_description))
                    , entity(std::move(_entity))
                { }

                Common::MetricType metricType;
                std::string metricName;
                std::string description;
                ScreenTextEntity::Ptr entity;
            };

        private:

            void CreateEntities();

            uint32_t CreateEntity(Common::MetricType metricType,
                                  const std::string& description,
                                  const std::string& metricName,
                                  const Platform::TextProperties& textProperties,
                                  const uint32_t& yOffset);

            void DestroyInternal();

        private:

            glm::vec3 m_position;
            uint32_t m_refreshInterval;

            std::vector<MetricEntity> m_entities;
            uintmax_t m_stepCounter{0};
    };
}

#endif //LIBACCELAENGINE_INCLUDE_ACCELA_ENGINE_ENTITY_ENGINEPERFMONITORENTITY_H
