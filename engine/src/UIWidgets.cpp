#include "UIWidgets.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace astron {

namespace {

ImVec2 rotPt(ImVec2 center, float x, float y, float cosA, float sinA) {
    return ImVec2(center.x + x * cosA - y * sinA, center.y + x * sinA + y * cosA);
}

void drawTiltedRingArc(
    ImDrawList* dl,
    ImVec2 center,
    float rxOuter, float ryOuter,
    float rxInner, float ryInner,
    float tiltRad,
    float aStart, float aEnd,
    int segs,
    ImU32 col
) {
    const float c = std::cos(tiltRad);
    const float s = std::sin(tiltRad);
    for (int i = 0; i < segs; ++i) {
        float t0 = aStart + (aEnd - aStart) * (static_cast<float>(i) / segs);
        float t1 = aStart + (aEnd - aStart) * (static_cast<float>(i + 1) / segs);

        ImVec2 p0 = rotPt(center, std::cos(t0) * rxOuter, std::sin(t0) * ryOuter, c, s);
        ImVec2 p1 = rotPt(center, std::cos(t1) * rxOuter, std::sin(t1) * ryOuter, c, s);
        ImVec2 p2 = rotPt(center, std::cos(t1) * rxInner, std::sin(t1) * ryInner, c, s);
        ImVec2 p3 = rotPt(center, std::cos(t0) * rxInner, std::sin(t0) * ryInner, c, s);

        dl->AddQuadFilled(p0, p1, p2, p3, col);
    }
}

} // namespace

void UIWidgets::DrawPlanetMark(ImDrawList* dl, ImVec2 center, float radius, ImU32 fgCol, ImU32 bgCol) {
    const float tilt = -24.0f * static_cast<float>(M_PI / 180.0);
    const float planetR = radius * 0.52f;
    const float rxOut = radius * 1.05f;
    const float ryOut = radius * 0.33f;
    const float rxIn  = radius * 0.72f;
    const float ryIn  = radius * 0.17f;

    // 1. Back half of Saturn ring (behind upper dome of planet)
    drawTiltedRingArc(dl, center, rxOut, ryOut, rxIn, ryIn, tilt,
                      static_cast<float>(M_PI), static_cast<float>(2.0 * M_PI), 28, fgCol);

    // 2. Dark occlusion halo around planet body + solid white planet sphere
    dl->AddCircleFilled(center, planetR + std::max(1.5f, radius * 0.07f), bgCol, 40);
    dl->AddCircleFilled(center, planetR, fgCol, 40);

    // 3. Dark gap channel before front ring sweep + front half of Saturn ring
    drawTiltedRingArc(dl, center, rxIn, ryIn, rxIn * 0.86f, ryIn * 0.72f, tilt,
                      0.08f, static_cast<float>(M_PI) - 0.08f, 24, bgCol);
    drawTiltedRingArc(dl, center, rxOut, ryOut, rxIn, ryIn, tilt,
                      -0.12f, static_cast<float>(M_PI) + 0.12f, 32, fgCol);
}

void UIWidgets::DrawWindowChrome(
    float windowWidth,
    const std::function<void()>& onMin,
    const std::function<void()>& onMax,
    const std::function<void()>& onClose
) {
    ImGui::SetCursorScreenPos(ImVec2(windowWidth - 118.0f, 6.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 22));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 255, 255, 36));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (ImGui::Button("##win_min", ImVec2(32, 26))) { if (onMin) onMin(); }
    {
        ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
        float cy = (mn.y + mx.y) * 0.5f;
        dl->AddLine(ImVec2(mn.x + 10, cy), ImVec2(mx.x - 10, cy), IM_COL32(220, 222, 228, 255), 1.6f);
    }
    ImGui::SameLine(0, 4);

    if (ImGui::Button("##win_max", ImVec2(32, 26))) { if (onMax) onMax(); }
    {
        ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
        dl->AddRect(ImVec2(mn.x + 10.5f, mn.y + 7.5f), ImVec2(mx.x - 10.5f, mx.y - 7.5f), IM_COL32(220, 222, 228, 255), 1.5f, 0, 1.5f);
    }
    ImGui::SameLine(0, 4);

    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(220, 56, 62, 220));
    if (ImGui::Button("##win_close", ImVec2(32, 26))) { if (onClose) onClose(); }
    {
        ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
        dl->AddLine(ImVec2(mn.x + 10.5f, mn.y + 7.5f), ImVec2(mx.x - 10.5f, mx.y - 7.5f), IM_COL32(235, 236, 240, 255), 1.7f);
        dl->AddLine(ImVec2(mx.x - 10.5f, mn.y + 7.5f), ImVec2(mn.x + 10.5f, mx.y - 7.5f), IM_COL32(235, 236, 240, 255), 1.7f);
    }
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
}

bool UIWidgets::ToggleSwitch(const char* strId, bool* v, bool blueActive) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const float height = 22.0f;
    const float width  = 40.0f;
    const float radius = height * 0.5f;

    bool clicked = ImGui::InvisibleButton(strId, ImVec2(width, height));
    if (clicked) *v = !*v;

    ImU32 bg = *v
        ? (blueActive ? IM_COL32(43, 121, 255, 255) : IM_COL32(240, 240, 245, 255))
        : IM_COL32(52, 54, 62, 255);
    ImU32 knob = (*v && !blueActive) ? IM_COL32(14, 15, 18, 255) : IM_COL32(255, 255, 255, 255);

    dl->AddRectFilled(p, ImVec2(p.x + width, p.y + height), bg, radius);
    float kx = *v ? (p.x + width - radius) : (p.x + radius);
    dl->AddCircleFilled(ImVec2(kx, p.y + radius), radius - 2.5f, knob, 24);
    return clicked;
}

void UIWidgets::DrawPlatformIcon(ImDrawList* dl, ImVec2 c, float size, PlatformIconKind kind) {
    const float s = size * 0.5f;
    const ImU32 w = IM_COL32(255, 255, 255, 255);
    const ImU32 dark = IM_COL32(12, 13, 16, 255);

    switch (kind) {
        case PlatformIconKind::APK: {
            // Android robot icon
            dl->AddCircleFilled(ImVec2(c.x, c.y - s * 0.16f), s * 0.44f, w, 28);
            dl->AddRectFilled(ImVec2(c.x - s * 0.50f, c.y - s * 0.16f), ImVec2(c.x + s * 0.50f, c.y + s * 0.02f), dark);
            dl->AddRectFilled(ImVec2(c.x - s * 0.44f, c.y - s * 0.12f), ImVec2(c.x + s * 0.44f, c.y + s * 0.48f), w, 4.0f);
            dl->AddRectFilled(ImVec2(c.x - s * 0.64f, c.y - s * 0.10f), ImVec2(c.x - s * 0.49f, c.y + s * 0.36f), w, 3.5f);
            dl->AddRectFilled(ImVec2(c.x + s * 0.49f, c.y - s * 0.10f), ImVec2(c.x + s * 0.64f, c.y + s * 0.36f), w, 3.5f);
            dl->AddRectFilled(ImVec2(c.x - s * 0.27f, c.y + s * 0.44f), ImVec2(c.x - s * 0.10f, c.y + s * 0.74f), w, 3.5f);
            dl->AddRectFilled(ImVec2(c.x + s * 0.10f, c.y + s * 0.44f), ImVec2(c.x + s * 0.27f, c.y + s * 0.74f), w, 3.5f);
            dl->AddLine(ImVec2(c.x - s * 0.20f, c.y - s * 0.46f), ImVec2(c.x - s * 0.34f, c.y - s * 0.68f), w, 2.2f);
            dl->AddLine(ImVec2(c.x + s * 0.20f, c.y - s * 0.46f), ImVec2(c.x + s * 0.34f, c.y - s * 0.68f), w, 2.2f);
            dl->AddCircleFilled(ImVec2(c.x - s * 0.18f, c.y - s * 0.30f), 2.2f, dark);
            dl->AddCircleFilled(ImVec2(c.x + s * 0.18f, c.y - s * 0.30f), 2.2f, dark);
            break;
        }
        case PlatformIconKind::IPA: {
            // Apple silhouette
            dl->AddCircleFilled(ImVec2(c.x - s * 0.17f, c.y + s * 0.05f), s * 0.38f, w, 28);
            dl->AddCircleFilled(ImVec2(c.x + s * 0.17f, c.y + s * 0.05f), s * 0.38f, w, 28);
            dl->AddCircleFilled(ImVec2(c.x - s * 0.14f, c.y + s * 0.28f), s * 0.30f, w, 24);
            dl->AddCircleFilled(ImVec2(c.x + s * 0.14f, c.y + s * 0.28f), s * 0.30f, w, 24);
            dl->AddCircleFilled(ImVec2(c.x + s * 0.50f, c.y + s * 0.02f), s * 0.22f, dark, 20);
            dl->AddQuadFilled(
                ImVec2(c.x + s * 0.02f, c.y - s * 0.34f),
                ImVec2(c.x + s * 0.24f, c.y - s * 0.66f),
                ImVec2(c.x + s * 0.30f, c.y - s * 0.44f),
                ImVec2(c.x + s * 0.08f, c.y - s * 0.28f),
                w
            );
            break;
        }
        case PlatformIconKind::EXE: {
            // Windows 4-pane square
            float g = 2.6f, r = s * 0.62f;
            dl->AddRectFilled(ImVec2(c.x - r, c.y - r), ImVec2(c.x - g, c.y - g), w, 2.0f);
            dl->AddRectFilled(ImVec2(c.x + g, c.y - r), ImVec2(c.x + r, c.y - g), w, 2.0f);
            dl->AddRectFilled(ImVec2(c.x - r, c.y + g), ImVec2(c.x - g, c.y + r), w, 2.0f);
            dl->AddRectFilled(ImVec2(c.x + g, c.y + g), ImVec2(c.x + r, c.y + r), w, 2.0f);
            break;
        }
        case PlatformIconKind::Linux: {
            // Native Linux terminal / binary box (>_ )
            float rx = s * 0.68f, ry = s * 0.54f;
            dl->AddRect(ImVec2(c.x - rx, c.y - ry), ImVec2(c.x + rx, c.y + ry), w, 6.0f, 0, 2.6f);
            dl->AddLine(ImVec2(c.x - rx * 0.55f, c.y - ry * 0.28f), ImVec2(c.x - rx * 0.12f, c.y + ry * 0.06f), w, 2.6f);
            dl->AddLine(ImVec2(c.x - rx * 0.55f, c.y + ry * 0.40f), ImVec2(c.x - rx * 0.12f, c.y + ry * 0.06f), w, 2.6f);
            dl->AddLine(ImVec2(c.x + rx * 0.02f, c.y + ry * 0.40f), ImVec2(c.x + rx * 0.52f, c.y + ry * 0.40f), w, 2.6f);
            break;
        }
        case PlatformIconKind::ZIP: {
            float rx = s * 0.54f, ry = s * 0.68f;
            dl->AddRectFilled(ImVec2(c.x - rx, c.y - ry), ImVec2(c.x + rx, c.y + ry), w, 5.0f);
            for (int i = 0; i < 4; ++i) {
                float yy = c.y - ry + 6.0f + i * 7.0f;
                dl->AddRectFilled(ImVec2(c.x - 4.0f, yy), ImVec2(c.x + 4.0f, yy + 4.0f), dark);
            }
            dl->AddRectFilled(ImVec2(c.x - 6.0f, c.y + 4.0f), ImVec2(c.x + 6.0f, c.y + 14.0f), dark, 2.5f);
            break;
        }
        case PlatformIconKind::Outros: {
            dl->AddCircleFilled(c, s * 0.66f, w, 32);
            dl->AddCircleFilled(ImVec2(c.x - s * 0.30f, c.y), 3.2f, dark);
            dl->AddCircleFilled(ImVec2(c.x, c.y), 3.2f, dark);
            dl->AddCircleFilled(ImVec2(c.x + s * 0.30f, c.y), 3.2f, dark);
            break;
        }
    }
}

void UIWidgets::DrawToolboxShape(ImDrawList* dl, ImVec2 c, float size, MeshKind kind) {
    const float r = size * 0.42f;
    if (kind == MeshKind::Sphere) {
        dl->AddCircleFilled(c, r, IM_COL32(168, 170, 178, 255), 32);
        dl->AddCircleFilled(ImVec2(c.x - r * 0.18f, c.y - r * 0.18f), r * 0.72f, IM_COL32(212, 214, 222, 255), 28);
        dl->AddCircleFilled(ImVec2(c.x - r * 0.32f, c.y - r * 0.32f), r * 0.34f, IM_COL32(246, 247, 250, 255), 20);
        return;
    }
    if (kind == MeshKind::Cylinder) {
        float w = r * 0.82f, h = r * 0.72f;
        dl->AddRectFilled(ImVec2(c.x - w, c.y - h * 0.7f), ImVec2(c.x + w, c.y + h * 0.85f), IM_COL32(188, 190, 198, 255));
        dl->AddRectFilled(ImVec2(c.x - w * 0.45f, c.y - h * 0.7f), ImVec2(c.x + w * 0.15f, c.y + h * 0.85f), IM_COL32(226, 228, 234, 255));
        dl->AddEllipseFilled(ImVec2(c.x, c.y + h * 0.85f), ImVec2(w, h * 0.34f), IM_COL32(175, 177, 185, 255), 0.f, 24);
        dl->AddEllipseFilled(ImVec2(c.x, c.y - h * 0.7f),  ImVec2(w, h * 0.34f), IM_COL32(242, 243, 247, 255), 0.f, 24);
        return;
    }

    // Isometric Cube ("Part") or flat "Block"
    float sy = (kind == MeshKind::Block) ? (r * 0.44f) : (r * 0.82f);
    float sx = r * 0.96f;
    float dy = r * 0.48f;

    ImVec2 top0(c.x, c.y - sy - dy * 0.5f);
    ImVec2 topR(c.x + sx, c.y - sy + dy * 0.5f);
    ImVec2 topB(c.x, c.y - sy + dy * 1.5f);
    ImVec2 topL(c.x - sx, c.y - sy + dy * 0.5f);

    ImVec2 botR(topR.x, topR.y + sy * 1.15f);
    ImVec2 botB(topB.x, topB.y + sy * 1.15f);
    ImVec2 botL(topL.x, topL.y + sy * 1.15f);

    dl->AddQuadFilled(top0, topR, topB, topL, IM_COL32(242, 243, 247, 255));
    dl->AddQuadFilled(topL, topB, botB, botL, IM_COL32(176, 178, 186, 255));
    dl->AddQuadFilled(topB, topR, botR, botB, IM_COL32(204, 206, 214, 255));
}

void UIWidgets::DrawOrientationGizmo(ImDrawList* dl, ImVec2 center, float radius, const AstronMat4& view) {
    struct AxisNode {
        const char* label;
        AstronVec3 dir;
        ImU32 col;
        float z;
        ImVec2 pos;
    };

    AxisNode axes[3] = {
        { "X", { 1, 0, 0 }, IM_COL32(235, 58, 58, 255), 0.f, center },
        { "Y", { 0, 1, 0 }, IM_COL32(62, 212, 74, 255), 0.f, center },
        { "Z", { 0, 0, 1 }, IM_COL32(48, 132, 250, 255), 0.f, center }
    };

    for (auto& a : axes) {
        float vx = view.m[0] * a.dir.x + view.m[4] * a.dir.y + view.m[8]  * a.dir.z;
        float vy = view.m[1] * a.dir.x + view.m[5] * a.dir.y + view.m[9]  * a.dir.z;
        float vz = view.m[2] * a.dir.x + view.m[6] * a.dir.y + view.m[10] * a.dir.z;
        a.z = vz;
        a.pos = ImVec2(center.x + vx * radius, center.y - vy * radius);
    }

    std::sort(std::begin(axes), std::end(axes), [](const AxisNode& a, const AxisNode& b) {
        return a.z < b.z;
    });

    for (const auto& a : axes) {
        dl->AddLine(center, a.pos, a.col, 2.4f);
        dl->AddCircleFilled(a.pos, 9.5f, a.col, 20);
        ImVec2 ts = ImGui::CalcTextSize(a.label);
        dl->AddText(ImVec2(a.pos.x - ts.x * 0.5f, a.pos.y - ts.y * 0.5f), IM_COL32(255, 255, 255, 255), a.label);
    }
}

bool UIWidgets::WhitePillButton(const char* label, ImVec2 size, float rounding) {
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(250, 250, 252, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(228, 230, 236, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(205, 208, 216, 255));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(10, 11, 14, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
    bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    return pressed;
}

bool UIWidgets::OutlineButton(const char* label, ImVec2 size, float rounding) {
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(14, 15, 19, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(26, 28, 34, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(36, 38, 46, 255));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(58, 60, 70, 255));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(236, 237, 242, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    bool pressed = ImGui::Button(label, size);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);
    return pressed;
}

} // namespace astron
