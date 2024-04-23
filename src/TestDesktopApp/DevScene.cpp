/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "DevScene.h"
#include "CubeMesh.h"

#include <Accela/Engine/Component/Components.h>

#include <sstream>

namespace Accela
{

static constexpr auto FONT_FILE_NAME = "jovanny_lemonad_bender.otf";

void DevScene::OnSceneStart(const Engine::IEngineRuntime::Ptr& engine)
{
    Scene::OnSceneStart(engine);

    if (!LoadAssets())
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
    engine->GetWorldState()->SetAmbientLighting(Engine::DEFAULT_SCENE, 0.1f, glm::vec3(1));

    // Display a skybox
    engine->GetWorldState()->SetSkyBox(Engine::DEFAULT_SCENE, m_skyBoxTextureId);

    // Create player entity
    m_player = Player::Create(engine, Engine::DEFAULT_SCENE, GetEvents(), {0,0.5f,1});
}

void DevScene::CreateSceneEntities()
{
    //
    // Configuration for which entities are placed in the test world
    //

    CreateLight({0,1,1});
    CreateTerrainEntity(1.0f, {0, -5, 0});
    CreateFloorEntity({0,0,0}, 10);
    CreateVampireEntity({0,0,-2});
}

bool DevScene::LoadAssets()
{
    //
    // Fonts
    //
    if (!engine->GetWorldResources()->LoadFontBlocking(FONT_FILE_NAME, 10, 20)) { return false; }

    //
    // Textures
    //
    if (!engine->GetWorldResources()->Textures()->LoadAllAssetTextures(Engine::ResultWhen::Ready).get()) { return false; }

    const std::array<std::string, 6> skyBoxFileNames = {"skybox_right.jpg", "skybox_left.jpg", "skybox_top.jpg", "skybox_bottom.jpg", "skybox_front.jpg", "skybox_back.jpg"};
    m_skyBoxTextureId = engine->GetWorldResources()->Textures()->LoadAssetCubeTexture(skyBoxFileNames, "skybox", Engine::ResultWhen::Ready).get();
    if (m_skyBoxTextureId == Render::INVALID_ID) { return false; }

    //
    // Audio
    //
    auto audio = engine->GetAssets()->ReadAudioBlocking("sine.wav");
    if (!audio || !engine->GetWorldResources()->RegisterAudio("sine", *audio)) { return false; }

    audio = engine->GetAssets()->ReadAudioBlocking("whoosh.wav");
    if (!audio || !engine->GetWorldResources()->RegisterAudio("whoosh", *audio)) { return false; }

    //
    // Meshes
    //
    m_cubeMeshId = engine->GetWorldResources()->RegisterStaticMesh(CubeVertices,
                                                                   CubeIndices,
                                                                   Render::MeshUsage::Immutable,
                                                                   "Cube");
    if (m_cubeMeshId == Render::INVALID_ID) { return false; }

    //
    // Height Maps
    //
    m_terrainHeightMapMeshId = engine->GetWorldResources()->GenerateHeightMapMesh(
        *engine->GetWorldResources()->Textures()->GetAssetTextureId("rolling_hills_heightmap.png"),
        Render::USize(300,300), // How many data points to create from the height map image
        Render::USize(100,100), // World-space x/z size of the resulting terrain mesh
        20.0f, // Constant that's multiplied against height map height values
        Render::MeshUsage::Immutable,
        "TerrainHeightMap"
    );
    if (m_terrainHeightMapMeshId == Render::INVALID_ID) { return false; }

    //
    // Materials
    //
    Render::ObjectMaterialProperties solidRedMaterial{};
    solidRedMaterial.isAffectedByLighting = true;
    solidRedMaterial.ambientColor = {0.1f,0.0f,0.0f};
    solidRedMaterial.diffuseColor = {0.1f,0,0};
    solidRedMaterial.specularColor = {0.1f,0,0};
    solidRedMaterial.shininess = 32.0f;
    solidRedMaterial.ambientTextureBind = Render::TextureId{Render::INVALID_ID};
    solidRedMaterial.diffuseTextureBind = Render::TextureId{Render::INVALID_ID};
    solidRedMaterial.specularTextureBind = Render::TextureId{Render::INVALID_ID};
    m_solidRedMaterialId = engine->GetWorldResources()->RegisterObjectMaterial(solidRedMaterial, "solidRed");
    if (m_solidRedMaterialId == Render::INVALID_ID) { return false; }

    const auto terrainTextureId = *engine->GetWorldResources()->Textures()->GetAssetTextureId("rolling_hills_bitmap.png");
    Render::ObjectMaterialProperties terrainMaterial{};
    terrainMaterial.isAffectedByLighting = true;
    terrainMaterial.ambientColor = {0.0f, 0.0f, 0.0f};
    terrainMaterial.diffuseColor = {0.0f, 0.0f, 0.0f};
    terrainMaterial.specularColor = {0.1f, 0.1f, 0.1f};
    terrainMaterial.shininess = 32.0f;
    terrainMaterial.ambientTextureBind = terrainTextureId;
    terrainMaterial.diffuseTextureBind = terrainTextureId;
    terrainMaterial.specularTextureBind = Render::TextureId{Render::INVALID_ID};
    m_terrainMaterialId = engine->GetWorldResources()->RegisterObjectMaterial(terrainMaterial, "terrain");
    if (m_terrainMaterialId == Render::INVALID_ID) { return false; }

    //
    // Models
    //
    const auto model = engine->GetAssets()->ReadModelBlocking("dancing_vampire", ".dae");
    if (!model || !engine->GetWorldResources()->RegisterModel("dancing_vampire", *model)) { return false; }

    return true;
}

void DevScene::CreateLight(const glm::vec3& position)
{
    const auto eid = engine->GetWorldState()->CreateEntity();

    //
    // LightComponent
    //
    auto lightBaseProperties = Render::LightBaseProperties{};
    lightBaseProperties.attenuationMode = Render::AttenuationMode::Linear;
    lightBaseProperties.diffuseColor = glm::vec3(1,1,1);
    lightBaseProperties.diffuseIntensity = glm::vec3(1,1,1);
    lightBaseProperties.specularColor = glm::vec3(1,1,1);
    lightBaseProperties.specularIntensity = glm::vec3(1,1,1);

    auto lightTypeProperties = Render::LightProperties_PointLight{};

    auto lightComponent = Engine::LightComponent(
        Render::LightProperties(lightBaseProperties, lightTypeProperties)
    );
    lightComponent.castsShadows = true;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, lightComponent);

    //
    // TransformComponent
    //
    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(position);
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);

    if (m_lightEid == 0)
    {
        m_lightEid = eid;
    }
}

void DevScene::CreateVampireEntity(const glm::vec3& position)
{
    const auto eid = engine->GetWorldState()->CreateEntity();

    //
    // ModelRenderableComponent
    //
    auto modelRenderableComponent = Engine::ModelRenderableComponent{};
    modelRenderableComponent.sceneName = "default";
    modelRenderableComponent.modelName = "dancing_vampire";
    modelRenderableComponent.animationState = Engine::ModelAnimationState(Engine::ModelAnimationType::Looping, "Hips");
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, modelRenderableComponent);

    //
    // TransformComponent
    //
    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(position);
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);
}

void DevScene::CreateFloorEntity(glm::vec3 position,
                                  float sideLength,
                                  glm::quat orientation)
{
    const auto eid = engine->GetWorldState()->CreateEntity();

    //
    // ObjectRenderableComponent
    //
    auto objectRenderableComponent = Engine::ObjectRenderableComponent{};
    objectRenderableComponent.sceneName = "default";
    objectRenderableComponent.meshId = m_cubeMeshId;
    objectRenderableComponent.materialId = m_solidRedMaterialId;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, objectRenderableComponent);

    //
    // TransformComponent
    //
    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(position);
    transformComponent.SetScale(glm::vec3{sideLength, 0.1f, sideLength});
    transformComponent.SetOrientation(orientation);
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);

    //
    // PhysicsComponent
    //
    Engine::PhysicsComponent physicsComponent = Engine::PhysicsComponent::StaticBody();
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, physicsComponent);

    //
    // BoundsComponent
    //
    auto boundsComponent = Engine::BoundsComponent(
        Engine::Bounds_AABB(
            glm::vec3{-0.5f, -0.5f, -0.5f},
            glm::vec3{0.5f, 0.5f, 0.5f}
        )
    );
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, boundsComponent);
}

void DevScene::CreateTerrainEntity(const float& scale, const glm::vec3& position)
{
    const auto eid = engine->GetWorldState()->CreateEntity();

    //
    // ObjectRenderableComponent
    //
    auto objectRenderableComponent = Engine::ObjectRenderableComponent{};
    objectRenderableComponent.meshId = m_terrainHeightMapMeshId;
    objectRenderableComponent.materialId = m_terrainMaterialId;
    objectRenderableComponent.shadowPass = true;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, objectRenderableComponent);

    //
    // TransformComponent
    //
    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(position);
    transformComponent.SetScale({scale, 1, scale});
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);

    //
    // PhysicsComponent
    //
    Engine::PhysicsComponent physicsComponent = Engine::PhysicsComponent::StaticBody();
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, physicsComponent);

    //
    // BoundsComponent
    //
    auto boundsComponent = Engine::BoundsComponent(Engine::Bounds_HeightMap(m_terrainHeightMapMeshId));
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, boundsComponent);
}

void DevScene::CreateCubeEntity(glm::vec3 position,
                                 glm::vec3 scale,
                                 bool isStatic,
                                 glm::vec3 linearVelocity) const
{
    const auto eid = engine->GetWorldState()->CreateEntity();

    //
    // ObjectRenderableComponent
    //
    auto objectRenderableComponent = Engine::ObjectRenderableComponent{};
    objectRenderableComponent.sceneName = "default";
    objectRenderableComponent.meshId = m_cubeMeshId;
    objectRenderableComponent.materialId = m_solidRedMaterialId;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, objectRenderableComponent);

    //
    // TransformComponent
    //
    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(position);
    transformComponent.SetScale(scale);
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);

    //
    // PhysicsComponent
    //
    Engine::PhysicsComponent physicsComponent;

    if (isStatic)
    {
        physicsComponent = Engine::PhysicsComponent::StaticBody();
    }
    else
    {
        physicsComponent = Engine::PhysicsComponent::DynamicBody(3.0f);
    }

    physicsComponent.linearVelocity = linearVelocity;
    physicsComponent.frictionCoefficient = 0.4f;
    physicsComponent.linearDamping = 0.4f;
    physicsComponent.angularDamping = 0.4f;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, physicsComponent);

    //
    // BoundsComponent
    //
    auto boundsComponent = Engine::BoundsComponent(
        Engine::Bounds_AABB(
            glm::vec3{-0.5f, -0.5f, -0.5f},
            glm::vec3{0.5f, 0.5f, 0.5f}
        )
    );
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, boundsComponent);
}

void DevScene::OnSimulationStep(unsigned int timeStep)
{
    Scene::OnSimulationStep(timeStep);

    // If we're not in command entry mode, get the currently pressed keys and apply them as movement
    // commands to either the player or camera, depending on free fly mode setting
    if (!m_commandEntryEntity)
    {
        if (m_freeFlyCamera)
        {
            // Move the camera
            ApplyMovementToCamera(GetActiveMovementCommands());
        }
        else
        {
            // Move the player
            ApplyMovementToPlayer(GetActiveMovementCommands());
        }
    }

    // If we're not free flying, sync the camera position to the player position
    if (!m_freeFlyCamera)
    {
        // Sync the camera to the player's position
        engine->GetWorldState()->GetWorldCamera(Engine::DEFAULT_SCENE)->SetPosition(m_player->GetEyesPosition());
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
        (float)event.yRel * -0.002f,
        (float)event.xRel * -0.002f
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

MovementCommands DevScene::GetActiveMovementCommands()
{
    MovementCommands movementCommands{};

    if (engine->GetKeyboardState()->IsKeyPressed(Platform::Key::A)) {
        movementCommands.SetLeft();
    }
    if (engine->GetKeyboardState()->IsKeyPressed(Platform::Key::D)) {
        movementCommands.SetRight();
    }
    if (engine->GetKeyboardState()->IsKeyPressed(Platform::Key::W)) {
        movementCommands.SetForward();
    }
    if (engine->GetKeyboardState()->IsKeyPressed(Platform::Key::S)) {
        movementCommands.SetBackward();
    }
    if (engine->GetKeyboardState()->IsKeyPressed(Platform::Key::LeftControl)) {
        movementCommands.SetDown();
    }
    if (engine->GetKeyboardState()->IsKeyPressed(Platform::Key::Space)) {
        movementCommands.SetUp();
    }

    return movementCommands;
}

void DevScene::ApplyMovementToPlayer(const MovementCommands& movementCommands) const
{
    const auto xzInput = movementCommands.GetXZNormalizedVector();

    const bool hasNonZeroMovementCommanded = xzInput.has_value();

    if (hasNonZeroMovementCommanded)
    {
        m_player->OnMovementCommanded(
            *xzInput,
            engine->GetWorldState()->GetWorldCamera(Engine::DEFAULT_SCENE)->GetLookUnit()
        );
    }
}

void DevScene::ApplyMovementToCamera(const MovementCommands& movementCommands) const
{
    const auto xyzInput = movementCommands.GetXYZNormalizedVector();

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

    const float scale = std::uniform_real_distribution<float>(0.1f, 0.4f)(m_rd);

    CreateCubeEntity(camera->GetPosition() + camera->GetLookUnit(),
                     glm::vec3{scale},
                     false,
                     shootVelocity);

    //
    // Play the whoosh sound effect
    //
    (void)engine->GetWorldState()->PlayGlobalSound("whoosh", Engine::AudioSourceProperties{});
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

        // Command the player to jump when space is pressed
        if (event.key == Platform::Key::Space)
        {
            if (!m_freeFlyCamera)
            {
                m_player->OnJumpCommanded();
            }
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

        if (event.key == Platform::Key::P)
        {
            if (m_perfMonitor)
            {
                m_perfMonitor = std::nullopt;
            }
            else
            {
                m_perfMonitor = Engine::EnginePerfMonitorEntity::Create(engine, GetEvents(), FONT_FILE_NAME);
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

    if (tokens[0] == "set")
    {
        HandleSetCommand(tokens);
    }
}

void DevScene::HandleSetCommand(const std::vector<std::string>& tokens)
{
    if (tokens.size() != 3)  { return; }

    Render::RenderSettings renderSettings = engine->GetRenderSettings();

    const std::string& k = tokens[1];
    const std::string& v = tokens[2];

    if (k == "freefly")
    {
        if (v == "0") { m_freeFlyCamera = false; }
        if (v == "1") { m_freeFlyCamera = true; }
    }
}

}
