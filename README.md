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

## Building the Project (Curently you build engine with project)

This guide provides the necessary steps for setting up your environment, compiling the custom build tools, and finally compiling the project itself.

---

### Dependencies

You must install all the following dependencies for your specific operating system before proceeding.

#### Windows Requirements

| Tool | Recommended Installation | Notes |
| :--- | :--- | :--- |
| **Git** | Standard installer. | Used for version control and managing submodules. |
| **CMake** | Official Windows x64 Installer. | The project configuration tool. **Ensure it is added to your System `PATH`** during installation. |
| **Ninja-build** | `winget install Ninja-build.Ninja` | A fast build system generator. |
| **Clang** (LLVM) | `winget install -e --id LLVM.LLVM` | The required C++ compiler. You need **7-Zip** for the installation to succeed. Remember to add the Clang binaries to your `PATH`: `C:\Program Files\LLVM\bin`. |
| **Windows SDK** | Install via the **Visual Studio 2022 Installer**. | Provides essential system headers and libraries. |
| **Vulkan SDK** | Download from the **[LunarG website](https://www.lunarg.com/vulkan-sdk/)**. | Must include the **Validation Layers**. Other headers (`glm`, `SDL`, `volk`) are already included as submodules, so you don't need to install their SDK versions. |

---

#### Linux Requirements

| Tool | Notes |
| :--- | :--- |
| **cmake** | Standard package manager installation. |
| **ninja-build** | Standard package manager installation. |
| **clang** | The required C++ compiler. |
| **Vulkan SDK** | Standard package manager installation (with validation layers). |
| **System Libraries** | Additional libraries, primarily for **Wayland** and **X11** display support, are also required. A full list will be documented soon. |

---

### Build Steps

#### 1. Build the Build Tools

The engine uses a custom compilation utility called **`ProjectBuilder`**. You must build this tool first.

1.  Navigate to the **`Engine/BuildTools`** directory in your terminal.
2.  Run the script for your platform:
    * **Windows:** `build-windows.bat`
    * **Linux:** `./build-linux.sh`

Upon successful completion, the `ProjectBuilder` executable will be placed in the **`Engine/BuildTools/build`** directory.

---

#### 2. Build Your Project

Use the compiled `ProjectBuilder` to build your game project.

You need to provide the tool with the path to your project's root folder and specify the build type.

##### Usage

```bash
# Example for Windows (Debug Mode):
.\Engine\BuildTools\build\ProjectBuilder.exe C:\Path\To\MyProject -d

# Example for Linux (Release Mode):
./Engine/BuildTools/build/ProjectBuilder /path/to/MyProject -r
```

| Argument | Description |
| :--- | :--- |
| **< path >** | The absolute path to your project's root directory. |
| **-d** | Compiles the project in Debug mode (for development and debugging). |
| **-r** | Compiles the project in Release mode (optimized for final deployment). |

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
