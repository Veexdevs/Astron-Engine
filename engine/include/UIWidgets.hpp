#pragma once
#include "../core_c/astron_math.h"
#include "Scene.hpp"
#include <imgui.h>
#include <functional>
#include <string>

namespace astron {

enum class PlatformIconKind {
    APK,
    IPA,
    EXE,
    Linux,
    ZIP,
    Outros
};

struct UIWidgets {
    // Official Astron Engine Saturn-ring planet mark (vector, crisp at any size)
    static void DrawPlanetMark(ImDrawList* dl, ImVec2 center, float radius, ImU32 fgCol = IM_COL32(255, 255, 255, 255), ImU32 bgCol = IM_COL32(8, 9, 12, 255));

    // Top-right window controls (— □ ✕)
    static void DrawWindowChrome(
        float windowWidth,
        const std::function<void()>& onMin,
        const std::function<void()>& onMax,
        const std::function<void()>& onClose
    );

    // Custom pill toggle switch
    static bool ToggleSwitch(const char* strId, bool* v, bool blueActive = true);

    // Platform vector icons for the Build screen
    static void DrawPlatformIcon(ImDrawList* dl, ImVec2 center, float size, PlatformIconKind kind);

    // 3D isometric shaded shapes for Editor "Caixa de Ferramentas"
    static void DrawToolboxShape(ImDrawList* dl, ImVec2 center, float size, MeshKind kind);

    // Top-right 3D RGB X/Y/Z orientation gizmo inside the Editor Viewport
    static void DrawOrientationGizmo(ImDrawList* dl, ImVec2 center, float radius, const AstronMat4& viewMatrix);

    // Rounded white primary button & dark outline button
    static bool WhitePillButton(const char* label, ImVec2 size, float rounding = 12.0f);
    static bool OutlineButton(const char* label, ImVec2 size, float rounding = 12.0f);
};

} // namespace astron
