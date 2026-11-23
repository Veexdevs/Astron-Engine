#include "UIWidgets.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
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

ImU32 scaleAlpha(ImU32 c, float f) {
    int a = static_cast<int>(((c >> IM_COL32_A_SHIFT) & 0xFF) * f);
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    return (c & ~IM_COL32_A_MASK) | (static_cast<ImU32>(a) << IM_COL32_A_SHIFT);
}

ImU32 scaleRgb(ImU32 c, float f) {
    int r = std::clamp(static_cast<int>(((c >> IM_COL32_R_SHIFT) & 0xFF) * f), 0, 255);
    int g = std::clamp(static_cast<int>(((c >> IM_COL32_G_SHIFT) & 0xFF) * f), 0, 255);
    int b = std::clamp(static_cast<int>(((c >> IM_COL32_B_SHIFT) & 0xFF) * f), 0, 255);
    int a = (c >> IM_COL32_A_SHIFT) & 0xFF;
    return IM_COL32(r, g, b, a);
}

bool projectWorldToViewport(const AstronMat4& vp, const AstronVec3& p,
                            ImVec2 vpMin, ImVec2 vpMax, ImVec2& outScreen) {
    float cx = vp.m[0] * p.x + vp.m[4] * p.y + vp.m[8]  * p.z + vp.m[12];
    float cy = vp.m[1] * p.x + vp.m[5] * p.y + vp.m[9]  * p.z + vp.m[13];
    float cw = vp.m[3] * p.x + vp.m[7] * p.y + vp.m[11] * p.z + vp.m[15];
    if (cw <= 0.05f) return false;
    float ndcX = cx / cw;
    float ndcY = cy / cw;
    float w = vpMax.x - vpMin.x;
    float h = vpMax.y - vpMin.y;
    outScreen.x = vpMin.x + (ndcX * 0.5f + 0.5f) * w;
    outScreen.y = vpMin.y + (0.5f - ndcY * 0.5f) * h;
    return true;
}

const char* visibleLabelEnd(const char* label) {
    const char* p = std::strstr(label, "##");
    return p ? p : (label + std::strlen(label));
}

} // namespace

void UIWidgets::DrawPlanetMark(ImDrawList* dl, ImVec2 center, float radius, ImU32 fgCol, ImU32 bgCol) {
    const float tilt = -24.0f * static_cast<float>(M_PI / 180.0);
    const float planetR = radius * 0.52f;
    const float rxOut = radius * 1.05f;
    const float ryOut = radius * 0.33f;
    const float rxIn  = radius * 0.72f;
    const float ryIn  = radius * 0.17f;

    drawTiltedRingArc(dl, center, rxOut, ryOut, rxIn, ryIn, tilt,
                      static_cast<float>(M_PI), static_cast<float>(2.0 * M_PI), 28, fgCol);

    dl->AddCircleFilled(center, planetR + std::max(1.5f, radius * 0.07f), bgCol, 40);
    dl->AddCircleFilled(center, planetR, fgCol, 40);

    drawTiltedRingArc(dl, center, rxIn, ryIn, rxIn * 0.86f, ryIn * 0.72f, tilt,
                      0.08f, static_cast<float>(M_PI) - 0.08f, 24, bgCol);
    drawTiltedRingArc(dl, center, rxOut, ryOut, rxIn, ryIn, tilt,
                      -0.12f, static_cast<float>(M_PI) + 0.12f, 32, fgCol);
}

void UIWidgets::DrawIcon(ImDrawList* dl, ImVec2 c, float size, UiIcon icon, ImU32 col) {
    if (icon == UiIcon::None) return;
    const float r = size * 0.5f;
    const float sw = std::max(1.35f, size * 0.095f);

    switch (icon) {
        case UiIcon::None:
            break;

        case UiIcon::User: {
            dl->AddCircle(ImVec2(c.x, c.y - r * 0.34f), r * 0.32f, col, 18, sw);
            dl->PathArcTo(ImVec2(c.x, c.y + r * 0.78f), r * 0.68f,
                          static_cast<float>(M_PI * 1.16), static_cast<float>(M_PI * 1.84), 16);
            dl->PathStroke(col, 0, sw);
            break;
        }
        case UiIcon::Lock: {
            dl->PathArcTo(ImVec2(c.x, c.y - r * 0.18f), r * 0.34f,
                          static_cast<float>(M_PI), static_cast<float>(M_PI * 2.0), 14);
            dl->PathStroke(col, 0, sw);
            dl->AddRect(ImVec2(c.x - r * 0.52f, c.y - r * 0.16f),
                        ImVec2(c.x + r * 0.52f, c.y + r * 0.68f), col, 2.5f, 0, sw);
            dl->AddCircleFilled(ImVec2(c.x, c.y + r * 0.25f), sw * 0.9f, col);
            break;
        }
        case UiIcon::Eye:
        case UiIcon::EyeOff: {
            dl->PathArcTo(ImVec2(c.x, c.y + r * 0.52f), r * 0.88f,
                          static_cast<float>(M_PI * 1.22), static_cast<float>(M_PI * 1.78), 14);
            dl->PathStroke(col, 0, sw);
            dl->PathArcTo(ImVec2(c.x, c.y - r * 0.52f), r * 0.88f,
                          static_cast<float>(M_PI * 0.22), static_cast<float>(M_PI * 0.78), 14);
            dl->PathStroke(col, 0, sw);
            dl->AddCircle(c, r * 0.24f, col, 14, sw);
            if (icon == UiIcon::EyeOff) {
                dl->AddLine(ImVec2(c.x - r * 0.68f, c.y + r * 0.62f),
                            ImVec2(c.x + r * 0.68f, c.y - r * 0.62f), col, sw);
            }
            break;
        }
        case UiIcon::GitHub: {
            dl->AddCircleFilled(c, r * 0.82f, col, 24);
            ImU32 cut = IM_COL32(14, 15, 19, 255);
            dl->AddCircleFilled(ImVec2(c.x, c.y - r * 0.05f), r * 0.44f, cut, 18);
            dl->AddTriangleFilled(ImVec2(c.x - r * 0.42f, c.y - r * 0.18f),
                                  ImVec2(c.x - r * 0.28f, c.y - r * 0.56f),
                                  ImVec2(c.x - r * 0.08f, c.y - r * 0.34f), cut);
            dl->AddTriangleFilled(ImVec2(c.x + r * 0.42f, c.y - r * 0.18f),
                                  ImVec2(c.x + r * 0.28f, c.y - r * 0.56f),
                                  ImVec2(c.x + r * 0.08f, c.y - r * 0.34f), cut);
            dl->AddRectFilled(ImVec2(c.x - r * 0.18f, c.y + r * 0.28f),
                              ImVec2(c.x + r * 0.18f, c.y + r * 0.84f), cut, 2.0f);
            break;
        }
        case UiIcon::Home: {
            ImVec2 top(c.x, c.y - r * 0.72f);
            ImVec2 lft(c.x - r * 0.74f, c.y - r * 0.04f);
            ImVec2 rgt(c.x + r * 0.74f, c.y - r * 0.04f);
            dl->AddTriangleFilled(top, rgt, lft, col);
            dl->AddRectFilled(ImVec2(c.x - r * 0.52f, c.y - r * 0.06f),
                              ImVec2(c.x + r * 0.52f, c.y + r * 0.68f), col, 1.5f);
            dl->AddRectFilled(ImVec2(c.x - r * 0.16f, c.y + r * 0.20f),
                              ImVec2(c.x + r * 0.16f, c.y + r * 0.70f), IM_COL32(14, 15, 19, 255), 1.0f);
            break;
        }
        case UiIcon::Folder: {
            dl->AddRectFilled(ImVec2(c.x - r * 0.76f, c.y - r * 0.56f),
                              ImVec2(c.x - r * 0.06f, c.y - r * 0.24f), scaleRgb(col, 0.88f), 2.0f);
            dl->AddRectFilled(ImVec2(c.x - r * 0.76f, c.y - r * 0.34f),
                              ImVec2(c.x + r * 0.76f, c.y + r * 0.62f), col, 2.5f);
            break;
        }
        case UiIcon::Cube3D: {
            float sx = r * 0.72f, sy = r * 0.40f, h = r * 0.76f;
            ImVec2 t0(c.x, c.y - h);
            ImVec2 tR(c.x + sx, c.y - h + sy);
            ImVec2 tB(c.x, c.y - h + sy * 2.0f);
            ImVec2 tL(c.x - sx, c.y - h + sy);
            ImVec2 bR(tR.x, tR.y + h * 0.96f);
            ImVec2 bB(tB.x, tB.y + h * 0.96f);
            ImVec2 bL(tL.x, tL.y + h * 0.96f);
            dl->AddQuadFilled(t0, tR, tB, tL, scaleRgb(col, 1.08f));
            dl->AddQuadFilled(tL, tB, bB, bL, scaleRgb(col, 0.72f));
            dl->AddQuadFilled(tB, tR, bR, bB, scaleRgb(col, 0.88f));
            break;
        }
        case UiIcon::Bag: {
            dl->AddRect(ImVec2(c.x - r * 0.58f, c.y - r * 0.22f),
                        ImVec2(c.x + r * 0.58f, c.y + r * 0.70f), col, 2.5f, 0, sw);
            dl->PathArcTo(ImVec2(c.x, c.y - r * 0.22f), r * 0.30f,
                          static_cast<float>(M_PI), static_cast<float>(M_PI * 2.0), 12);
            dl->PathStroke(col, 0, sw);
            break;
        }
        case UiIcon::Gear: {
            const int teeth = 8;
            for (int i = 0; i < teeth; ++i) {
                float a = static_cast<float>(i) * static_cast<float>(M_PI * 2.0 / teeth);
                ImVec2 p0(c.x + std::cos(a) * r * 0.46f, c.y + std::sin(a) * r * 0.46f);
                ImVec2 p1(c.x + std::cos(a) * r * 0.80f, c.y + std::sin(a) * r * 0.80f);
                dl->AddLine(p0, p1, col, sw * 1.45f);
            }
            dl->AddCircleFilled(c, r * 0.58f, col, 20);
            dl->AddCircleFilled(c, r * 0.26f, IM_COL32(15, 16, 21, 255), 16);
            break;
        }
        case UiIcon::InfoCircle: {
            dl->AddCircleFilled(c, r * 0.78f, col, 22);
            ImU32 fg = IM_COL32(12, 13, 17, 255);
            dl->AddCircleFilled(ImVec2(c.x, c.y - r * 0.34f), std::max(1.3f, r * 0.13f), fg, 10);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.06f), ImVec2(c.x, c.y + r * 0.46f), fg, sw * 1.25f);
            break;
        }
        case UiIcon::Bolt: {
            ImVec2 pts[6] = {
                ImVec2(c.x + r * 0.14f, c.y - r * 0.80f),
                ImVec2(c.x - r * 0.54f, c.y + r * 0.06f),
                ImVec2(c.x - r * 0.04f, c.y + r * 0.06f),
                ImVec2(c.x - r * 0.18f, c.y + r * 0.80f),
                ImVec2(c.x + r * 0.54f, c.y - r * 0.06f),
                ImVec2(c.x + r * 0.04f, c.y - r * 0.06f)
            };
            dl->AddTriangleFilled(pts[0], pts[1], pts[2], col);
            dl->AddTriangleFilled(pts[3], pts[4], pts[5], col);
            break;
        }
        case UiIcon::Clock: {
            dl->AddCircle(c, r * 0.74f, col, 20, sw);
            dl->AddLine(c, ImVec2(c.x, c.y - r * 0.42f), col, sw);
            dl->AddLine(c, ImVec2(c.x + r * 0.34f, c.y + r * 0.16f), col, sw);
            break;
        }
        case UiIcon::Rocket: {
            dl->AddTriangleFilled(ImVec2(c.x + r * 0.74f, c.y - r * 0.74f),
                                  ImVec2(c.x - r * 0.10f, c.y - r * 0.42f),
                                  ImVec2(c.x + r * 0.42f, c.y + r * 0.10f), col);
            dl->AddQuadFilled(ImVec2(c.x - r * 0.10f, c.y - r * 0.42f),
                              ImVec2(c.x + r * 0.42f, c.y + r * 0.10f),
                              ImVec2(c.x - r * 0.14f, c.y + r * 0.54f),
                              ImVec2(c.x - r * 0.54f, c.y + r * 0.14f), col);
            dl->AddTriangleFilled(ImVec2(c.x - r * 0.42f, c.y + r * 0.24f),
                                  ImVec2(c.x - r * 0.74f, c.y + r * 0.74f),
                                  ImVec2(c.x - r * 0.24f, c.y + r * 0.42f), scaleAlpha(col, 0.7f));
            break;
        }
        case UiIcon::ArrowRight: {
            dl->AddLine(ImVec2(c.x - r * 0.64f, c.y), ImVec2(c.x + r * 0.58f, c.y), col, sw);
            dl->AddLine(ImVec2(c.x + r * 0.14f, c.y - r * 0.44f), ImVec2(c.x + r * 0.62f, c.y), col, sw);
            dl->AddLine(ImVec2(c.x + r * 0.14f, c.y + r * 0.44f), ImVec2(c.x + r * 0.62f, c.y), col, sw);
            break;
        }
        case UiIcon::Play: {
            dl->AddTriangleFilled(ImVec2(c.x - r * 0.46f, c.y - r * 0.66f),
                                  ImVec2(c.x + r * 0.66f, c.y),
                                  ImVec2(c.x - r * 0.46f, c.y + r * 0.66f), col);
            break;
        }
        case UiIcon::Pause: {
            dl->AddRectFilled(ImVec2(c.x - r * 0.50f, c.y - r * 0.62f),
                              ImVec2(c.x - r * 0.14f, c.y + r * 0.62f), col, 1.5f);
            dl->AddRectFilled(ImVec2(c.x + r * 0.14f, c.y - r * 0.62f),
                              ImVec2(c.x + r * 0.50f, c.y + r * 0.62f), col, 1.5f);
            break;
        }
        case UiIcon::Stop: {
            dl->AddRectFilled(ImVec2(c.x - r * 0.56f, c.y - r * 0.56f),
                              ImVec2(c.x + r * 0.56f, c.y + r * 0.56f), col, 2.5f);
            break;
        }
        case UiIcon::Plus: {
            dl->AddLine(ImVec2(c.x - r * 0.62f, c.y), ImVec2(c.x + r * 0.62f, c.y), col, sw * 1.25f);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.62f), ImVec2(c.x, c.y + r * 0.62f), col, sw * 1.25f);
            break;
        }
        case UiIcon::Close: {
            dl->AddLine(ImVec2(c.x - r * 0.52f, c.y - r * 0.52f), ImVec2(c.x + r * 0.52f, c.y + r * 0.52f), col, sw);
            dl->AddLine(ImVec2(c.x + r * 0.52f, c.y - r * 0.52f), ImVec2(c.x - r * 0.52f, c.y + r * 0.52f), col, sw);
            break;
        }
        case UiIcon::Pin: {
            dl->AddLine(ImVec2(c.x + r * 0.36f, c.y - r * 0.54f), ImVec2(c.x - r * 0.16f, c.y - r * 0.02f), col, sw * 1.6f);
            dl->AddLine(ImVec2(c.x - r * 0.42f, c.y - r * 0.24f), ImVec2(c.x + r * 0.24f, c.y + r * 0.42f), col, sw);
            dl->AddLine(ImVec2(c.x - r * 0.08f, c.y + r * 0.08f), ImVec2(c.x - r * 0.54f, c.y + r * 0.54f), col, sw);
            break;
        }
        case UiIcon::Search: {
            ImVec2 lc(c.x - r * 0.12f, c.y - r * 0.12f);
            dl->AddCircle(lc, r * 0.48f, col, 18, sw);
            dl->AddLine(ImVec2(lc.x + r * 0.34f, lc.y + r * 0.34f),
                        ImVec2(c.x + r * 0.66f, c.y + r * 0.66f), col, sw * 1.2f);
            break;
        }
        case UiIcon::DotsH: {
            float d = std::max(1.5f, r * 0.20f);
            dl->AddCircleFilled(ImVec2(c.x - r * 0.54f, c.y), d, col, 10);
            dl->AddCircleFilled(c, d, col, 10);
            dl->AddCircleFilled(ImVec2(c.x + r * 0.54f, c.y), d, col, 10);
            break;
        }
        case UiIcon::DotsV: {
            float d = std::max(1.4f, r * 0.18f);
            dl->AddCircleFilled(ImVec2(c.x, c.y - r * 0.52f), d, col, 10);
            dl->AddCircleFilled(c, d, col, 10);
            dl->AddCircleFilled(ImVec2(c.x, c.y + r * 0.52f), d, col, 10);
            break;
        }
        case UiIcon::CursorSelect: {
            ImVec2 tip(c.x - r * 0.46f, c.y - r * 0.74f);
            ImVec2 bL(c.x - r * 0.46f, c.y + r * 0.56f);
            ImVec2 mC(c.x - r * 0.06f, c.y + r * 0.24f);
            ImVec2 bR(c.x + r * 0.52f, c.y + r * 0.18f);
            dl->AddTriangleFilled(tip, bR, mC, col);
            dl->AddTriangleFilled(tip, mC, bL, col);
            dl->AddLine(mC, ImVec2(c.x + r * 0.20f, c.y + r * 0.72f), col, sw * 1.6f);
            break;
        }
        case UiIcon::Move4Way:
        case UiIcon::TransformGizmo: {
            float L = r * 0.76f, ah = r * 0.28f;
            dl->AddLine(ImVec2(c.x - L, c.y), ImVec2(c.x + L, c.y), col, sw);
            dl->AddLine(ImVec2(c.x, c.y - L), ImVec2(c.x, c.y + L), col, sw);
            dl->AddTriangleFilled(ImVec2(c.x, c.y - L - 1.5f), ImVec2(c.x - ah, c.y - L + ah), ImVec2(c.x + ah, c.y - L + ah), col);
            dl->AddTriangleFilled(ImVec2(c.x, c.y + L + 1.5f), ImVec2(c.x - ah, c.y + L - ah), ImVec2(c.x + ah, c.y + L - ah), col);
            dl->AddTriangleFilled(ImVec2(c.x - L - 1.5f, c.y), ImVec2(c.x - L + ah, c.y - ah), ImVec2(c.x - L + ah, c.y + ah), col);
            dl->AddTriangleFilled(ImVec2(c.x + L + 1.5f, c.y), ImVec2(c.x + L - ah, c.y - ah), ImVec2(c.x + L - ah, c.y + ah), col);
            if (icon == UiIcon::TransformGizmo) {
                dl->AddRect(ImVec2(c.x - r * 0.24f, c.y - r * 0.24f),
                            ImVec2(c.x + r * 0.24f, c.y + r * 0.24f), col, 1.0f, 0, sw * 0.85f);
            }
            break;
        }
        case UiIcon::RotateCw: {
            dl->PathArcTo(c, r * 0.66f, static_cast<float>(-M_PI * 0.25), static_cast<float>(M_PI * 1.35), 20);
            dl->PathStroke(col, 0, sw * 1.15f);
            ImVec2 tip(c.x + r * 0.56f, c.y - r * 0.42f);
            dl->AddTriangleFilled(tip,
                                  ImVec2(tip.x - r * 0.36f, tip.y - r * 0.06f),
                                  ImVec2(tip.x - r * 0.04f, tip.y - r * 0.38f), col);
            break;
        }
        case UiIcon::ScaleDiag: {
            dl->AddRect(ImVec2(c.x - r * 0.66f, c.y - r * 0.66f),
                        ImVec2(c.x + r * 0.66f, c.y + r * 0.66f), scaleAlpha(col, 0.65f), 2.5f, 0, sw);
            dl->AddLine(ImVec2(c.x - r * 0.42f, c.y + r * 0.42f),
                        ImVec2(c.x + r * 0.42f, c.y - r * 0.42f), col, sw * 1.15f);
            dl->AddLine(ImVec2(c.x + r * 0.10f, c.y - r * 0.42f), ImVec2(c.x + r * 0.42f, c.y - r * 0.42f), col, sw);
            dl->AddLine(ImVec2(c.x + r * 0.42f, c.y - r * 0.10f), ImVec2(c.x + r * 0.42f, c.y - r * 0.42f), col, sw);
            dl->AddLine(ImVec2(c.x - r * 0.10f, c.y + r * 0.42f), ImVec2(c.x - r * 0.42f, c.y + r * 0.42f), col, sw);
            dl->AddLine(ImVec2(c.x - r * 0.42f, c.y + r * 0.10f), ImVec2(c.x - r * 0.42f, c.y + r * 0.42f), col, sw);
            break;
        }
        case UiIcon::Camera: {
            dl->AddRectFilled(ImVec2(c.x - r * 0.74f, c.y - r * 0.38f),
                              ImVec2(c.x + r * 0.74f, c.y + r * 0.60f), col, 3.0f);
            dl->AddRectFilled(ImVec2(c.x - r * 0.30f, c.y - r * 0.62f),
                              ImVec2(c.x + r * 0.30f, c.y - r * 0.34f), col, 2.0f);
            dl->AddCircleFilled(ImVec2(c.x, c.y + r * 0.11f), r * 0.32f, IM_COL32(16, 18, 24, 255), 16);
            dl->AddCircle(ImVec2(c.x, c.y + r * 0.11f), r * 0.20f, col, 14, 1.3f);
            break;
        }
        case UiIcon::Lightbulb: {
            dl->AddCircleFilled(ImVec2(c.x, c.y - r * 0.16f), r * 0.48f, col, 20);
            dl->AddRectFilled(ImVec2(c.x - r * 0.24f, c.y + r * 0.24f),
                              ImVec2(c.x + r * 0.24f, c.y + r * 0.62f), scaleRgb(col, 0.80f), 1.5f);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.84f), ImVec2(c.x, c.y - r * 0.70f), col, sw);
            dl->AddLine(ImVec2(c.x - r * 0.76f, c.y - r * 0.18f), ImVec2(c.x - r * 0.62f, c.y - r * 0.18f), col, sw);
            dl->AddLine(ImVec2(c.x + r * 0.76f, c.y - r * 0.18f), ImVec2(c.x + r * 0.62f, c.y - r * 0.18f), col, sw);
            break;
        }
        case UiIcon::Mountain: {
            dl->AddTriangle(ImVec2(c.x - r * 0.16f, c.y - r * 0.64f),
                            ImVec2(c.x + r * 0.56f, c.y + r * 0.58f),
                            ImVec2(c.x - r * 0.78f, c.y + r * 0.58f), col, sw * 1.15f);
            dl->AddTriangle(ImVec2(c.x + r * 0.34f, c.y - r * 0.22f),
                            ImVec2(c.x + r * 0.80f, c.y + r * 0.58f),
                            ImVec2(c.x - r * 0.08f, c.y + r * 0.58f), scaleAlpha(col, 0.85f), sw);
            break;
        }
        case UiIcon::Puzzle: {
            dl->AddRectFilled(ImVec2(c.x - r * 0.54f, c.y - r * 0.48f),
                              ImVec2(c.x + r * 0.54f, c.y + r * 0.60f), col, 2.5f);
            dl->AddCircleFilled(ImVec2(c.x, c.y - r * 0.58f), r * 0.22f, col, 14);
            dl->AddCircleFilled(ImVec2(c.x + r * 0.62f, c.y + r * 0.06f), r * 0.22f, col, 14);
            break;
        }
        case UiIcon::Monitor: {
            dl->AddRect(ImVec2(c.x - r * 0.74f, c.y - r * 0.58f),
                        ImVec2(c.x + r * 0.74f, c.y + r * 0.34f), col, 2.5f, 0, sw);
            dl->AddLine(ImVec2(c.x, c.y + r * 0.34f), ImVec2(c.x, c.y + r * 0.66f), col, sw);
            dl->AddLine(ImVec2(c.x - r * 0.40f, c.y + r * 0.66f), ImVec2(c.x + r * 0.40f, c.y + r * 0.66f), col, sw);
            break;
        }
        case UiIcon::MusicNote: {
            dl->AddCircleFilled(ImVec2(c.x - r * 0.34f, c.y + r * 0.44f), r * 0.24f, col, 14);
            dl->AddCircleFilled(ImVec2(c.x + r * 0.32f, c.y + r * 0.30f), r * 0.24f, col, 14);
            dl->AddLine(ImVec2(c.x - r * 0.12f, c.y + r * 0.42f), ImVec2(c.x - r * 0.12f, c.y - r * 0.48f), col, sw);
            dl->AddLine(ImVec2(c.x + r * 0.54f, c.y + r * 0.28f), ImVec2(c.x + r * 0.54f, c.y - r * 0.62f), col, sw);
            dl->AddLine(ImVec2(c.x - r * 0.12f, c.y - r * 0.48f), ImVec2(c.x + r * 0.54f, c.y - r * 0.62f), col, sw * 1.4f);
            break;
        }
        case UiIcon::CodeTags: {
            dl->AddLine(ImVec2(c.x - r * 0.34f, c.y - r * 0.40f), ImVec2(c.x - r * 0.76f, c.y), col, sw);
            dl->AddLine(ImVec2(c.x - r * 0.76f, c.y), ImVec2(c.x - r * 0.34f, c.y + r * 0.40f), col, sw);
            dl->AddLine(ImVec2(c.x + r * 0.34f, c.y - r * 0.40f), ImVec2(c.x + r * 0.76f, c.y), col, sw);
            dl->AddLine(ImVec2(c.x + r * 0.76f, c.y), ImVec2(c.x + r * 0.34f, c.y + r * 0.40f), col, sw);
            dl->AddLine(ImVec2(c.x + r * 0.14f, c.y - r * 0.54f), ImVec2(c.x - r * 0.14f, c.y + r * 0.54f), scaleRgb(col, 1.1f), sw);
            break;
        }
        case UiIcon::SphereBall: {
            dl->AddCircleFilled(c, r * 0.70f, scaleRgb(col, 0.85f), 20);
            dl->AddCircleFilled(ImVec2(c.x - r * 0.20f, c.y - r * 0.22f), r * 0.34f, IM_COL32(225, 240, 255, 190), 14);
            break;
        }
        case UiIcon::FileDoc:
        case UiIcon::FileCode: {
            dl->AddRectFilled(ImVec2(c.x - r * 0.56f, c.y - r * 0.72f),
                              ImVec2(c.x + r * 0.56f, c.y + r * 0.72f), col, 2.0f);
            ImU32 lineCol = IM_COL32(20, 22, 28, 220);
            dl->AddLine(ImVec2(c.x - r * 0.32f, c.y - r * 0.22f), ImVec2(c.x + r * 0.32f, c.y - r * 0.22f), lineCol, 1.4f);
            dl->AddLine(ImVec2(c.x - r * 0.32f, c.y + r * 0.08f), ImVec2(c.x + r * 0.32f, c.y + r * 0.08f), lineCol, 1.4f);
            if (icon == UiIcon::FileCode) {
                dl->AddLine(ImVec2(c.x - r * 0.32f, c.y + r * 0.36f), ImVec2(c.x + r * 0.12f, c.y + r * 0.36f), lineCol, 1.4f);
            }
            break;
        }
        case UiIcon::Terminal: {
            dl->AddRect(ImVec2(c.x - r * 0.74f, c.y - r * 0.58f),
                        ImVec2(c.x + r * 0.74f, c.y + r * 0.58f), col, 2.5f, 0, sw);
            dl->AddLine(ImVec2(c.x - r * 0.42f, c.y - r * 0.22f), ImVec2(c.x - r * 0.10f, c.y + r * 0.04f), col, sw);
            dl->AddLine(ImVec2(c.x - r * 0.42f, c.y + r * 0.30f), ImVec2(c.x - r * 0.10f, c.y + r * 0.04f), col, sw);
            dl->AddLine(ImVec2(c.x + r * 0.04f, c.y + r * 0.30f), ImVec2(c.x + r * 0.42f, c.y + r * 0.30f), col, sw);
            break;
        }
        case UiIcon::Speaker: {
            dl->AddRectFilled(ImVec2(c.x - r * 0.68f, c.y - r * 0.26f),
                              ImVec2(c.x - r * 0.32f, c.y + r * 0.26f), col, 1.0f);
            dl->AddTriangleFilled(ImVec2(c.x - r * 0.36f, c.y - r * 0.26f),
                                  ImVec2(c.x + r * 0.12f, c.y - r * 0.64f),
                                  ImVec2(c.x + r * 0.12f, c.y + r * 0.64f), col);
            dl->AddTriangleFilled(ImVec2(c.x - r * 0.36f, c.y - r * 0.26f),
                                  ImVec2(c.x + r * 0.12f, c.y + r * 0.64f),
                                  ImVec2(c.x - r * 0.36f, c.y + r * 0.26f), col);
            dl->PathArcTo(ImVec2(c.x + r * 0.14f, c.y), r * 0.44f, -0.7f, 0.7f, 10);
            dl->PathStroke(col, 0, sw);
            break;
        }
        case UiIcon::Keyframe: {
            ImVec2 d0(c.x + r * 0.10f, c.y - r * 0.56f);
            ImVec2 d1(c.x + r * 0.66f, c.y);
            ImVec2 d2(c.x + r * 0.10f, c.y + r * 0.56f);
            ImVec2 d3(c.x - r * 0.46f, c.y);
            dl->AddQuad(d0, d1, d2, d3, col, sw);
            dl->AddLine(ImVec2(c.x - r * 0.76f, c.y - r * 0.32f), ImVec2(c.x - r * 0.42f, c.y - r * 0.32f), col, sw);
            dl->AddLine(ImVec2(c.x - r * 0.76f, c.y + r * 0.32f), ImVec2(c.x - r * 0.42f, c.y + r * 0.32f), col, sw);
            break;
        }
        case UiIcon::ShaderEdit: {
            dl->AddRect(ImVec2(c.x - r * 0.68f, c.y - r * 0.62f),
                        ImVec2(c.x + r * 0.68f, c.y + r * 0.62f), col, 2.5f, 0, sw);
            dl->AddLine(ImVec2(c.x - r * 0.30f, c.y + r * 0.30f),
                        ImVec2(c.x + r * 0.34f, c.y - r * 0.34f), col, sw * 1.2f);
            break;
        }
        case UiIcon::SparklesAI: {
            dl->AddQuadFilled(ImVec2(c.x, c.y - r * 0.78f), ImVec2(c.x + r * 0.22f, c.y - r * 0.22f),
                              ImVec2(c.x, c.y + r * 0.78f), ImVec2(c.x - r * 0.22f, c.y - r * 0.22f), col);
            dl->AddQuadFilled(ImVec2(c.x - r * 0.78f, c.y), ImVec2(c.x - r * 0.22f, c.y - r * 0.22f),
                              ImVec2(c.x + r * 0.78f, c.y), ImVec2(c.x - r * 0.22f, c.y + r * 0.22f), col);
            break;
        }
        case UiIcon::WarnTriangle: {
            dl->AddTriangleFilled(ImVec2(c.x, c.y - r * 0.74f),
                                  ImVec2(c.x + r * 0.78f, c.y + r * 0.68f),
                                  ImVec2(c.x - r * 0.78f, c.y + r * 0.68f), col);
            ImU32 fg = IM_COL32(16, 16, 20, 255);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.22f), ImVec2(c.x, c.y + r * 0.22f), fg, sw * 1.25f);
            dl->AddCircleFilled(ImVec2(c.x, c.y + r * 0.46f), std::max(1.2f, r * 0.12f), fg, 8);
            break;
        }
        case UiIcon::ErrorCircle: {
            dl->AddCircleFilled(c, r * 0.76f, col, 20);
            ImU32 fg = IM_COL32(255, 255, 255, 255);
            dl->AddLine(ImVec2(c.x - r * 0.30f, c.y - r * 0.30f), ImVec2(c.x + r * 0.30f, c.y + r * 0.30f), fg, sw);
            dl->AddLine(ImVec2(c.x + r * 0.30f, c.y - r * 0.30f), ImVec2(c.x - r * 0.30f, c.y + r * 0.30f), fg, sw);
            break;
        }
        case UiIcon::FilterFunnel: {
            dl->AddTriangle(ImVec2(c.x - r * 0.68f, c.y - r * 0.56f),
                            ImVec2(c.x + r * 0.68f, c.y - r * 0.56f),
                            ImVec2(c.x, c.y + r * 0.16f), col, sw);
            dl->AddLine(ImVec2(c.x, c.y + r * 0.12f), ImVec2(c.x, c.y + r * 0.66f), col, sw * 1.3f);
            break;
        }
        case UiIcon::Trash: {
            dl->AddLine(ImVec2(c.x - r * 0.62f, c.y - r * 0.46f), ImVec2(c.x + r * 0.62f, c.y - r * 0.46f), col, sw);
            dl->AddRect(ImVec2(c.x - r * 0.46f, c.y - r * 0.42f),
                        ImVec2(c.x + r * 0.46f, c.y + r * 0.66f), col, 2.0f, 0, sw);
            dl->AddLine(ImVec2(c.x - r * 0.14f, c.y - r * 0.18f), ImVec2(c.x - r * 0.14f, c.y + r * 0.42f), col, sw * 0.85f);
            dl->AddLine(ImVec2(c.x + r * 0.14f, c.y - r * 0.18f), ImVec2(c.x + r * 0.14f, c.y + r * 0.42f), col, sw * 0.85f);
            break;
        }
        case UiIcon::SendPlane: {
            ImVec2 tip(c.x + r * 0.72f, c.y);
            ImVec2 tL(c.x - r * 0.66f, c.y - r * 0.60f);
            ImVec2 bL(c.x - r * 0.66f, c.y + r * 0.60f);
            ImVec2 mid(c.x - r * 0.26f, c.y);
            dl->AddTriangle(tL, tip, mid, col, sw);
            dl->AddTriangle(mid, tip, bL, col, sw);
            break;
        }
        case UiIcon::Eyedropper: {
            dl->AddLine(ImVec2(c.x - r * 0.52f, c.y + r * 0.52f),
                        ImVec2(c.x + r * 0.22f, c.y - r * 0.22f), col, sw * 1.4f);
            dl->AddCircleFilled(ImVec2(c.x + r * 0.36f, c.y - r * 0.36f), r * 0.28f, col, 12);
            break;
        }
        case UiIcon::ExpandCorners: {
            float L = r * 0.62f, k = r * 0.34f;
            dl->AddLine(ImVec2(c.x - L, c.y - L), ImVec2(c.x - L + k, c.y - L), col, sw);
            dl->AddLine(ImVec2(c.x - L, c.y - L), ImVec2(c.x - L, c.y - L + k), col, sw);
            dl->AddLine(ImVec2(c.x + L, c.y - L), ImVec2(c.x + L - k, c.y - L), col, sw);
            dl->AddLine(ImVec2(c.x + L, c.y - L), ImVec2(c.x + L, c.y - L + k), col, sw);
            dl->AddLine(ImVec2(c.x - L, c.y + L), ImVec2(c.x - L + k, c.y + L), col, sw);
            dl->AddLine(ImVec2(c.x - L, c.y + L), ImVec2(c.x - L, c.y + L - k), col, sw);
            dl->AddLine(ImVec2(c.x + L, c.y + L), ImVec2(c.x + L - k, c.y + L), col, sw);
            dl->AddLine(ImVec2(c.x + L, c.y + L), ImVec2(c.x + L, c.y + L - k), col, sw);
            break;
        }
        case UiIcon::SnapTarget: {
            dl->AddCircle(c, r * 0.56f, col, 18, sw);
            dl->AddLine(ImVec2(c.x - r * 0.80f, c.y), ImVec2(c.x - r * 0.32f, c.y), col, sw);
            dl->AddLine(ImVec2(c.x + r * 0.32f, c.y), ImVec2(c.x + r * 0.80f, c.y), col, sw);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.80f), ImVec2(c.x, c.y - r * 0.32f), col, sw);
            dl->AddLine(ImVec2(c.x, c.y + r * 0.32f), ImVec2(c.x, c.y + r * 0.80f), col, sw);
            break;
        }
        case UiIcon::ShieldLight: {
            dl->AddCircle(c, r * 0.62f, col, 18, sw);
            dl->PathArcTo(c, r * 0.62f, static_cast<float>(-M_PI * 0.5), static_cast<float>(M_PI * 0.5), 12);
            dl->PathFillConvex(col);
            break;
        }
        case UiIcon::FocusCube: {
            DrawIcon(dl, c, size, UiIcon::ExpandCorners, scaleAlpha(col, 0.8f));
            DrawIcon(dl, c, size * 0.58f, UiIcon::Cube3D, col);
            break;
        }
        case UiIcon::GridIcon: {
            float L = r * 0.64f;
            dl->AddRect(ImVec2(c.x - L, c.y - L), ImVec2(c.x + L, c.y + L), col, 2.0f, 0, sw);
            dl->AddLine(ImVec2(c.x, c.y - L), ImVec2(c.x, c.y + L), col, sw * 0.9f);
            dl->AddLine(ImVec2(c.x - L, c.y), ImVec2(c.x + L, c.y), col, sw * 0.9f);
            break;
        }
    }
}

void UIWidgets::DrawWindowChrome(
    float windowWidth,
    const std::function<void()>& onMin,
    const std::function<void()>& onMax,
    const std::function<void()>& onClose
) {
    ImGui::SetCursorScreenPos(ImVec2(windowWidth - 118.0f, 4.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 22));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 255, 255, 36));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (ImGui::Button("##win_min", ImVec2(32, 24))) { if (onMin) onMin(); }
    {
        ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
        float cy = (mn.y + mx.y) * 0.5f;
        dl->AddLine(ImVec2(mn.x + 10, cy), ImVec2(mx.x - 10, cy), IM_COL32(210, 212, 220, 255), 1.5f);
    }
    ImGui::SameLine(0, 4);

    if (ImGui::Button("##win_max", ImVec2(32, 24))) { if (onMax) onMax(); }
    {
        ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
        dl->AddRect(ImVec2(mn.x + 10.5f, mn.y + 6.5f), ImVec2(mx.x - 10.5f, mx.y - 6.5f), IM_COL32(210, 212, 220, 255), 1.5f, 0, 1.4f);
    }
    ImGui::SameLine(0, 4);

    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(220, 56, 62, 220));
    if (ImGui::Button("##win_close", ImVec2(32, 24))) { if (onClose) onClose(); }
    {
        ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
        dl->AddLine(ImVec2(mn.x + 10.5f, mn.y + 6.5f), ImVec2(mx.x - 10.5f, mx.y - 6.5f), IM_COL32(230, 232, 238, 255), 1.6f);
        dl->AddLine(ImVec2(mx.x - 10.5f, mn.y + 6.5f), ImVec2(mn.x + 10.5f, mx.y - 6.5f), IM_COL32(230, 232, 238, 255), 1.6f);
    }
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
}

bool UIWidgets::ToggleSwitch(const char* strId, bool* v, bool blueActive) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const float height = 20.0f;
    const float width  = 38.0f;
    const float radius = height * 0.5f;

    bool clicked = ImGui::InvisibleButton(strId, ImVec2(width, height));
    if (clicked) *v = !*v;

    ImU32 bg = *v
        ? (blueActive ? IM_COL32(37, 114, 246, 255) : IM_COL32(240, 240, 245, 255))
        : IM_COL32(48, 50, 58, 255);
    ImU32 knob = (*v && !blueActive) ? IM_COL32(14, 15, 18, 255) : IM_COL32(255, 255, 255, 255);

    dl->AddRectFilled(p, ImVec2(p.x + width, p.y + height), bg, radius);
    float kx = *v ? (p.x + width - radius) : (p.x + radius);
    dl->AddCircleFilled(ImVec2(kx, p.y + radius), radius - 2.4f, knob, 24);
    return clicked;
}

void UIWidgets::DrawPlatformIcon(ImDrawList* dl, ImVec2 c, float size, PlatformIconKind kind) {
    const float s = size * 0.5f;
    const ImU32 w = IM_COL32(255, 255, 255, 255);
    const ImU32 dark = IM_COL32(14, 15, 18, 255);

    switch (kind) {
        case PlatformIconKind::APK: {
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
            float g = 2.4f, r = s * 0.62f;
            dl->AddQuadFilled(ImVec2(c.x - r, c.y - r * 0.82f), ImVec2(c.x - g, c.y - r), ImVec2(c.x - g, c.y - g), ImVec2(c.x - r, c.y - g), w);
            dl->AddQuadFilled(ImVec2(c.x + g, c.y - r), ImVec2(c.x + r, c.y - r * 1.12f), ImVec2(c.x + r, c.y - g), ImVec2(c.x + g, c.y - g), w);
            dl->AddQuadFilled(ImVec2(c.x - r, c.y + g), ImVec2(c.x - g, c.y + g), ImVec2(c.x - g, c.y + r), ImVec2(c.x - r, c.y + r * 0.82f), w);
            dl->AddQuadFilled(ImVec2(c.x + g, c.y + g), ImVec2(c.x + r, c.y + g), ImVec2(c.x + r, c.y + r * 1.12f), ImVec2(c.x + g, c.y + r), w);
            break;
        }
        case PlatformIconKind::Linux: {
            // Wireframe globe / multi-arch network sphere aesthetic matching Build card 4 visual style
            float rad = s * 0.66f;
            dl->AddCircle(c, rad, w, 28, 2.3f);
            dl->AddEllipse(c, ImVec2(rad * 0.48f, rad), w, 0.0f, 24, 2.0f);
            dl->AddLine(ImVec2(c.x - rad, c.y), ImVec2(c.x + rad, c.y), w, 2.0f);
            dl->AddLine(ImVec2(c.x - rad * 0.84f, c.y - rad * 0.46f), ImVec2(c.x + rad * 0.84f, c.y - rad * 0.46f), w, 1.8f);
            dl->AddLine(ImVec2(c.x - rad * 0.84f, c.y + rad * 0.46f), ImVec2(c.x + rad * 0.84f, c.y + rad * 0.46f), w, 1.8f);
            break;
        }
        case PlatformIconKind::ZIP: {
            float rx = s * 0.54f, ry = s * 0.68f;
            dl->AddRectFilled(ImVec2(c.x - rx, c.y - ry), ImVec2(c.x + rx, c.y + ry), w, 5.0f);
            for (int i = 0; i < 4; ++i) {
                float yy = c.y - ry + 6.0f + i * 6.5f;
                dl->AddRectFilled(ImVec2(c.x - 4.0f, yy), ImVec2(c.x + 4.0f, yy + 3.8f), dark);
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
    const float r = size * 0.44f;
    if (kind == MeshKind::Sphere) {
        dl->AddCircleFilled(c, r, IM_COL32(154, 156, 166, 255), 32);
        dl->AddCircleFilled(ImVec2(c.x - r * 0.16f, c.y - r * 0.16f), r * 0.74f, IM_COL32(204, 206, 214, 255), 28);
        dl->AddCircleFilled(ImVec2(c.x - r * 0.30f, c.y - r * 0.30f), r * 0.36f, IM_COL32(246, 247, 252, 255), 20);
        return;
    }
    if (kind == MeshKind::Cylinder) {
        float w = r * 0.78f, h = r * 0.74f;
        dl->AddRectFilled(ImVec2(c.x - w, c.y - h * 0.68f), ImVec2(c.x + w, c.y + h * 0.82f), IM_COL32(182, 184, 194, 255));
        dl->AddRectFilled(ImVec2(c.x - w * 0.46f, c.y - h * 0.68f), ImVec2(c.x + w * 0.14f, c.y + h * 0.82f), IM_COL32(222, 224, 232, 255));
        dl->AddEllipseFilled(ImVec2(c.x, c.y + h * 0.82f), ImVec2(w, h * 0.34f), IM_COL32(170, 172, 182, 255), 0.f, 24);
        dl->AddEllipseFilled(ImVec2(c.x, c.y - h * 0.68f), ImVec2(w, h * 0.34f), IM_COL32(244, 245, 250, 255), 0.f, 24);
        return;
    }
    if (kind == MeshKind::Block) {
        // Stepped / wedge architectural block matching "Block" in Toolbox screenshot
        float sx = r * 0.96f, dy = r * 0.46f, h = r * 0.72f;
        ImVec2 bL(c.x - sx, c.y + dy * 0.55f);
        ImVec2 bB(c.x,      c.y + dy * 1.45f);
        ImVec2 bR(c.x + sx, c.y + dy * 0.55f);
        ImVec2 tL(bL.x, bL.y - h * 0.55f);
        ImVec2 mB(bB.x, bB.y - h * 0.55f);
        ImVec2 tR(bR.x, bR.y - h * 1.05f);
        ImVec2 topApex(c.x, c.y - h * 1.05f - dy * 0.35f);
        dl->AddQuadFilled(topApex, tR, mB, tL, IM_COL32(238, 240, 246, 255));
        dl->AddQuadFilled(tL, mB, bB, bL, IM_COL32(172, 174, 184, 255));
        dl->AddQuadFilled(mB, tR, bR, bB, IM_COL32(204, 206, 216, 255));
        return;
    }

    // Isometric Cube ("Part")
    float sy = r * 0.80f;
    float sx = r * 0.94f;
    float dy = r * 0.46f;

    ImVec2 top0(c.x, c.y - sy - dy * 0.45f);
    ImVec2 topR(c.x + sx, c.y - sy + dy * 0.55f);
    ImVec2 topB(c.x, c.y - sy + dy * 1.55f);
    ImVec2 topL(c.x - sx, c.y - sy + dy * 0.55f);

    ImVec2 botR(topR.x, topR.y + sy * 1.16f);
    ImVec2 botB(topB.x, topB.y + sy * 1.16f);
    ImVec2 botL(topL.x, topL.y + sy * 1.16f);

    dl->AddQuadFilled(top0, topR, topB, topL, IM_COL32(244, 245, 250, 255));
    dl->AddQuadFilled(topL, topB, botB, botL, IM_COL32(174, 176, 186, 255));
    dl->AddQuadFilled(topB, topR, botR, botB, IM_COL32(206, 208, 218, 255));
}

void UIWidgets::DrawOrientationGizmo(ImDrawList* dl, ImVec2 center, float radius, const AstronMat4& view) {
    struct AxisNode {
        const char* label;
        AstronVec3 dir;
        ImU32 col;
        bool positive;
        float z;
        ImVec2 pos;
    };

    AxisNode nodes[6] = {
        { "X",  {  1,  0,  0 }, IM_COL32(238,  60,  60, 255), true,  0.f, center },
        { "Y",  {  0,  1,  0 }, IM_COL32( 74, 218,  72, 255), true,  0.f, center },
        { "Z",  {  0,  0,  1 }, IM_COL32( 48, 134, 252, 255), true,  0.f, center },
        { "",   { -1,  0,  0 }, IM_COL32(142,  46,  48, 210), false, 0.f, center },
        { "",   {  0, -1,  0 }, IM_COL32( 50, 128,  52, 210), false, 0.f, center },
        { "Z",  {  0,  0, -1 }, IM_COL32( 42, 112, 218, 235), false, 0.f, center }
    };

    for (auto& a : nodes) {
        float vx = view.m[0] * a.dir.x + view.m[4] * a.dir.y + view.m[8]  * a.dir.z;
        float vy = view.m[1] * a.dir.x + view.m[5] * a.dir.y + view.m[9]  * a.dir.z;
        float vz = view.m[2] * a.dir.x + view.m[6] * a.dir.y + view.m[10] * a.dir.z;
        a.z = vz;
        float rScale = a.positive ? 1.0f : 0.78f;
        a.pos = ImVec2(center.x + vx * radius * rScale, center.y - vy * radius * rScale);
    }

    std::sort(std::begin(nodes), std::end(nodes), [](const AxisNode& a, const AxisNode& b) {
        return a.z < b.z;
    });

    dl->AddCircleFilled(center, 4.0f, IM_COL32(140, 146, 160, 180), 12);

    for (const auto& a : nodes) {
        if (a.positive) {
            dl->AddLine(center, a.pos, a.col, 2.5f);
        }
        float ballR = a.positive ? 10.0f : (a.label[0] != '\0' ? 8.5f : 6.0f);
        dl->AddCircleFilled(a.pos, ballR, a.col, 20);
        if (a.label[0] != '\0') {
            ImVec2 ts = ImGui::CalcTextSize(a.label);
            dl->AddText(ImVec2(a.pos.x - ts.x * 0.5f, a.pos.y - ts.y * 0.5f),
                        IM_COL32(255, 255, 255, 255), a.label);
        }
    }
}

void UIWidgets::DrawSceneTranslationGizmo(
    ImDrawList* dl, ImVec2 vpMin, ImVec2 vpMax,
    const AstronMat4& viewProj, const AstronVec3& origin, float axisLen
) {
    ImVec2 oScreen;
    if (!projectWorldToViewport(viewProj, origin, vpMin, vpMax, oScreen)) return;

    struct GizAxis {
        AstronVec3 tip;
        ImU32 col;
    };
    const GizAxis axes[3] = {
        { { origin.x + axisLen, origin.y, origin.z }, IM_COL32(242,  54,  58, 255) }, // X Red
        { { origin.x, origin.y + axisLen, origin.z }, IM_COL32( 68, 228,  68, 255) }, // Y Green
        { { origin.x, origin.y, origin.z + axisLen }, IM_COL32( 46, 132, 255, 255) }  // Z Blue
    };

    dl->PushClipRect(vpMin, vpMax, true);
    for (const auto& ax : axes) {
        ImVec2 tipScreen;
        if (!projectWorldToViewport(viewProj, ax.tip, vpMin, vpMax, tipScreen)) continue;
        float dx = tipScreen.x - oScreen.x;
        float dy = tipScreen.y - oScreen.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 6.0f) continue;

        float ux = dx / len;
        float uy = dy / len;
        float px = -uy;
        float py = ux;

        const float headLen = 14.5f;
        const float headHalfW = 6.2f;
        ImVec2 shaftEnd(tipScreen.x - ux * (headLen * 0.75f), tipScreen.y - uy * (headLen * 0.75f));
        dl->AddLine(oScreen, shaftEnd, ax.col, 2.8f);
        dl->AddTriangleFilled(
            tipScreen,
            ImVec2(tipScreen.x - ux * headLen + px * headHalfW, tipScreen.y - uy * headLen + py * headHalfW),
            ImVec2(tipScreen.x - ux * headLen - px * headHalfW, tipScreen.y - uy * headLen - py * headHalfW),
            ax.col
        );
    }
    dl->PopClipRect();
}

bool UIWidgets::WhitePillButton(const char* label, ImVec2 size, float rounding, UiIcon leftIcon) {
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(252, 252, 254, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(232, 234, 240, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(210, 212, 220, 255));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 0)); // custom centered draw with icon
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);

    ImVec2 bSize = size;
    const char* end = visibleLabelEnd(label);
    ImVec2 txtSz = ImGui::CalcTextSize(label, end);
    if (bSize.x <= 0.0f) bSize.x = txtSz.x + (leftIcon != UiIcon::None ? 44.0f : 28.0f);
    if (bSize.y <= 0.0f) bSize.y = txtSz.y + 16.0f;

    bool pressed = ImGui::Button(label, bSize);
    ImVec2 mn = ImGui::GetItemRectMin();
    ImVec2 mx = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 fg = IM_COL32(10, 11, 14, 255);

    float iconSpace = (leftIcon != UiIcon::None) ? 22.0f : 0.0f;
    float totalW = txtSz.x + iconSpace;
    float sx = (mn.x + mx.x - totalW) * 0.5f;
    float cy = (mn.y + mx.y) * 0.5f;

    if (leftIcon != UiIcon::None) {
        DrawIcon(dl, ImVec2(sx + 7.0f, cy), 14.0f, leftIcon, fg);
        sx += iconSpace;
    }
    dl->AddText(ImVec2(sx, cy - txtSz.y * 0.5f), fg, label, end);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);
    return pressed;
}

bool UIWidgets::OutlineButton(const char* label, ImVec2 size, float rounding, UiIcon leftIcon, UiIcon rightIcon) {
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(16, 17, 22, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(28, 30, 38, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(38, 40, 50, 255));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(52, 55, 66, 255));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

    ImVec2 bSize = size;
    const char* end = visibleLabelEnd(label);
    ImVec2 txtSz = ImGui::CalcTextSize(label, end);
    float leftW  = (leftIcon  != UiIcon::None) ? (txtSz.x > 0.5f ? 22.0f : 15.0f) : 0.0f;
    float rightW = (rightIcon != UiIcon::None) ? (txtSz.x > 0.5f ? 20.0f : 15.0f) : 0.0f;
    if (bSize.x <= 0.0f) bSize.x = txtSz.x + leftW + rightW + 24.0f;
    if (bSize.y <= 0.0f) bSize.y = txtSz.y + 14.0f;

    bool pressed = ImGui::Button(label, bSize);
    ImVec2 mn = ImGui::GetItemRectMin();
    ImVec2 mx = ImGui::GetItemRectMax();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImU32 fg = IM_COL32(236, 238, 244, 255);

    float totalW = txtSz.x + leftW + rightW;
    float sx = (mn.x + mx.x - totalW) * 0.5f;
    float cy = (mn.y + mx.y) * 0.5f;

    if (leftIcon != UiIcon::None) {
        DrawIcon(dl, ImVec2(sx + (txtSz.x > 0.5f ? 7.0f : totalW * 0.5f), cy), 14.5f, leftIcon, fg);
        sx += leftW;
    }
    if (txtSz.x > 0.5f) {
        dl->AddText(ImVec2(sx, cy - txtSz.y * 0.5f), fg, label, end);
        sx += txtSz.x;
    }
    if (rightIcon != UiIcon::None) {
        DrawIcon(dl, ImVec2(sx + (txtSz.x > 0.5f ? 12.0f : totalW * 0.5f), cy), 13.0f, rightIcon, fg);
    }

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(5);
    return pressed;
}

} // namespace astron
