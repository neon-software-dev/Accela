/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "DevScene.h"
#include "CubeMesh.h"
#include "SphereMesh.h"

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

    //CreateSpotLight({0,1,0}, true);
    CreatePointLight({2,1,2}, true);
    CreateTerrainEntity(1.0f, {0, -2.2, 0});
    CreateFloorEntity({0,0,0}, 20);
    //CreateModelEntity("dancing_vampire", {0,0,-2}, glm::vec3(1.0f),
    //                  Engine::ModelAnimationState(Engine::ModelAnimationType::Looping, "Hips"));
    //CreateModelEntity("AlphaBlendModeTest", {0,0,0});
    //CreateModelEntity("TextureSettingsTest", {0,3,0}, glm::vec3(0.5f));
    CreateModelEntity("CesiumMan", {0,0.1f,-2}, glm::vec3(1.0f),
                      Engine::ModelAnimationState(Engine::ModelAnimationType::Looping, ""));
}

bool DevScene::LoadAssets()
{
    //
    // Fonts
    //
    if (!engine->GetWorldResources()->Fonts()->LoadFont(FONT_FILE_NAME, 10, 20).get()) { return false; }

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
    if (!audio || !engine->GetWorldResources()->Audio()->RegisterAudio("sine", *audio)) { return false; }

    audio = engine->GetAssets()->ReadAudioBlocking("whoosh.wav");
    if (!audio || !engine->GetWorldResources()->Audio()->RegisterAudio("whoosh", *audio)) { return false; }

    //
    // Meshes
    //
    m_cubeMeshId = engine->GetWorldResources()->Meshes()->LoadStaticMesh(
        CubeVertices,
        CubeIndices,
        Render::MeshUsage::Immutable,
        "Cube",
        Engine::ResultWhen::Ready).get();
    if (m_cubeMeshId == Render::INVALID_ID) { return false; }

    m_sphereMeshId = engine->GetWorldResources()->Meshes()->LoadStaticMesh(
        CreateSphereMeshVertices(1.0f),
        CreateSphereMeshIndices(),
        Render::MeshUsage::Immutable,
        "Sphere",
        Engine::ResultWhen::Ready).get();
    if (m_sphereMeshId == Render::INVALID_ID) { return false; }

    //
    // Height Maps
    //
    m_terrainHeightMapMeshId = engine->GetWorldResources()->Meshes()->LoadHeightMapMesh(
        *engine->GetWorldResources()->Textures()->GetAssetTextureId("rolling_hills_heightmap.png"),
        Render::USize(300,300), // How many data points to create from the height map image
        Render::USize(100,100), // World-space x/z size of the resulting terrain mesh
        20.0f, // Constant that's multiplied against height map height values
        Render::MeshUsage::Immutable,
        "TerrainHeightMap",
        Engine::ResultWhen::Ready
    ).get();
    if (m_terrainHeightMapMeshId == Render::INVALID_ID) { return false; }

    //
    // Materials
    //
    m_solidRedMaterialId = engine->GetWorldResources()->Materials()->LoadObjectMaterial(
        MakeSolidColorMaterial({1,0,0}), "red", Engine::ResultWhen::Ready
    ).get();
    if (m_solidRedMaterialId == Render::INVALID_ID) { return false; }

    m_solidWhiteMaterialId = engine->GetWorldResources()->Materials()->LoadObjectMaterial(
        MakeSolidColorMaterial({1,1,1}), "white", Engine::ResultWhen::Ready
    ).get();
    if (m_solidWhiteMaterialId == Render::INVALID_ID) { return false; }

    const auto terrainTextureId = *engine->GetWorldResources()->Textures()->GetAssetTextureId("rolling_hills_bitmap.png");
    Render::ObjectMaterialProperties terrainMaterial{};
    terrainMaterial.isAffectedByLighting = true;
    terrainMaterial.ambientColor = {1,1,1};
    terrainMaterial.diffuseColor = {1,1,1};
    terrainMaterial.specularColor = {0.1f, 0.1f, 0.1f};
    terrainMaterial.shininess = 32.0f;
    terrainMaterial.ambientTextureBind = terrainTextureId;
    terrainMaterial.diffuseTextureBind = terrainTextureId;
    terrainMaterial.specularTextureBind = Render::TextureId{Render::INVALID_ID};
    m_terrainMaterialId = engine->GetWorldResources()->Materials()->LoadObjectMaterial(
        terrainMaterial, "terrain", Engine::ResultWhen::Ready
    ).get();
    if (m_terrainMaterialId == Render::INVALID_ID) { return false; }

    //
    // Models
    //
    if (!engine->GetWorldResources()->Models()->LoadAssetsModel("dancing_vampire", "dae", Engine::ResultWhen::Ready).get()) { return false; }
    if (!engine->GetWorldResources()->Models()->LoadAssetsModel("AlphaBlendModeTest", "glb", Engine::ResultWhen::Ready).get()) { return false; }
    if (!engine->GetWorldResources()->Models()->LoadAssetsModel("TextureSettingsTest", "glb", Engine::ResultWhen::Ready).get()) { return false; }
    if (!engine->GetWorldResources()->Models()->LoadAssetsModel("CesiumMan", "glb", Engine::ResultWhen::Ready).get()) { return false; }

    return true;
}

Render::ObjectMaterialProperties DevScene::MakeSolidColorMaterial(const glm::vec3& color)
{
    Render::ObjectMaterialProperties solidMaterial{};
    solidMaterial.isAffectedByLighting = true;
    solidMaterial.ambientColor = color;
    solidMaterial.diffuseColor = color;
    solidMaterial.specularColor = color;
    solidMaterial.shininess = 32.0f;
    solidMaterial.ambientTextureBind = Render::TextureId{Render::INVALID_ID};
    solidMaterial.diffuseTextureBind = Render::TextureId{Render::INVALID_ID};
    solidMaterial.specularTextureBind = Render::TextureId{Render::INVALID_ID};
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

    //
    // LightComponent
    //
    auto lightComponent = Engine::LightComponent(properties);
    lightComponent.castsShadows = true;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, lightComponent);

    //
    // TransformComponent
    //
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

    if (m_lightEid == 0)
    {
        m_lightEid = eid;
    }
}

void DevScene::CreateModelEntity(const std::string& modelName,
                                 const glm::vec3& position,
                                 const glm::vec3& scale,
                                 const std::optional<Engine::ModelAnimationState>& animationState)
{
    const auto eid = engine->GetWorldState()->CreateEntity();

    //
    // ModelRenderableComponent
    //
    auto modelRenderableComponent = Engine::ModelRenderableComponent{};
    modelRenderableComponent.modelName = modelName;
    modelRenderableComponent.animationState = animationState;
    Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, modelRenderableComponent);

    //
    // TransformComponent
    //
    auto transformComponent = Engine::TransformComponent{};
    transformComponent.SetPosition(position);
    transformComponent.SetScale(scale);
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
