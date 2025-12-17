#  VEX (Vulkan Early eXperiences)
![Vex logo rendered in engine](/RepoStuff/LogoAlpha1.png)
A game engine built with Vulkan that aims to replicate the feel of PS1-era games. The main goal of this project is to learn graphics programming without the complexity of PBR and other modern techniques, while still building something visually appealing.
![Vex screen_shot_low_res](/RepoStuff/logo_anim.gif)

## Current Project State:
### Build in stylistic features for retro graphics
Effects like texture warping, vertex snapping, crt shader, color quantization, and more.
![ObjectTest release build 03-10-2025](/RepoStuff/style.png)
With full control:
![ObjectTest release build 03-10-2025](/RepoStuff/control.png)

### Performant Physics system implemented
Implemented Jolt for its stability and performance.
![StraterProjects/PhysicsTest](/RepoStuff/Physics.gif)

### Editor
GUI system implemented for easy asset management and scene editing.
![StraterProjects/LightingTest](/RepoStuff/editor.png)

### Different Asset packing system for debug and release builds
For easier debugging debug builds use raw asset files.
![StraterProjects/LightingTest](/RepoStuff/assets.png)

## Engine Systems
- Custom Input System: Flexible input handling supporting multiple input methods and devices
- Virtual File System: Abstracted file access supporting virtual paths, with ability to load normal files
- Transform Hierarchy: Proper object parenting with hierarchical transformation system
- Object/ECS System Hybrid: Similarly to Unreals Actor system here you can create Object c++ classes with its own components
- Scene System: Json based with abbility to load custom classes, like player/enemy/gun, as long as they register them selfs.
- Custom UI system: Json based UI system using facebook/yoga layout engine.
- Physics System: Simply implemented Jolt as its performant and stable.

## Other Notes
- Current builds are early in development. But making a game is possible.
- Currently mostly tested on Linux with AMD GPUs (both discrete and integrated), Windows builds are now possible but not every merge is tested for windows compatibility.
- Editor is in early stages, currently its mostly basic scene editor.

## Engine documentation
- Core Documentation: Detailed, doxygen-generated reference for engine modification and development. [https://odevu.github.io/VEX/](https://odevu.github.io/VEX/)
- Code Samples: Simple, system-by-system examples focused on game creation and engine usage. [https://odevu.github.io/VEX/samples/](https://odevu.github.io/VEX/samples/)

## Building the Project
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
| **SlangC** | Make sure to select it during VulkanSDK installation. | Required for shader compilation. |

---

#### Linux Requirements
| Tool | Notes |
| :--- | :--- |
| **cmake** | Standard package manager installation. |
| **ninja-build** | Standard package manager installation. |
| **clang** | The required C++ compiler. |
| **Vulkan SDK** | Standard package manager installation (with validation layers). |
| **slangc** | extranct from vulkan sdk tarball |
| **System Libraries** | Additional libraries, primarily for **Wayland** and **X11** display support, are also required. A full list will be documented soon. |

---

### Build Steps

#### 1. Build the Engine
This step will build **`Core Engine`** lib, **`ProjectBuilder`** and **`Editor`**. Later on you can use both Editor, to build in gui, or ProjectBuilder to build from cmd.

1.  Navigate to the **`Engine`** directory in your terminal.
2.  Run the script for your platform:
    * **Windows:** `build-windows.bat`
    * **Linux:** `./build-linux.sh`

Upon successful completion, `VexEditor` and `VexProjectSelector` executable will be placed in the **`Engine/Editor/bin`** and `ProjectBuilder` executable will be placed in the **`Engine/BuildTools/build`** directory.

---

#### 2. Build Your Project
Most straight forward way is to use `VexEditor` to build your game project from gui. You can use `VexProjectSelector` to select your project from files and open it in `VexEditor` or provide `VexEditor` with the path to your project's root folder. In editor eaither build desired build type from menu bar, or use RUN button on the right to build and run you project.

#### Alternative:
Use the compiled `ProjectBuilder` to build your game project from cli.
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
### Must haves to create games:
- Billboard rendering
- Particle system
- Support for animated 3D models
### Should be implemented but can make games without:
- Vertex colors for models (for AO or fog effects)
### Things that will make me happy but wont help me do games:
- offscreen rendering (to a texture for example, with support for multiple render targets)
- Better shader pipeline

## Liblaries use by engine:
All liblaries are included as git submodules. Their respective licenses may differ from engine license but all of them are on some sort of opensource license.

- [SDL3](https://github.com/libsdl-org/SDL/)
- [Vulkan Memory Allocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/)
- [Volk](https://github.com/zeux/volk/)
- [glm](https://github.com/g-truc/glm/)
- [stb](https://github.com/nothings/stb)
- [assimp](https://github.com/assimp/assimp)
- [cpptrace](https://github.com/jeremy-rifkin/cpptrace)
- [cr](https://github.com/fungos/cr)
- [entt](https://github.com/skypjack/entt)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)
- [ImReflect](https://github.com/Sven-vh/ImReflect)
- [nlohmann/json](https://github.com/nlohmann/json/)
- [yoga](https://github.com/facebook/yoga)
- [JoltPhysics](https://github.com/jrouwe/JoltPhysics)
