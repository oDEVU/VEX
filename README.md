# VEX (Vulkan Early eXperiences)

![Vex logo rendered in engine](/RepoStuff/RenderedLogo.png)

 Game engine made in vulkan that aim to replicate feel of ps1 era games. Main goal of the project is to learn graphics programming without complicationg my life with PBR, etc. But still build somethink 'cool'.

## Current features:

### PS1 style graphics, with possibility to enabling and disabling certain features.

![ObjectTest release build 03-10-2025](/RepoStuff/VEX_Release.gif)

### Build in imgui, error catching and traicing for debug builds built in.

![ObjectTest debug build 03-10-2025](/RepoStuff/VEX_Debug.gif)

### Custom asset packing system with support for auto registered custom classes.

![ObjectTest debug build and build tools 03-10-2025](/RepoStuff/VEX_features.gif)

### Other

- Current builds are very early in development and are very basic.
- Currently only tested platform is Linux with AMD GPUs (dGPU and iGPU).
- Editor is in plan but first i need working engine core in place.

## TODO:

- UI rendering
- Virtual file system for asset packing
- Scene system (so object wont need ti me manually added to main.cpp, esentialy abstrasting that part with graphics editor late on)

- Vertext Colors for models (to use as ao or fog)
- Vertext lighting
- Toggable Z buffer issues (idk if its possible but still would be cool)
- Add option to reduce passive jitter (as it wasnt common in ps1 games with static objects, it was artifact of GTE)
- Add billboard rendering
- Add particle system
