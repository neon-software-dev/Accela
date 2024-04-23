/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef TESTDESKTOPAPP_TESTSCENE_H
#define TESTDESKTOPAPP_TESTSCENE_H

#include "MovementCommands.h"
#include "Player.h"

#include <Accela/Engine/Scene/Scene.h>
#include <Accela/Engine/Entity/EnginePerfMonitorEntity.h>

#include <random>
#include <optional>

namespace Accela
{
    class TestScene : public Engine::Scene
    {
        public:

            [[nodiscard]] std::string GetName() const override { return "TestScene"; };

            //
            // Methods called by the engine for various scene/engine events
            //
            void OnSceneStart(const Engine::IEngineRuntime::Ptr& engine) override;
            void OnSimulationStep(unsigned int timeStep) override;
            void OnKeyEvent(const Platform::KeyEvent& event) override;
            void OnMouseMoveEvent(const Platform::MouseMoveEvent& event) override;
            void OnMouseButtonEvent(const Platform::MouseButtonEvent& event) override;

        private:

            //
            // Scene Setup
            //
            [[nodiscard]] bool LoadAssets();
            void ConfigureScene();
            void CreateSceneEntities();

            /** Add a light at the specified position */
            void CreateLight(const glm::vec3& position);

            /** Add the dancing vampire model at the specified position */
            void CreateVampireEntity(const glm::vec3& position);

            /** Add a floor object at the specified position/orientation with a certain x/z side length */
            void CreateFloorEntity(glm::vec3 position,
                                   float sideLength,
                                   glm::quat orientation = glm::identity<glm::quat>());

            /** Add a height mapped terrain entity at the specified position with a certain x/z scale factor */
            void CreateTerrainEntity(const float& scale, const glm::vec3& position);

            /** Add a cube entity with the specified physical properties */
            void CreateCubeEntity(glm::vec3 position,
                                  glm::vec3 scale,
                                  bool isStatic,
                                  glm::vec3 linearVelocity = glm::vec3(0)) const;

            //
            // Scene Manipulation
            //

            /** Functions to turn key presses into camera or player movements */
            [[nodiscard]] MovementCommands GetActiveMovementCommands();
            void ApplyMovementToPlayer( const MovementCommands& movementCommands) const;
            void ApplyMovementToCamera(const MovementCommands& movementCommands) const;

            /** Moves the main light to be position where the world camera is currently positioned */
            void SyncLightToCamera() const;

            /** Spawns a randomly sized cube that shoots out from the current camera position/look direction */
            void ShootCubeFromCamera();

        private:

            Player::UPtr m_player{nullptr};

            bool m_freeFlyCamera{false};
            float m_cameraTranslationSpeed{0.1f};

            Engine::EntityId m_lightEid{0};
            Render::TextureId m_skyBoxTextureId{};
            Render::MeshId m_cubeMeshId{};
            Render::MeshId m_terrainHeightMapMeshId{};
            Render::MaterialId m_solidRedMaterialId{};
            Render::MaterialId m_terrainMaterialId{};

            std::optional<Engine::EnginePerfMonitorEntity::UPtr> m_perfMonitor;

            std::random_device m_rd;
            std::mt19937 m_mt{m_rd()};
    };
}

#endif //TESTDESKTOPAPP_TESTSCENE_H
