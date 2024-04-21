/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#ifndef TESTDESKTOPAPP_TESTSCENE_H
#define TESTDESKTOPAPP_TESTSCENE_H

#include "MovementCommands.h"
#include "Player.h"

#include <Accela/Engine/Scene/Scene.h>

namespace Accela
{
    class TestScene : public Engine::Scene
    {
        public:

            [[nodiscard]] std::string GetName() const override { return "TestScene"; };

            void OnSceneStart(const Engine::IEngineRuntime::Ptr& engine) override;
            void OnSimulationStep(const Engine::IEngineRuntime::Ptr& engine, unsigned int timeStep) override;
            void OnKeyEvent(const Engine::IEngineRuntime::Ptr& engine, const Platform::KeyEvent& event) override;
            void OnMouseMoveEvent(const Engine::IEngineRuntime::Ptr& engine, const Platform::MouseMoveEvent& event) override;

        private:

            [[nodiscard]] bool LoadAssets(const Engine::IEngineRuntime::Ptr& engine);
            void ConfigureScene(const Engine::IEngineRuntime::Ptr& engine);
            void CreateSceneEntities(const Engine::IEngineRuntime::Ptr& engine);

            static void CreateLight(const Engine::IEngineRuntime::Ptr& engine, const glm::vec3& position);
            static void CreateVampireEntity(const Engine::IEngineRuntime::Ptr& engine, const glm::vec3& position);
            void CreateFloorEntity(const Engine::IEngineRuntime::Ptr& engine,
                                   glm::vec3 position,
                                   float sideLength,
                                   glm::quat orientation = glm::identity<glm::quat>());
            void CreateTerrainEntity(const Engine::IEngineRuntime::Ptr& engine, const float scale, const glm::vec3& position);

            [[nodiscard]] static MovementCommands GetActiveMovementCommands(const Engine::IEngineRuntime::Ptr& engine);
            void ApplyMovementToPlayer(const Engine::IEngineRuntime::Ptr& engine, const MovementCommands& movementCommands) const;
            void ApplyMovementToCamera(const Engine::IEngineRuntime::Ptr& engine, const MovementCommands& movementCommands) const;

        private:

            Player::UPtr m_player{nullptr};

            bool m_freeFlyCamera{false};
            float m_cameraTranslationSpeed{0.1f};

            Render::MeshId m_cubeMeshId{};
            Render::MeshId m_terrainHeightMapMeshId{};
            Render::MaterialId m_solidRedMaterialId{};
            Render::MaterialId m_terrainMaterialId{};
    };
}

#endif //TESTDESKTOPAPP_TESTSCENE_H
