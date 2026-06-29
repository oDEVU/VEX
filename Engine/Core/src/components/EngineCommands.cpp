#include "components/EngineCommands.hpp"
#include "Engine.hpp"
#include "components/DebugConsole.hpp"
#include "components/HardwareInfo.hpp"
#include "components/SceneManager.hpp"
#include <string>
#include <sstream>

namespace vex {

    /// @brief Helper to parse booleans
    /// @details accepts "1", "true", "on"
    static bool parseBool(const std::string& arg) {
        return arg == "1" || arg == "true" || arg == "on";
    }

    /// @brief Helper to parse Vec3
    static glm::vec3 parseVec3(const std::vector<std::string>& args, size_t startIndex) {
        if (args.size() < startIndex + 3) return glm::vec3(0.0f);
        return glm::vec3(std::stof(args[startIndex]), std::stof(args[startIndex+1]), std::stof(args[startIndex+2]));
    }

    void RegisterEngineCommands(Engine* engine) {
        auto& console = DebugConsole::Get();

        // sys commands
        console.RegisterCommand("sys_info", [](auto args) {
            vex::log(LogLevel::INFO, "--- SYSTEM ---");
            vex::log(LogLevel::INFO, " CPU: %s", HardwareInfo::GetCPUName().c_str());
            vex::log(LogLevel::INFO, " RAM: %s", HardwareInfo::GetSystemMemory().c_str());
            vex::log(LogLevel::INFO, " AVX2 Support: %s", HardwareInfo::HasAVX2() ? "YES" : "NO");

            vex::log(LogLevel::INFO, "--- GPU ---");
            vex::log(LogLevel::INFO, " Device: %s", HardwareInfo::GetGPUName().c_str());
            vex::log(LogLevel::INFO, " Driver: %s", HardwareInfo::GetDriverVersion().c_str());

            vex::log(LogLevel::INFO, "--- VULKAN API ---");
            vex::log(LogLevel::INFO, " Device Version: %s", HardwareInfo::GetVulkanDeviceVersion().c_str());
            vex::log(LogLevel::INFO, " Requested Version: %s", HardwareInfo::GetVulkanRequestedVersion().c_str());

            VulkanFeatures feats = HardwareInfo::GetVulkanFeatures();
            vex::log(LogLevel::INFO, "--- ENGINE FEATURES ---");
            vex::log(LogLevel::INFO, " Multi-Draw Indirect: %s", feats.multiDraw ? "ENABLED" : "DISABLED");
            vex::log(LogLevel::INFO, " Indirect Draw:       %s", feats.indirectDraw ? "ENABLED" : "DISABLED");
            vex::log(LogLevel::INFO, " Bindless Textures:   %s", feats.bindlessTextures ? "ENABLED" : "DISABLED");
            vex::log(LogLevel::INFO, " Shader Draw Params:  %s", feats.shaderDrawParameters ? "ENABLED" : "DISABLED");

            vex::log(LogLevel::INFO, " Build Hash: %s", Engine::GetBuildHash());
        });

        console.RegisterCommand("sys_fps_max", [engine](auto args) {
            if (args.empty()) {
                vex::log(LogLevel::INFO, "Current FPS Limit: %d", engine->getFrameLimit());
            } else {
                int limit = std::stoi(args[0]);
                engine->setFrameLimit(limit);
                vex::log(LogLevel::INFO, "FPS Limit set to %d", limit);
            }
        });

        console.RegisterCommand("sys_vsync", [engine](auto args) {
            if (args.empty()) {
                vex::log(LogLevel::INFO, "VSync is %s", engine->getVSync() ? "ON" : "OFF");
            } else {
                bool enable = parseBool(args[0]);
                engine->setVSync(enable);
                vex::log(LogLevel::INFO, "VSync set to %s", enable ? "ON" : "OFF");
            }
        });

        console.RegisterCommand("quit", [engine](auto args) {
            engine->quit();
        });

        // scene commands
        console.RegisterCommand("scene_load", [engine](auto args) {
            if (args.empty()) {
                vex::log(LogLevel::ERROR, "Usage: scene_load <filename.json>");
                return;
            }
            std::string path = args[0];
            engine->getSceneManager()->loadScene(path, *engine);
            vex::log(LogLevel::INFO, "Loading scene: %s", path.c_str());
        });

        console.RegisterCommand("scene_list", [engine](auto args) {
            auto scenes = engine->getSceneManager()->GetAllSceneNames();
            vex::log(LogLevel::INFO, "--- Loaded Scenes ---");
            for (const auto& name : scenes) {
                vex::log(LogLevel::INFO, " > %s", name.c_str());
            }
        });

        console.RegisterCommand("scene_reload", [engine](auto args) {
            engine->prepareScenesForHotReload();
            std::string last = engine->getSceneManager()->getLastSceneName();
            if (!last.empty()) {
                engine->getSceneManager()->loadScene(last, *engine);
                vex::log(LogLevel::INFO, "Reloaded scene: %s", last.c_str());
            }
        });

        // render commands
        auto modifyEnv = [engine](std::function<void(environment&)> modFunc) {
            environment env = engine->getEnvironmentSettings();
            modFunc(env);
            engine->setEnvironmentSettings(env);
        };

        console.RegisterCommand("r_ps1_jitter", [modifyEnv](auto args) {
            if (args.empty()) return;
            bool val = parseBool(args[0]);
            modifyEnv([val](environment& e) {
                e.passiveVertexJitter = val;
                e.vertexSnapping = val;
            });
            vex::log(LogLevel::INFO, "PS1 Jitter/Snapping: %s", val ? "ON" : "OFF");
        });

        console.RegisterCommand("r_ps1_warp", [modifyEnv](auto args) {
            if (args.empty()) return;
            bool val = parseBool(args[0]);
            modifyEnv([val](environment& e) { e.affineWarping = val; });
            vex::log(LogLevel::INFO, "Affine Warping: %s", val ? "ON" : "OFF");
        });

        console.RegisterCommand("r_dither", [modifyEnv](auto args) {
            if (args.empty()) return;
            bool val = parseBool(args[0]);
            modifyEnv([val](environment& e) { e.screenDither = val; });
            vex::log(LogLevel::INFO, "Screen Dither: %s", val ? "ON" : "OFF");
        });

        console.RegisterCommand("r_crt", [modifyEnv](auto args) {
            if (args.empty()) return;
            bool val = parseBool(args[0]);
            modifyEnv([val](environment& e) { e.ntfsArtifacts = val; });
            vex::log(LogLevel::INFO, "CRT Artifacts: %s", val ? "ON" : "OFF");
        });

        console.RegisterCommand("r_resolution_mode", [engine](auto args) {
            if (args.empty()) return;
            int mode = std::stoi(args[0]);
            engine->setResolutionMode(static_cast<ResolutionMode>(mode));
            vex::log(LogLevel::INFO, "Resolution Mode set to: %d", mode);
        });

        console.RegisterCommand("r_ambient", [modifyEnv](auto args) {
            if (args.size() < 4) {
                vex::log(LogLevel::ERROR, "Usage: r_ambient <r> <g> <b> <strength>");
                return;
            }
            glm::vec3 color = parseVec3(args, 0);
            float strength = std::stof(args[3]);
            modifyEnv([color, strength](environment& e) {
                e.ambientLight = color;
                e.ambientLightStrength = strength;
            });
            vex::log(LogLevel::INFO, "Ambient light updated");
        });

        // physics commands
        console.RegisterCommand("phys_debug", [engine](auto args) {
            if (args.empty()) return;
            bool val = parseBool(args[0]);
            engine->setRenderPhysicsDebug(val);
            vex::log(LogLevel::INFO, "Physics Debug Draw: %s", val ? "ON" : "OFF");
        });

        console.RegisterCommand("phys_gravity", [engine](auto args) {
            if (args.size() < 3) {
                vex::log(LogLevel::ERROR, "Usage: phys_gravity <x> <y> <z>");
                return;
            }
            glm::vec3 grav = parseVec3(args, 0);
            if (auto* phys = engine->getPhysicsSystem()) {
                phys->SetGravityVector(grav);
                vex::log(LogLevel::INFO, "Gravity set to [%.2f, %.2f, %.2f]", grav.x, grav.y, grav.z);
            }
        });

        console.RegisterCommand("phys_steps", [engine](auto args) {
            if (args.empty()) return;
            int steps = std::stoi(args[0]);
            if (auto* phys = engine->getPhysicsSystem()) {
                phys->setCollisionSteps(steps);
                vex::log(LogLevel::INFO, "Physics steps set to %d", steps);
            }
        });
    }
}
