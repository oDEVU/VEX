#  VEX (Vulkan Early eXperiences)
![Vex logo rendered in engine](/RepoStuff/RenderedLogo.png)
A game engine built with Vulkan that aims to replicate the feel of PS1-era games. The main goal of this project is to learn graphics programming without the complexity of PBR and other modern techniques, while still building something visually appealing.
![Vex logo rendered in engine](/RepoStuff/VEX_SS_LOWRES.png)

## Current Project State:
### PS1-style graphics with the ability to enable and disable specific features
![ObjectTest release build 03-10-2025](/RepoStuff/VEX_Release.gif)

### Built-in ImGui, error handling, and debugging tools for debug builds
![ObjectTest debug build 03-10-2025](/RepoStuff/VEX_Debug.gif)

### Custom asset packing system with support for auto-registered custom classes
![ObjectTest debug build and build tools 03-10-2025](/RepoStuff/VEX_features.gif)

## Engine Systems
- Custom Input System: Flexible input handling supporting multiple input methods and devices
- Virtual File System: Abstracted file access supporting virtual paths, with ability to load normal files
- Transform Hierarchy: Proper object parenting with hierarchical transformation system
- Object/ECS System Hybrid: Similarly to Unreals Actor system here you can create Object c++ classes with its own components

## Other Notes
- Current builds are early in development and represent basic functionality
- Currently only tested on Linux with AMD GPUs (both discrete and integrated)
- An editor is planned, but requires a working engine core to do so

## Engine documentation
Engine documentation can be found under: https://odevu.github.io/VEX/
It can also be regenerated locally by using doxygen.

## Building

### Dependencies

#### Windows:
- Git
- Cmake (Recomended: Official windows_x64 installer, rememeber to make sure its added to $PATH)
- ninja-build (Recomended: 'winget install Ninja-build.Ninja')
- clang (Recomended: 'winget install -e --id LLVM.LLVM' (you need 7-zip for installation to succed) remember to add 'C:\Program Files\LLVM\bin' to the $PATH)
- Windows SDK (Recomended: Installing Visual Studio 2022)
- Vulkan SDK (with validation layers, download from lunarg.org website, here also you can choose to install glm,SDL and volk headers, there is no need for them since all that is added as submodule, but you can use them at your own risk)

#### Linux:
- cmake
- ninja-build
- clang
- Vulkan SDK (with validation layers)
- (thousands other libs mostly for wayland/x11 stuff, i will write them down later)

### Building build tools

Simply run build script for your platform (build-linux.sh/build-windows.bat) thats located inside Engine/BuildTools

### Building project

If you built BuildTools succesfully inside Engine/BuildTools/build there should be now ProjectBuilder executable, simply pass it project path and build type. (ProjectBuilder < path > <-d/-r>)

## TODO:
### Must have to create games:
- UI rendering system
- Billboard rendering
- Particle system
- Support for animated 3D models
### Should be implemented but can make games without:
- Vertex colors for models (for AO or fog effects)
- Vertex lighting implementation
### Things that will make me happy but wont help me do games:
- Frustum culling
- offscreen rendering (to a texture for example, with support for multiple render targets)
- Better shader pipeline
