# Accela

<!-- Version and License Badges -->
![Version](https://img.shields.io/badge/version-0.0.1-green.svg?style=flat-square) 
![License](https://img.shields.io/badge/license-GNU%20GPLv3-green?style=flat-square) 
![Language](https://img.shields.io/badge/language-C++23-green.svg?style=flat-square) 

Accela is a cross-platform C++23 game and render engine utilizing Vulkan 1.3.

## Features

- Builds and runs on both Windows and Linux (Mac support planned)
- Create 2D, 3D and VR (via OpenXR) applications
- ECS system for entity management
- Asset loading system for textures, audio, fonts and models
- Loads most texture formats (16+) and 3D model formats (40+)
- Dynamic lighting with deferred and forward lighting passes
- Dynamic shadows with cascaded and point-based shadow maps
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

![Alt text](screenshots/sponza.png?raw=true "Sponza")
*Obligatory Sponza render*

# Quick Start Guide

## Building The Engine From Source

### Overview

The engine is defined by a standard CMake project.

A vcpkg file is provided which will allow the engine to automatically fetch its build dependencies. You can choose whether to use vcpkg or provide these dependencies manually. The dependencies fetched via vcpkg are:
- glm
- entt
- audiofile
- assimp
- nlohmann-json
- vulkan
- vulkan-memory-allocator
- spirv-reflect
- sdl2
- sdl2-image
- sdl2-ttf
- OpenXR

A prepare_dependencies script is provided in the `external` directory which will download and build dependencies which can not be fetched from vcpkg. You can choose whether to use this script or provide these dependencies manually. The dependencies fetched via prepare_dependencies script are:
- PhysX
- OpenAL

The prepare_dependencies script also creates a project-local vcpkg installation so that you don't need to install it yourself. This can be disabled if desired.

### System Dependencies

#### Qt6

On Windows: Download and install the (LGPL / open source) development kit from the Qt website.

On Linux: Most distributions have Qt6 development files in the package management system for easy installation (e.g. the qt6-base-dev package). 

Alternatively: Build Qt from source. (Note: If building from source, the qtbase project must be built with Vulkan support or else the Accela build will fail with missing Vulkan-related Qt headers.)

### Python

Download/install python3 if you want to use the prepare_dependencies script

### Windows Developer Prompt

On Windows: The instructions below for running the prepare_dependencies script and building the project from a command prompt must be run from a Visual Studio Developer Command Prompt which has msbuild available.

### Building Accela

#### Obtain the code

Pull the project code from Github:

- `git clone https://github.com/neon-software-dev/Accela`
- `cd Accela`

#### Prepare Dependencies

If you want to use the prepare_dependencies script to provide the non-vcpkg dependenices, and to create a package-local vcpkg installation:

- `cd external`
- Windows: `prepare_dependencies.bat` , Linux: `./prepare_dependencies.sh`

Options:

- By default, the script will create a project-local vcpkg install. If you want to use your own separately installed vcpkg repo, or if you don't want to use vcpkg at all, then provide the argument `--no-local-vcpkg` when running the script.
- To remove script prompts relating to PhysX build variant, provide a `-physx-preset={variant}` argument, where variant is one of: `[linux, linux-aarch64, vc16win64, vc17win64]`

#### Configure the project

Use cmake to configure the project. 

Note that all values in braces must be filled in by you with the proper values.

- `cd ../`
- `mkdir build`
- `cd build`
- `cmake -DCMAKE_INSTALL_PREFIX="{/desired/install/directory}" ../src/`

Options:

If you want to use the project-local vcpkg install provided by the prepare_dependencies script, also provide this argument:
- `-DCMAKE_TOOLCHAIN_FILE="../external/vcpkg/scripts/buildsystems/vcpkg.cmake"`

(If using your own vcpkg install, then provide a -DCMAKE_TOOLCHAIN_FILE argument which points to your vcpkg install.)

On Linux: If you want to create a release build, also provide this argument:
- `-DCMAKE_BUILD_TYPE=Release`

On Windows: You need to point CMake to your Qt installation by providing this argument:
- `-DCMAKE_PREFIX_PATH="C:\path\to\qt\6.7.0\{variant}"`

On Windows: Configure CMake to use a Visual Studio toolchain. Ideally, also use a Visual Studio generator. If not using a Visual Studio generator, then some files that are copied to the build output during post-build step may be copied to the wrong location and will need to be relocated.

#### Build the project

- Linux: `make` Windows: `msbuild Accela.sln /p:Configuration=[Debug/Release]`

On Windows, in order to run the AccelaEditor project, you need to deploy Qt6 files to the build output directory:

- `windeployqt6.exe [--debug/--release] C:\{path\to\}Accela\{build_output_dir}`

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


