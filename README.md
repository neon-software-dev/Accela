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
- ECS system for entity management
- Asset loading system for textures, audio, fonts and models
- Loads most 3D model formats (40+)
- Node-based and skeleton/bone-based model animations
- Realistic world physics system
- Positional and global audio sources
- Height-mapped terrain generation
- Skybox rendering
- Text/font rendering
- For non-game applications, the renderer can be used standalone

## Disclaimer

This project is still under **heavy development** and an official initial release has yet to be created.

Please give Accela a try (while adhering to the license terms) and provide feedback during this pre-alpha period, but please note that it is currently far from a finished product and is changing often. 

## License

Accela is currently distributed under the GPL v3 software license. Please see the LICENSE file for the legally binding details.

The intent in choosing this license was to accomplish the following:
1. Users may freely use and modify Accela in private, non-public software
2. Users may use and modify Accela in public software, whether free or commericial, but in doing so are required to also adopt GPL v3 for their own software and release that software as free and open source software

That being said, any user wishing to use Accela **without a GPL v3 license attached to it** may contact the owner / copyright holder of Accela to purchase such a license. This process will be standardized when Accela is offically launched.

## Screenshots

![Alt text](screenshots/vampire_dancing.png?raw=true "Vampire Dancing")
*Bone-based animated model dancing in Sponza with dynamic lighting and shadow mapping*


![Alt text](screenshots/cubes.png?raw=true "Cubes")
*62,500 completely separate entities with independent entity and physics state*

![Alt text](screenshots/physics.png?raw=true "Physics")
*A collection of entities with physics state tossed on top of each other on height-mapped terrain*

# Quick Start Guide

## Building From Source

Accela utilizes CMake for a build system and vcpkg for dependency management.

It's highly recommended to use vcpkg as it will handle fetching all of Accela's required dependencies. Visit https://vcpkg.io/ for instructions on how to install it.

The following commands are specific to Linux but should be easy to adopt to Windows.

Pull the project code from Github:

- `git clone https://github.com/neon-software-dev/Accela`


Navigate to the project's source and execute CMake:

- `cd Accela`
- `mkdir build`
- `cd build`
- `cmake -DACCELA_TARGET_PLATFORM=Desktop -DCMAKE_TOOLCHAIN_FILE="/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_INSTALL_PREFIX="/desired/install/directory" ../src/`

Build and Install Accela

- `make`
- `make install`

## Integration

Once Accela is built and installed, its public includes and libraries will be located in the previously specified CMake install directory.

A CMake-based client project can then be linked against Accela by passing the following CMake argument when configuring the client project:

- `-DCMAKE_PREFIX_PATH=/accela/install/directory/lib/cmake`

If using a different build system, point it towards the installed includes and libraries as appropriate for that build system.

# Usage

Please see the included TestDesktopApp project in src/TestDesktopApp for a reference implementation of how to use the Accela engine.

A full usage guide and API documentation will be coming soon.
