#pragma once

#include <imgui.h>
#include "Engine.hpp"
#include "components/enviroment.hpp"

namespace vex {

    inline void DrawWorldSettings(Engine& engine) {
        auto env = engine.getEnvironmentSettings();
        bool changed = false;

        if (ImGui::CollapsingHeader("Shading & Retro Effects", ImGuiTreeNodeFlags_DefaultOpen)) {
            changed |= ImGui::Checkbox("Gouraud Shading", &env.gourardShading);
            changed |= ImGui::Checkbox("Vertex Jitter (Passive)", &env.passiveVertexJitter);
            changed |= ImGui::Checkbox("Vertex Snapping (Active)", &env.vertexSnapping);
            changed |= ImGui::Checkbox("Affine Texture Warping", &env.affineWarping);
            changed |= ImGui::Checkbox("Screen Color Quantization", &env.screenQuantization);
            changed |= ImGui::Checkbox("Texture Color Quantization", &env.textureQuantization);
            changed |= ImGui::Checkbox("Screen Dithering", &env.screenDither);
            changed |= ImGui::Checkbox("NTSC Artifacts", &env.ntfsArtifacts);
        }

        if (ImGui::CollapsingHeader("Lighting & Atmosphere", ImGuiTreeNodeFlags_DefaultOpen)) {
            changed |= ImGui::ColorEdit3("Clear Color", &env.clearColor.x);
            ImGui::Spacing();

            changed |= ImGui::ColorEdit3("Ambient Color", &env.ambientLight.x);
            changed |= ImGui::DragFloat("Ambient Intensity", &env.ambientLightStrength, 0.01f, 0.0f, 5.0f);

            ImGui::Spacing();
            changed |= ImGui::ColorEdit3("Sun Color", &env.sunLight.x);
            changed |= ImGui::DragFloat3("Sun Direction", &env.sunDirection.x, 0.01f, -1.0f, 1.0f);
        }

        if (changed) {
            engine.setEnvironmentSettings(env);
        }
    }
}
