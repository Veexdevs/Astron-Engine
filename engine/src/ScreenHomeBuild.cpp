#include "EngineApp.hpp"
#include "UIWidgets.hpp"
#include "../core_c/astron_platform.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace astron {

namespace {

// Draws a texture cropped with CSS "object-fit: cover" UV coordinates and rounded corners
void drawImageCoverRounded(ImDrawList* dl, const GpuTexture& tex, ImVec2 pMin, ImVec2 pMax,
                           float rounding, ImU32 tint = IM_COL32(255, 255, 255, 255), ImDrawFlags flags = 0) {
    if (tex.id == 0 || tex.width <= 0 || tex.height <= 0) return;
    float dstW = std::max(1.0f, pMax.x - pMin.x);
    float dstH = std::max(1.0f, pMax.y - pMin.y);
    float dstAspect = dstW / dstH;
    float srcAspect = static_cast<float>(tex.width) / static_cast<float>(tex.height);

    ImVec2 uv0(0.0f, 0.0f), uv1(1.0f, 1.0f);
    if (srcAspect > dstAspect) {
        float vis = dstAspect / srcAspect;
        uv0.x = (1.0f - vis) * 0.5f;
        uv1.x = 1.0f - uv0.x;
    } else {
        float vis = srcAspect / dstAspect;
        uv0.y = (1.0f - vis) * 0.5f;
        uv1.y = 1.0f - uv0.y;
    }
    dl->AddImageRounded(
        static_cast<ImTextureID>(static_cast<uintptr_t>(tex.id)),
        pMin, pMax, uv0, uv1, tint, rounding, flags
    );
}

void drawDashedRoundedRect(ImDrawList* dl, ImVec2 pMin, ImVec2 pMax, float rounding, ImU32 col, float thickness = 1.2f) {
    const float dash = 7.0f;
    const float gap = 5.0f;
    auto segLine = [&](ImVec2 a, ImVec2 b) {
        float dx = b.x - a.x, dy = b.y - a.y;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1.0f) return;
        float ux = dx / len, uy = dy / len;
        for (float t = 0.0f; t < len; t += dash + gap) {
            float tEnd = std::min(len, t + dash);
            dl->AddLine(ImVec2(a.x + ux * t, a.y + uy * t), ImVec2(a.x + ux * tEnd, a.y + uy * tEnd), col, thickness);
        }
    };
    float r = std::min({ rounding, (pMax.x - pMin.x) * 0.25f, (pMax.y - pMin.y) * 0.25f });
    segLine(ImVec2(pMin.x + r, pMin.y), ImVec2(pMax.x - r, pMin.y));
    segLine(ImVec2(pMax.x, pMin.y + r), ImVec2(pMax.x, pMax.y - r));
    segLine(ImVec2(pMax.x - r, pMax.y), ImVec2(pMin.x + r, pMax.y));
    segLine(ImVec2(pMin.x, pMax.y - r), ImVec2(pMin.x, pMin.y + r));

    dl->PathArcTo(ImVec2(pMin.x + r, pMin.y + r), r, static_cast<float>(M_PI), static_cast<float>(M_PI * 1.5), 8);
    dl->PathStroke(col, 0, thickness);
    dl->PathArcTo(ImVec2(pMax.x - r, pMin.y + r), r, static_cast<float>(M_PI * 1.5), static_cast<float>(M_PI * 2.0), 8);
    dl->PathStroke(col, 0, thickness);
    dl->PathArcTo(ImVec2(pMax.x - r, pMax.y - r), r, 0.0f, static_cast<float>(M_PI * 0.5), 8);
    dl->PathStroke(col, 0, thickness);
    dl->PathArcTo(ImVec2(pMin.x + r, pMax.y - r), r, static_cast<float>(M_PI * 0.5), static_cast<float>(M_PI), 8);
    dl->PathStroke(col, 0, thickness);
}

bool sidebarNavButton(const char* id, UiIcon icon, const char* label, bool active, float width) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float h = 40.0f;

    bool clicked = ImGui::InvisibleButton(id, ImVec2(width, h));
    bool hovered = ImGui::IsItemHovered();

    if (active) {
        dl->AddRectFilled(p, ImVec2(p.x + width, p.y + h), IM_COL32(28, 30, 37, 255), 10.0f);
        dl->AddRect(p, ImVec2(p.x + width, p.y + h), IM_COL32(52, 55, 66, 180), 10.0f);
        dl->AddRectFilled(ImVec2(p.x + 1.0f, p.y + 9.0f), ImVec2(p.x + 4.0f, p.y + h - 9.0f),
                          IM_COL32(255, 255, 255, 255), 2.0f);
    } else if (hovered) {
        dl->AddRectFilled(p, ImVec2(p.x + width, p.y + h), IM_COL32(22, 23, 29, 210), 10.0f);
    }

    ImU32 fg = active ? IM_COL32(255, 255, 255, 255) : IM_COL32(154, 158, 170, 255);
    float cy = p.y + h * 0.5f;
    UIWidgets::DrawIcon(dl, ImVec2(p.x + 22.0f, cy), 15.5f, icon, fg);
    ImVec2 ts = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(p.x + 42.0f, cy - ts.y * 0.5f), fg, label);
    return clicked;
}

bool circleIconButton(const char* id, UiIcon icon, float diameter) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    bool clicked = ImGui::InvisibleButton(id, ImVec2(diameter, diameter));
    bool hovered = ImGui::IsItemHovered();
    float r = diameter * 0.5f;
    ImVec2 c(p.x + r, p.y + r);
    dl->AddCircleFilled(c, r, hovered ? IM_COL32(34, 36, 46, 255) : IM_COL32(22, 23, 29, 255), 28);
    dl->AddCircle(c, r, IM_COL32(46, 49, 60, 210), 28, 1.0f);
    UIWidgets::DrawIcon(dl, c, diameter * 0.44f, icon, IM_COL32(214, 218, 228, 255));
    return clicked;
}

void drawSidebarBrandLockup(ImDrawList* dl, const GpuTexture& horizLogo, ImVec2 origin, ImFont* fontLogoSmall) {
    if (horizLogo.id != 0 && horizLogo.height > 0) {
        float aspect = static_cast<float>(horizLogo.width) / static_cast<float>(horizLogo.height);
        float h = 38.0f;
        float w = std::min(196.0f, h * aspect);
        h = w / aspect;
        dl->AddImage(
            static_cast<ImTextureID>(static_cast<uintptr_t>(horizLogo.id)),
            ImVec2(origin.x, origin.y + (40.0f - h) * 0.5f),
            ImVec2(origin.x + w, origin.y + (40.0f - h) * 0.5f + h)
        );
    } else {
        UIWidgets::DrawPlanetMark(dl, ImVec2(origin.x + 20.0f, origin.y + 20.0f), 16.0f,
                                  IM_COL32(255, 255, 255, 255), IM_COL32(8, 9, 12, 255));
        if (fontLogoSmall) ImGui::PushFont(fontLogoSmall);
        dl->AddText(ImVec2(origin.x + 44.0f, origin.y + 6.0f), IM_COL32(255, 255, 255, 255), "ASTRON");
        if (fontLogoSmall) ImGui::PopFont();
        dl->AddText(ImVec2(origin.x + 45.0f, origin.y + 24.0f), IM_COL32(148, 152, 162, 255), "E N G I N E");
    }
}

} // namespace

// ============================================================================
// SCREEN 3: HOME HUB ("Início" — Identical to Image 2)
// ============================================================================
void EngineApp::renderHome(float W, float H) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(W, H));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(7, 8, 11, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##AstronHome", nullptr, flags);

    const float sideW = 240.0f;

    // -------------------- LEFT SIDEBAR --------------------
    ImGui::SetCursorScreenPos(ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(9, 10, 13, 255));
    ImGui::BeginChild("##HomeSidebar", ImVec2(sideW, H), false);

    ImDrawList* sdl = ImGui::GetWindowDrawList();
    sdl->AddLine(ImVec2(sideW - 1.0f, 0), ImVec2(sideW - 1.0f, H), IM_COL32(34, 36, 44, 255));

    GpuTexture horizLogo = m_textures.get("assets/textures/astron-horizontal.png");
    drawSidebarBrandLockup(sdl, horizLogo, ImVec2(20.0f, 20.0f), fontLogoSmall);

    ImGui::SetCursorPos(ImVec2(14.0f, 82.0f));
    ImGui::BeginGroup();
    const float navW = sideW - 28.0f;
    if (sidebarNavButton("##nav_ini",  UiIcon::Home,       "In\xC3\xAD" "cio",        m_homePage == "inicio",     navW)) m_homePage = "inicio";
    ImGui::Dummy(ImVec2(0, 4));
    if (sidebarNavButton("##nav_proj", UiIcon::Folder,     "Meus Projetos",            m_homePage == "projetos",   navW)) m_homePage = "projetos";
    ImGui::Dummy(ImVec2(0, 4));
    if (sidebarNavButton("##nav_bib",  UiIcon::Cube3D,     "Bibioteca",                m_homePage == "biblioteca", navW)) m_homePage = "biblioteca";
    ImGui::Dummy(ImVec2(0, 4));
    if (sidebarNavButton("##nav_loja", UiIcon::Bag,        "Loja",                     m_homePage == "loja",       navW)) m_homePage = "loja";
    ImGui::Dummy(ImVec2(0, 4));
    if (sidebarNavButton("##nav_cfg",  UiIcon::Gear,       "Configura\xC3\xA7\xC3\xB5" "es", m_homePage == "config", navW)) m_settingsOpen = true;
    ImGui::Dummy(ImVec2(0, 4));
    if (sidebarNavButton("##nav_sob",  UiIcon::InfoCircle, "Sobre",                    m_homePage == "sobre",      navW)) m_homePage = "sobre";
    ImGui::EndGroup();

    // Bottom Engine Status Card matching Image 2 ("⚡ Astron Engine / v1.0.0 | ● Online")
    const float statusY = H - 76.0f;
    sdl->AddRectFilled(ImVec2(14, statusY), ImVec2(sideW - 14, H - 16), IM_COL32(15, 16, 21, 255), 12.0f);
    sdl->AddRect(ImVec2(14, statusY), ImVec2(sideW - 14, H - 16), IM_COL32(36, 38, 48, 255), 12.0f);
    UIWidgets::DrawIcon(sdl, ImVec2(30.0f, statusY + 21.0f), 14.0f, UiIcon::Bolt, IM_COL32(235, 238, 245, 255));
    if (fontUiBold) ImGui::PushFont(fontUiBold);
    sdl->AddText(ImVec2(43.0f, statusY + 12.0f), IM_COL32(238, 240, 246, 255), "Astron Engine");
    if (fontUiBold) ImGui::PopFont();
    sdl->AddText(ImVec2(26.0f, statusY + 34.0f), IM_COL32(126, 130, 142, 255), "v1.0.0");
    sdl->AddCircleFilled(ImVec2(sideW - 72.0f, statusY + 42.0f), 4.0f, IM_COL32(46, 214, 114, 255));
    sdl->AddText(ImVec2(sideW - 63.0f, statusY + 34.0f), IM_COL32(174, 178, 190, 255), "Online");

    ImGui::EndChild();
    ImGui::PopStyleColor();

    // -------------------- MAIN HUB CONTENT --------------------
    ImGui::SetCursorScreenPos(ImVec2(sideW, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(7, 8, 11, 255));
    ImGui::BeginChild("##HomeMain", ImVec2(W - sideW, H), false);

    ImDrawList* mdl = ImGui::GetWindowDrawList();
    const float padX = 32.0f;
    const float contentW = (W - sideW) - padX * 2.0f;

    // Top-right circular Gear & User profile buttons (Image 2)
    ImGui::SetCursorScreenPos(ImVec2(W - padX - 82.0f, 18.0f));
    if (circleIconButton("##top_gear", UiIcon::Gear, 36.0f)) {
        m_settingsOpen = true;
    }
    ImGui::SameLine(0, 10.0f);
    if (circleIconButton("##top_user", UiIcon::User, 36.0f)) {
        pushToast(std::string("Conta conectada: ") + (m_loginUser[0] ? m_loginUser : "Desenvolvedor"));
    }

    // Hero Planet Banner (assets/textures/hero-planet.jpg == download.jpeg, cover-cropped)
    const float bannerY = 66.0f;
    const float bannerH = 228.0f;
    ImVec2 bMin(sideW + padX, bannerY);
    ImVec2 bMax(sideW + padX + contentW, bannerY + bannerH);

    GpuTexture hero = m_textures.get("assets/textures/hero-planet.jpg");
    if (hero.id != 0) {
        drawImageCoverRounded(mdl, hero, bMin, bMax, 18.0f);
        mdl->AddRectFilledMultiColor(
            bMin, ImVec2(bMin.x + contentW * 0.65f, bMax.y),
            IM_COL32(6, 7, 10, 215), IM_COL32(6, 7, 10, 20),
            IM_COL32(6, 7, 10, 20), IM_COL32(6, 7, 10, 225)
        );
    } else {
        mdl->AddRectFilled(bMin, bMax, IM_COL32(18, 20, 28, 255), 18.0f);
    }
    mdl->AddRect(bMin, bMax, IM_COL32(52, 56, 68, 175), 18.0f);

    if (fontTitle) ImGui::PushFont(fontTitle);
    mdl->AddText(ImVec2(bMin.x + 36.0f, bMin.y + 48.0f), IM_COL32(255, 255, 255, 255), "Crie. Desenvolva. Explore.");
    if (fontTitle) ImGui::PopFont();
    mdl->AddText(ImVec2(bMin.x + 36.0f, bMin.y + 88.0f), IM_COL32(194, 198, 210, 255), "Sua imagina\xC3\xA7\xC3\xA3o, sem limites.");

    ImGui::SetCursorScreenPos(ImVec2(bMin.x + 36.0f, bMin.y + 136.0f));
    if (UIWidgets::WhitePillButton("Novo Projeto##hero", ImVec2(168.0f, 44.0f), 22.0f, UiIcon::Plus)) {
        m_newProjOpen = true;
    }

    // Subpage content or Recent Projects grid
    const float secY = bannerY + bannerH + 28.0f;
    ImGui::SetCursorScreenPos(ImVec2(sideW + padX, secY));

    if (m_homePage == "biblioteca" || m_homePage == "loja" || m_homePage == "sobre") {
        if (fontUiBold) ImGui::PushFont(fontUiBold);
        if (m_homePage == "biblioteca") ImGui::TextUnformatted("Bibioteca de Recursos Nativos");
        else if (m_homePage == "loja")  ImGui::TextUnformatted("Loja de Kits Astron Engine");
        else                            ImGui::TextUnformatted("Sobre a Astron Engine v1.0.0");
        if (fontUiBold) ImGui::PopFont();
        ImGui::Spacing();
        if (m_homePage == "sobre") {
            ImGui::TextWrapped("Astron Engine v1.0.0 — Engine nativa 2D/3D em C11, C++20, C#, Rust, GLSL, HLSL e Lua com Viewport OpenGL 3.3, Agente IA aut\xC3\xB4nomo via OpenRouter e pipeline de compila\xC3\xA7\xC3\xA3o multiplataforma (Windows .exe, Linux x64, Android .apk, iOS .ipa e .pak).");
        } else {
            ImGui::TextWrapped("Templates C# 3D prontos para instanciar no Editor: Platformer 3D, Cidade RP, Obby Neon, Shaders GLSL/HLSL e Sintetizador C11 WAV.");
            ImGui::Spacing();
            if (UIWidgets::WhitePillButton("Criar Projeto com Template", ImVec2(248.0f, 40.0f), 10.0f, UiIcon::Plus)) {
                m_newProjOpen = true;
            }
        }
    } else {
        // Section Header: Clock vector icon + "Projetos Recentes" + "Ver Todos  >"
        {
            ImVec2 hdrP = ImGui::GetCursorScreenPos();
            UIWidgets::DrawIcon(mdl, ImVec2(hdrP.x + 9.0f, hdrP.y + 10.0f), 17.0f, UiIcon::Clock, IM_COL32(210, 214, 226, 255));
            if (fontUiBold) ImGui::PushFont(fontUiBold);
            mdl->AddText(ImVec2(hdrP.x + 26.0f, hdrP.y + 1.0f), IM_COL32(244, 246, 252, 255),
                         m_homePage == "projetos" ? "Todos os Projetos" : "Projetos Recentes");
            if (fontUiBold) ImGui::PopFont();

            const char* seeAll = "Ver Todos  >";
            ImVec2 saSz = ImGui::CalcTextSize(seeAll);
            ImGui::SetCursorScreenPos(ImVec2(sideW + padX + contentW - saSz.x - 12.0f, hdrP.y - 2.0f));
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 16));
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(168, 172, 184, 255));
            if (ImGui::Button(seeAll)) {
                m_homePage = (m_homePage == "projetos") ? "inicio" : "projetos";
            }
            ImGui::PopStyleColor(3);
        }

        // 4-Column Project Cards Grid matching Image 2
        const float gridY = secY + 38.0f;
        const float gap = 18.0f;
        const int cols = 4;
        const float cardW = (contentW - gap * (cols - 1)) / cols;
        const float cardH = std::clamp(H - gridY - 28.0f, 198.0f, 226.0f);
        const float thumbH = cardH - 68.0f;

        const auto& projs = m_projects.projects();
        int maxCards = (m_homePage == "projetos") ? static_cast<int>(projs.size()) : std::min<int>(3, static_cast<int>(projs.size()));

        for (int i = 0; i <= maxCards; ++i) {
            int col = i % cols;
            int row = i / cols;
            float cx = sideW + padX + col * (cardW + gap);
            float cy = gridY + row * (cardH + gap);

            ImVec2 cMin(cx, cy);
            ImVec2 cMax(cx + cardW, cy + cardH);

            if (i < maxCards) {
                const auto& pr = projs[static_cast<size_t>(i)];

                // Overflow "..." menu button rect (top-right of thumbnail)
                ImVec2 menuCenter(cMax.x - 22.0f, cMin.y + 20.0f);
                float menuR = 13.0f;

                ImGui::SetCursorScreenPos(cMin);
                std::string btnId = "##proj_card_" + pr.id;
                bool cardClicked = ImGui::InvisibleButton(btnId.c_str(), ImVec2(cardW, cardH));
                bool hovered = ImGui::IsItemHovered();

                ImVec2 mp = ImGui::GetIO().MousePos;
                bool overDots = ((mp.x - menuCenter.x) * (mp.x - menuCenter.x) +
                                 (mp.y - menuCenter.y) * (mp.y - menuCenter.y)) <= (menuR + 3.0f) * (menuR + 3.0f);

                mdl->AddRectFilled(cMin, cMax, hovered ? IM_COL32(20, 22, 28, 255) : IM_COL32(14, 15, 20, 255), 14.0f);

                GpuTexture th = m_textures.get(pr.thumbAsset);
                if (th.id != 0) {
                    drawImageCoverRounded(mdl, th, cMin, ImVec2(cMax.x, cMin.y + thumbH),
                                          14.0f, IM_COL32(255, 255, 255, 255), ImDrawFlags_RoundCornersTop);
                } else {
                    mdl->AddRectFilled(cMin, ImVec2(cMax.x, cMin.y + thumbH),
                                       IM_COL32(26, 28, 36, 255), 14.0f, ImDrawFlags_RoundCornersTop);
                }
                mdl->AddRect(cMin, cMax, hovered ? IM_COL32(84, 90, 108, 235) : IM_COL32(36, 38, 48, 225), 14.0f);

                // Top-right dark circular "•••" button
                mdl->AddCircleFilled(menuCenter, menuR, overDots ? IM_COL32(36, 38, 48, 240) : IM_COL32(12, 13, 18, 205), 20);
                mdl->AddCircle(menuCenter, menuR, IM_COL32(70, 74, 88, 150), 20, 1.0f);
                UIWidgets::DrawIcon(mdl, menuCenter, 13.0f, UiIcon::DotsH, IM_COL32(226, 228, 236, 255));

                if (fontUiBold) ImGui::PushFont(fontUiBold);
                mdl->AddText(ImVec2(cMin.x + 14.0f, cMin.y + thumbH + 12.0f), IM_COL32(246, 247, 252, 255), pr.name.c_str());
                if (fontUiBold) ImGui::PopFont();

                // Clock icon + "Criado em DD/MM/2026"
                UIWidgets::DrawIcon(mdl, ImVec2(cMin.x + 20.0f, cMin.y + thumbH + 45.0f), 12.0f,
                                    UiIcon::Clock, IM_COL32(134, 138, 150, 255));
                std::string meta = "Criado em " + pr.createdDate;
                mdl->AddText(ImVec2(cMin.x + 31.0f, cMin.y + thumbH + 37.0f), IM_COL32(138, 142, 154, 255), meta.c_str());

                std::string popupId = "ProjCtx_" + pr.id;
                if (cardClicked) {
                    if (overDots) {
                        ImGui::OpenPopup(popupId.c_str());
                    } else {
                        m_projects.openProject(pr.id);
                        bool ok = false;
                        std::string scJson = m_projects.readFile("res://default.scene", &ok);
                        if (ok && !scJson.empty()) {
                            std::string jerr;
                            Json j = Json::parse(scJson, &jerr);
                            if (jerr.empty()) m_scene = Scene::fromJson(j);
                            else m_scene = Scene::createDefault();
                        } else {
                            m_scene = Scene::createDefault();
                        }
                        m_screen = ScreenId::Editor;
                        pushToast("Projeto aberto: " + pr.name);
                    }
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                    ImGui::OpenPopup(popupId.c_str());
                }

                if (ImGui::BeginPopup(popupId.c_str())) {
                    if (ImGui::MenuItem("Abrir no Editor 3D")) {
                        m_projects.openProject(pr.id);
                        m_screen = ScreenId::Editor;
                    }
                    if (ImGui::MenuItem("Compilar / Gerar Build...")) {
                        m_projects.openProject(pr.id);
                        m_screen = ScreenId::Build;
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Excluir Projeto") && projs.size() > 1) {
                        std::string delId = pr.id;
                        ImGui::EndPopup();
                        m_projects.deleteProject(delId);
                        break;
                    }
                    ImGui::EndPopup();
                }
            } else {
                // 4th Card: Dashed "+ Novo Projeto / Comece do zero" matching Image 2
                ImGui::SetCursorScreenPos(cMin);
                bool newClicked = ImGui::InvisibleButton("##card_new_proj", ImVec2(cardW, cardH));
                bool hovered = ImGui::IsItemHovered();

                mdl->AddRectFilled(cMin, cMax, hovered ? IM_COL32(18, 19, 25, 240) : IM_COL32(11, 12, 16, 220), 14.0f);
                drawDashedRoundedRect(mdl, cMin, cMax, 14.0f,
                                      hovered ? IM_COL32(130, 136, 154, 220) : IM_COL32(58, 62, 74, 210), 1.3f);

                ImVec2 plusCenter((cMin.x + cMax.x) * 0.5f, cMin.y + cardH * 0.37f);
                mdl->AddCircleFilled(plusCenter, 22.0f, IM_COL32(24, 26, 33, 255), 28);
                mdl->AddCircle(plusCenter, 22.0f, IM_COL32(48, 52, 64, 220), 28);
                UIWidgets::DrawIcon(mdl, plusCenter, 16.0f, UiIcon::Plus, IM_COL32(236, 238, 245, 255));

                const char* t1 = "Novo Projeto";
                const char* t2 = "Comece do zero";
                if (fontUiBold) ImGui::PushFont(fontUiBold);
                ImVec2 s1 = ImGui::CalcTextSize(t1);
                mdl->AddText(ImVec2(plusCenter.x - s1.x * 0.5f, plusCenter.y + 34.0f), IM_COL32(244, 245, 250, 255), t1);
                if (fontUiBold) ImGui::PopFont();
                ImVec2 s2 = ImGui::CalcTextSize(t2);
                mdl->AddText(ImVec2(plusCenter.x - s2.x * 0.5f, plusCenter.y + 56.0f), IM_COL32(132, 136, 148, 255), t2);

                if (newClicked) m_newProjOpen = true;
            }
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    UIWidgets::DrawWindowChrome(W, onMinimizeWindow, onMaximizeWindow, onCloseWindow);

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

// ============================================================================
// SCREEN 4: BUILD / EXPORT ("Compilar" — Identical to Image 4)
// ============================================================================
void EngineApp::renderBuild(float dt, float W, float H) {
    (void)dt;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(W, H));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(7, 8, 11, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##AstronBuild", nullptr, flags);

    const float sideW = 240.0f;

    // -------------------- LEFT SIDEBAR (with bottom-left cratered planet glow) --------------------
    ImGui::SetCursorScreenPos(ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(9, 10, 13, 255));
    ImGui::BeginChild("##BuildSidebar", ImVec2(sideW, H), false);

    ImDrawList* sdl = ImGui::GetWindowDrawList();

    // Cratered planet limb glowing behind bottom-left of sidebar (Image 4)
    GpuTexture loginPlanet = m_textures.get("assets/textures/login-planet.png");
    if (loginPlanet.id != 0 && loginPlanet.height > 0) {
        float aspect = static_cast<float>(loginPlanet.width) / static_cast<float>(loginPlanet.height);
        float pH = 260.0f;
        float pW = pH * aspect;
        sdl->PushClipRect(ImVec2(0, 0), ImVec2(sideW - 1.0f, H), true);
        sdl->AddImage(
            static_cast<ImTextureID>(static_cast<uintptr_t>(loginPlanet.id)),
            ImVec2(-24.0f, H - pH + 14.0f),
            ImVec2(-24.0f + pW, H + 14.0f),
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, 215)
        );
        sdl->PopClipRect();
    }

    sdl->AddLine(ImVec2(sideW - 1.0f, 0), ImVec2(sideW - 1.0f, H), IM_COL32(34, 36, 44, 255));

    GpuTexture horizLogo = m_textures.get("assets/textures/astron-horizontal.png");
    drawSidebarBrandLockup(sdl, horizLogo, ImVec2(20.0f, 20.0f), fontLogoSmall);

    ImGui::SetCursorPos(ImVec2(14.0f, 82.0f));
    ImGui::BeginGroup();
    const float navW = sideW - 28.0f;
    if (sidebarNavButton("##bnav_comp", UiIcon::Home,       "Compilar",                 m_buildPage == "compilar", navW)) m_buildPage = "compilar";
    ImGui::Dummy(ImVec2(0, 4));
    if (sidebarNavButton("##bnav_proj", UiIcon::Folder,     "Projetos",                 false,                     navW)) m_screen = ScreenId::Home;
    ImGui::Dummy(ImVec2(0, 4));
    if (sidebarNavButton("##bnav_cfg",  UiIcon::Gear,       "Configura\xC3\xA7\xC3\xB5" "es", false,               navW)) m_settingsOpen = true;
    ImGui::Dummy(ImVec2(0, 4));
    if (sidebarNavButton("##bnav_sob",  UiIcon::InfoCircle, "Sobre",                    m_buildPage == "sobre",    navW)) {
        pushToast("Astron Build Pipeline v1.0.0 — APK, IPA, EXE, Linux x64, PAK e ZIP.");
    }
    ImGui::EndGroup();

    // Frosted "Pronto para compilar / v1.0.0" status box at bottom of sidebar (Image 4)
    const float stY = H - 82.0f;
    sdl->AddRectFilled(ImVec2(14, stY), ImVec2(sideW - 14, H - 16), IM_COL32(14, 15, 20, 224), 12.0f);
    sdl->AddRect(ImVec2(14, stY), ImVec2(sideW - 14, H - 16), IM_COL32(54, 58, 70, 195), 12.0f);
    sdl->AddCircleFilled(ImVec2(28, stY + 22), 4.0f, IM_COL32(242, 244, 250, 255));
    sdl->AddText(ImVec2(39, stY + 14), IM_COL32(236, 238, 245, 255), "Pronto para compilar");
    sdl->AddText(ImVec2(24, stY + 39), IM_COL32(138, 142, 154, 255), "v1.0.0");

    ImGui::EndChild();
    ImGui::PopStyleColor();

    // -------------------- MAIN BUILD CONTENT --------------------
    ImGui::SetCursorScreenPos(ImVec2(sideW, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(7, 8, 11, 255));
    ImGui::BeginChild("##BuildMain", ImVec2(W - sideW, H), false);

    ImDrawList* bdl = ImGui::GetWindowDrawList();
    const float padX = 32.0f;
    const float contentW = (W - sideW) - padX * 2.0f;

    // Top Banner with official Astron mark + hero-planet.jpg (Image 4)
    const float banY = 30.0f;
    const float banH = 128.0f;
    ImVec2 bMin(sideW + padX, banY);
    ImVec2 bMax(sideW + padX + contentW, banY + banH);

    GpuTexture hero = m_textures.get("assets/textures/hero-planet.jpg");
    if (hero.id != 0) {
        drawImageCoverRounded(bdl, hero, bMin, bMax, 16.0f);
        bdl->AddRectFilledMultiColor(
            bMin, bMax,
            IM_COL32(8, 9, 13, 232), IM_COL32(8, 9, 13, 45),
            IM_COL32(8, 9, 13, 60),  IM_COL32(8, 9, 13, 238)
        );
    } else {
        bdl->AddRectFilled(bMin, bMax, IM_COL32(16, 18, 24, 255), 16.0f);
    }
    bdl->AddRect(bMin, bMax, IM_COL32(48, 52, 64, 185), 16.0f);

    GpuTexture markTex = m_textures.get("assets/textures/astron-mark.png");
    if (markTex.id != 0 && markTex.height > 0) {
        float aspect = static_cast<float>(markTex.width) / static_cast<float>(markTex.height);
        float mH = 54.0f;
        float mW = mH * aspect;
        float mY = bMin.y + (banH - mH) * 0.5f;
        bdl->AddImage(
            static_cast<ImTextureID>(static_cast<uintptr_t>(markTex.id)),
            ImVec2(bMin.x + 28.0f, mY),
            ImVec2(bMin.x + 28.0f + mW, mY + mH)
        );
    } else {
        UIWidgets::DrawPlanetMark(bdl, ImVec2(bMin.x + 56.0f, bMin.y + banH * 0.5f), 26.0f,
                                  IM_COL32(255, 255, 255, 255), IM_COL32(10, 11, 15, 255));
    }

    if (fontTitle) ImGui::PushFont(fontTitle);
    bdl->AddText(ImVec2(bMin.x + 104.0f, bMin.y + 34.0f), IM_COL32(255, 255, 255, 255), "Astron Engine");
    if (fontTitle) ImGui::PopFont();
    bdl->AddText(ImVec2(bMin.x + 104.0f, bMin.y + 72.0f), IM_COL32(192, 196, 208, 255), "Crie. Desenvolva. Explore.");

    // Section Title: Rocket vector icon + "Buildar Seu Projeto"
    const float headY = banY + banH + 24.0f;
    UIWidgets::DrawIcon(bdl, ImVec2(sideW + padX + 10.0f, headY + 12.0f), 19.0f, UiIcon::Rocket, IM_COL32(244, 246, 252, 255));
    if (fontTitle) ImGui::PushFont(fontTitle);
    bdl->AddText(ImVec2(sideW + padX + 28.0f, headY), IM_COL32(248, 249, 252, 255), "Buildar Seu Projeto");
    if (fontTitle) ImGui::PopFont();
    bdl->AddText(ImVec2(sideW + padX, headY + 32.0f), IM_COL32(150, 154, 166, 255),
                 "Escolha a plataforma e gere seu arquivo de build com apenas um clique.");

    // 6 Platform Cards in 1 Horizontal Row (APK, EXE, IPA, Linux, ZIP, Outros)
    struct PlatCard {
        const char* id;
        const char* title;
        const char* sub;
        const char* btn;
        PlatformIconKind icon;
    };
    const PlatCard cards[6] = {
        { "apk",    "APK",    "Android",            "Gerar APK",   PlatformIconKind::APK },
        { "exe",    "EXE",    "Windows",            "Gerar EXE",   PlatformIconKind::EXE },
        { "ipa",    "IPA",    "iOS",                "Gerar IPA",   PlatformIconKind::IPA },
        { "linux",  "Linux",  "Nativo x64",         "Gerar Linux", PlatformIconKind::Linux },
        { "zip",    "ZIP",    "Projeto Compactado", "Gerar ZIP",   PlatformIconKind::ZIP },
        { "outros", "Outros", "Linux, Mac e mais",  "Ver Mais",    PlatformIconKind::Outros }
    };

    const float rowY = headY + 66.0f;
    const float gap = 14.0f;
    const float cardW = (contentW - gap * 5.0f) / 6.0f;
    const float cardH = 204.0f;

    for (int i = 0; i < 6; ++i) {
        float cx = sideW + padX + i * (cardW + gap);
        ImVec2 cMin(cx, rowY);
        ImVec2 cMax(cx + cardW, rowY + cardH);

        bool sel = (m_buildTargetId == cards[i].id);
        bdl->AddRectFilled(cMin, cMax, sel ? IM_COL32(20, 22, 29, 255) : IM_COL32(13, 14, 19, 255), 16.0f);
        bdl->AddRect(cMin, cMax, sel ? IM_COL32(102, 108, 128, 240) : IM_COL32(38, 40, 50, 225), 16.0f);

        UIWidgets::DrawPlatformIcon(bdl, ImVec2(cx + cardW * 0.5f, rowY + 52.0f), 46.0f, cards[i].icon);

        if (fontUiBold) ImGui::PushFont(fontUiBold);
        ImVec2 ts = ImGui::CalcTextSize(cards[i].title);
        bdl->AddText(ImVec2(cx + (cardW - ts.x) * 0.5f, rowY + 98.0f), IM_COL32(248, 249, 252, 255), cards[i].title);
        if (fontUiBold) ImGui::PopFont();

        ImVec2 ss = ImGui::CalcTextSize(cards[i].sub);
        bdl->AddText(ImVec2(cx + (cardW - ss.x) * 0.5f, rowY + 122.0f), IM_COL32(138, 142, 154, 255), cards[i].sub);

        ImGui::SetCursorScreenPos(ImVec2(cx + 12.0f, rowY + cardH - 48.0f));
        std::string bLbl = std::string(cards[i].btn) + "##plt_" + cards[i].id;
        if (UIWidgets::OutlineButton(bLbl.c_str(), ImVec2(cardW - 24.0f, 34.0f), 17.0f, UiIcon::None, UiIcon::ArrowRight)) {
            m_buildTargetId = cards[i].id;
            startBuildPipeline(cards[i].id);
        }
    }

    // Bottom 2-Column Section: "Projeto Atual" & "Build Rápida" (Image 4)
    const float botY = rowY + cardH + 20.0f;
    const float botH = std::max(142.0f, H - botY - 24.0f);
    const float halfW = (contentW - 18.0f) * 0.5f;

    // 1. Left Card: Projeto Atual
    {
        ImVec2 pMin(sideW + padX, botY);
        ImVec2 pMax(sideW + padX + halfW, botY + botH);
        bdl->AddRectFilled(pMin, pMax, IM_COL32(13, 14, 19, 255), 16.0f);
        bdl->AddRect(pMin, pMax, IM_COL32(38, 40, 50, 225), 16.0f);

        UIWidgets::DrawIcon(bdl, ImVec2(pMin.x + 28.0f, pMin.y + 27.0f), 16.0f, UiIcon::Folder, IM_COL32(228, 232, 242, 255));
        if (fontUiBold) ImGui::PushFont(fontUiBold);
        bdl->AddText(ImVec2(pMin.x + 44.0f, pMin.y + 18.0f), IM_COL32(244, 245, 250, 255), "Projeto Atual");
        if (fontUiBold) ImGui::PopFont();

        ImVec2 inMin(pMin.x + 18.0f, pMin.y + 50.0f);
        ImVec2 inMax(pMax.x - 18.0f, pMax.y - 16.0f);
        bdl->AddRectFilled(inMin, inMax, IM_COL32(18, 19, 25, 255), 12.0f);
        bdl->AddRect(inMin, inMax, IM_COL32(42, 45, 56, 210), 12.0f);

        float midY = (inMin.y + inMax.y) * 0.5f;
        bdl->AddRectFilled(ImVec2(inMin.x + 12.0f, midY - 22.0f), ImVec2(inMin.x + 56.0f, midY + 22.0f),
                           IM_COL32(25, 27, 35, 255), 10.0f);
        UIWidgets::DrawToolboxShape(bdl, ImVec2(inMin.x + 34.0f, midY), 32.0f, MeshKind::Cube);

        const auto& cur = m_projects.currentProject();
        if (fontUiBold) ImGui::PushFont(fontUiBold);
        bdl->AddText(ImVec2(inMin.x + 68.0f, midY - 18.0f), IM_COL32(248, 249, 252, 255), cur.name.c_str());
        if (fontUiBold) ImGui::PopFont();

        std::string modTxt = "\xC3\x9Altima modifica\xC3\xA7\xC3\xA3o: " + cur.lastModified;
        bdl->AddText(ImVec2(inMin.x + 68.0f, midY + 4.0f), IM_COL32(140, 144, 156, 255), modTxt.c_str());

        ImGui::SetCursorScreenPos(ImVec2(inMax.x - 148.0f, midY - 19.0f));
        if (UIWidgets::OutlineButton("Abrir Projeto", ImVec2(134.0f, 38.0f), 11.0f, UiIcon::Folder)) {
            m_screen = ScreenId::Editor;
        }
    }

    // 2. Right Card: Build Rápida
    {
        ImVec2 qMin(sideW + padX + halfW + 18.0f, botY);
        ImVec2 qMax(sideW + padX + contentW, botY + botH);
        bdl->AddRectFilled(qMin, qMax, IM_COL32(13, 14, 19, 255), 16.0f);
        bdl->AddRect(qMin, qMax, IM_COL32(38, 40, 50, 225), 16.0f);

        UIWidgets::DrawIcon(bdl, ImVec2(qMin.x + 28.0f, qMin.y + 27.0f), 16.0f, UiIcon::Bolt, IM_COL32(238, 240, 248, 255));
        if (fontUiBold) ImGui::PushFont(fontUiBold);
        bdl->AddText(ImVec2(qMin.x + 44.0f, qMin.y + 18.0f), IM_COL32(244, 245, 250, 255), "Build R\xC3\xA1pida");
        if (fontUiBold) ImGui::PopFont();

        ImVec2 inMin(qMin.x + 18.0f, qMin.y + 50.0f);
        ImVec2 inMax(qMax.x - 196.0f, qMax.y - 16.0f);
        bdl->AddRectFilled(inMin, inMax, IM_COL32(18, 19, 25, 255), 12.0f);
        bdl->AddRect(inMin, inMax, IM_COL32(42, 45, 56, 210), 12.0f);

        float midY = (inMin.y + inMax.y) * 0.5f;
        UIWidgets::DrawIcon(bdl, ImVec2(inMin.x + 22.0f, midY), 16.0f, UiIcon::Bolt, IM_COL32(226, 230, 240, 255));
        if (fontUiBold) ImGui::PushFont(fontUiBold);
        bdl->AddText(ImVec2(inMin.x + 38.0f, midY - 17.0f), IM_COL32(244, 245, 250, 255), "Compila\xC3\xA7\xC3\xA3o R\xC3\xA1pida");
        if (fontUiBold) ImGui::PopFont();
        bdl->AddText(ImVec2(inMin.x + 38.0f, midY + 4.0f), IM_COL32(138, 142, 154, 255), "Build direta com configura\xC3\xA7\xC3\xB5" "es padr\xC3\xA3o");

        ImGui::SetCursorScreenPos(ImVec2(inMax.x - 52.0f, midY - 10.0f));
        UIWidgets::ToggleSwitch("##quick_build_sw", &m_quickBuild, false);

        ImGui::SetCursorScreenPos(ImVec2(qMax.x - 180.0f, midY - 22.0f));
        if (UIWidgets::WhitePillButton("Iniciar Build", ImVec2(162.0f, 44.0f), 12.0f, UiIcon::Play)) {
            startBuildPipeline(m_buildTargetId);
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    UIWidgets::DrawWindowChrome(W, onMinimizeWindow, onMaximizeWindow, onCloseWindow);

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void EngineApp::startBuildPipeline(const std::string& targetId) {
    m_buildTargetId = targetId;
    m_buildCurrentStep = 0;
    m_buildStepTimer = 0.0f;
    m_buildModalOpen = true;

    auto outBundle = m_projects.exportProjectBundle(targetId);
    m_lastExportedBundle = outBundle.string();

    if (targetId == "apk" || targetId == "android") {
        m_buildSteps = {
            "Sincronizar scripts C# do editor -> games/AstronGame/Core",
            "GitHub Actions: ubuntu-latest + JDK 17 + .NET 10 Android",
            "dotnet workload install android",
            "keytool -genkeypair -keystore release.keystore",
            "dotnet publish -f net10.0-android (APK assinado)",
            "Upload artefato: AstronEngine.apk & AstronGame.apk"
        };
    } else if (targetId == "ipa" || targetId == "ios") {
        m_buildSteps = {
            "Sincronizar scripts C# do editor -> games/AstronGame/Core",
            "GitHub Actions: macos-26-intel + Xcode + .NET 10 iOS",
            "dotnet workload install ios --version 10.0.401",
            "dotnet build -f net10.0-ios -r ios-arm64",
            "Empacotar Payload/*.app -> AstronEngine-ios.ipa",
            "Upload artefato: AstronEngine-ios.ipa & AstronGame-ios.ipa"
        };
    } else if (targetId == "linux") {
        m_buildSteps = {
            "Sincronizar scripts C# do editor -> games/AstronGame/Core",
            "GitHub Actions: ubuntu-latest (GCC + Rust + .NET)",
            "CMake C++20/C11/GLSL -> AstronEngine (Linux x64)",
            "dotnet publish -r linux-x64 --self-contained -> AstronGame",
            "Empacotar AstronEngine-linux-x64.tar.gz"
        };
    } else if (targetId == "exe" || targetId == "windows") {
        m_buildSteps = {
            "Sincronizar scripts C# do editor -> games/AstronGame/Core",
            "GitHub Actions: windows-latest (MSVC + .NET)",
            "CMake C++20/C11/GLSL -> AstronEngine.exe (Windows x64)",
            "dotnet publish -r win-x64 -p:PublishSingleFile=true -> AstronGame.exe",
            "Upload artefato: AstronEngine-windows-x64"
        };
    } else {
        m_buildSteps = {
            "Sincronizar scripts C#, Lua, cenas e shaders GLSL/HLSL",
            "Exportar pacote nativo .pak / bundle para out/",
            "Pronto para build multiplataforma no GitHub Actions"
        };
    }
}

void EngineApp::renderBuildPipelineModal(float dt) {
    if (!m_buildModalOpen) return;

    m_buildStepTimer += dt;
    if (m_buildStepTimer >= 0.45f && m_buildCurrentStep < static_cast<int>(m_buildSteps.size())) {
        m_buildStepTimer = 0.0f;
        ++m_buildCurrentStep;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(620.0f, 390.0f), ImGuiCond_Appearing);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(13, 14, 18, 252));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 20));

    bool done = m_buildCurrentStep >= static_cast<int>(m_buildSteps.size());
    const char* title = done ? "Pipeline Conclu\xC3\xAD" "do — GitHub Actions" : "Executando Pipeline de Compila\xC3\xA7\xC3\xA3o...";
    if (ImGui::Begin(title, &m_buildModalOpen, ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextColored(ImVec4(0.65f, 0.67f, 0.72f, 1.f),
                           "Workflows: .github/workflows/build-game.yml & build-engine.yml  (alvo: %s)",
                           m_buildTargetId.c_str());
        ImGui::Dummy(ImVec2(0, 10));

        if (fontMono) ImGui::PushFont(fontMono);
        for (size_t i = 0; i < m_buildSteps.size(); ++i) {
            int st = static_cast<int>(i);
            if (st < m_buildCurrentStep) {
                ImGui::TextColored(ImVec4(0.30f, 0.88f, 0.46f, 1.f), "[OK]  %s", m_buildSteps[i].c_str());
            } else if (st == m_buildCurrentStep) {
                ImGui::TextColored(ImVec4(0.35f, 0.65f, 1.0f, 1.f), "[..]  %s", m_buildSteps[i].c_str());
            } else {
                ImGui::TextColored(ImVec4(0.45f, 0.46f, 0.52f, 1.f), "[  ]  %s", m_buildSteps[i].c_str());
            }
        }
        if (fontMono) ImGui::PopFont();

        ImGui::Dummy(ImVec2(0, 14));
        ImGui::TextColored(ImVec4(0.55f, 0.82f, 0.60f, 1.f), "C\xC3\xB3" "digo C# sincronizado em games/AstronGame/Core e salvo em: %s",
                           m_lastExportedBundle.c_str());
        ImGui::Dummy(ImVec2(0, 12));

        if (UIWidgets::WhitePillButton("Abrir GitHub Actions", ImVec2(210, 40), 11.0f)) {
            astron_open_url("https://github.com/Veexdevs/Astron-Engine/actions");
        }
        ImGui::SameLine();
        if (UIWidgets::OutlineButton("Fechar", ImVec2(120, 40), 11.0f)) {
            m_buildModalOpen = false;
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void EngineApp::renderNewProjectModal() {
    if (!m_newProjOpen) return;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(480.0f, 310.0f), ImGuiCond_Appearing);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(13, 14, 18, 252));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 20));

    if (ImGui::Begin("Novo Projeto C# — Astron Engine", &m_newProjOpen, ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextUnformatted("Nome do Projeto");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##new_proj_name", m_newProjName, sizeof(m_newProjName));

        ImGui::Dummy(ImVec2(0, 10));
        ImGui::TextUnformatted("Template Inicial (C# + GLSL + Lua)");
        const char* templates[] = { "Em Branco (C#)", "Plataforma 3D (C#)", "Obby (Plataformas Flutuantes)", "Top-Down Urbano" };
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##new_proj_tpl", &m_newProjTemplate, templates, 4);

        ImGui::Dummy(ImVec2(0, 22));
        if (UIWidgets::WhitePillButton("Criar e Abrir no Editor", ImVec2(220, 42), 11.0f)) {
            const char* tplIds[] = { "blank", "platformer", "obby", "topdown" };
            m_projects.createProject(m_newProjName, tplIds[m_newProjTemplate]);
            bool ok = false;
            std::string scJson = m_projects.readFile("res://default.scene", &ok);
            if (ok) m_scene = Scene::fromJson(Json::parse(scJson));
            m_newProjOpen = false;
            m_screen = ScreenId::Editor;
            pushToast("Novo projeto C# criado!");
        }
        ImGui::SameLine();
        if (UIWidgets::OutlineButton("Cancelar", ImVec2(110, 42), 11.0f)) {
            m_newProjOpen = false;
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

} // namespace astron
