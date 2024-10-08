/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "TestScene.h"
#include "CubeMesh.h"
#include "SphereMesh.h"

#include <Accela/Engine/Component/Components.h>
#include <Accela/Engine/Physics/KinematicPlayerController.h>

namespace Accela
{

static constexpr auto FONT_FILE_NAME = "jovanny_lemonad_bender.otf";

void TestScene::OnSceneStart(const Engine::IEngineRuntime::Ptr& engine)
{
    Scene::OnSceneStart(engine);

    if (!LoadResources())
    {
        engine->StopEngine();
        return;
    }

    ConfigureScene();
    CreateSceneEntities();
}

void TestScene::ConfigureScene()
{
    // Set the camera away from the origin, looking at the origin
    engine->GetWorldState()->SetWorldCamera(Engine::DEFAULT_SCENE, std::make_shared<Engine::Camera3D>(glm::vec3{0,1,1}));

    // Audio listener should be synced to the world camera's position
    engine->SyncAudioListenerToWorldCamera(Engine::DEFAULT_SCENE, true);

    // Configure ambient lighting levels
    engine->GetWorldState()->SetAmbientLighting(Engine::DEFAULT_SCENE, 0.02f, glm::vec3(1));

    // Display a skybox
    engine->GetWorldState()->SetSkyBox(Engine::DEFAULT_SCENE, m_skyBoxTextureId);

    // Create a physics scene
    (void)engine->GetWorldState()->GetPhysics()->CreateScene(Engine::DEFAULT_PHYSICS_SCENE, Engine::PhysicsSceneParams{});

    // Create a player entity
    m_player = *Engine::KinematicPlayerController::Create(
        engine,
        Engine::DEFAULT_PHYSICS_SCENE,
        Engine::DEFAULT_PLAYER_NAME,
        {0,7,0},
        .4f,
        1.8f
    );
}

void TestScene::CreateSceneEntities()
{
    //
    // Configuration for which entities are placed in the test world
    //

    CreateLight({0,1,1});
    CreateTerrainEntity(5.0f, {0, -2.2, 0});
    CreateFloorEntity({0,0,0}, 10);

    CreateModelEntity(
        Engine::PRI("TestDesktopApp", "CesiumMan.glb"),
        {-3,0,0},
        {1.0f, 1.0f, 1.0f},
        Engine::ModelAnimationState(Engine::ModelAnimationType::Looping, "")
    );
}

bool TestScene::LoadResources()
{
    //
    // Load package assets
    //
    if (!engine->GetWorldResources()->EnsurePackageResources(Engine::PackageName("TestDesktopApp"), Engine::ResultWhen::Ready).get())
    {
        return false;
    }

    //
    // Load textures
    //
    const std::array<Engine::PackageResourceIdentifier, 6> skyBoxResources = {
        Engine::PRI("TestDesktopApp", "skybox_right.jpg"),
        Engine::PRI("TestDesktopApp", "skybox_left.jpg"),
        Engine::PRI("TestDesktopApp", "skybox_top.jpg"),
        Engine::PRI("TestDesktopApp", "skybox_bottom.jpg"),
        Engine::PRI("TestDesktopApp", "skybox_front.jpg"),
        Engine::PRI("TestDesktopApp", "skybox_back.jpg"),
    };
    m_skyBoxTextureId = engine->GetWorldResources()->Textures()->LoadPackageCubeTexture(skyBoxResources, {}, "skybox", Engine::ResultWhen::Ready).get();
    if (!m_skyBoxTextureId.IsValid()) { return false; }

    const auto heightMapTextureId = engine->GetWorldResources()->Textures()->LoadPackageTexture(
        Engine::PRI("TestDesktopApp", "rolling_hills_heightmap.png"),
        { .numMipLevels = 1 },
        Engine::ResultWhen::Ready
    ).get();
    if (!heightMapTextureId.IsValid()) { return false; }

    const auto terrainTextureId = engine->GetWorldResources()->Textures()->LoadPackageTexture(
        Engine::PRI("TestDesktopApp", "rolling_hills_bitmap.png"),
        { .numMipLevels = 1 },
        Engine::ResultWhen::Ready
    ).get();
    if (!terrainTextureId.IsValid()) { return false; }

    //
    // Load custom meshes
    //
    m_cubeMeshId = engine->GetWorldResources()->Meshes()->LoadStaticMesh(
        Engine::CRI("Cube"),
        CubeVertices,
        CubeIndices,
        Render::MeshUsage::Immutable,
        Engine::ResultWhen::Ready).get();
    if (!m_cubeMeshId.IsValid()) { return false; }

    m_sphereMeshId = engine->GetWorldResources()->Meshes()->LoadStaticMesh(
        Engine::CRI("Sphere"),
        CreateSphereMeshVertices(1.0f),
        CreateSphereMeshIndices(),
        Render::MeshUsage::Immutable,
        Engine::ResultWhen::Ready).get();
    if (!m_sphereMeshId.IsValid()) { return false; }

    m_terrainHeightMapMeshId = engine->GetWorldResources()->Meshes()->LoadHeightMapMesh(
        Engine::CRI("TerrainHeightMap"),
        heightMapTextureId,
        Render::USize(40,40), // How many data points to create from the height map image
        Render::FSize(10.0f,10.0f), // World-space x/z size of the resulting terrain mesh
        20.0f, // Constant that's multiplied against height map height values
        std::nullopt,
        Render::MeshUsage::Immutable,
        Engine::ResultWhen::Ready).get();
    if (!m_terrainHeightMapMeshId.IsValid()) { return false; }

    //
    // Load custom materials
    //
    m_solidRedMaterialId = engine->GetWorldResources()->Materials()->LoadObjectMaterial(
        Engine::CRI("Red"),
        DefineColorMaterial({1,0,0,1}),
        Engine::ResultWhen::Ready).get();
    if (!m_solidRedMaterialId.IsValid()) { return false; }

    m_solidWhiteMaterialId = engine->GetWorldResources()->Materials()->LoadObjectMaterial(
        Engine::CRI("White"),
        DefineColorMaterial({1,1,1,1}),
        Engine::ResultWhen::Ready).get();
    if (!m_solidWhiteMaterialId.IsValid()) { return false; }

    Engine::ObjectMaterialProperties terrainMaterial{};
    terrainMaterial.isAffectedByLighting = true;
    terrainMaterial.ambientColor = {1,1,1,1};
    terrainMaterial.diffuseColor = {1,1,1,1};
    terrainMaterial.specularColor = {0.0f, 0.0f, 0.0f, 1.0f};
    terrainMaterial.shininess = 32.0f;
    terrainMaterial.ambientTexture = terrainTextureId;
    terrainMaterial.diffuseTexture = terrainTextureId;
    m_terrainMaterialId = engine->GetWorldResources()->Materials()->LoadObjectMaterial(
        Engine::CRI("Terrain"),
        terrainMaterial,
        Engine::ResultWhen::Ready).get();
    if (!m_terrainMaterialId.IsValid()) { return false; }

    return true;
}

Engine::ObjectMaterialProperties TestScene::DefineColorMaterial(const glm::vec4& color)
{
    Engine::ObjectMaterialProperties solidMaterial{};
    solidMaterial.isAffectedByLighting = true;
    solidMaterial.ambientColor = color;
    solidMaterial.diffuseColor = color;
    solidMaterial.specularColor = color;
    solidMaterial.shininess = 32.0f;
    solidMaterial.alphaMode = color.a == 1.0f ? Render::AlphaMode::Opaque : Render::AlphaMode::Blend;
    return solidMaterial;
}

void TestScene::CreateLight(const glm::vec3& position)
{
    const auto eid = engine->GetWorldState()->CreateEntity();

    auto lightProperties = Render::LightProperties{};
    lightProperties.type = Render::LightType::Point;
    lightProperties.attenuationMode = Render::AttenuationMode::Linear;
    lightProperties.diffuseColor = glm::vec3(1,1,1);
    lightProperties.diffuseIntensity = glm::vec3(1,1,1);
    lightProperties.specularColor = glm::vec3(1,1,1);
    lightProperties.specularIntensity = glm::vec3(1,1,1);

    auto lightComponent = Engine::LightComponent(Render::LightProperties(lightProperties));
    lightComponent.castsShadows = true;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, lightComponent);

    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(position);
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);

    auto objectRenderableComponent = Engine::ObjectRenderableComponent{};
    objectRenderableComponent.sceneName = "default";
    objectRenderableComponent.meshId = m_sphereMeshId;
    objectRenderableComponent.materialId = m_solidWhiteMaterialId;
    objectRenderableComponent.shadowPass = false;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, objectRenderableComponent);

    if (m_lightEid == 0) { m_lightEid = eid; }
}

void TestScene::CreateModelEntity(const Engine::ResourceIdentifier& modelResource,
                                  const glm::vec3& position,
                                  const glm::vec3& scale,
                                  const std::optional<Engine::ModelAnimationState>& animationState)
{
    const auto eid = engine->GetWorldState()->CreateEntity();

    auto modelRenderableComponent = Engine::ModelRenderableComponent{};
    modelRenderableComponent.sceneName = "default";
    modelRenderableComponent.modelResource = modelResource;
    modelRenderableComponent.animationState = animationState;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, modelRenderableComponent);

    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(position);
    transformComponent.SetScale(scale);
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);
}

void TestScene::CreateFloorEntity(glm::vec3 position, float sideLength, glm::quat orientation)
{
    const auto eid = engine->GetWorldState()->CreateEntity();

    auto objectRenderableComponent = Engine::ObjectRenderableComponent{};
    objectRenderableComponent.sceneName = "default";
    objectRenderableComponent.meshId = m_cubeMeshId;
    objectRenderableComponent.materialId = m_solidRedMaterialId;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, objectRenderableComponent);

    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(position);
    transformComponent.SetScale(glm::vec3{sideLength, 0.1f, sideLength});
    transformComponent.SetOrientation(orientation);
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);

    Engine::PhysicsComponent physicsComponent = Engine::PhysicsComponent::StaticBody(Engine::DEFAULT_PHYSICS_SCENE,
        {Engine::PhysicsShape(
        Engine::PhysicsMaterial(),
        Engine::Bounds_AABB(
            glm::vec3{-0.5f, -0.5f, -0.5f},
            glm::vec3{0.5f, 0.5f, 0.5f}
        ))}
    );
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, physicsComponent);
}

void TestScene::CreateTerrainEntity(const float& scale, const glm::vec3& position)
{
    const auto eid = engine->GetWorldState()->CreateEntity();

    auto objectRenderableComponent = Engine::ObjectRenderableComponent{};
    objectRenderableComponent.meshId = m_terrainHeightMapMeshId;
    objectRenderableComponent.materialId = m_terrainMaterialId;
    objectRenderableComponent.shadowPass = true;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, objectRenderableComponent);

    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(position);
    transformComponent.SetScale({scale, 1, scale});
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);

    Engine::PhysicsComponent physicsComponent = Engine::PhysicsComponent::StaticBody(Engine::DEFAULT_PHYSICS_SCENE,
        {Engine::PhysicsShape(
        Engine::PhysicsMaterial(),
        Engine::Bounds_StaticMesh(Engine::CRI("TerrainHeightMap"), false))}
    );
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, physicsComponent);
}

void TestScene::CreateCubeEntity(glm::vec3 position, glm::vec3 scale, bool isStatic, glm::vec3 linearVelocity) const
{
    const auto eid = engine->GetWorldState()->CreateEntity();
    auto objectRenderableComponent = Engine::ObjectRenderableComponent{};
    objectRenderableComponent.sceneName = "default";
    objectRenderableComponent.meshId = m_cubeMeshId;
    objectRenderableComponent.materialId = m_solidRedMaterialId;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, objectRenderableComponent);

    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(position);
    transformComponent.SetScale(scale);
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);

    std::optional<Engine::PhysicsComponent> physicsComponent;

    const auto shape = Engine::PhysicsShape(
        Engine::PhysicsMaterial(),
        Engine::Bounds_AABB(
        glm::vec3{-0.5f, -0.5f, -0.5f},
        glm::vec3{0.5f, 0.5f, 0.5f}
    ));

    if (isStatic)
    {
        physicsComponent = Engine::PhysicsComponent::StaticBody(Engine::DEFAULT_PHYSICS_SCENE, {shape});
    }
    else
    {
        physicsComponent = Engine::PhysicsComponent::DynamicBody(Engine::DEFAULT_PHYSICS_SCENE, {shape}, 3.0f);
    }

    physicsComponent->linearVelocity = linearVelocity;
    physicsComponent->linearDamping = 0.4f;
    physicsComponent->angularDamping = 0.4f;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, *physicsComponent);
}

void TestScene::OnSimulationStep(unsigned int timeStep)
{
    Scene::OnSimulationStep(timeStep);

    //
    // Get movement commands from key presses, and apply movement to either the free fly camera or
    // the player entity, depending on whether we're in free fly mode
    //
    const auto movementCommands = GetActiveMovementCommands();

    if (m_freeFlyCamera)
    {
        // Move the camera
        ApplyMovementToCamera(movementCommands);
    }
    else
    {
        // Move the player
        m_player->OnSimulationStep(
            movementCommands,
            engine->GetWorldState()->GetWorldCamera(Engine::DEFAULT_SCENE)->GetLookUnit()
        );

        // Sync the camera to the player's position
        engine->GetWorldState()->GetWorldCamera(Engine::DEFAULT_SCENE)->SetPosition(m_player->GetPosition());
    }
}

void TestScene::OnKeyEvent(const Platform::KeyEvent& event)
{
    Scene::OnKeyEvent(event);

    if (event.action == Platform::KeyEvent::Action::KeyPress)
    {
        // Exit the app when escape is pressed
        if (event.action == Platform::KeyEvent::Action::KeyPress && event.logicalKey == Platform::LogicalKey::Escape)
        {
            engine->StopEngine();
            return;
        }

        // Fullscreen and cursor lock is enabled when 1 is pressed
        if (event.logicalKey == Platform::LogicalKey::_1)
        {
            engine->SetWindowFullscreen(true);
            engine->SetWindowCursorLock(true);
        }

        // Fullscreen and cursor lock is disabled when 2 is pressed
        if (event.logicalKey == Platform::LogicalKey::_2)
        {
            engine->SetWindowFullscreen(false);
            engine->SetWindowCursorLock(false);
        }

        // When C is pressed, sync the primary light's position to the camera's position
        if (event.logicalKey == Platform::LogicalKey::C)
        {
            SyncLightToCamera();
        }

        if (event.logicalKey == Platform::LogicalKey::P)
        {
            if (m_perfMonitor)
            {
                m_perfMonitor = std::nullopt;
            }
            else
            {
                m_perfMonitor = Engine::EnginePerfMonitorEntity::Create(
                    engine,
                    GetEvents(),
                    Engine::PRI("TestDesktopApp",FONT_FILE_NAME),
                    28
                );
            }
        }
    }
}

void TestScene::OnMouseMoveEvent(const Platform::MouseMoveEvent& event)
{
    Scene::OnMouseMoveEvent(event);

    // Apply mouse movements as camera view rotations
    engine->GetWorldState()->GetWorldCamera("default")->RotateBy(
        event.yRel * -0.002f,
        event.xRel * -0.002f
    );
}

void TestScene::OnMouseButtonEvent(const Platform::MouseButtonEvent& event)
{
    Scene::OnMouseButtonEvent(event);

    if (event.clickType == Platform::ClickType::Press)
    {
        // Shoot a cube out when the left mouse button is clicked
        if (event.button == Platform::MouseButton::Left)
        {
            ShootCubeFromCamera();
        }
    }
}

Engine::PlayerMovement TestScene::GetActiveMovementCommands() const
{
    Engine::PlayerMovement movementCommands{};

    if (engine->GetKeyboardState()->IsPhysicalKeyPressed(Platform::PhysicalKey::A)) {
        movementCommands.left = true;
    }
    if (engine->GetKeyboardState()->IsPhysicalKeyPressed(Platform::PhysicalKey::D)) {
        movementCommands.right = true;
    }
    if (engine->GetKeyboardState()->IsPhysicalKeyPressed(Platform::PhysicalKey::W)) {
        movementCommands.forward = true;
    }
    if (engine->GetKeyboardState()->IsPhysicalKeyPressed(Platform::PhysicalKey::S)) {
        movementCommands.backward = true;
    }
    if (engine->GetKeyboardState()->IsPhysicalKeyPressed(Platform::PhysicalKey::LControl)) {
        movementCommands.down = true;
    }
    if (engine->GetKeyboardState()->IsPhysicalKeyPressed(Platform::PhysicalKey::Space)) {
        movementCommands.up = true;
    }
    if (engine->GetKeyboardState()->IsPhysicalKeyPressed(Platform::PhysicalKey::LShift)) {
        movementCommands.sprint = true;
    }

    return movementCommands;
}

void TestScene::ApplyMovementToCamera(const Engine::PlayerMovement& playerMovement) const
{
    const auto xyzInput = Engine::PlayerController::GetNormalizedXYZVector(playerMovement);

    const bool hasNonZeroMovementCommanded = xyzInput.has_value();

    if (hasNonZeroMovementCommanded)
    {
        // Translate camera move speed in the direction that was commanded
        const glm::vec3 translation = *xyzInput * m_cameraTranslationSpeed;
        engine->GetWorldState()->GetWorldCamera("default")->TranslateBy(translation);
    }
}

void TestScene::SyncLightToCamera() const
{
    auto transformComponent = *Engine::GetComponent<Engine::TransformComponent>(engine->GetWorldState(), m_lightEid);
    transformComponent.SetPosition(engine->GetWorldState()->GetWorldCamera(Engine::DEFAULT_SCENE)->GetPosition());
    Engine::AddOrUpdateComponent(engine->GetWorldState(), m_lightEid, transformComponent);
}

void TestScene::ShootCubeFromCamera()
{
    //
    // Create a cube entity
    //
    const auto camera = engine->GetWorldState()->GetWorldCamera("default");
    const float shootSpeed = 10.0f; // m/s
    const auto shootVelocity = camera->GetLookUnit() * shootSpeed;

    const float scale = std::uniform_real_distribution<float>(0.1f, 0.4f)(m_rd);

    CreateCubeEntity(camera->GetPosition() + camera->GetLookUnit(),
                     glm::vec3{scale},
                     false,
                     shootVelocity);

    //
    // Play the whoosh sound effect
    //
    (void)engine->GetWorldState()->PlayGlobalSound(
        Engine::PackageResourceIdentifier("TestDesktopApp", "whoosh.wav"),
        Engine::AudioSourceProperties{}
    );
}

}
