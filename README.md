# Accela

<!-- Version and License Badges -->
![Version](https://img.shields.io/badge/version-0.0.1-green.svg?style=flat-square) 
![License](https://img.shields.io/badge/license-GNU%20GPLv3-green?style=flat-square) 
![Language](https://img.shields.io/badge/language-C++23-green.svg?style=flat-square) 

Accela is a cross-platform C++23 game and render engine utilizing Vulkan 1.3.

## Features

- Builds and runs on both Windows and Linux (Mac support planned)
- Create 2D, 3D and VR applications
- Dynamic lighting and shadows with deferred lighting passes
- Multiple, configurable, frames in flight rendered in parallel
- ECS system for entity management
- Asset loading system for textures, audio, fonts and models
- Loads most 3D model formats (40+)
- Node-based and skeleton/bone-based model animations
- Realistic world physics system
- Positional and global audio sources
- Height-mapped terrain generation
- Cubemap-based skybox rendering
- Arbitrary text/font rendering
- For non-game applications, the renderer can be used standalone

## Disclaimer

This project is still under **heavy development** and an official initial release has yet to be created.

Please give Accela a try (while adhering to the license terms) and provide feedback during this pre-alpha period, but please note that it is currently far from a finished product and is changing often. 

## License

Accela is currently distributed under the GPL v3 software license. Please see the LICENSE file for the legally binding details.

The intent in choosing this license was to accomplish the following:
1. Users may freely use and modify Accela in private, non-public software
2. Users may use and modify Accela in public software, whether free or commericial, but in doing so are required to also adopt GPL v3 for their own software and release that software as open source software

That being said, any user wishing to use Accela **without a GPL v3 license attached to it** may contact the owner / copyright holder of Accela to purchase such a license. This process will be standardized when Accela is offically launched.

## Screenshots

![Alt text](screenshots/vampire_dancing.png?raw=true "Vampire Dancing")
*Bone-based animated model dancing in Sponza with dynamic lighting and shadow mapping*

![Alt text](screenshots/cubes.gif?raw=true "Physics")
*A collection of entities launched into the air over height-mapped terrain with dynamic lighting*

# Quick Start Guide

## Building From Source

Accela utilizes CMake for a build system and vcpkg for dependency management.

It's highly recommended to use vcpkg as it will handle fetching all of Accela's required dependencies. Visit https://vcpkg.io/ for instructions on how to install it.

The following commands are specific to Linux but should be easy to adopt to Windows.

Pull the project code from Github:

- `git clone https://github.com/neon-software-dev/Accela`
- `cd Accela`

### Building Nvidia PhysX

Accela depends on the Nvidia PhysX library, but this library currently has a broken vcpkg package for linux consumers, so PhysX is currently included as part of the Accela source and must be manually built.

- `cd src/External/PhysX-5.3.1/physx/`
- `./generate_projects.sh`

You can choose which variants (checked, debug, profile, release) of PhysX you want to build. Only building release is fine if you don't want to debug into it. Go into the compiler directory for each variant you want, and build and install it.

- `cd compiler/linux-{variant}`
- `make`
- `make install`

When configuring CMake below to build Accela, you need to supply some paths to the PhysX variant's files as a CMake input parameter.

### Building Accela

Navigate back to Accela's root directory and execute CMake:

- `cd ../../../../../../`
- `mkdir build`
- `cd build`
- `cmake -DACCELA_TARGET_PLATFORM=Desktop -DCMAKE_TOOLCHAIN_FILE="{/path/to}/vcpkg/scripts/buildsystems/vcpkg.cmake" -DPHYSX_INSTALL_DIR="{/path/to}/Accela/src/External/PhysX-5.3.1/physx/install/linux/PhysX"
-DPHYSX_BIN_DIR="{/path/to}/Accela/src/External/PhysX-5.3.1/physx/install/linux/PhysX/bin/linux.clang/{variant}" -DCMAKE_INSTALL_PREFIX="{/desired/install/directory}" ../src/`

Note that all strings enclosed in curly braces need to be replaced with values that make sense for your build environment/choices.

Build and Install Accela

- `make`
- `make install`

If desired, run the TestDesktopApp that was built:

- `cd TestDesktopApp`
- `./TestDesktopApp`

(On Windows, you currently also need to copy the OpenAl and PhysX dlls from their build directories to the TestDesktopApp's build directory)

## Integration

Once Accela is built and installed, its public includes and libraries will be located in the previously specified CMake install directory.

A CMake-based client project can then be linked against Accela by passing the following CMake argument when configuring the client project:

- `-DCMAKE_PREFIX_PATH=/accela/install/directory/lib/cmake`

If using a different build system, point it towards the installed includes and libraries as appropriate for that build system.

# Usage

Please see the included TestDesktopApp project in src/TestDesktopApp for a full reference implementation of how to use the Accela engine.

The engine currently only supports a programmatic interface for defining scenes. In the future a graphical scene editor will be created which will provide a secondary mechanism for defining scenes.

A full usage guide and API documentation will be coming soon.

## Sample minimal Accela client

This following is a minimal, complete, code example for initializing Accela and giving it control to run a Scene which renders text on the screen and displays a loaded 3D model file.

The text is displayed using one of the helper Entity classes (ScreenTextEntity) that Accela provides. 

The model is displayed by directly manipulating the ECS system to create an entity and attach relevant components to it.

Both are valid mechanisms for defining the game world within Accela.

```
#include <Accela/Engine/EngineDesktop.h>
#include <Accela/Engine/Entity/ScreenTextEntity.h>
#include <Accela/Engine/Component/Components.h>

#include <Accela/Common/Log/StdLogger.h>
#include <Accela/Common/Metrics/InMemoryMetrics.h>

using namespace Accela;

class TestScene : public Engine::Scene
{
    public:

        std::string GetName() const override { return "TestScene"; }

        void OnSceneStart(const Engine::IEngineRuntime::Ptr& engine) override
        {
	    Scene::OnSceneStart(engine);
	    
            ConfigureScene();
            LoadResources();
            CreateTextEntity();
            CreateModelEntity();
        }

        void ConfigureScene()
        {
	    // Configure dim white light ambient lighting
            engine->GetWorldState()->SetAmbientLighting(Engine::DEFAULT_SCENE, 1.0f, {1,1,1});
        }

        void LoadResources()
        {
	    // Load a font from assets for text rendering
            engine->GetWorldResources()->Fonts()->LoadFont("font.ttf", 64).get();

	    // Load a model from assets
            engine->GetWorldResources()->Models()->LoadAssetsModel("model.obj", Engine::ResultWhen::Ready).get();
        }

        void CreateTextEntity()
        {
	    // Create a TextEntity to render text on the screen

            m_textEntity = Engine::ScreenTextEntity::Create(
                engine,
                Engine::ScreenTextEntity::Params()
                    .WithText("Hello World!")
                    .WithPosition({0, 0, 0})
                    .WithTextLayoutMode(Engine::TextLayoutMode::TopLeft)
                    .WithProperties(Platform::TextProperties(
                        "font.ttf",                     // Font file name
                        64,                             // Font size
                        0,                              // Wrap length
                        Platform::Color::Red(),         // Foreground color
                        Platform::Color::Transparent()  // Background color
                    ))
            );
        }

        void CreateModelEntity()
        {
	    // Manually create a Model to render in 3D space (alternatively, use ModelEntity)

            const auto eid = engine->GetWorldState()->CreateEntity();

            auto modelRenderableComponent = Engine::ModelRenderableComponent{};
            modelRenderableComponent.modelName = "model";
            Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, modelRenderableComponent);

            auto transformComponent = Engine::TransformComponent{};
            transformComponent.SetPosition({0,0,-2});
            Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);
        }

    private:

        Engine::ScreenTextEntity::UPtr m_textEntity;
};

int main()
{
    auto desktopEngine = Engine::EngineDesktop(
        std::make_shared<Common::StdLogger>(Common::LogLevel::Warning),
        std::make_shared<Common::InMemoryMetrics>()
    );

    if (!desktopEngine.Startup()) { return 1; }

    desktopEngine.Run(
        "AppName",
        1, // App version
        Engine::WindowParams("Window Title", Render::USize(2560, 1440)),
        Engine::VROutput::None,
        std::make_unique<TestScene>()
    );

    desktopEngine.Shutdown();

    return 0;
}


```


