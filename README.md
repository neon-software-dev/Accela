# Accela

<!-- Version and License Badges -->
![Version](https://img.shields.io/badge/version-0.0.1-green.svg?style=flat-square) 
![License](https://img.shields.io/badge/license-GNU%20GPLv3-green?style=flat-square) 
![Language](https://img.shields.io/badge/language-C++23-green.svg?style=flat-square) 

Accela is a cross-platform C++23 game and render engine utilizing Vulkan 1.3.

## Features

- Builds and runs on both Windows and Linux (Mac support planned)
- Create 2D, 3D and VR (via OpenVR) applications
- Multiple, configurable, frames in flight rendered in parallel
- ECS system for entity management
- Asset loading system for textures, audio, fonts and models
- Loads most texture formats (16+) and 3D model formats (40+)
- Dynamic lighting and shadows with deferred and forward lighting passes
- Compute-based post-processing: HDR Tone Mapping, Gamma Correction, FXAA
- Node-based and skeleton/bone-based model animations
- Realistic 3D physics simulation via PhysX
- Positional and global audio sources
- Height-mapped terrain generation
- Cubemap-based skybox rendering
- Arbitrary text/font rendering
- Renderer can be used standalone, for non-game applications

## Disclaimer

This project is still under **heavy development** and an official initial release has yet to be created.

Please give Accela a try (while adhering to the license terms) and provide feedback during this pre-alpha period, but please note that it is currently far from a finished product and is changing often. 

## License

Accela is currently distributed under the GPL v3 software license. Please see the LICENSE file for the details.

## Screenshots

![Alt text](screenshots/forest.png?raw=true "Forest")
*A forest scene with dynamically generated trees and grass*

![Alt text](screenshots/vampire_dancing.png?raw=true "Vampire Dancing")
*Bone-based animated model dancing in Sponza with dynamic lighting and shadow mapping*

# Quick Start Guide

## Building The Engine From Source

The commands listed below are specific to Linux but should be easy to adopt to Windows.

### Dependencies

#### Qt6

The AccelaEditor project depends on Qt6.

On Windows, download and install the (LGPL / open source) development kit from the Qt website.

On Linux, most distributions have Qt6 development files in the package management system for easy installation (e.g. qt6-base-dev package). Alternatively, build it from source. (Note: If building from source, the qtbase project must be built with Vulkan support or else the Accela build will fail with missing Vulkan-related Qt headers.)

#### Nvidia PhysX

The AccelaEngine project depends on PhysX.

- `git clone https://github.com/NVIDIA-Omniverse/PhysX`
- `cd PhysX/physx/`
- `./generate_projects.sh`

You can choose which variants (checked, debug, profile, release) of PhysX you want to build. Only building release is fine if you don't want to debug into it. Go into the compiler directory for each variant you want, and build and install it.

- `cd compiler/linux-{variant}`
- `make`
- `make install`

When configuring CMake for Accela (see below), you need to supply some paths to the installed PhysX build files as parameters.

### Other Dependencies

A vcpkg file is provided which will automatically supply all the remaining Accela dependencies. Visit https://vcpkg.io/ for installation instructions.

Alternatively, manually install / make available:

- glm
- entt
- audiofile
- assimp
- nlohmann-json
- vulkan
- vulkan-memory-allocator
- openvr
- sdl2
- sdl2-image
- sdl2-ttf

### Building Accela

#### Obtain the code

Pull the project code from Github:

- `git clone https://github.com/neon-software-dev/Accela`

#### Configure the project

Use cmake to configure the project. Note that all values in braces must be filled in by you with the proper values.

- `mkdir Accela/build`
- `cd Accela/build`
- `cmake -DACCELA_TARGET_PLATFORM=Desktop -DPHYSX_INSTALL_DIR="{/path/to}/PhysX-5.3.1/physx/install/linux/PhysX"
-DPHYSX_BIN_DIR="{/path/to}/PhysX-5.3.1/physx/install/linux/PhysX/bin/linux.clang/{variant}" -DCMAKE_INSTALL_PREFIX="{/desired/install/directory}" ../src/`

If using vcpkg, also append:
- `-DCMAKE_TOOLCHAIN_FILE="{/path/to}/vcpkg/scripts/buildsystems/vcpkg.cmake"`

Optional: If you want to create a release build, append:
- `-DCMAKE_BUILD_TYPE=Release`

On Windows, you may need to point CMake to your Qt installation with a parameter such as:
- `-DCMAKE_PREFIX_PATH="C:\path\to\qt\6.7.0\{variant}"`

#### Build the project

- `make`
- `make install`

If desired, run the TestDesktopApp that was built:

- `./built/TestDesktopApp`

On Windows, in order to run the AccelaEditor project, you need to also deploy Qt6 files to the build directory:

- `windeployqt6.exe [--debug/--release] C:\{path\to\}Accela\build\{variant}\built`

# Usage

Please see the included TestDesktopApp project in src/TestDesktopApp for a reference implementation of how to use the Accela engine.

The engine currently only supports a programmatic interface for defining scenes. A graphical editor (Accela Editor) is actively under development.

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

        [[nodiscard]] std::string GetName() const override { return "TestScene"; }

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
            // Configure white light ambient lighting
            engine->GetWorldState()->SetAmbientLighting(
                Engine::DEFAULT_SCENE,
                0.8f, 	// Light intensity
                {1,1,1} // Light color
            );
        }

        void LoadResources()
        {
            // Load all resources from the TestPackage package
            engine->GetWorldResources()->EnsurePackageResources(
                Engine::PackageName("TestPackage"),
                Engine::ResultWhen::Ready
            ).get();
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
                        "font.ttf", 			// Font file name
                        20, 				// Font size
                        0, 				// Wrap length
                        Platform::Color::Red(), 	// Foreground color
                        Platform::Color::Transparent() 	// Background color
                    ))
            );
        }

        void CreateModelEntity()
        {
            // Manually create a Model to render in 3D space (alternatively, use ModelEntity)

            const auto eid = engine->GetWorldState()->CreateEntity();

            auto modelRenderableComponent = Engine::ModelRenderableComponent{};
            modelRenderableComponent.modelResource = Engine::PRI("TestPackage", "model.glb");
            Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, modelRenderableComponent);

            auto transformComponent = Engine::TransformComponent{};
            transformComponent.SetPosition({0,0,-2});
            Engine::AddOrUpdateComponent(engine->GetWorldState(), eid, transformComponent);
        }

    private:

        Engine::ScreenTextEntity::UPtr m_textEntity;
};
```


