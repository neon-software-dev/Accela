/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "DevScene.h"
#include "CubeMesh.h"
#include "SphereMesh.h"

#include <Accela/Engine/Component/Components.h>
#include <Accela/Engine/Physics/KinematicPlayerController.h>
#include <Accela/Engine/Util/HeightMapUtil.h>

#include <sstream>

namespace Accela
{

static constexpr auto FONT_FILE_NAME = "jovanny_lemonad_bender.otf";

void DevScene::OnSceneStart(const Engine::IEngineRuntime::Ptr& engine)
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

void DevScene::ConfigureScene()
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

    // Create player entity
    m_player = *Engine::KinematicPlayerController::Create(
        engine,
        Engine::DEFAULT_PHYSICS_SCENE,
        Engine::DEFAULT_PLAYER_NAME,
        {0,7,0},
        .4f,
        1.8f
    );
}

void DevScene::CreateSceneEntities()
{
    //
    // Configuration for which entities are placed in the test world
    //

    //CreateSpotLight({0,1,0}, true);
    CreatePointLight({0, 2, 2}, true);
    CreateTerrainEntity(100.0f, {0, 0, 0});
    //CreateFloorEntity({0,0,0}, 150);

    /*CreateModelEntity(
        Engine::PRI("TestDesktopApp", "CesiumMan.glb"),
        {0,0.1f,-2},
        glm::vec3(1.0f),
        Engine::ModelAnimationState(Engine::ModelAnimationType::Looping, "")
    );*/

    CreateTreeEntity(0, {5,0,-2});

    //CreateForest(m_terrainEid, 100);
}

bool DevScene::LoadResources()
{
    //
    // Load package resources
    //
    if (!engine->GetWorldResources()->EnsurePackageResources(Engine::PackageName("TestDesktopApp"), Engine::ResultWhen::Ready).get())
    {
        return false;
    }

    //
    // Fonts
    //
    (void)engine->GetWorldResources()->Fonts()->LoadFont(Engine::PRI("TestDesktopApp", FONT_FILE_NAME), 64).get();

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

    const auto forestFloorTextureId = engine->GetWorldResources()->Textures()->LoadPackageTexture(
        Engine::PRI("TestDesktopApp", "forest_ground.jpg"),
        Engine::TextureLoadConfig {
            .uvAddressMode = Render::WRAP_ADDRESS_MODE
        },
        Engine::ResultWhen::Ready
    ).get();
    if (!forestFloorTextureId.IsValid()) { return false; }

    const auto barkTextureId = engine->GetWorldResources()->Textures()->LoadPackageTexture(
        Engine::PRI("TestDesktopApp", "bark.png"),
        { },
        Engine::ResultWhen::Ready
    ).get();
    if (!barkTextureId.IsValid()) { return false; }

    const auto ashTextureId = engine->GetWorldResources()->Textures()->LoadPackageTexture(
        Engine::PRI("TestDesktopApp", "ash.png"),
        { .numMipLevels = 4 },
        Engine::ResultWhen::Ready
    ).get();
    if (!ashTextureId.IsValid()) { return false; }

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
        1.0f, // Repeat the material texture every 1 unit in world space
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

    const glm::vec4 barkColor{ 0.835f, 0.615f, 0.388f, 1.0f};

    Engine::ObjectMaterialProperties barkMaterial{};
    barkMaterial.isAffectedByLighting = true;
    barkMaterial.ambientColor = barkColor;
    barkMaterial.ambientTexture = barkTextureId;
    barkMaterial.diffuseColor = barkColor;
    barkMaterial.diffuseTexture = barkTextureId;
    barkMaterial.specularColor = {1,1,1,1};
    barkMaterial.shininess = 0.0f;
    m_barkMaterialId = engine->GetWorldResources()->Materials()->LoadObjectMaterial(
        Engine::CRI("Bark"),
        barkMaterial,
        Engine::ResultWhen::Ready).get();
    if (!m_barkMaterialId.IsValid()) { return false; }

    Engine::ObjectMaterialProperties leafMaterial{};
    leafMaterial.isAffectedByLighting = true;
    leafMaterial.ambientColor = {1,1,1,1};
    leafMaterial.ambientTexture = ashTextureId;
    leafMaterial.diffuseColor = {1,1,1,1};
    leafMaterial.diffuseTexture = ashTextureId;
    leafMaterial.specularColor = {0, 0, 0,0};
    leafMaterial.shininess = 0.0f;
    leafMaterial.twoSided = true;
    leafMaterial.alphaMode = Render::AlphaMode::Mask;
    leafMaterial.alphaCutoff = 0.9f;
    m_leafMaterialId = engine->GetWorldResources()->Materials()->LoadObjectMaterial(
        Engine::CRI("Leaf"),
        leafMaterial,
        Engine::ResultWhen::Ready).get();
    if (!m_leafMaterialId.IsValid()) { return false; }

    Engine::ObjectMaterialProperties terrainMaterial{};
    terrainMaterial.isAffectedByLighting = true;
    terrainMaterial.ambientColor = {1,1,1,1};
    terrainMaterial.diffuseColor = {1,1,1,1};
    terrainMaterial.specularColor = {0.0f, 0.0f, 0.0f, 1.0f};
    terrainMaterial.shininess = 32.0f;
    terrainMaterial.ambientTexture = forestFloorTextureId;
    terrainMaterial.diffuseTexture = forestFloorTextureId;
    m_terrainMaterialId = engine->GetWorldResources()->Materials()->LoadObjectMaterial(
        Engine::CRI("Terrain"),
        terrainMaterial,
        Engine::ResultWhen::Ready).get();
    if (!m_terrainMaterialId.IsValid()) { return false; }

    return true;
}

Engine::ObjectMaterialProperties DevScene::DefineColorMaterial(const glm::vec4& color)
{
    Engine::ObjectMaterialProperties solidMaterial{};
    solidMaterial.isAffectedByLighting = true;
    solidMaterial.ambientColor = color;
    solidMaterial.diffuseColor = color;
    solidMaterial.specularColor = color;
    solidMaterial.shininess = 32.0f;
    return solidMaterial;
}

void DevScene::CreatePointLight(const glm::vec3& position, bool drawEntity)
{
    auto lightProperties = Render::LightProperties{};
    lightProperties.attenuationMode = Render::AttenuationMode::Linear;
    lightProperties.diffuseColor = glm::vec3(1,1,1);
    lightProperties.diffuseIntensity = glm::vec3(1,1,1);
    lightProperties.specularColor = glm::vec3(1,1,1);
    lightProperties.specularIntensity = glm::vec3(1,1,1);
    lightProperties.directionUnit = glm::vec3(0,0,-1);
    lightProperties.coneFovDegrees = 360.0f;

    CreateLight(position, drawEntity, lightProperties);
}

void DevScene::CreateSpotLight(const glm::vec3& position, bool drawEntity)
{
    auto lightProperties = Render::LightProperties{};
    lightProperties.attenuationMode = Render::AttenuationMode::Linear;
    lightProperties.diffuseColor = glm::vec3(1,1,1);
    lightProperties.diffuseIntensity = glm::vec3(1,1,1);
    lightProperties.specularColor = glm::vec3(1,1,1);
    lightProperties.specularIntensity = glm::vec3(1,1,1);
    lightProperties.directionUnit = glm::vec3(1,0,0);
    lightProperties.coneFovDegrees = 90.0f;

    CreateLight(position, drawEntity, lightProperties);
}

void DevScene::CreateLight(const glm::vec3& position, bool drawEntity, const Render::LightProperties& properties)
{
    const auto eid = engine->GetWorldState()->CreateEntity();

    auto lightComponent = Engine::LightComponent(properties);
    lightComponent.castsShadows = true;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, lightComponent);

    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(position);
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);

    if (drawEntity)
    {
        auto objectRenderableComponent = Engine::ObjectRenderableComponent{};
        objectRenderableComponent.sceneName = "default";
        objectRenderableComponent.meshId = m_sphereMeshId;
        objectRenderableComponent.materialId = m_solidWhiteMaterialId;
        objectRenderableComponent.shadowPass = false;
        Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, objectRenderableComponent);
    }

    if (m_lightEid == 0) { m_lightEid = eid; }
}

void DevScene::CreateModelEntity(const Engine::ResourceIdentifier& model,
                                 const glm::vec3& position,
                                 const glm::vec3& scale,
                                 const std::optional<Engine::ModelAnimationState>& animationState)
{
    const auto eid = engine->GetWorldState()->CreateEntity();

    auto modelRenderableComponent = Engine::ModelRenderableComponent{};
    modelRenderableComponent.modelResource = model;
    modelRenderableComponent.animationState = animationState;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, modelRenderableComponent);

    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(position);
    transformComponent.SetScale(scale);
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);
}

void DevScene::CreateFloorEntity(glm::vec3 position, float sideLength, glm::quat orientation)
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

void DevScene::CreateTerrainEntity(const float& scale, const glm::vec3& position)
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
        {Engine::PhysicsShape(Engine::PhysicsMaterial(),
        Engine::Bounds_StaticMesh(Engine::CRI("TerrainHeightMap"), false))}
    );
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, physicsComponent);

    m_terrainEid = eid;
}

void DevScene::CreateCubeEntity(glm::vec3 position,
                                 glm::vec3 scale,
                                 bool isStatic,
                                 glm::vec3 linearVelocity) const
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

    auto shape = Engine::PhysicsShape(
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

void DevScene::CreateTreeEntity(unsigned int id, const glm::vec3& pos, Engine::StandardTreeParams treeParams, Engine::TreeMeshParams meshParams)
{
    const auto tree = Engine::StandardTreeGenerator().GenerateTree(treeParams);

    const auto treeMeshParams = Engine::TreeMeshCreator::QualityBasedMeshParams(10.0f);
    const auto treeMesh = Engine::TreeMeshCreator().CreateTreeMesh(treeMeshParams, tree, std::format("Tree-{}", id));

    const auto branchesMeshRI = Engine::CRI(std::format("Branches-{}", id));

    auto branchesMeshId = engine->GetWorldResources()->Meshes()->LoadStaticMesh(
        branchesMeshRI,
        treeMesh.branchesMesh->vertices,
        treeMesh.branchesMesh->indices,
        Render::MeshUsage::Immutable,
        Engine::ResultWhen::Ready).get();
    if (!branchesMeshId.IsValid()) { return; }

    auto leavesMeshId = engine->GetWorldResources()->Meshes()->LoadStaticMesh(
        Engine::CRI(std::format("Leaves-{}", id)),
        treeMesh.leavesMesh->vertices,
        treeMesh.leavesMesh->indices,
        Render::MeshUsage::Immutable,
        Engine::ResultWhen::Ready).get();
    if (!leavesMeshId.IsValid()) { return; }

    {
        const auto eid = engine->GetWorldState()->CreateEntity();

        auto transformComponent = Engine::TransformComponent{};
        transformComponent.SetPosition(pos);
        transformComponent.SetScale(glm::vec3(1.0f));
        Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);

        auto objectRenderableComponent = Engine::ObjectRenderableComponent{};
        objectRenderableComponent.sceneName = "default";
        objectRenderableComponent.meshId = branchesMeshId;
        objectRenderableComponent.materialId = m_barkMaterialId;
        Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, objectRenderableComponent);

        Engine::PhysicsComponent physicsComponent = Engine::PhysicsComponent::StaticBody(
            Engine::DEFAULT_PHYSICS_SCENE,
            {Engine::PhysicsShape(
                Engine::PhysicsMaterial(),
                Engine::Bounds_StaticMesh(
                    branchesMeshRI,
                    true,
                    Engine::Bounds_StaticMesh::MeshSlice{
                        .verticesStartIndex = treeMesh.trunkVerticesStartIndex,
                        .verticesCount = treeMesh.trunkVerticesCount,
                        .indicesStartIndex = treeMesh.trunkIndicesStartIndex,
                        .indicesCount = treeMesh.trunkIndicesCount
                    }
                ))}
        );
        Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, physicsComponent);
    }

    {
        const auto eid = engine->GetWorldState()->CreateEntity();

        auto transformComponent = Engine::TransformComponent{};
        transformComponent.SetPosition(pos);
        transformComponent.SetScale(glm::vec3(1.0f));
        Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);

        auto objectRenderableComponent = Engine::ObjectRenderableComponent{};
        objectRenderableComponent.sceneName = "default";
        objectRenderableComponent.meshId = leavesMeshId;
        objectRenderableComponent.materialId = m_leafMaterialId;
        Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, objectRenderableComponent);
    }
}

void DevScene::CreateForest(Engine::EntityId terrainEid, unsigned int numTrees)
{
    const auto terrainTransform = Engine::GetComponent<Engine::TransformComponent>(engine->GetWorldState(), terrainEid).value();

    const auto heightMapMesh = engine->GetWorldResources()->Meshes()->GetStaticMeshData(Engine::CRI("TerrainHeightMap")).value();
    const auto heightMapData = engine->GetWorldResources()->Meshes()->GetHeightMapData(Engine::CRI("TerrainHeightMap")).value();

    for (unsigned int x = 0; x < numTrees; ++x)
    {
        const float halfWidthBounds = heightMapData.worldWidth / 2.0f;
        const float halfHeightBounds = heightMapData.worldHeight / 2.0f;

        const float xPos = std::uniform_real_distribution<float>(-halfWidthBounds, halfWidthBounds)(m_mt);
        const float zPos = std::uniform_real_distribution<float>(-halfHeightBounds, halfHeightBounds)(m_mt);
        const float yPos = Engine::QueryLoadedHeightMap(heightMapMesh, heightMapData, {xPos, zPos})->pointHeight_modelSpace;

        const glm::vec3 treePosition = (glm::vec3{xPos, yPos, zPos} * terrainTransform.GetScale()) + terrainTransform.GetPosition();

        CreateTreeEntity(x, treePosition);
    }
}

void DevScene::OnSimulationStep(unsigned int timeStep)
{
    Scene::OnSimulationStep(timeStep);

    // If we're not in command entry mode, get the currently pressed keys and apply them as movement
    // commands to either the player or camera, depending on free fly mode setting
    if (!m_commandEntryEntity)
    {
        const auto movementCommands = GetActiveMovementCommands();

        if (m_freeFlyCamera)
        {
            // Move the camera
            ApplyMovementToCamera(movementCommands);
        }
        else
        {
            // Update the player controller
            m_player->OnSimulationStep(
                movementCommands,
                engine->GetWorldState()->GetWorldCamera(Engine::DEFAULT_SCENE)->GetLookUnit()
            );

            // Sync the camera to the player's position
            engine->GetWorldState()->GetWorldCamera(Engine::DEFAULT_SCENE)->SetPosition(m_player->GetPosition());
        }
    }
}

void DevScene::OnKeyEvent(const Platform::KeyEvent& event)
{
    Scene::OnKeyEvent(event);

    // Exit the app whenever escape is pressed
    if (event.action == Platform::KeyEvent::Action::KeyPress && event.key == Platform::Key::Escape)
    {
        engine->StopEngine();
        return;
    }

    // If the command entry prompt is open, funnel key events into typing into it
    if (m_commandEntryEntity)
    {
        OnCommandEntryKeyEvent(event);
    }
    // Otherwise if command entry prompt is not open, handle key presses normally
    else
    {
        OnNormalKeyEvent(event);
    }
}

void DevScene::OnMouseMoveEvent(const Platform::MouseMoveEvent& event)
{
    Scene::OnMouseMoveEvent(event);

    // Apply mouse movements as camera view rotations
    engine->GetWorldState()->GetWorldCamera("default")->RotateBy(
        event.yRel * -0.002f,
        event.xRel * -0.002f
    );
}

void DevScene::OnMouseButtonEvent(const Platform::MouseButtonEvent& event)
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

Engine::PlayerMovement DevScene::GetActiveMovementCommands()
{
    Engine::PlayerMovement movementCommands{};

    if (engine->GetKeyboardState()->IsKeyPressed(Platform::Key::A)) {
        movementCommands.left = true;
    }
    if (engine->GetKeyboardState()->IsKeyPressed(Platform::Key::D)) {
        movementCommands.right = true;
    }
    if (engine->GetKeyboardState()->IsKeyPressed(Platform::Key::W)) {
        movementCommands.forward = true;
    }
    if (engine->GetKeyboardState()->IsKeyPressed(Platform::Key::S)) {
        movementCommands.backward = true;
    }
    if (engine->GetKeyboardState()->IsKeyPressed(Platform::Key::Control)) {
        movementCommands.down = true;
    }
    if (engine->GetKeyboardState()->IsKeyPressed(Platform::Key::Space)) {
        movementCommands.up = true;
    }
    if (engine->GetKeyboardState()->IsKeyPressed(Platform::Key::Shift)) {
        movementCommands.sprint = true;
    }

    return movementCommands;
}

void DevScene::ApplyMovementToCamera(const Engine::PlayerMovement& playerMovement) const
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

void DevScene::SyncLightToCamera() const
{
    auto lightComponent = *Engine::GetComponent<Engine::LightComponent>(engine->GetWorldState(), m_lightEid);
    lightComponent.lightProperties.directionUnit = engine->GetWorldState()->GetWorldCamera(Engine::DEFAULT_SCENE)->GetLookUnit();
    Engine::AddOrUpdateComponent(engine->GetWorldState(), m_lightEid, lightComponent);

    auto transformComponent = *Engine::GetComponent<Engine::TransformComponent>(engine->GetWorldState(), m_lightEid);
    transformComponent.SetPosition(engine->GetWorldState()->GetWorldCamera(Engine::DEFAULT_SCENE)->GetPosition());
    Engine::AddOrUpdateComponent(engine->GetWorldState(), m_lightEid, transformComponent);
}

void DevScene::ShootCubeFromCamera()
{
    //
    // Create a cube entity
    //
    const auto camera = engine->GetWorldState()->GetWorldCamera("default");
    const float shootSpeed = 10.0f; // m/s
    const auto shootVelocity = camera->GetLookUnit() * shootSpeed;

    const float scale = std::uniform_real_distribution<float>(0.1f, 0.4f)(m_mt);

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

void DevScene::OnCommandEntryKeyEvent(const Platform::KeyEvent& event)
{
    if (event.action == Platform::KeyEvent::Action::KeyPress)
    {
        // Close the command entry on tilde presses
        if (event.key == Platform::Key::BackQuote)
        {
            m_commandEntryEntity = std::nullopt;
            return;
        }

        // Close and process the command entry on enter press
        if (event.key == Platform::Key::Return)
        {
            HandleCommand((*m_commandEntryEntity)->GetEntry());
            m_commandEntryEntity = std::nullopt;
            return;
        }
        // Clear last command char on backspace press
        else if (event.key == Platform::Key::Backspace)
        {
            (*m_commandEntryEntity)->DeleteLastEntryChar();
        }
        // Otherwise, append the pressed key to the command, if its a typed key
        else if (Platform::IsTypedKey(event.key))
        {
            const auto typedKeyChar = Platform::ToTypedChar(event.key);

            (*m_commandEntryEntity)->AppendToEntry({typedKeyChar});
        }
    }
}

void DevScene::OnNormalKeyEvent(const Platform::KeyEvent& event)
{
    if (event.action == Platform::KeyEvent::Action::KeyPress)
    {
        // Exit the app when escape is pressed
        if (event.key == Platform::Key::Escape)
        {
            engine->StopEngine();
            return;
        }

        // Fullscreen and cursor lock is enabled when 1 is pressed
        if (event.key == Platform::Key::One)
        {
            engine->SetWindowFullscreen(true);
            engine->SetWindowCursorLock(true);
        }

        // Fullscreen and cursor lock is disabled when 2 is pressed
        if (event.key == Platform::Key::Two)
        {
            engine->SetWindowFullscreen(false);
            engine->SetWindowCursorLock(false);
        }

        // When C is pressed, sync the primary light's position to the camera's position
        if (event.key == Platform::Key::C)
        {
            SyncLightToCamera();
        }

        if (event.key == Platform::Key::E)
        {
            int sideLength = 5;

            for (int x = -sideLength; x < sideLength; x = x + 2)
            {
                for (int y = 0; y < sideLength; y = y + 2)
                {
                    for (int z = -sideLength; z < sideLength; z = z + 2)
                    {
                        const glm::vec3 velocity(
                            std::uniform_real_distribution<float>(-40.0f, 40.0f)(m_mt),
                            std::uniform_real_distribution<float>(1.0f, 40.0f)(m_mt),
                            std::uniform_real_distribution<float>(-40.0f, 40.0f)(m_mt)
                        );

                        CreateCubeEntity({x,y+3,z}, {1,1,1}, false, velocity);
                    }
                }
            }
        }

        if (event.key == Platform::Key::P)
        {
            if (m_perfMonitor)
            {
                m_perfMonitor = std::nullopt;
            }
            else
            {
                m_perfMonitor = Engine::EnginePerfMonitorEntity::Create(engine, GetEvents(), Engine::PRI("TestDesktopApp", FONT_FILE_NAME), 28);
            }
        }

        if (event.key == Platform::Key::BackQuote)
        {
            if (m_commandEntryEntity)
            {
                m_commandEntryEntity = std::nullopt;
            }
            else
            {
                m_commandEntryEntity = Engine::CommandEntryEntity::Create(engine, Platform::TextProperties(
                    FONT_FILE_NAME,
                    64,
                    0,
                    Platform::Color::Green(),
                    Platform::Color(0,0,0,80)
                ));
            }
        }
    }
}

void DevScene::HandleCommand(const std::string& command)
{
    // Tokenize the command
    std::vector<std::string> tokens;
    std::stringstream ss(command);
    std::string token;

    while (std::getline(ss, token, ' '))
    {
        tokens.push_back(token);
    }

    if (tokens.empty()) { return; }

    const auto k = tokens[0];

    if (k == "set")
    {
        HandleSetCommand(tokens);
    }
    else if (k == "spawn")
    {
        HandleSpawnCommand(tokens);
    }
}

void DevScene::HandleSetCommand(const std::vector<std::string>& tokens)
{
    if (tokens.size() < 3)  { return; }

    Render::RenderSettings renderSettings = engine->GetRenderSettings();

    const std::string& k = tokens[1];
    const std::string& v = tokens[2];

    if (k == "freefly")
    {
        if (v == "0") { m_freeFlyCamera = false; }
        if (v == "1") { m_freeFlyCamera = true; }
    }
    else if (k == "camera.fov")
    {
        if (tokens.size() != 3) { return; }
        engine->GetWorldState()->GetWorldCamera(Engine::DEFAULT_SCENE)->SetFovYDegrees(std::stof(v));
    }
    else if (k == "camera.fov")
    {
        if (tokens.size() != 3) { return; }
        m_cameraTranslationSpeed = std::stof(v);
    }
    else if (k == "physics.debug_render")
    {
        engine->SetPhysicsDebugRender(v == "1");
        engine->SetRenderSettings(renderSettings);
    }
    else if (k == "rs.resolution")
    {
        if (tokens.size() != 4) { return; }
        renderSettings.resolution = Render::USize(std::stoi(tokens[2]), std::stoi(tokens[3]));
        engine->SetRenderSettings(renderSettings);
    }
    else if (k == "rs.shadow_quality")
    {
        if (tokens.size() != 3) { return; }
        renderSettings.shadowQuality = static_cast<Render::QualityLevel>(std::stoi(v));
        engine->SetRenderSettings(renderSettings);
    }
    else if (k == "rs.present_scaling")
    {
        if (tokens.size() != 3) { return; }
        renderSettings.presentScaling = static_cast<Render::PresentScaling>(std::stoi(v));
        engine->SetRenderSettings(renderSettings);
    }
    else if (k == "rs.vsync")
    {
        if (tokens.size() != 3) { return; }
        if (v == "0") { renderSettings.presentMode = Render::PresentMode::Immediate; }
        if (v == "1") { renderSettings.presentMode = Render::PresentMode::VSync; }
        engine->SetRenderSettings(renderSettings);
    }
    else if (k == "rs.fif")
    {
        if (tokens.size() != 3) { return; }
        renderSettings.framesInFlight = std::stoi(v);
        engine->SetRenderSettings(renderSettings);
    }
    else if (k == "rs.objects.wireframe")
    {
        if (tokens.size() != 3) { return; }
        if (v == "0") { renderSettings.objectsWireframe = false; }
        if (v == "1") { renderSettings.objectsWireframe = true; }
        engine->SetRenderSettings(renderSettings);
    }
    else if (k == "rs.hdr")
    {
        if (tokens.size() != 3) { return; }
        if (v == "0") { renderSettings.hdr = false; }
        if (v == "1") { renderSettings.hdr = true; }
        engine->SetRenderSettings(renderSettings);
    }
    else if (k == "rs.exposure")
    {
        if (tokens.size() != 3) { return; }
        renderSettings.exposure = std::stof(v);
        engine->SetRenderSettings(renderSettings);
    }
    else if (k == "rs.gamma")
    {
        if (tokens.size() != 3) { return; }
        renderSettings.gamma = std::stof(v);
        engine->SetRenderSettings(renderSettings);
    }
    else if (k == "rs.fxaa")
    {
        if (tokens.size() != 3) { return; }
        if (v == "0") { renderSettings.fxaa = false; }
        if (v == "1") { renderSettings.fxaa = true; }
        engine->SetRenderSettings(renderSettings);
    }
}

void DevScene::HandleSpawnCommand(const std::vector<std::string>& tokens)
{
    if (tokens.size() < 2)  { return; }

    const std::string& k = tokens[1];

    if (k == "light")
    {
        CreatePointLight(engine->GetWorldState()->GetWorldCamera(Engine::DEFAULT_SCENE)->GetPosition(), true);
    }
}

}
