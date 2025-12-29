#  VEX (Vulkan Early eXperiences)
![Vex logo rendered in engine](/RepoStuff/LogoAlpha1.png)
A game engine built with Vulkan that aims to replicate the feel of PS1-era games. The main goal of this project is to learn graphics programming without the complexity of PBR and other modern techniques, while still building something visually appealing.
![Vex screen_shot_low_res](/RepoStuff/logo_anim.gif)

## ⚠️ Installation Philosophy
**VEX is designed to be compiled from source.**
There are no pre-built binaries or installers. Whether you are on Windows or Linux, setting up the development environment and compiling the engine yourself is the expected workflow. This ensures you have full control over the engine and its tools.

---

## Current Project State:
### Retro Graphics Pipeline
Build-in stylistic features designed to emulate 90s hardware limitations, including affine texture warping, vertex snapping (jitter), CRT emulation, and color quantization.
![ObjectTest release build 03-10-2025](/RepoStuff/style.png)

Full control over these effects is exposed to the user:
![ObjectTest release build 03-10-2025](/RepoStuff/control.png)

### The Editor: Your Creative Hub
The VEX Editor is the primary medium for game creation. While game logic is written in C++, the Editor is where you compose scenes, manage assets, layout UI, and tweak lighting. It is designed to be the starting point for your daily workflow.
![StraterProjects/LightingTest](/RepoStuff/editor.png)

### Physics & Simulation
Integrated **Jolt Physics** for stable, performant, and industry-standard physical simulations.
![StraterProjects/PhysicsTest](/RepoStuff/Physics.gif)

### Asset & Build System
Custom packing system that differentiates between development and deployment. Debug builds use raw asset files for hot-reloading and easy iteration, while release builds pack data for efficiency.
![StraterProjects/LightingTest](/RepoStuff/assets.png)

## Engine Systems
- **Custom Input System:** Flexible handling supporting multiple input methods and devices.
- **Virtual File System:** Abstracted file access supporting virtual paths, capable of transparently loading raw files or packed archives.
- **Transform Hierarchy:** Robust object parenting with a hierarchical transformation system.
- **Object/ECS Hybrid:** A pragmatic architecture similar to Unreal's Actor system; you create Object C++ classes that hold ECS components.
- **Scene System:** JSON-based serialization with the ability to load custom user classes (Player, Enemy, Gun) via a registration system.
- **UI System:** JSON-based UI layout engine powered by **Facebook Yoga** for responsive design.

---

## Building the Project

### Dependencies
You must install all the following dependencies for your specific operating system before proceeding.

#### Windows Requirements
| Tool | Recommended Installation | Notes |
| :--- | :--- | :--- |
| **Git** | Standard installer. | Used for version control. |
| **CMake** | Windows x64 Installer. | **Ensure it is added to your System `PATH`** during installation. |
| **Ninja-build** | `winget install Ninja-build.Ninja` | A fast build system generator. |
| **Clang** (LLVM) | `winget install -e --id LLVM.LLVM` | Required C++ compiler. You need **7-Zip** for the installation to succeed. Add binaries to `PATH`: `C:\Program Files\LLVM\bin`. |
| **ImageMagick** | `winget install ImageMagick.ImageMagick` | **Required** for generating project icons (`.ico` files) during the build process. |
| **Windows SDK** | Visual Studio 2022 Installer. | Provides essential system headers. |
| **Vulkan SDK** | **[LunarG website](https://www.lunarg.com/vulkan-sdk/)** | Must include **Validation Layers** and **SlangC**. |

#### Linux Requirements
| Tool | Notes |
| :--- | :--- |
| **cmake** | Standard package manager installation. |
| **ninja-build** | Standard package manager installation. |
| **clang** | The required C++ compiler. |
| **ImageMagick** | Standard package manager (`sudo apt install imagemagick` or equivalent). |
| **Vulkan SDK** | Standard package manager (ensure validation layers are included). |
| **slangc** | Extract from Vulkan SDK tarball if not in repo. |
| **System Libraries** | Additional libraries for **Wayland** and **X11** support are required. |

---

### Build Steps

#### 1. Build the Engine & Tools
This step builds the **Core Engine** library, the **ProjectBuilder** CLI tool, and the **Editor**.

1.  Navigate to the **`Engine`** directory in your terminal.
2.  Run the script for your platform:
    * **Windows:** `build-windows.bat`
    * **Linux:** `./build-linux.sh`

Upon completion:
* `VexEditor` and `VexProjectSelector` will be in **`Engine/Editor/bin`**.
> **Note:** If you opted-in during the build process, shortcuts will be automatically created on your Desktop and Start Menu.
* `ProjectBuilder` will be in **`Engine/BuildTools/build`**.

#### 2. Create & Build Your Game
The recommended workflow is to use the **VexEditor**:
1. Launch **`VexProjectSelector`** (via the Vex Engine desktop shortcut or directly from the `bin` folder) to create a new project or select an existing one.
2. This will automatically open the **VexEditor**.
3. Inside the Editor, click the **Run** button (on the right-hand side) to compile and launch your game instantly.

#### Alternative: CLI Build
You can use the compiled `ProjectBuilder` to build your game project manually from the command line.

```bash
# Windows (Debug Mode)
.\Engine\BuildTools\build\ProjectBuilder.exe C:\Path\To\MyProject -d

# Linux (Release Mode)
./Engine/BuildTools/build/ProjectBuilder /path/to/MyProject -r
```

Argument | Description
--- | ---
`<path>` | Absolute path to your project's root directory.
`-d` | Debug mode (for development and hot-reloading).
`-r `| Release mode (optimized).
`-dist` | Distribution mode (stripped binaries without hot reloading).

## Documentation
- API Documentation: Detailed, doxygen-generated reference for engine modification and development. [https://odevu.github.io/VEX/](https://odevu.github.io/VEX/)
- Code Samples: Simple, system-by-system examples focused on game creation and engine usage. [https://odevu.github.io/VEX/samples/](https://odevu.github.io/VEX/samples/)

## Roadmap & Notes
- **Status:** Early development. Making a full game is possible but expect API changes.
- **Testing:** Primarily tested on Linux (AMD GPUs). Windows builds are functional but less rigorously tested.
- **Editor:** Work in progress, Currently focuses on Scene composition and Asset management.

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
