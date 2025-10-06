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

## TODO:
- UI rendering system
- Vertex colors for models (for AO or fog effects)
- Vertex lighting implementation
- Billboard rendering
- Particle system
- Support for animated 3D models
