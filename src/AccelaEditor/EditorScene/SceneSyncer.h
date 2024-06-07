/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_EDITORSCENE_SCENESYNCER_H
#define ACCELAEDITOR_EDITORSCENE_SCENESYNCER_H

#include <Accela/Engine/Common.h>
#include <Accela/Engine/Package/Construct.h>
#include <Accela/Engine/Package/Component.h>

#include <Accela/Common/Log/ILogger.h>

#include <memory>
#include <unordered_map>
#include <string>
#include <future>

namespace Accela
{
    class AccelaWindow;

    class SceneSyncer
    {
        public:

            explicit SceneSyncer(Common::ILogger::Ptr logger);

            /**
             * Provide the reference to an AccelaWindow that this SceneSyncer
             * should communicate with to keep an Accela scene in sync
             */
            void AttachToAccelaWindow(AccelaWindow* pAccelaWindow);

            /**
             * Loads all resources for the provided package (as well as opening the package if
             * it isn't already open).
             */
            [[nodiscard]] std::future<bool> LoadPackageResources(const Engine::PackageName& packageName) const;

            /**
             * Instructs the scene to destroy all resources
             */
            [[nodiscard]] std::future<bool> DestroyAllResources() const;

            //

            /**
             * Blocking destroys all existing entities and creates all entities within the specified construct
             */
            void BlockingFullSyncConstruct(const std::optional<Engine::Construct::Ptr>& construct);

            /**
             * Blocking creates an entity with the provided initial data
             */
            void BlockingCreateEntity(const Engine::CEntity::Ptr& entity);

            /**
             * Destroys an entity by name
             */
            [[nodiscard]] std::future<bool> DestroyEntity(const std::string& entityName);

            /**
             * Destroys all previously created entities
             */
            [[nodiscard]] std::future<bool> DestroyAllEntities();

            /**
             * Async updates the data for an entity component
             */
            [[nodiscard]] std::future<bool> UpdateEntityComponent(const std::string& entityName, const Engine::Component::Ptr& component) const;

        private:

            Common::ILogger::Ptr m_logger;

            std::optional<AccelaWindow*> m_accelaWindow;

            // Entity name -> Entity Id
            std::unordered_map<std::string, Engine::EntityId> m_entities;
    };
}

#endif //ACCELAEDITOR_EDITORSCENE_SCENESYNCER_H
