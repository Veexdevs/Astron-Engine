#include "EngineApp.hpp"
#include "UIWidgets.hpp"
#include "../core_c/astron_platform.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace astron {

namespace {

bool drawSidebarNavButton(const char* id, const char* label, bool active, float width) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float h = 48.0f;

    bool clicked = ImGui::InvisibleButton(id, ImVec2(width, h));
    bool hovered = ImGui::IsItemHovered();

    if (active) {
        dl->AddRectFilled(p, ImVec2(p.x + width, p.y + h), IM_COL32(22, 23, 28, 255), 12.0f);
        dl->AddRectFilled(ImVec2(p.x - 12.0f, p.y + 11.0f), ImVec2(p.x - 8.0f, p.y + h - 11.0f),
                          IM_COL32(255, 255, 255, 255), 3.0f);
    } else if (hovered) {
        dl->AddRectFilled(p, ImVec2(p.x + width, p.y + h), IM_COL32(15, 16, 20, 255), 12.0f);
    }

    ImU32 tc = active ? IM_COL32(255, 255, 255, 255) : IM_COL32(198, 200, 208, 255);
    ImVec2 ts = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(p.x + 18.0f, p.y + (h - ts.y) * 0.5f), tc, label);
    return clicked;
}

void drawDashedRect(ImDrawList* dl, ImVec2 mn, ImVec2 mx, ImU32 col, float dashLen = 8.0f, float gapLen = 6.0f) {
    auto segH = [&](float x0, float x1, float y) {
        for (float x = x0; x < x1; x += dashLen + gapLen) {
            dl->AddLine(ImVec2(x, y), ImVec2(std::min(x + dashLen, x1), y), col, 1.5f);
        }
    };
    auto segV = [&](float y0, float y1, float x) {
        for (float y = y0; y < y1; y += dashLen + gapLen) {
            dl->AddLine(ImVec2(x, y), ImVec2(x, std::min(y + dashLen, y1)), col, 1.5f);
        }
    };
    segH(mn.x + 8.0f, mx.x - 8.0f, mn.y);
    segH(mn.x + 8.0f, mx.x - 8.0f, mx.y);
    segV(mn.y + 8.0f, mx.y - 8.0f, mn.x);
    segV(mn.y + 8.0f, mx.y - 8.0f, mx.x);
}

} // namespace

void EngineApp::renderHome(float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(3, 3, 5, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##HomeHub", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float sideW = 264.0f;

    // Sidebar background
    dl->AddRectFilled(ImVec2(0, 0), ImVec2(sideW, h), IM_COL32(6, 6, 8, 255));
    dl->AddLine(ImVec2(sideW, 0), ImVec2(sideW, h), IM_COL32(24, 25, 30, 255));

    // Sidebar Brand: Official Astron Engine horizontal logo texture (with vector fallback)
    GpuTexture horizLogo = m_textures.get("assets/textures/astron-horizontal.png");
    if (horizLogo.id != 0) {
        const float lw = 216.0f;
        const float lh = lw * (static_cast<float>(horizLogo.height) / std::max(1, horizLogo.width));
        dl->AddImage(static_cast<ImTextureID>(static_cast<uintptr_t>(horizLogo.id)),
                     ImVec2(22.0f, 26.0f),
                     ImVec2(22.0f + lw, 26.0f + lh));
    } else {
        UIWidgets::DrawPlanetMark(dl, ImVec2(50.0f, 52.0f), 26.0f, IM_COL32(255, 255, 255, 255), IM_COL32(6, 6, 8, 255));
        if (fontLogoMed) ImGui::PushFont(fontLogoMed);
        dl->AddText(ImVec2(88.0f, 32.0f), IM_COL32(255, 255, 255, 255), "Astron");
        if (fontLogoMed) ImGui::PopFont();
        if (fontLogoSmall) ImGui::PushFont(fontLogoSmall);
        dl->AddText(ImVec2(138.0f, 58.0f), IM_COL32(215, 216, 224, 255), "E n g i n e");
        if (fontLogoSmall) ImGui::PopFont();
    }

    // Sidebar Nav Items (matching exact Portuguese labels from mockup)
    ImGui::SetCursorScreenPos(ImVec2(14.0f, 114.0f));
    ImGui::BeginGroup();
    struct NavItem { const char* id; const char* label; };
    const NavItem navs[] = {
        { "inicio",     "⌂   Início" },
        { "projetos",   "📁  Meus Projetos" },
        { "biblioteca", "📦  Bibioteca" },
        { "loja",       "🛍   Loja" },
        { "config",     "⚙   Configurações" },
        { "sobre",      "ℹ   Sobre" }
    };
    for (const auto& n : navs) {
        if (drawSidebarNavButton(n.id, n.label, m_homePage == n.id, sideW - 28.0f)) {
            if (std::string(n.id) == "config") m_settingsOpen = true;
            else m_homePage = n.id;
        }
        ImGui::Dummy(ImVec2(0, 3));
    }
    ImGui::EndGroup();

    // Bottom Account / Online Status Card
    ImVec2 accMin(14.0f, h - 108.0f), accMax(sideW - 14.0f, h - 16.0f);
    dl->AddRectFilled(accMin, accMax, IM_COL32(10, 11, 14, 255), 14.0f);
    dl->AddRect(accMin, accMax, IM_COL32(34, 35, 42, 255), 14.0f);
    dl->AddCircleFilled(ImVec2(accMin.x + 26.0f, accMin.y + 28.0f), 16.0f, IM_COL32(22, 24, 30, 255));
    dl->AddText(ImVec2(accMin.x + 21.0f, accMin.y + 19.0f), IM_COL32(255, 255, 255, 255), "⚡");
    dl->AddText(ImVec2(accMin.x + 52.0f, accMin.y + 13.0f), IM_COL32(245, 246, 250, 255), "Astron Engine");
    dl->AddText(ImVec2(accMin.x + 52.0f, accMin.y + 31.0f), IM_COL32(138, 140, 150, 255), "v1.0.0");
    dl->AddLine(ImVec2(accMin.x + 14.0f, accMin.y + 56.0f), ImVec2(accMax.x - 14.0f, accMin.y + 56.0f), IM_COL32(30, 32, 38, 255));
    dl->AddCircleFilled(ImVec2(accMin.x + 22.0f, accMin.y + 73.0f), 4.5f, IM_COL32(47, 209, 88, 255));
    dl->AddText(ImVec2(accMin.x + 34.0f, accMin.y + 65.0f), IM_COL32(205, 207, 214, 255), "Online");

    // Top-Right Window Chrome + Gear/User Circle Buttons
    UIWidgets::DrawWindowChrome(w, onMinimizeWindow, onMaximizeWindow, [this]() { m_screen = ScreenId::Login; });

    ImGui::SetCursorScreenPos(ImVec2(w - 134.0f, 44.0f));
    if (UIWidgets::OutlineButton("⚙##top_cfg", ImVec2(42, 42), 21.0f)) m_settingsOpen = true;
    ImGui::SameLine(0, 10);
    if (UIWidgets::OutlineButton("👤##top_usr", ImVec2(42, 42), 21.0f)) m_screen = ScreenId::Build;

    // Main Content Region
    const float mainX = sideW + 32.0f;
    const float mainW = w - sideW - 64.0f;

    if (m_homePage == "inicio" || m_homePage == "projetos") {
        float curY = 98.0f;

        if (m_homePage == "inicio") {
            const float heroH = 248.0f;
            ImVec2 hMin(mainX, curY), hMax(mainX + mainW, curY + heroH);
            dl->AddRectFilled(hMin, hMax, IM_COL32(8, 9, 12, 255), 20.0f);

            GpuTexture heroTex = m_textures.get("assets/textures/hero-planet.jpg");
            if (heroTex.id != 0) {
                dl->AddImageRounded(static_cast<ImTextureID>(static_cast<uintptr_t>(heroTex.id)),
                                    hMin, hMax, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 235), 20.0f);
            }
            dl->AddRectFilledMultiColor(hMin, ImVec2(hMin.x + mainW * 0.68f, hMax.y),
                                        IM_COL32(0, 0, 0, 240), IM_COL32(0, 0, 0, 40),
                                        IM_COL32(0, 0, 0, 40), IM_COL32(0, 0, 0, 240));
            dl->AddRect(hMin, hMax, IM_COL32(34, 36, 44, 255), 20.0f);

            if (fontTitle) ImGui::PushFont(fontTitle);
            dl->AddText(ImVec2(hMin.x + 44.0f, hMin.y + 46.0f), IM_COL32(255, 255, 255, 255), "Crie. Desenvolva. Explore.");
            if (fontTitle) ImGui::PopFont();

            dl->AddText(ImVec2(hMin.x + 44.0f, hMin.y + 92.0f), IM_COL32(212, 214, 222, 255), "Sua imaginação, sem limites.");

            ImGui::SetCursorScreenPos(ImVec2(hMin.x + 44.0f, hMin.y + 142.0f));
            if (UIWidgets::WhitePillButton("+   Novo Projeto", ImVec2(198.0f, 50.0f), 25.0f)) {
                m_newProjOpen = true;
            }
            curY += heroH + 28.0f;
        }

        // Section Header: Projetos Recentes + Ver Todos >
        if (fontUiBold) ImGui::PushFont(fontUiBold);
        dl->AddText(ImVec2(mainX, curY + 6.0f), IM_COL32(255, 255, 255, 255),
                    m_homePage == "inicio" ? "🕒  Projetos Recentes" : "📁  Meus Projetos");
        if (fontUiBold) ImGui::PopFont();

        ImGui::SetCursorScreenPos(ImVec2(mainX + mainW - 128.0f, curY));
        if (UIWidgets::OutlineButton(m_homePage == "inicio" ? "Ver Todos  >" : "Compilar  >", ImVec2(128.0f, 38.0f), 11.0f)) {
            if (m_homePage == "inicio") m_homePage = "projetos";
            else m_screen = ScreenId::Build;
        }

        curY += 52.0f;

        // 4-column Project Cards Grid
        const auto& projs = m_projects.projects();
        const int cols = 4;
        const float gap = 18.0f;
        const float cardW = (mainW - gap * (cols - 1)) / cols;
        const float cardH = std::min(238.0f, std::max(195.0f, h - curY - 24.0f));

        size_t maxShow = (m_homePage == "inicio") ? std::min<size_t>(3, projs.size()) : projs.size();
        for (size_t i = 0; i < maxShow; ++i) {
            int col = static_cast<int>(i) % cols;
            int row = static_cast<int>(i) / cols;
            ImVec2 cMin(mainX + col * (cardW + gap), curY + row * (cardH + gap));
            ImVec2 cMax(cMin.x + cardW, cMin.y + cardH);
            const float thumbH = cardH * 0.66f;

            dl->AddRectFilled(cMin, cMax, IM_COL32(10, 11, 14, 255), 16.0f);

            GpuTexture th = m_textures.get(projs[i].thumbAsset);
            if (th.id != 0) {
                dl->AddImageRounded(static_cast<ImTextureID>(static_cast<uintptr_t>(th.id)),
                                    cMin, ImVec2(cMax.x, cMin.y + thumbH),
                                    ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255),
                                    16.0f, ImDrawFlags_RoundCornersTop);
            }
            dl->AddRect(cMin, cMax, IM_COL32(34, 36, 44, 255), 16.0f);

            if (fontUiBold) ImGui::PushFont(fontUiBold);
            dl->AddText(ImVec2(cMin.x + 14.0f, cMin.y + thumbH + 13.0f), IM_COL32(250, 250, 252, 255), projs[i].name.c_str());
            if (fontUiBold) ImGui::PopFont();

            std::string createdLine = "Criado em " + projs[i].createdDate;
            dl->AddText(ImVec2(cMin.x + 14.0f, cMin.y + thumbH + 39.0f), IM_COL32(142, 144, 154, 255), createdLine.c_str());

            // "..." button on top-right of card
            std::string menuBtnId = "•••##pmenu_" + projs[i].id;
            std::string popupId   = "proj_popup_" + projs[i].id;
            ImGui::SetCursorScreenPos(ImVec2(cMax.x - 42.0f, cMin.y + 10.0f));
            if (UIWidgets::OutlineButton(menuBtnId.c_str(), ImVec2(32, 30), 15.0f)) {
                ImGui::OpenPopup(popupId.c_str());
            }

            // Click main card body to open in Editor
            ImGui::SetCursorScreenPos(ImVec2(cMin.x, cMin.y + 42.0f));
            std::string openHitId = "##open_proj_" + projs[i].id;
            if (ImGui::InvisibleButton(openHitId.c_str(), ImVec2(cardW, cardH - 42.0f))) {
                m_projects.openProject(projs[i].id);
                bool ok = false;
                std::string scJson = m_projects.readFile("res://default.scene", &ok);
                if (ok) m_scene = Scene::fromJson(Json::parse(scJson));
                m_screen = ScreenId::Editor;
            }

            if (ImGui::BeginPopup(popupId.c_str())) {
                if (ImGui::MenuItem("Abrir no Editor")) {
                    m_projects.openProject(projs[i].id);
                    m_screen = ScreenId::Editor;
                }
                if (ImGui::MenuItem("Compilar (APK / IPA / EXE / Linux)")) {
                    m_projects.openProject(projs[i].id);
                    m_screen = ScreenId::Build;
                }
                if (projs.size() > 1 && ImGui::MenuItem("Excluir Projeto")) {
                    m_projects.deleteProject(projs[i].id);
                    ImGui::EndPopup();
                    break;
                }
                ImGui::EndPopup();
            }
        }

        // Dashed "+ Novo Projeto / Comece do zero" card
        {
            int col = static_cast<int>(maxShow) % cols;
            int row = static_cast<int>(maxShow) / cols;
            ImVec2 nMin(mainX + col * (cardW + gap), curY + row * (cardH + gap));
            ImVec2 nMax(nMin.x + cardW, nMin.y + cardH);
            drawDashedRect(dl, nMin, nMax, IM_COL32(66, 68, 78, 255));

            float ncx = (nMin.x + nMax.x) * 0.5f;
            float ncy = (nMin.y + nMax.y) * 0.5f;
            dl->AddLine(ImVec2(ncx - 16, ncy - 24), ImVec2(ncx + 16, ncy - 24), IM_COL32(245, 245, 250, 255), 2.5f);
            dl->AddLine(ImVec2(ncx, ncy - 40), ImVec2(ncx, ncy - 8), IM_COL32(245, 245, 250, 255), 2.5f);

            ImVec2 t1 = ImGui::CalcTextSize("Novo Projeto");
            dl->AddText(ImVec2(ncx - t1.x * 0.5f, ncy + 8.0f), IM_COL32(245, 245, 250, 255), "Novo Projeto");
            ImVec2 t2 = ImGui::CalcTextSize("Comece do zero");
            dl->AddText(ImVec2(ncx - t2.x * 0.5f, ncy + 31.0f), IM_COL32(140, 142, 152, 255), "Comece do zero");

            ImGui::SetCursorScreenPos(nMin);
            if (ImGui::InvisibleButton("##dashed_new_proj", ImVec2(cardW, cardH))) {
                m_newProjOpen = true;
            }
        }
    } else if (m_homePage == "biblioteca" || m_homePage == "loja") {
        ImGui::SetCursorScreenPos(ImVec2(mainX, 110.0f));
        ImGui::BeginGroup();
        if (fontTitle) ImGui::PushFont(fontTitle);
        ImGui::TextUnformatted(m_homePage == "biblioteca" ? "Bibioteca de Recursos C# & GLSL" : "Loja Astron");
        if (fontTitle) ImGui::PopFont();
        ImGui::Dummy(ImVec2(0, 14));
        ImGui::TextColored(ImVec4(0.65f, 0.67f, 0.72f, 1.f),
                           "Kits nativos em C#, malhas 3D e shaders GLSL 3.30 prontos para importar.");
        ImGui::Dummy(ImVec2(0, 18));
        if (UIWidgets::WhitePillButton("Importar Kit Plataforma C#", ImVec2(240, 44), 12.0f)) {
            pushToast("Kit Plataforma C# importado para o projeto atual ✓");
        }
        ImGui::EndGroup();
    } else if (m_homePage == "sobre") {
        const float acx = mainX + mainW * 0.5f;
        const float acy = h * 0.44f;
        GpuTexture aboutLockup = m_textures.get("assets/textures/astron-lockup.png");
        if (aboutLockup.id != 0) {
            const float lw = 290.0f;
            const float lh = lw * (static_cast<float>(aboutLockup.height) / std::max(1, aboutLockup.width));
            dl->AddImage(static_cast<ImTextureID>(static_cast<uintptr_t>(aboutLockup.id)),
                         ImVec2(acx - lw * 0.5f, acy - lh + 64.0f),
                         ImVec2(acx + lw * 0.5f, acy + 64.0f));
        } else {
            UIWidgets::DrawPlanetMark(dl, ImVec2(acx, acy - 70.0f), 58.0f, IM_COL32(255, 255, 255, 255), IM_COL32(3, 3, 5, 255));
            if (fontLogoBig) ImGui::PushFont(fontLogoBig);
            ImVec2 sA = ImGui::CalcTextSize("Astron");
            dl->AddText(ImVec2(acx - sA.x * 0.5f, acy + 8.0f), IM_COL32(255, 255, 255, 255), "Astron");
            if (fontLogoBig) ImGui::PopFont();
        }
        const char* infoLine = "Engine 100% Nativa em C++20 · C# (.NET) · C11 · GLSL · HLSL · Rust · Lua";
        ImVec2 sI = ImGui::CalcTextSize(infoLine);
        dl->AddText(ImVec2(acx - sI.x * 0.5f, acy + 86.0f), IM_COL32(175, 178, 188, 255), infoLine);
    }

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void EngineApp::renderBuild(float dt, float w, float h) {
    (void)dt;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(3, 3, 5, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##BuildScreen", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float sideW = 264.0f;

    // Sidebar with bottom-left glowing planet horizon matching Build mockup
    dl->AddRectFilled(ImVec2(0, 0), ImVec2(sideW, h), IM_COL32(6, 6, 8, 255));
    GpuTexture sidePlanet = m_textures.get("assets/textures/login-planet.png");
    if (sidePlanet.id != 0) {
        dl->PushClipRect(ImVec2(0, 0), ImVec2(sideW, h), true);
        dl->AddImage(static_cast<ImTextureID>(static_cast<uintptr_t>(sidePlanet.id)),
                     ImVec2(-110.0f, h - 300.0f), ImVec2(260.0f, h + 70.0f),
                     ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 195));
        dl->PopClipRect();
    }
    dl->AddLine(ImVec2(sideW, 0), ImVec2(sideW, h), IM_COL32(24, 25, 30, 255));

    GpuTexture buildHorizLogo = m_textures.get("assets/textures/astron-horizontal.png");
    if (buildHorizLogo.id != 0) {
        const float lw = 212.0f;
        const float lh = lw * (static_cast<float>(buildHorizLogo.height) / std::max(1, buildHorizLogo.width));
        dl->AddImage(static_cast<ImTextureID>(static_cast<uintptr_t>(buildHorizLogo.id)),
                     ImVec2(22.0f, 25.0f),
                     ImVec2(22.0f + lw, 25.0f + lh));
    } else {
        UIWidgets::DrawPlanetMark(dl, ImVec2(48.0f, 50.0f), 24.0f, IM_COL32(255, 255, 255, 255), IM_COL32(6, 6, 8, 255));
        if (fontLogoMed) ImGui::PushFont(fontLogoMed);
        dl->AddText(ImVec2(84.0f, 31.0f), IM_COL32(255, 255, 255, 255), "Astron");
        if (fontLogoMed) ImGui::PopFont();
        if (fontLogoSmall) ImGui::PushFont(fontLogoSmall);
        dl->AddText(ImVec2(134.0f, 56.0f), IM_COL32(215, 216, 224, 255), "E n g i n e");
        if (fontLogoSmall) ImGui::PopFont();
    }

    ImGui::SetCursorScreenPos(ImVec2(14.0f, 110.0f));
    ImGui::BeginGroup();
    if (drawSidebarNavButton("b_compilar", "⌂   Compilar", m_buildPage == "compilar", sideW - 28.0f)) m_buildPage = "compilar";
    ImGui::Dummy(ImVec2(0, 3));
    if (drawSidebarNavButton("b_projetos", "📁  Projetos", false, sideW - 28.0f)) { m_homePage = "projetos"; m_screen = ScreenId::Home; }
    ImGui::Dummy(ImVec2(0, 3));
    if (drawSidebarNavButton("b_config", "⚙   Configurações", false, sideW - 28.0f)) m_settingsOpen = true;
    ImGui::Dummy(ImVec2(0, 3));
    if (drawSidebarNavButton("b_sobre", "ℹ   Sobre", m_buildPage == "sobre", sideW - 28.0f)) m_buildPage = "sobre";
    ImGui::EndGroup();

    // Bottom status card: "Pronto para compilar / v1.0.0"
    ImVec2 stMin(14.0f, h - 102.0f), stMax(sideW - 14.0f, h - 16.0f);
    dl->AddRectFilled(stMin, stMax, IM_COL32(10, 11, 14, 230), 14.0f);
    dl->AddRect(stMin, stMax, IM_COL32(36, 38, 46, 255), 14.0f);
    dl->AddCircleFilled(ImVec2(stMin.x + 22.0f, stMin.y + 26.0f), 5.5f, IM_COL32(148, 150, 160, 255));
    dl->AddText(ImVec2(stMin.x + 36.0f, stMin.y + 18.0f), IM_COL32(226, 228, 236, 255), "Pronto para compilar");
    dl->AddLine(ImVec2(stMin.x + 14.0f, stMin.y + 48.0f), ImVec2(stMax.x - 14.0f, stMin.y + 48.0f), IM_COL32(30, 32, 38, 255));
    dl->AddText(ImVec2(stMin.x + 16.0f, stMin.y + 58.0f), IM_COL32(148, 150, 160, 255), "v1.0.0");

    UIWidgets::DrawWindowChrome(w, onMinimizeWindow, onMaximizeWindow, [this]() { m_screen = ScreenId::Home; });

    const float mainX = sideW + 30.0f;
    const float mainW = w - sideW - 60.0f;
    float curY = 42.0f;

    // Top Banner: Planet Mark + "Astron Engine" + "Crie. Desenvolva. Explore."
    const float banH = 154.0f;
    ImVec2 bMin(mainX, curY), bMax(mainX + mainW, curY + banH);
    dl->AddRectFilled(bMin, bMax, IM_COL32(8, 9, 12, 255), 18.0f);
    GpuTexture heroTex = m_textures.get("assets/textures/hero-planet.jpg");
    if (heroTex.id != 0) {
        dl->AddImageRounded(static_cast<ImTextureID>(static_cast<uintptr_t>(heroTex.id)),
                            bMin, bMax, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 220), 18.0f);
    }
    dl->AddRectFilledMultiColor(bMin, ImVec2(bMin.x + mainW * 0.72f, bMax.y),
                                IM_COL32(0, 0, 0, 235), IM_COL32(0, 0, 0, 50),
                                IM_COL32(0, 0, 0, 50), IM_COL32(0, 0, 0, 235));
    dl->AddRect(bMin, bMax, IM_COL32(34, 36, 44, 255), 18.0f);

    GpuTexture markTex = m_textures.get("assets/textures/astron-mark.png");
    if (markTex.id != 0) {
        const float ms = 118.0f;
        dl->AddImage(static_cast<ImTextureID>(static_cast<uintptr_t>(markTex.id)),
                     ImVec2(bMin.x + 92.0f, bMin.y + (banH - ms) * 0.5f),
                     ImVec2(bMin.x + 92.0f + ms, bMin.y + (banH + ms) * 0.5f));
    } else {
        UIWidgets::DrawPlanetMark(dl, ImVec2(bMin.x + 150.0f, bMin.y + banH * 0.5f), 54.0f,
                                  IM_COL32(255, 255, 255, 255), IM_COL32(6, 7, 10, 255));
    }
    if (fontTitle) ImGui::PushFont(fontTitle);
    dl->AddText(ImVec2(bMin.x + 258.0f, bMin.y + 42.0f), IM_COL32(255, 255, 255, 255), "Astron Engine");
    if (fontTitle) ImGui::PopFont();
    dl->AddText(ImVec2(bMin.x + 258.0f, bMin.y + 88.0f), IM_COL32(212, 214, 222, 255), "Crie. Desenvolva. Explore.");

    curY += banH + 22.0f;

    // Heading: Buildar Seu Projeto
    if (fontUiBold) ImGui::PushFont(fontUiBold);
    dl->AddText(ImVec2(mainX, curY), IM_COL32(255, 255, 255, 255), "🚀  Buildar Seu Projeto");
    if (fontUiBold) ImGui::PopFont();
    dl->AddText(ImVec2(mainX + 28.0f, curY + 22.0f), IM_COL32(156, 158, 168, 255),
                "Escolha a plataforma nativa e gere seu arquivo de build via GitHub Actions (.apk, .ipa, .exe e Linux).");

    curY += 54.0f;

    // 6 Platform Cards (100% Native targets: APK, IPA, EXE, Linux, ZIP, Outros — no HTML/Web!)
    struct PlatCard {
        const char* id;
        const char* title;
        const char* sub;
        const char* btn;
        PlatformIconKind icon;
    };
    const PlatCard cards[6] = {
        { "android", "APK",    "Android",            "Gerar APK  ->",   PlatformIconKind::APK },
        { "ios",     "IPA",    "iOS",                "Gerar IPA  ->",   PlatformIconKind::IPA },
        { "windows", "EXE",    "Windows",            "Gerar EXE  ->",   PlatformIconKind::EXE },
        { "linux",   "Linux",  "Linux x64",          "Gerar Linux ->",  PlatformIconKind::Linux },
        { "zip",     "ZIP",    "Projeto Compactado", "Gerar ZIP  ->",   PlatformIconKind::ZIP },
        { "all",     "Outros", "macOS, C++ e mais",  "Ver Mais   ->",   PlatformIconKind::Outros }
    };

    const float gap = 14.0f;
    const float cw = (mainW - gap * 5.0f) / 6.0f;
    const float ch = 196.0f;

    for (int i = 0; i < 6; ++i) {
        ImVec2 cMin(mainX + i * (cw + gap), curY);
        ImVec2 cMax(cMin.x + cw, cMin.y + ch);
        dl->AddRectFilled(cMin, cMax, IM_COL32(11, 12, 15, 255), 16.0f);
        dl->AddRect(cMin, cMax, IM_COL32(34, 36, 44, 255), 16.0f);

        float cx = (cMin.x + cMax.x) * 0.5f;
        UIWidgets::DrawPlatformIcon(dl, ImVec2(cx, cMin.y + 52.0f), 48.0f, cards[i].icon);

        if (fontUiBold) ImGui::PushFont(fontUiBold);
        ImVec2 ts = ImGui::CalcTextSize(cards[i].title);
        dl->AddText(ImVec2(cx - ts.x * 0.5f, cMin.y + 94.0f), IM_COL32(255, 255, 255, 255), cards[i].title);
        if (fontUiBold) ImGui::PopFont();

        ImVec2 ss = ImGui::CalcTextSize(cards[i].sub);
        dl->AddText(ImVec2(cx - ss.x * 0.5f, cMin.y + 118.0f), IM_COL32(142, 144, 154, 255), cards[i].sub);

        ImGui::SetCursorScreenPos(ImVec2(cMin.x + 12.0f, cMax.y - 46.0f));
        std::string bid = std::string(cards[i].btn) + "##pf_" + cards[i].id;
        if (UIWidgets::OutlineButton(bid.c_str(), ImVec2(cw - 24.0f, 34.0f), 17.0f)) {
            startBuildPipeline(cards[i].id);
        }
    }

    curY += ch + 20.0f;

    // Bottom 2 Cards: Projeto Atual + Build Rápida
    const float halfW = (mainW - 18.0f) * 0.5f;
    const float botH = std::max(168.0f, h - curY - 22.0f);

    // Left: Projeto Atual
    {
        ImVec2 pMin(mainX, curY), pMax(mainX + halfW, curY + botH);
        dl->AddRectFilled(pMin, pMax, IM_COL32(10, 11, 14, 255), 16.0f);
        dl->AddRect(pMin, pMax, IM_COL32(34, 36, 44, 255), 16.0f);

        if (fontUiBold) ImGui::PushFont(fontUiBold);
        dl->AddText(ImVec2(pMin.x + 18.0f, pMin.y + 14.0f), IM_COL32(255, 255, 255, 255), "📁  Projeto Atual");
        if (fontUiBold) ImGui::PopFont();

        ImVec2 innerMin(pMin.x + 18.0f, pMin.y + 44.0f), innerMax(pMax.x - 18.0f, pMax.y - 56.0f);
        dl->AddRectFilled(innerMin, innerMax, IM_COL32(15, 16, 20, 255), 12.0f);
        UIWidgets::DrawToolboxShape(dl, ImVec2(innerMin.x + 42.0f, (innerMin.y + innerMax.y) * 0.5f), 46.0f, MeshKind::Cube);

        const auto& cur = m_projects.currentProject();
        if (fontUiBold) ImGui::PushFont(fontUiBold);
        dl->AddText(ImVec2(innerMin.x + 88.0f, innerMin.y + 12.0f), IM_COL32(255, 255, 255, 255), cur.name.c_str());
        if (fontUiBold) ImGui::PopFont();
        dl->AddText(ImVec2(innerMin.x + 88.0f, innerMin.y + 34.0f), IM_COL32(146, 148, 158, 255), "Última modificação:");
        dl->AddText(ImVec2(innerMin.x + 88.0f, innerMin.y + 52.0f), IM_COL32(146, 148, 158, 255), cur.lastModified.c_str());

        ImGui::SetCursorScreenPos(ImVec2(pMin.x + 18.0f, pMax.y - 46.0f));
        if (UIWidgets::OutlineButton("📁  Abrir Projeto", ImVec2(halfW - 36.0f, 36.0f), 11.0f)) {
            m_screen = ScreenId::Editor;
        }
    }

    // Right: Build Rápida
    {
        ImVec2 qMin(mainX + halfW + 18.0f, curY), qMax(mainX + mainW, curY + botH);
        dl->AddRectFilled(qMin, qMax, IM_COL32(10, 11, 14, 255), 16.0f);
        dl->AddRect(qMin, qMax, IM_COL32(34, 36, 44, 255), 16.0f);

        if (fontUiBold) ImGui::PushFont(fontUiBold);
        dl->AddText(ImVec2(qMin.x + 18.0f, qMin.y + 14.0f), IM_COL32(255, 255, 255, 255), "⚡  Build Rápida");
        if (fontUiBold) ImGui::PopFont();

        ImVec2 innerMin(qMin.x + 18.0f, qMin.y + 44.0f), innerMax(qMax.x - 18.0f, qMax.y - 58.0f);
        dl->AddRectFilled(innerMin, innerMax, IM_COL32(15, 16, 20, 255), 12.0f);
        dl->AddText(ImVec2(innerMin.x + 22.0f, innerMin.y + 22.0f), IM_COL32(255, 255, 255, 255), "⚡");
        if (fontUiBold) ImGui::PushFont(fontUiBold);
        dl->AddText(ImVec2(innerMin.x + 54.0f, innerMin.y + 14.0f), IM_COL32(255, 255, 255, 255), "Compilação Rápida");
        if (fontUiBold) ImGui::PopFont();
        dl->AddText(ImVec2(innerMin.x + 54.0f, innerMin.y + 36.0f), IM_COL32(146, 148, 158, 255), "Build direta com configurações padrão");

        ImGui::SetCursorScreenPos(ImVec2(innerMax.x - 56.0f, (innerMin.y + innerMax.y) * 0.5f - 11.0f));
        UIWidgets::ToggleSwitch("##quick_build_sw", &m_quickBuild, false);

        ImGui::SetCursorScreenPos(ImVec2(qMin.x + 18.0f, qMax.y - 48.0f));
        if (UIWidgets::WhitePillButton("▶   Iniciar Build", ImVec2(halfW - 36.0f, 38.0f), 11.0f)) {
            startBuildPipeline(m_quickBuild ? "all" : "windows");
        }
    }

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

    if (targetId == "android") {
        m_buildSteps = {
            "Sincronizar scripts C# do editor -> games/AstronGame/Core",
            "GitHub Actions: windows-latest + JDK 17 + .NET 8",
            "dotnet workload install android",
            "keytool -genkeypair -keystore release.keystore",
            "dotnet publish -f net8.0-android (APK assinado)",
            "Upload artefato: AstronGame-release.apk"
        };
    } else if (targetId == "ios") {
        m_buildSteps = {
            "Sincronizar scripts C# do editor -> games/AstronGame/Core",
            "GitHub Actions: macos-latest + Xcode + .NET 8",
            "dotnet workload install ios",
            "dotnet publish -f net8.0-ios -r ios-arm64",
            "Empacotar Payload/*.app -> AstronGame-ios.ipa",
            "Upload artefato: AstronGame-ios.ipa"
        };
    } else if (targetId == "linux") {
        m_buildSteps = {
            "Sincronizar scripts C# do editor -> games/AstronGame/Core",
            "GitHub Actions: ubuntu-latest (GCC 12 + .NET 8)",
            "CMake C++/GLSL -> AstronEngine (Linux x64)",
            "dotnet publish -r linux-x64 --self-contained -> AstronGame",
            "Empacotar AstronGame-linux-x64.tar.gz"
        };
    } else if (targetId == "windows") {
        m_buildSteps = {
            "Sincronizar scripts C# do editor -> games/AstronGame/Core",
            "GitHub Actions: windows-latest (MSVC + .NET 8)",
            "CMake C++/GLSL -> AstronEngine.exe (Windows x64)",
            "dotnet publish -r win-x64 -p:PublishSingleFile=true -> AstronGame.exe",
            "Upload artefato: AstronGame-windows-x64"
        };
    } else {
        m_buildSteps = {
            "Sincronizar scripts C#, cenas e shaders GLSL",
            "Exportar pacote nativo para out/",
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
    const char* title = done ? "Pipeline Concluído — GitHub Actions" : "Executando Pipeline de Compilação...";
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
        ImGui::TextColored(ImVec4(0.55f, 0.82f, 0.60f, 1.f), "Código C# sincronizado em games/AstronGame/Core e salvo em: %s",
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
        ImGui::TextUnformatted("Template Inicial (C# + GLSL)");
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
            pushToast("Novo projeto C# criado ✓");
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
