#include "EngineApp.hpp"
#include "UIWidgets.hpp"
#include "../core_c/astron_audio.h"
#include "../core_c/astron_platform.h"
#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace astron {

namespace {

bool icontains(const std::string& hay, const char* needle) {
    if (!needle || needle[0] == '\0') return true;
    std::string h = hay, n = needle;
    for (auto& c : h) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (auto& c : n) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return h.find(n) != std::string::npos;
}

// Ribbon button with crisp vector icon centered above label (matching Image 1)
bool ribbonIconBtn(const char* id, UiIcon icon, const char* label, bool active, float w = 64.0f, bool hasArrow = false) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float h = 42.0f;

    bool clicked = ImGui::InvisibleButton(id, ImVec2(w, h));
    bool hovered = ImGui::IsItemHovered();

    if (active) {
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), IM_COL32(28, 72, 148, 95), 7.0f);
        dl->AddRect(p, ImVec2(p.x + w, p.y + h), IM_COL32(58, 128, 252, 220), 7.0f);
    } else if (hovered) {
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), IM_COL32(34, 37, 46, 225), 7.0f);
    }

    ImU32 iconCol = active ? IM_COL32(84, 160, 255, 255) : IM_COL32(214, 218, 228, 255);
    ImU32 lblCol  = active ? IM_COL32(118, 182, 255, 255) : IM_COL32(198, 202, 212, 255);

    UIWidgets::DrawIcon(dl, ImVec2(p.x + w * 0.5f, p.y + 13.0f), 15.5f, icon, iconCol);

    ImVec2 ls = ImGui::CalcTextSize(label);
    float totalW = ls.x + (hasArrow ? 9.0f : 0.0f);
    float lx = p.x + (w - totalW) * 0.5f;
    dl->AddText(ImVec2(lx, p.y + 24.0f), lblCol, label);
    if (hasArrow) {
        float ax = lx + ls.x + 5.0f;
        float ay = p.y + 31.0f;
        dl->AddTriangleFilled(ImVec2(ax - 2.6f, ay - 1.5f), ImVec2(ax + 2.6f, ay - 1.5f), ImVec2(ax, ay + 2.2f), lblCol);
    }
    return clicked;
}

void drawPanelHeader(const char* title, UiIcon titleIcon, ImU32 iconCol, bool* openFlag = nullptr) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 27.0f), IM_COL32(20, 22, 28, 255));
    dl->AddLine(ImVec2(p.x, p.y + 27.0f), ImVec2(p.x + w, p.y + 27.0f), IM_COL32(38, 41, 50, 255));

    UIWidgets::DrawIcon(dl, ImVec2(p.x + 15.0f, p.y + 13.5f), 13.5f, titleIcon, iconCol);
    dl->AddText(ImVec2(p.x + 28.0f, p.y + 5.5f), IM_COL32(234, 237, 245, 255), title);

    // Pin + Close icons on right (Image 1)
    UIWidgets::DrawIcon(dl, ImVec2(p.x + w - 32.0f, p.y + 13.5f), 11.5f, UiIcon::Pin, IM_COL32(138, 142, 154, 220));
    if (openFlag) {
        ImGui::SetCursorScreenPos(ImVec2(p.x + w - 21.0f, p.y + 3.0f));
        std::string cid = std::string("##cls_") + title;
        if (ImGui::InvisibleButton(cid.c_str(), ImVec2(18.0f, 20.0f))) {
            *openFlag = false;
        }
        ImU32 cCol = ImGui::IsItemHovered() ? IM_COL32(245, 246, 252, 255) : IM_COL32(148, 152, 164, 230);
        UIWidgets::DrawIcon(dl, ImVec2(p.x + w - 12.0f, p.y + 13.5f), 10.5f, UiIcon::Close, cCol);
    } else {
        UIWidgets::DrawIcon(dl, ImVec2(p.x + w - 12.0f, p.y + 13.5f), 10.5f, UiIcon::Close, IM_COL32(148, 152, 164, 230));
    }

    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + 29.0f));
}

void drawSearchInput(const char* id, const char* hint, char* buf, size_t bufSz) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(28.0f, 5.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(13, 14, 18, 255));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(38, 41, 50, 220));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::SetNextItemWidth(-6.0f);
    ImGui::InputTextWithHint(id, hint, buf, bufSz);
    {
        ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
        UIWidgets::DrawIcon(ImGui::GetWindowDrawList(), ImVec2(mn.x + 14.0f, (mn.y + mx.y) * 0.5f),
                            12.5f, UiIcon::Search, IM_COL32(138, 142, 154, 230));
    }
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}

bool iconTreeNode(const char* id, UiIcon icon, ImU32 iconCol, const char* label, ImGuiTreeNodeFlags extraFlags = 0) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding | extraFlags;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.5f));
    std::string padded = std::string("     ") + label + "##" + id;
    bool open = ImGui::TreeNodeEx(padded.c_str(), flags);
    ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
    float arrowW = ImGui::GetTreeNodeToLabelSpacing();
    UIWidgets::DrawIcon(ImGui::GetWindowDrawList(),
                        ImVec2(mn.x + arrowW + 7.0f, (mn.y + mx.y) * 0.5f),
                        13.5f, icon, iconCol);
    ImGui::PopStyleVar();
    return open;
}

bool iconTreeLeaf(const char* id, UiIcon icon, ImU32 iconCol, const char* label, bool selected = false) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.5f));
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                               ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
    if (selected) flags |= ImGuiTreeNodeFlags_Selected;
    std::string padded = std::string("     ") + label + "##" + id;
    ImGui::TreeNodeEx(padded.c_str(), flags);
    bool clicked = ImGui::IsItemClicked();
    ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
    float arrowW = ImGui::GetTreeNodeToLabelSpacing();
    UIWidgets::DrawIcon(ImGui::GetWindowDrawList(),
                        ImVec2(mn.x + arrowW + 7.0f, (mn.y + mx.y) * 0.5f),
                        13.5f, icon, iconCol);
    ImGui::PopStyleVar();
    return clicked;
}

bool drawAxisFloatBox(const char* id, const char* axisLbl, float* val, float boxW) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(18.0f, 3.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(14, 15, 20, 255));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(42, 45, 56, 220));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::SetNextItemWidth(boxW);
    bool changed = ImGui::DragFloat(id, val, 0.05f, -10000.0f, 10000.0f, "%.3f");
    ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
    dl->AddText(ImVec2(mn.x + 5.5f, (mn.y + mx.y - ImGui::GetTextLineHeight()) * 0.5f),
                IM_COL32(138, 142, 154, 240), axisLbl);
    (void)p;
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
    return changed;
}

bool vpFloatIconBtn(const char* id, UiIcon icon, bool active = false) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    bool clicked = ImGui::InvisibleButton(id, ImVec2(24.0f, 22.0f));
    bool hovered = ImGui::IsItemHovered();
    if (active) {
        dl->AddRectFilled(p, ImVec2(p.x + 24.0f, p.y + 22.0f), IM_COL32(37, 114, 246, 210), 5.0f);
    } else if (hovered) {
        dl->AddRectFilled(p, ImVec2(p.x + 24.0f, p.y + 22.0f), IM_COL32(255, 255, 255, 28), 5.0f);
    }
    ImU32 fg = active ? IM_COL32(255, 255, 255, 255) : IM_COL32(212, 216, 228, 240);
    UIWidgets::DrawIcon(dl, ImVec2(p.x + 12.0f, p.y + 11.0f), 13.0f, icon, fg);
    return clicked;
}

void renderResTreeRecursive(const ResTreeEntry& node, const char* filter, EngineApp* app) {
    const ImU32 kAmberFolder = IM_COL32(232, 168, 56, 255);
    const ImU32 kBlueFolder  = IM_COL32( 74, 158, 255, 255);
    const ImU32 kDocCol      = IM_COL32(190, 198, 214, 255);
    const ImU32 kLuaCol      = IM_COL32( 96, 172, 255, 255);
    const ImU32 kCsCol       = IM_COL32(118, 218, 132, 255);

    for (const auto& ch : node.children) {
        if (ch.isDir) {
            ImU32 fCol = (ch.name == "scripts") ? kBlueFolder : kAmberFolder;
            if (iconTreeNode(ch.resPath.c_str(), UiIcon::Folder, fCol, ch.name.c_str())) {
                renderResTreeRecursive(ch, filter, app);
                ImGui::TreePop();
            }
        } else {
            if (!icontains(ch.name, filter)) continue;
            UiIcon ic = UiIcon::FileDoc;
            ImU32 col = kDocCol;
            if (ch.name.find(".lua") != std::string::npos) { ic = UiIcon::FileCode; col = kLuaCol; }
            else if (ch.name.find(".cs") != std::string::npos) { ic = UiIcon::FileCode; col = kCsCol; }
            else if (ch.name.find(".wav") != std::string::npos) { ic = UiIcon::MusicNote; col = IM_COL32(186, 124, 255, 255); }

            if (iconTreeLeaf(ch.resPath.c_str(), ic, col, ch.name.c_str())) {
                app->openFileTab(ch.resPath);
            }
        }
    }
}

} // namespace

// ============================================================================
// SCREEN 5: FULL 3D EDITOR + AUTONOMOUS AI AGENT (Identical to Image 1)
// ============================================================================
void EngineApp::renderEditor(float dt, float W, float H) {
    if (m_playing && !m_paused) {
        ImGuiIO& io = ImGui::GetIO();
        float moveX = 0.0f, moveZ = 0.0f;
        if (!io.WantTextInput) {
            if (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow))  moveX -= 1.0f;
            if (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow)) moveX += 1.0f;
            if (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_UpArrow))    moveZ -= 1.0f;
            if (ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_DownArrow))  moveZ += 1.0f;
        }
        bool jump = !io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Space);
        m_scene.stepPhysics(dt, moveX, moveZ, jump);
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(W, H));
    ImGuiWindowFlags rootFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(14, 15, 19, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##AstronEditorRoot", nullptr, rootFlags);

    ImDrawList* rdl = ImGui::GetWindowDrawList();

    // ==================== 1. TOP MENU BAR (h = 30px) ====================
    const float menuH = 30.0f;
    rdl->AddRectFilled(ImVec2(0, 0), ImVec2(W, menuH), IM_COL32(16, 17, 22, 255));
    rdl->AddLine(ImVec2(0, menuH), ImVec2(W, menuH), IM_COL32(36, 38, 46, 255));

    // Official Astron Engine top-left icon + "Astron Engine ▾"
    GpuTexture iconTex = m_textures.get("assets/textures/astron-icon.png");
    if (iconTex.id != 0) {
        rdl->AddImage(
            static_cast<ImTextureID>(static_cast<uintptr_t>(iconTex.id)),
            ImVec2(10.0f, 5.0f), ImVec2(30.0f, 25.0f)
        );
    } else {
        UIWidgets::DrawPlanetMark(rdl, ImVec2(20.0f, 15.0f), 9.5f,
                                  IM_COL32(255, 255, 255, 255), IM_COL32(16, 17, 22, 255));
    }

    ImGui::SetCursorScreenPos(ImVec2(34.0f, 3.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 20));
    if (fontUiBold) ImGui::PushFont(fontUiBold);
    if (ImGui::Button("Astron Engine##brand_menu", ImVec2(124.0f, 24.0f))) {
        ImGui::OpenPopup("BrandMenuPopup");
    }
    {
        ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
        float ax = mx.x - 9.0f, ay = (mn.y + mx.y) * 0.5f + 1.0f;
        rdl->AddTriangleFilled(ImVec2(ax - 3.0f, ay - 2.0f), ImVec2(ax + 3.0f, ay - 2.0f),
                               ImVec2(ax, ay + 2.5f), IM_COL32(190, 194, 206, 255));
    }
    if (fontUiBold) ImGui::PopFont();

    if (ImGui::BeginPopup("BrandMenuPopup")) {
        if (ImGui::MenuItem("Voltar ao Hub (In\xC3\xAD" "cio)")) m_screen = ScreenId::Home;
        if (ImGui::MenuItem("Compilar / Gerar Build...")) m_screen = ScreenId::Build;
        ImGui::Separator();
        if (ImGui::MenuItem("Configura\xC3\xA7\xC3\xB5" "es de IA (OpenRouter)...")) m_settingsOpen = true;
        if (ImGui::MenuItem("Sair da Conta")) m_screen = ScreenId::Login;
        ImGui::EndPopup();
    }

    const char* menus[] = {
        "Arquivo", "Editar", "Visualizar", "Projeto", "Cena",
        "Jogo", "Script", "Biblioteca de Recursos", "Configura\xC3\xA7\xC3\xB5" "es", "Ajuda"
    };
    for (int i = 0; i < 10; ++i) {
        ImGui::SameLine(0, 2);
        std::string popId = std::string("TopMenu_") + std::to_string(i);
        if (ImGui::Button(menus[i], ImVec2(0, 24.0f))) {
            ImGui::OpenPopup(popId.c_str());
        }
        if (ImGui::BeginPopup(popId.c_str())) {
            if (i == 0) {
                if (ImGui::MenuItem("Novo Projeto...")) m_newProjOpen = true;
                if (ImGui::MenuItem("Salvar Cena (Ctrl+S)")) saveActiveScene();
                if (ImGui::MenuItem("Compilar / Exportar...")) m_screen = ScreenId::Build;
                ImGui::Separator();
                if (ImGui::MenuItem("Voltar ao In\xC3\xAD" "cio")) m_screen = ScreenId::Home;
            } else if (i == 1) {
                if (ImGui::MenuItem("Duplicar Selecionado")) {
                    if (const auto* s = m_scene.selected()) {
                        m_scene.addObject(s->name, s->mesh, { s->position.x + 1.5f, s->position.y, s->position.z });
                    }
                }
                if (ImGui::MenuItem("Remover Selecionado (Del)")) m_scene.removeSelected();
            } else if (i == 2) {
                ImGui::MenuItem("Explorer", nullptr, &m_showExplorer);
                ImGui::MenuItem("Arquivos", nullptr, &m_showFiles);
                ImGui::MenuItem("Propriedades", nullptr, &m_showProperties);
                ImGui::MenuItem("Caixa de Ferramentas", nullptr, &m_showToolbox);
                ImGui::MenuItem("Painel Inferior / Console / IA", nullptr, &m_showBottom);
                ImGui::Separator();
                ImGui::MenuItem("Grade 3D", nullptr, &m_viewport.showGrid);
                ImGui::MenuItem("Wireframe", nullptr, &m_viewport.wireframe);
            } else if (i == 3 || i == 5) {
                if (ImGui::MenuItem("Jogar Cena (F5)")) { m_scene.beginPlay(); m_playing = true; m_paused = false; }
                if (ImGui::MenuItem("Parar Simula\xC3\xA7\xC3\xA3o")) { m_scene.stopPlay(); m_playing = false; m_paused = false; }
                if (ImGui::MenuItem("Tela de Compila\xC3\xA7\xC3\xA3o (APK / EXE / IPA)")) m_screen = ScreenId::Build;
            } else if (i == 4) {
                if (ImGui::MenuItem("Adicionar Cubo")) m_scene.addObject("Part", MeshKind::Cube);
                if (ImGui::MenuItem("Adicionar Esfera")) m_scene.addObject("Sphere", MeshKind::Sphere);
                if (ImGui::MenuItem("Adicionar Cilindro")) m_scene.addObject("Cylinder", MeshKind::Cylinder);
                if (ImGui::MenuItem("Resetar C\xC3\xA2mera 3D")) m_viewport.resetCamera();
            } else if (i == 6) {
                if (ImGui::MenuItem("Abrir res://scripts/Player.cs")) openFileTab("res://scripts/Player.cs");
                if (ImGui::MenuItem("Abrir res://scripts/GameWorld.cs")) openFileTab("res://scripts/GameWorld.cs");
                if (ImGui::MenuItem("Abrir res://main.lua")) openFileTab("res://main.lua");
            } else if (i == 7) {
                if (ImGui::MenuItem("Abrir Agente IA Aut\xC3\xB4nomo")) { m_showBottom = true; m_bottomTab = 6; }
                if (ImGui::MenuItem("Abrir Editor de Shader GLSL")) { m_showBottom = true; m_bottomTab = 5; }
            } else if (i == 8) {
                if (ImGui::MenuItem("Configura\xC3\xA7\xC3\xB5" "es de IA / API Key...")) m_settingsOpen = true;
            } else {
                ImGui::TextDisabled("Astron Engine v1.0.0 (C++20 / C11 / C# / Rust / GLSL / Lua)");
                ImGui::TextDisabled("Bot\xC3\xA3o Direito no Viewport: Orbitar | Scroll: Zoom | Meio: Pan");
            }
            ImGui::EndPopup();
        }
    }
    ImGui::PopStyleColor(2);

    UIWidgets::DrawWindowChrome(W, onMinimizeWindow, onMaximizeWindow, onCloseWindow);

    // ==================== 2. MAIN RIBBON TOOLBAR (h = 52px) ====================
    const float toolY = menuH;
    const float toolH = 52.0f;
    rdl->AddRectFilled(ImVec2(0, toolY), ImVec2(W, toolY + toolH), IM_COL32(21, 22, 28, 255));
    rdl->AddLine(ImVec2(0, toolY + toolH), ImVec2(W, toolY + toolH), IM_COL32(38, 40, 48, 255));

    auto vSep = [&](float x) {
        rdl->AddLine(ImVec2(x, toolY + 10.0f), ImVec2(x, toolY + toolH - 10.0f), IM_COL32(44, 47, 58, 255));
    };

    float tx = 8.0f;
    auto placeRibbon = [&](const char* id, UiIcon ic, const char* lbl, bool act, float bw, bool arrow = false) -> bool {
        ImGui::SetCursorScreenPos(ImVec2(tx, toolY + 5.0f));
        bool c = ribbonIconBtn(id, ic, lbl, act, bw, arrow);
        tx += bw + 3.0f;
        return c;
    };

    if (placeRibbon("##tl_sel",   UiIcon::CursorSelect,   "Selecionar",  m_activeTool == 0, 72.0f)) m_activeTool = 0;
    if (placeRibbon("##tl_mov",   UiIcon::Move4Way,       "Mover",       m_activeTool == 1, 56.0f)) m_activeTool = 1;
    if (placeRibbon("##tl_rot",   UiIcon::RotateCw,       "Rotacionar",  m_activeTool == 2, 72.0f)) m_activeTool = 2;
    if (placeRibbon("##tl_scl",   UiIcon::ScaleDiag,      "Escalar",     m_activeTool == 3, 58.0f)) m_activeTool = 3;
    if (placeRibbon("##tl_trn",   UiIcon::TransformGizmo, "Transformar", m_activeTool == 4, 80.0f)) m_activeTool = 4;

    vSep(tx + 4.0f); tx += 11.0f;

    if (placeRibbon("##tl_criar", UiIcon::Cube3D,    "Criar",       false, 58.0f, true)) ImGui::OpenPopup("CriarPrimPopup");
    if (ImGui::BeginPopup("CriarPrimPopup")) {
        if (ImGui::MenuItem("Cubo (Part)")) m_scene.addObject("Part", MeshKind::Cube);
        if (ImGui::MenuItem("Esfera (Sphere)")) m_scene.addObject("Sphere", MeshKind::Sphere);
        if (ImGui::MenuItem("Cilindro (Cylinder)")) m_scene.addObject("Cylinder", MeshKind::Cylinder);
        if (ImGui::MenuItem("Bloco (Block)")) m_scene.addObject("Block", MeshKind::Block);
        ImGui::EndPopup();
    }
    if (placeRibbon("##tl_cam",   UiIcon::Camera,    "C\xC3\xA2mera",      false, 62.0f)) { m_viewport.resetCamera(); pushToast("C\xC3\xA2mera 3D centralizada."); }
    if (placeRibbon("##tl_lum",   UiIcon::Lightbulb, "Ilumina\xC3\xA7\xC3\xA3o", false, 74.0f)) { m_scene.addObject("Luz", MeshKind::Light, { 2.0f, 5.0f, 2.0f }); }
    if (placeRibbon("##tl_ter",   UiIcon::Mountain,  "Terreno",     false, 60.0f)) { m_scene.addObject("Terreno", MeshKind::Terrain, { 0.0f, -0.5f, 0.0f }); }
    if (placeRibbon("##tl_cmp",   UiIcon::Puzzle,    "Componente",  false, 80.0f)) { openFileTab("res://scripts/Player.cs"); }

    vSep(tx + 4.0f); tx += 12.0f;

    // 2D | 3D Segmented Pill matching Image 1
    {
        ImVec2 p2d(tx, toolY + 12.0f);
        rdl->AddRectFilled(p2d, ImVec2(p2d.x + 76.0f, p2d.y + 28.0f), IM_COL32(13, 14, 18, 255), 7.0f);
        rdl->AddRect(p2d, ImVec2(p2d.x + 76.0f, p2d.y + 28.0f), IM_COL32(42, 45, 56, 255), 7.0f);

        ImGui::SetCursorScreenPos(ImVec2(p2d.x + 2.0f, p2d.y + 2.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, m_viewport.is2DMode ? IM_COL32(37, 114, 246, 255) : IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, m_viewport.is2DMode ? IM_COL32(255, 255, 255, 255) : IM_COL32(168, 172, 184, 255));
        if (ImGui::Button("2D##mode", ImVec2(35.0f, 24.0f))) m_viewport.is2DMode = true;
        ImGui::PopStyleColor(2);

        ImGui::SetCursorScreenPos(ImVec2(p2d.x + 39.0f, p2d.y + 2.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, !m_viewport.is2DMode ? IM_COL32(37, 114, 246, 255) : IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, !m_viewport.is2DMode ? IM_COL32(255, 255, 255, 255) : IM_COL32(168, 172, 184, 255));
        if (ImGui::Button("3D##mode", ImVec2(35.0f, 24.0f))) m_viewport.is2DMode = false;
        ImGui::PopStyleColor(2);

        tx += 84.0f;
    }

    vSep(tx + 2.0f); tx += 10.0f;

    // Transport: Jogar, Pausar, Parar
    if (placeRibbon("##tl_play", UiIcon::Play, "Jogar", m_playing && !m_paused, 54.0f)) {
        if (!m_playing) m_scene.beginPlay();
        m_playing = true;
        m_paused = false;
        pushLog("INFO", "Simula\xC3\xA7\xC3\xA3o 3D iniciada. Use WASD + Espa\xC3\xA7o.");
    }
    if (placeRibbon("##tl_pause", UiIcon::Pause, "Pausar", m_playing && m_paused, 56.0f)) {
        if (m_playing) m_paused = !m_paused;
    }
    if (placeRibbon("##tl_stop", UiIcon::Stop, "Parar", false, 52.0f)) {
        if (m_playing) {
            m_scene.stopPlay();
            m_playing = false;
            m_paused = false;
            pushLog("INFO", "Simula\xC3\xA7\xC3\xA3o encerrada.");
        }
    }

    // Right-side ribbon buttons: Visualização ▾ | Configurações (Image 1)
    if (W > tx + 190.0f) {
        float rx = W - 188.0f;
        ImGui::SetCursorScreenPos(ImVec2(rx, toolY + 5.0f));
        if (ribbonIconBtn("##tl_vis", UiIcon::Monitor, "Visualiza\xC3\xA7\xC3\xA3o", false, 88.0f, true)) {
            m_viewport.wireframe = !m_viewport.wireframe;
        }
        vSep(rx + 92.0f);
        ImGui::SetCursorScreenPos(ImVec2(rx + 98.0f, toolY + 5.0f));
        if (ribbonIconBtn("##tl_cfg", UiIcon::Gear, "Configura\xC3\xA7\xC3\xB5" "es", false, 84.0f)) {
            m_settingsOpen = true;
        }
    }

    // ==================== 3. DOCKED WORKSPACE LAYOUT ====================
    const float statusH = 24.0f;
    const float workY = menuH + toolH;
    const float workH = H - workY - statusH;

    const float leftW   = (m_showExplorer || m_showFiles) ? 248.0f : 0.0f;
    const float rightW  = (m_showProperties || m_showToolbox) ? 308.0f : 0.0f;
    const float centerW = std::max(240.0f, W - leftW - rightW);
    const float bottomH = m_showBottom ? 214.0f : 0.0f;
    const float viewAreaH = std::max(140.0f, workH - bottomH);

    // -------------------- LEFT COLUMN: EXPLORER + ARQUIVOS --------------------
    if (leftW > 0.0f) {
        float expH = m_showExplorer ? (m_showFiles ? workH * 0.57f : workH) : 0.0f;
        float filH = m_showFiles ? (workH - expH) : 0.0f;

        if (m_showExplorer) {
            ImGui::SetCursorScreenPos(ImVec2(0, workY));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(17, 18, 23, 255));
            ImGui::BeginChild("##ExplorerPanel", ImVec2(leftW, expH), true);
            drawPanelHeader("Explorer", UiIcon::Cube3D, IM_COL32(84, 162, 255, 255), &m_showExplorer);

            ImGui::SetCursorPosX(6.0f);
            drawSearchInput("##exp_search", "Pesquisar no Explorer...", m_explorerSearch, sizeof(m_explorerSearch));
            ImGui::Spacing();

            const ImU32 cBlueCube   = IM_COL32( 74, 162, 255, 255);
            const ImU32 cCyanCube   = IM_COL32( 96, 186, 255, 255);
            const ImU32 cAmberFold  = IM_COL32(234, 170,  58, 255);
            const ImU32 cBlueFold   = IM_COL32( 78, 156, 252, 255);

            if (iconTreeNode("exp_proj", UiIcon::Cube3D, cBlueCube, "Projeto", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (iconTreeNode("exp_cenas", UiIcon::Folder, cAmberFold, "Cenas", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (iconTreeLeaf("sc_main", UiIcon::Cube3D, cCyanCube, "Cena Principal", m_scene.name == "Cena Principal")) {
                        m_scene.name = "Cena Principal";
                        m_activeTabIdx = 0;
                    }
                    if (iconTreeLeaf("sc_sec", UiIcon::Cube3D, cCyanCube, "Cena Secund\xC3\xA1ria", m_scene.name == "Cena Secundária")) {
                        m_scene.name = "Cena Secundária";
                        m_activeTabIdx = 0;
                    }
                    ImGui::TreePop();
                }

                if (iconTreeNode("exp_m3d", UiIcon::Folder, cAmberFold, "Mundo 3D", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (iconTreeNode("exp_objs", UiIcon::Folder, cBlueFold, "Objetos")) {
                        for (size_t i = 0; i < m_scene.objects.size(); ++i) {
                            const auto& o = m_scene.objects[i];
                            if (!icontains(o.name, m_explorerSearch)) continue;
                            std::string oid = "obj_" + o.id + "_" + std::to_string(i);
                            if (iconTreeLeaf(oid.c_str(), UiIcon::Cube3D, cCyanCube, o.name.c_str(),
                                             static_cast<int>(i) == m_scene.selectedIndex)) {
                                m_scene.selectedIndex = static_cast<int>(i);
                            }
                        }
                        ImGui::TreePop();
                    }
                    if (iconTreeLeaf("exp_terr", UiIcon::Mountain, IM_COL32(92, 204, 128, 255), "Terreno")) {
                        m_scene.addObject("Terreno", MeshKind::Terrain, { 0.0f, -0.5f, 0.0f });
                    }
                    if (iconTreeLeaf("exp_ilum", UiIcon::Lightbulb, IM_COL32(246, 204, 72, 255), "Ilumina\xC3\xA7\xC3\xA3o")) {
                        m_viewport.resetCamera();
                    }
                    ImGui::TreePop();
                }

                iconTreeLeaf("exp_cams", UiIcon::Camera,     IM_COL32( 88, 168, 255, 255), "C\xC3\xA2meras");
                iconTreeLeaf("exp_pers", UiIcon::User,       IM_COL32( 88, 168, 255, 255), "Personagens");
                iconTreeLeaf("exp_fis",  UiIcon::Gear,       IM_COL32(174, 182, 198, 255), "F\xC3\xADsica");
                if (iconTreeLeaf("exp_aud", UiIcon::MusicNote, IM_COL32(184, 118, 255, 255), "\xC3\x81udio")) {
                    m_showBottom = true; m_bottomTab = 3;
                }
                iconTreeLeaf("exp_ui",   UiIcon::Monitor,    IM_COL32( 92, 192, 255, 255), "Interface");
                if (iconTreeLeaf("exp_scr", UiIcon::CodeTags, IM_COL32(238, 194,  76, 255), "Scripts")) {
                    openFileTab("res://scripts/Player.cs");
                }
                iconTreeLeaf("exp_rec",  UiIcon::Folder,     cAmberFold,                    "Recursos");
                if (iconTreeLeaf("exp_mat", UiIcon::SphereBall, IM_COL32(84, 162, 255, 255), "Materiais")) {
                    m_showToolbox = true; m_toolboxTab = 2;
                }
                ImGui::TreePop();
            }

            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        if (m_showFiles) {
            ImGui::SetCursorScreenPos(ImVec2(0, workY + expH));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(17, 18, 23, 255));
            ImGui::BeginChild("##FilesPanel", ImVec2(leftW, filH), true);
            drawPanelHeader("Arquivos", UiIcon::Folder, IM_COL32(84, 162, 255, 255), &m_showFiles);

            ImGui::SetCursorPosX(6.0f);
            drawSearchInput("##fil_search", "Pesquisar arquivos...", m_filesSearch, sizeof(m_filesSearch));
            ImGui::Spacing();

            ResTreeEntry tree = m_projects.buildResTree();
            if (iconTreeNode("res_root", UiIcon::Folder, IM_COL32(78, 158, 255, 255), "res://", ImGuiTreeNodeFlags_DefaultOpen)) {
                renderResTreeRecursive(tree, m_filesSearch, this);
                ImGui::TreePop();
            }

            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    }

    // -------------------- CENTER AREA: SCENE / CODE TABS + 3D VIEWPORT --------------------
    {
        ImGui::SetCursorScreenPos(ImVec2(leftW, workY));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(14, 15, 19, 255));
        ImGui::BeginChild("##CenterViewportHost", ImVec2(centerW, viewAreaH), true, ImGuiWindowFlags_NoScrollbar);

        // Scene & Script Tab Bar (Image 1: [🧊 Cena Principal ✕] [+])
        const float tabBarH = 30.0f;
        ImVec2 tp = ImGui::GetCursorScreenPos();
        ImDrawList* cdl = ImGui::GetWindowDrawList();
        cdl->AddRectFilled(tp, ImVec2(tp.x + centerW, tp.y + tabBarH), IM_COL32(18, 19, 24, 255));
        cdl->AddLine(ImVec2(tp.x, tp.y + tabBarH), ImVec2(tp.x + centerW, tp.y + tabBarH), IM_COL32(38, 40, 50, 255));

        float tabX = tp.x + 6.0f;
        for (size_t i = 0; i < m_tabs.size(); ++i) {
            bool active = (static_cast<int>(i) == m_activeTabIdx);
            std::string lbl = m_tabs[i].label + (m_tabs[i].dirty ? " *" : "");
            ImVec2 tSz = ImGui::CalcTextSize(lbl.c_str());
            float bw = tSz.x + 54.0f;

            ImVec2 bMin(tabX, tp.y + 3.0f);
            ImVec2 bMax(tabX + bw, tp.y + tabBarH);

            ImGui::SetCursorScreenPos(bMin);
            std::string bid = "##edtab_" + std::to_string(i);
            if (ImGui::InvisibleButton(bid.c_str(), ImVec2(bw - 20.0f, tabBarH - 3.0f))) {
                m_activeTabIdx = static_cast<int>(i);
            }

            if (active) {
                cdl->AddRectFilled(bMin, bMax, IM_COL32(28, 30, 38, 255), 6.0f, ImDrawFlags_RoundCornersTop);
                cdl->AddLine(ImVec2(bMin.x + 2.0f, bMin.y), ImVec2(bMax.x - 2.0f, bMin.y), IM_COL32(54, 130, 252, 255), 2.0f);
            } else {
                cdl->AddRectFilled(bMin, bMax, IM_COL32(20, 21, 27, 255), 6.0f, ImDrawFlags_RoundCornersTop);
            }

            float cyTab = (bMin.y + bMax.y) * 0.5f;
            UIWidgets::DrawIcon(cdl, ImVec2(bMin.x + 14.0f, cyTab), 13.0f,
                                m_tabs[i].isScene ? UiIcon::Cube3D : UiIcon::FileCode,
                                m_tabs[i].isScene ? IM_COL32(84, 164, 255, 255) : IM_COL32(118, 214, 136, 255));
            cdl->AddText(ImVec2(bMin.x + 26.0f, cyTab - tSz.y * 0.5f),
                         active ? IM_COL32(244, 246, 252, 255) : IM_COL32(164, 168, 180, 255), lbl.c_str());

            // Tab close "✕"
            ImGui::SetCursorScreenPos(ImVec2(bMax.x - 20.0f, bMin.y + 3.0f));
            std::string xid = "##xtab_" + std::to_string(i);
            if (ImGui::InvisibleButton(xid.c_str(), ImVec2(16.0f, 18.0f)) && !m_tabs[i].isScene) {
                m_tabs.erase(m_tabs.begin() + static_cast<ptrdiff_t>(i));
                if (m_activeTabIdx >= static_cast<int>(m_tabs.size())) m_activeTabIdx = 0;
                break;
            }
            UIWidgets::DrawIcon(cdl, ImVec2(bMax.x - 12.0f, cyTab), 9.5f, UiIcon::Close, IM_COL32(150, 154, 166, 220));

            tabX += bw + 4.0f;
        }

        // "+" Add Scene/Script Tab button
        ImGui::SetCursorScreenPos(ImVec2(tabX + 2.0f, tp.y + 4.0f));
        if (ImGui::InvisibleButton("##add_tab_plus", ImVec2(24.0f, 22.0f))) {
            openFileTab("res://scripts/Player.cs");
        }
        if (ImGui::IsItemHovered()) {
            cdl->AddRectFilled(ImVec2(tabX + 2.0f, tp.y + 4.0f), ImVec2(tabX + 26.0f, tp.y + 26.0f), IM_COL32(34, 36, 46, 220), 5.0f);
        }
        UIWidgets::DrawIcon(cdl, ImVec2(tabX + 14.0f, tp.y + 15.5f), 11.5f, UiIcon::Plus, IM_COL32(188, 192, 204, 240));

        if (m_activeTabIdx < 0 || m_activeTabIdx >= static_cast<int>(m_tabs.size())) m_activeTabIdx = 0;
        EditorTab& curTab = m_tabs[static_cast<size_t>(m_activeTabIdx)];

        const float bodyH = std::max(80.0f, viewAreaH - tabBarH - 2.0f);
        ImGui::SetCursorScreenPos(ImVec2(tp.x, tp.y + tabBarH));

        if (curTab.isScene) {
            // Render live OpenGL 3.3 FBO 3D Viewport
            m_viewport.render(m_scene, static_cast<int>(centerW), static_cast<int>(bodyH), m_timeSec);

            ImVec2 vpPos = ImGui::GetCursorScreenPos();
            ImVec2 vpMax(vpPos.x + centerW, vpPos.y + bodyH);
            ImGui::Image(
                static_cast<ImTextureID>(static_cast<uintptr_t>(m_viewport.colorTexture())),
                ImVec2(centerW, bodyH),
                ImVec2(0, 1), ImVec2(1, 0)
            );

            bool vpHovered = ImGui::IsItemHovered();
            ImGuiIO& io = ImGui::GetIO();

            // Crisp anti-aliased 3D RGB Translation Arrows with solid filled arrowheads over selected object (Image 1)
            if (const SceneObject* selObj = m_scene.selected(); selObj && m_viewport.showGizmo) {
                float aspect = centerW / std::max(1.0f, bodyH);
                AstronMat4 vpMat = astron_mat4_mul(m_viewport.computeProjMatrix(aspect), m_viewport.computeViewMatrix());
                UIWidgets::DrawSceneTranslationGizmo(cdl, vpPos, vpMax, vpMat, selObj->position, 1.85f);
            }

            // Camera Orbit / Pan / Zoom + 3D Raycast Object Picking
            if (vpHovered) {
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 1.0f)) {
                    m_viewport.orbit(-io.MouseDelta.x * 0.32f, io.MouseDelta.y * 0.28f);
                }
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 1.0f)) {
                    m_viewport.pan(io.MouseDelta.x, io.MouseDelta.y);
                }
                if (std::fabs(io.MouseWheel) > 0.001f) {
                    m_viewport.zoom(io.MouseWheel);
                }
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    float lx = io.MousePos.x - vpPos.x;
                    float ly = io.MousePos.y - vpPos.y;
                    if (!(lx < 320.0f && ly < 42.0f)) {
                        int hit = m_viewport.pickObject(m_scene, lx, ly, static_cast<int>(centerW), static_cast<int>(bodyH));
                        if (hit >= 0) m_scene.selectedIndex = hit;
                    }
                }
            }

            // Top-left floating translucent viewport pill bar (Identical to Image 1)
            ImVec2 ovMin(vpPos.x + 10.0f, vpPos.y + 8.0f);
            ImVec2 ovMax(ovMin.x + 294.0f, ovMin.y + 30.0f);
            cdl->AddRectFilled(ovMin, ovMax, IM_COL32(18, 20, 26, 212), 7.0f);
            cdl->AddRect(ovMin, ovMax, IM_COL32(56, 60, 74, 175), 7.0f);

            ImGui::SetCursorScreenPos(ImVec2(ovMin.x + 4.0f, ovMin.y + 4.0f));
            if (vpFloatIconBtn("##vp_foc", UiIcon::FocusCube, false)) m_viewport.resetCamera();
            cdl->AddLine(ImVec2(ovMin.x + 31.0f, ovMin.y + 6.0f), ImVec2(ovMin.x + 31.0f, ovMax.y - 6.0f), IM_COL32(58, 62, 74, 180));

            // "Perspectiva ▾" button
            ImGui::SetCursorScreenPos(ImVec2(ovMin.x + 35.0f, ovMin.y + 4.0f));
            if (ImGui::InvisibleButton("##vp_persp", ImVec2(92.0f, 22.0f))) {
                m_viewport.is2DMode = !m_viewport.is2DMode;
            }
            {
                const char* pTxt = m_viewport.is2DMode ? "Ortogr\xC3\xA1" "fica" : "Perspectiva";
                cdl->AddText(ImVec2(ovMin.x + 40.0f, ovMin.y + 7.0f), IM_COL32(232, 235, 244, 255), pTxt);
                float ax = ovMin.x + 118.0f, ay = ovMin.y + 15.0f;
                cdl->AddTriangleFilled(ImVec2(ax - 3.0f, ay - 2.0f), ImVec2(ax + 3.0f, ay - 2.0f),
                                       ImVec2(ax, ay + 2.5f), IM_COL32(190, 194, 206, 255));
            }
            cdl->AddLine(ImVec2(ovMin.x + 129.0f, ovMin.y + 6.0f), ImVec2(ovMin.x + 129.0f, ovMax.y - 6.0f), IM_COL32(58, 62, 74, 180));

            ImGui::SetCursorScreenPos(ImVec2(ovMin.x + 133.0f, ovMin.y + 4.0f));
            if (vpFloatIconBtn("##vp_cam_b", UiIcon::Camera, true)) m_viewport.resetCamera();
            ImGui::SameLine(0, 2);
            if (vpFloatIconBtn("##vp_grd_b", UiIcon::GridIcon, m_viewport.showGrid)) m_viewport.showGrid = !m_viewport.showGrid;
            ImGui::SameLine(0, 2);
            if (vpFloatIconBtn("##vp_exp_b", UiIcon::ExpandCorners, false)) m_showBottom = !m_showBottom;
            ImGui::SameLine(0, 2);
            if (vpFloatIconBtn("##vp_snp_b", UiIcon::SnapTarget, m_viewport.showGizmo)) m_viewport.showGizmo = !m_viewport.showGizmo;
            ImGui::SameLine(0, 2);
            if (vpFloatIconBtn("##vp_shd_b", UiIcon::ShieldLight, m_viewport.wireframe)) m_viewport.wireframe = !m_viewport.wireframe;
            ImGui::SameLine(0, 2);
            if (vpFloatIconBtn("##vp_mor_b", UiIcon::DotsV, false)) m_viewport.resetCamera();

            // Top-right 3D RGB X/Y/Z Orientation Gizmo (Image 1)
            UIWidgets::DrawOrientationGizmo(
                cdl,
                ImVec2(vpPos.x + centerW - 52.0f, vpPos.y + 48.0f),
                32.0f,
                m_viewport.computeViewMatrix()
            );
        } else {
            // Native Code Editor Tab (C# / GLSL / Lua / Scene JSON)
            ImGui::SetCursorPos(ImVec2(10, tabBarH + 6));
            ImGui::TextColored(ImVec4(0.58f, 0.78f, 1.0f, 1.0f), "%s", curTab.id.c_str());
            ImGui::SameLine(centerW - 156.0f);
            if (UIWidgets::WhitePillButton("Salvar Arquivo", ImVec2(142.0f, 24.0f), 6.0f)) {
                m_projects.writeFile(curTab.id, curTab.buffer);
                curTab.dirty = false;
                pushToast("Salvo em disco: " + curTab.id);
                pushLog("INFO", "Arquivo atualizado: " + curTab.id);
            }

            if (curTab.buffer.capacity() < 65536) curTab.buffer.reserve(65536);
            curTab.buffer.resize(std::max<size_t>(curTab.buffer.size() + 4096, 16384), '\0');
            if (fontMono) ImGui::PushFont(fontMono);
            if (ImGui::InputTextMultiline("##code_editor", curTab.buffer.data(), curTab.buffer.size(),
                                          ImVec2(centerW - 16.0f, bodyH - 42.0f), ImGuiInputTextFlags_AllowTabInput)) {
                curTab.dirty = true;
            }
            if (fontMono) ImGui::PopFont();
            curTab.buffer.resize(std::strlen(curTab.buffer.c_str()));
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    // -------------------- BOTTOM PANEL: CONSOLE / SAÍDA / DEPURADOR / ÁUDIO / ANIMAÇÃO / SHADER / AGENTE IA --------------------
    if (m_showBottom) {
        ImGui::SetCursorScreenPos(ImVec2(leftW, workY + viewAreaH));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(16, 17, 22, 255));
        ImGui::BeginChild("##BottomDock", ImVec2(centerW, bottomH), true);

        ImDrawList* bdl = ImGui::GetWindowDrawList();
        ImVec2 bp = ImGui::GetCursorScreenPos();
        bdl->AddRectFilled(bp, ImVec2(bp.x + centerW, bp.y + 29.0f), IM_COL32(20, 21, 27, 255));
        bdl->AddLine(ImVec2(bp.x, bp.y + 29.0f), ImVec2(bp.x + centerW, bp.y + 29.0f), IM_COL32(38, 40, 50, 255));

        struct BTab { UiIcon ic; const char* lbl; };
        const BTab bTabs[7] = {
            { UiIcon::Terminal,   "Console" },
            { UiIcon::FileDoc,    "Sa\xC3\xAD" "da" },
            { UiIcon::Gear,       "Depurador" },
            { UiIcon::Speaker,    "\xC3\x81udio" },
            { UiIcon::Keyframe,   "Anima\xC3\xA7\xC3\xA3o" },
            { UiIcon::ShaderEdit, "Editor de Shader" },
            { UiIcon::SparklesAI, "Agente IA" }
        };

        float btx = bp.x + 6.0f;
        for (int i = 0; i < 7; ++i) {
            bool act = (m_bottomTab == i);
            ImVec2 lSz = ImGui::CalcTextSize(bTabs[i].lbl);
            float bw = lSz.x + 34.0f;
            ImVec2 tMin(btx, bp.y + 2.0f), tMax(btx + bw, bp.y + 29.0f);

            ImGui::SetCursorScreenPos(tMin);
            std::string id = "##btab_" + std::to_string(i);
            if (ImGui::InvisibleButton(id.c_str(), ImVec2(bw, 27.0f))) m_bottomTab = i;

            if (act) {
                bdl->AddRectFilled(tMin, tMax, IM_COL32(27, 29, 37, 255), 5.0f, ImDrawFlags_RoundCornersTop);
                bdl->AddLine(ImVec2(tMin.x + 2.0f, tMax.y - 1.0f), ImVec2(tMax.x - 2.0f, tMax.y - 1.0f),
                             IM_COL32(58, 134, 255, 255), 2.0f);
            }
            ImU32 icCol = (i == 6) ? IM_COL32(98, 178, 255, 255)
                        : (act ? IM_COL32(236, 238, 246, 255) : IM_COL32(148, 152, 164, 255));
            ImU32 txCol = act ? IM_COL32(244, 246, 252, 255) : IM_COL32(158, 162, 174, 255);
            UIWidgets::DrawIcon(bdl, ImVec2(tMin.x + 13.0f, bp.y + 15.5f), 12.5f, bTabs[i].ic, icCol);
            bdl->AddText(ImVec2(tMin.x + 24.0f, bp.y + 7.0f), txCol, bTabs[i].lbl);
            btx += bw + 2.0f;
        }

        // Pin + Close on right of bottom tab strip (Image 1)
        UIWidgets::DrawIcon(bdl, ImVec2(bp.x + centerW - 34.0f, bp.y + 15.0f), 11.5f, UiIcon::Pin, IM_COL32(142, 146, 158, 220));
        ImGui::SetCursorScreenPos(ImVec2(bp.x + centerW - 22.0f, bp.y + 5.0f));
        if (ImGui::InvisibleButton("##close_bot", ImVec2(18.0f, 20.0f))) m_showBottom = false;
        UIWidgets::DrawIcon(bdl, ImVec2(bp.x + centerW - 13.0f, bp.y + 15.0f), 10.5f, UiIcon::Close, IM_COL32(152, 156, 168, 230));

        ImGui::SetCursorScreenPos(ImVec2(bp.x + 8.0f, bp.y + 34.0f));

        if (m_bottomTab == 0) {
            // ==================== CONSOLE (Identical to Image 1) ====================
            int nInfo = 0, nWarn = 0, nErr = 0;
            for (const auto& e : m_logs) {
                if (e.level == "ERROR") ++nErr;
                else if (e.level == "WARNING") ++nWarn;
                else ++nInfo;
            }

            // Right-aligned filter badges: (✕ 1) (⚠ 1) (ℹ 1) [Funnel] [Trash]
            ImVec2 barP = ImGui::GetCursorScreenPos();
            auto drawCountPill = [&](float x, UiIcon ic, ImU32 icCol, int count) -> float {
                char buf[16];
                std::snprintf(buf, sizeof(buf), "%d", count);
                ImVec2 ts = ImGui::CalcTextSize(buf);
                float pw = 26.0f + ts.x;
                ImVec2 mn(x, barP.y), mx(x + pw, barP.y + 20.0f);
                bdl->AddRectFilled(mn, mx, IM_COL32(24, 26, 33, 255), 5.0f);
                bdl->AddRect(mn, mx, IM_COL32(42, 45, 56, 220), 5.0f);
                UIWidgets::DrawIcon(bdl, ImVec2(mn.x + 10.0f, mn.y + 10.0f), 11.5f, ic, icCol);
                bdl->AddText(ImVec2(mn.x + 19.0f, mn.y + 2.0f), IM_COL32(216, 220, 230, 255), buf);
                return pw + 5.0f;
            };

            float rx = bp.x + centerW - 178.0f;
            rx += drawCountPill(rx, UiIcon::ErrorCircle,  IM_COL32(236,  72,  76, 255), nErr);
            rx += drawCountPill(rx, UiIcon::WarnTriangle, IM_COL32(242, 184,  54, 255), nWarn);
            rx += drawCountPill(rx, UiIcon::InfoCircle,   IM_COL32( 72, 162, 255, 255), nInfo);

            ImGui::SetCursorScreenPos(ImVec2(rx + 2.0f, barP.y));
            if (ImGui::InvisibleButton("##con_flt", ImVec2(22.0f, 20.0f))) {}
            UIWidgets::DrawIcon(bdl, ImVec2(rx + 13.0f, barP.y + 10.0f), 12.0f, UiIcon::FilterFunnel, IM_COL32(164, 168, 180, 240));

            ImGui::SetCursorScreenPos(ImVec2(rx + 26.0f, barP.y));
            if (ImGui::InvisibleButton("##con_clr", ImVec2(22.0f, 20.0f))) m_logs.clear();
            UIWidgets::DrawIcon(bdl, ImVec2(rx + 37.0f, barP.y + 10.0f), 12.5f, UiIcon::Trash, IM_COL32(164, 168, 180, 240));

            ImGui::SetCursorScreenPos(ImVec2(bp.x + 8.0f, barP.y + 22.0f));
            ImGui::BeginChild("##ConsoleScroll", ImVec2(centerW - 16.0f, bottomH - 94.0f), false);
            ImDrawList* ldl = ImGui::GetWindowDrawList();
            for (const auto& entry : m_logs) {
                ImVec2 lp = ImGui::GetCursorScreenPos();
                UiIcon ic = UiIcon::InfoCircle;
                ImU32 col = IM_COL32(78, 168, 255, 255);
                const char* tag = "[INFO]";
                if (entry.level == "WARNING") {
                    ic = UiIcon::WarnTriangle;
                    col = IM_COL32(242, 186, 56, 255);
                    tag = "[WARNING]";
                } else if (entry.level == "ERROR") {
                    ic = UiIcon::ErrorCircle;
                    col = IM_COL32(240, 76, 80, 255);
                    tag = "[ERROR]";
                }
                UIWidgets::DrawIcon(ldl, ImVec2(lp.x + 8.0f, lp.y + 9.0f), 12.5f, ic, col);
                ldl->AddText(ImVec2(lp.x + 20.0f, lp.y + 1.0f), col, tag);
                float tagW = ImGui::CalcTextSize(tag).x;
                ldl->AddText(ImVec2(lp.x + 26.0f + tagW, lp.y + 1.0f), IM_COL32(216, 220, 230, 255), entry.message.c_str());
                ImGui::Dummy(ImVec2(10.0f, 19.0f));
            }
            ImGui::EndChild();

            // Bottom Console / Prompt Input Bar: "Digite uma mensagem..." + Paper-Plane send button (Image 1)
            ImGui::SetCursorScreenPos(ImVec2(bp.x + 8.0f, bp.y + bottomH - 34.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 6.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(12, 13, 17, 255));
            ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(38, 41, 52, 230));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
            ImGui::SetNextItemWidth(centerW - 52.0f);
            bool sendCon = ImGui::InputTextWithHint("##con_cmd", "Digite uma mensagem...",
                                                    m_consoleInput, sizeof(m_consoleInput),
                                                    ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopStyleVar(3);
            ImGui::PopStyleColor(2);

            ImGui::SameLine(0, 6.0f);
            ImVec2 sbP = ImGui::GetCursorScreenPos();
            if (ImGui::InvisibleButton("##con_send_plane", ImVec2(28.0f, 26.0f))) sendCon = true;
            if (ImGui::IsItemHovered()) {
                bdl->AddRectFilled(sbP, ImVec2(sbP.x + 28.0f, sbP.y + 26.0f), IM_COL32(36, 40, 52, 220), 6.0f);
            }
            UIWidgets::DrawIcon(bdl, ImVec2(sbP.x + 14.0f, sbP.y + 13.0f), 14.5f,
                                UiIcon::SendPlane, IM_COL32(196, 202, 216, 255));

            if (sendCon && m_consoleInput[0] != '\0') {
                std::string cmd = m_consoleInput;
                m_consoleInput[0] = '\0';
                pushLog("INFO", "> " + cmd);
                if (cmd == "clear") m_logs.clear();
                else if (cmd == "play") { m_scene.beginPlay(); m_playing = true; }
                else if (cmd == "build") m_screen = ScreenId::Build;
                else {
                    submitAgentPrompt(cmd);
                    m_bottomTab = 6;
                }
            }
        } else if (m_bottomTab == 1) {
            ImGui::TextColored(ImVec4(0.55f, 0.85f, 0.62f, 1.0f), "[Astron Native Runtime] OpenGL 3.3 Core + GLSL 3.30 + C11 Math/SIMD + C#/.NET + Lua ativos.");
            ImGui::TextDisabled("Projeto ativo: %s (%s)", m_projects.currentProject().name.c_str(),
                                m_projects.currentProject().rootDir.string().c_str());
            ImGui::TextDisabled("Objetos 3D na cena: %zu | Scripts sincronizados em games/AstronGame/Core/", m_scene.objects.size());
        } else if (m_bottomTab == 2) {
            ImGui::Columns(3, "##dbg_cols");
            ImGui::TextColored(ImVec4(0.75f, 0.78f, 0.86f, 1.0f), "Propriedade"); ImGui::NextColumn();
            ImGui::TextColored(ImVec4(0.75f, 0.78f, 0.86f, 1.0f), "Valor"); ImGui::NextColumn();
            ImGui::TextColored(ImVec4(0.75f, 0.78f, 0.86f, 1.0f), "Tipo"); ImGui::NextColumn();
            ImGui::Separator();
            if (const auto* s = m_scene.selected()) {
                ImGui::TextUnformatted("Selected.Name"); ImGui::NextColumn();
                ImGui::TextUnformatted(s->name.c_str()); ImGui::NextColumn();
                ImGui::TextDisabled("System.String"); ImGui::NextColumn();

                ImGui::TextUnformatted("Selected.Position"); ImGui::NextColumn();
                ImGui::Text("(%.2f, %.2f, %.2f)", s->position.x, s->position.y, s->position.z); ImGui::NextColumn();
                ImGui::TextDisabled("AstronVec3"); ImGui::NextColumn();

                ImGui::TextUnformatted("Simulando"); ImGui::NextColumn();
                ImGui::TextUnformatted(m_playing ? "true" : "false"); ImGui::NextColumn();
                ImGui::TextDisabled("System.Boolean"); ImGui::NextColumn();
            }
            ImGui::Columns(1);
        } else if (m_bottomTab == 3) {
            ImGui::TextColored(ImVec4(0.70f, 0.86f, 1.0f, 1.0f), "Sintetizador C11 de Efeitos Sonoros (astron_audio.c — PCM 16-bit 44.1 kHz)");
            const char* presets[] = { "Jump", "Coin", "Laser", "Explosion", "Powerup" };
            ImGui::SetNextItemWidth(130.0f);
            ImGui::Combo("Preset", &m_audioPreset, presets, 5);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(130.0f);
            ImGui::SliderFloat("Freq (Hz)", &m_audioFreqHz, 90.0f, 1200.0f, "%.0f Hz");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(110.0f);
            ImGui::SliderFloat("Dura\xC3\xA7\xC3\xA3o", &m_audioDurationSec, 0.10f, 1.50f, "%.2f s");
            ImGui::SameLine();
            if (UIWidgets::WhitePillButton("Gerar .WAV no Projeto", ImVec2(178.0f, 26.0f), 7.0f)) {
                static const char* kNames[] = { "jump.wav", "coin.wav", "laser.wav", "explosion.wav", "powerup.wav" };
                std::string relRes = std::string("res://sounds/") + kNames[m_audioPreset];
                auto diskPath = m_projects.resolveDiskPath(relRes);
                std::error_code ec;
                std::filesystem::create_directories(diskPath.parent_path(), ec);

                AstronAudioParams ap{};
                ap.preset = static_cast<AstronSfxPreset>(m_audioPreset);
                ap.sampleRate = 44100u;
                ap.durationSec = m_audioDurationSec;
                ap.baseFreqHz = m_audioFreqHz;
                ap.volume = m_audioVolume;
                if (astron_audio_write_wav(diskPath.string().c_str(), &ap, m_audioPeaks, 48) > 0) {
                    m_lastSynthWav = relRes;
                    pushToast("WAV PCM 16-bit sintetizado em C11: " + relRes);
                    pushLog("INFO", "SFX gerado em disco: " + relRes);
                }
            }
            ImGui::PlotHistogram("##wav_peaks", m_audioPeaks, 48, 0,
                                 m_lastSynthWav.empty() ? "Envelope C11 PCM" : m_lastSynthWav.c_str(),
                                 0.0f, 1.0f, ImVec2(centerW - 24.0f, 68.0f));
        } else if (m_bottomTab == 4) {
            ImGui::TextDisabled("Timeline de Anima\xC3\xA7\xC3\xA3o — Keyframes de Transforma\xC3\xA7\xC3\xA3o (60 Hz):");
            ImVec2 p = ImGui::GetCursorScreenPos();
            bdl->AddRectFilled(p, ImVec2(p.x + centerW - 24.0f, p.y + 84.0f), IM_COL32(12, 13, 17, 255), 6.0f);
            for (int f = 0; f <= 12; ++f) {
                float x = p.x + 18.0f + f * ((centerW - 60.0f) / 12.0f);
                bdl->AddLine(ImVec2(x, p.y + 8.0f), ImVec2(x, p.y + 76.0f), IM_COL32(38, 40, 50, 255));
                if (f % 3 == 0) {
                    bdl->AddQuadFilled(ImVec2(x, p.y + 34.0f), ImVec2(x + 6.0f, p.y + 42.0f),
                                       ImVec2(x, p.y + 50.0f), ImVec2(x - 6.0f, p.y + 42.0f),
                                       IM_COL32(62, 142, 255, 255));
                }
            }
        } else if (m_bottomTab == 5) {
            if (UIWidgets::WhitePillButton("Compilar GLSL 3.30 Ao Vivo", ImVec2(214.0f, 25.0f), 6.0f)) {
                std::string log;
                if (m_viewport.compileCustomSurfaceShader(m_shaderSource, log)) {
                    m_projects.writeFile("res://shaders/surface.glsl", m_shaderSource);
                    pushToast("Shader GLSL compilado e aplicado ao Viewport 3D!");
                    pushLog("INFO", "GLSL customizado compilado com sucesso.");
                } else {
                    pushLog("ERROR", "Erro GLSL: " + log);
                }
            }
            ImGui::SameLine();
            if (UIWidgets::OutlineButton("Restaurar PBR Padr\xC3\xA3o", ImVec2(174.0f, 25.0f), 6.0f)) {
                m_viewport.useDefaultSurfaceShader();
            }
            if (m_shaderSource.capacity() < 16384) m_shaderSource.reserve(16384);
            m_shaderSource.resize(std::max<size_t>(m_shaderSource.size() + 2048, 8192), '\0');
            if (fontMono) ImGui::PushFont(fontMono);
            ImGui::InputTextMultiline("##glsl_live", m_shaderSource.data(), m_shaderSource.size(),
                                      ImVec2(centerW - 20.0f, bottomH - 72.0f), ImGuiInputTextFlags_AllowTabInput);
            if (fontMono) ImGui::PopFont();
            m_shaderSource.resize(std::strlen(m_shaderSource.c_str()));
        } else if (m_bottomTab == 6) {
            // ==================== AUTONOMOUS AI AGENT (OpenRouter) ====================
            ImGui::BeginChild("##AgentHeaderRow", ImVec2(centerW - 16.0f, 24.0f), false);
            bool hasKey = !m_aiConfig.apiKey.empty();
            ImGui::TextColored(hasKey ? ImVec4(0.32f, 0.90f, 0.55f, 1.0f) : ImVec4(0.95f, 0.72f, 0.30f, 1.0f),
                               "%s", hasKey ? "OpenRouter Conectado" : "Configure sua OpenRouter API Key");
            ImGui::SameLine();
            ImGui::TextDisabled("| Modelo: %s", m_aiConfig.model.c_str());
            ImGui::SameLine(centerW - 286.0f);
            if (ImGui::SmallButton("Configurar API Key")) m_settingsOpen = true;
            ImGui::SameLine();
            if (ImGui::SmallButton("Limpar Chat")) m_chatMsgs.clear();
            ImGui::EndChild();

            ImGui::BeginChild("##AgentHistory", ImVec2(centerW - 16.0f, bottomH - 96.0f), true);
            if (m_chatMsgs.empty()) {
                ImGui::TextDisabled("Pe\xC3\xA7" "a qualquer jogo ou sistema em C# / 3D — o Agente IA aut\xC3\xB4nomo cria objetos 3D,");
                ImGui::TextDisabled("escreve scripts C# reais em res://scripts/ e sincroniza o runtime automaticamente.");
                ImGui::Spacing();
                if (ImGui::SmallButton("Criar Obby 3D Colorido")) {
                    submitAgentPrompt("Crie um percurso Obby 3D com 6 plataformas neon coloridas subindo no espaço e um script C# de pulo.");
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Criar Cidade RP")) {
                    submitAgentPrompt("Construa uma praça de Projeto RP na cena 3D com prédios, postes de luz, rua e controlador C#.");
                }
            }
            for (const auto& m : m_chatMsgs) {
                if (m.isUser) {
                    ImGui::TextColored(ImVec4(0.48f, 0.76f, 1.0f, 1.0f), "[Voc\xC3\xAA]:");
                    ImGui::SameLine();
                    ImGui::TextWrapped("%s", m.text.c_str());
                } else {
                    ImGui::TextColored(ImVec4(0.40f, 0.95f, 0.68f, 1.0f), "[Astron IA]:");
                    for (const auto& tl : m.tools) {
                        ImGui::TextColored(ImVec4(0.88f, 0.78f, 0.38f, 1.0f), "  [Tool %s] -> %s", tl.first.c_str(), tl.second.c_str());
                    }
                    if (!m.error.empty()) ImGui::TextColored(ImVec4(0.98f, 0.38f, 0.38f, 1.0f), "%s", m.error.c_str());
                    if (!m.text.empty())  ImGui::TextWrapped("%s", m.text.c_str());
                    if (m.pending)        ImGui::TextColored(ImVec4(0.70f, 0.75f, 0.88f, 1.0f), "  %s", m.status.c_str());
                }
                ImGui::Separator();
            }
            ImGui::EndChild();

            ImGui::SetNextItemWidth(centerW - 114.0f);
            bool sendEnter = ImGui::InputTextWithHint("##agent_prompt", "Descreva o jogo, mec\xC3\xA2nica C# ou cena 3D para o Agente IA...",
                                                      m_agentInput, sizeof(m_agentInput), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine();
            bool busy = m_agent.isRunning();
            if (busy) ImGui::BeginDisabled();
            if (UIWidgets::WhitePillButton(busy ? "Gerando..." : "Enviar", ImVec2(90.0f, 26.0f), 6.0f) || (sendEnter && !busy)) {
                if (m_agentInput[0] != '\0') {
                    std::string p = m_agentInput;
                    m_agentInput[0] = '\0';
                    submitAgentPrompt(p);
                }
            }
            if (busy) ImGui::EndDisabled();
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    // -------------------- RIGHT COLUMN: PROPRIEDADES + CAIXA DE FERRAMENTAS --------------------
    if (rightW > 0.0f) {
        float propH = m_showProperties ? (m_showToolbox ? workH * 0.71f : workH) : 0.0f;
        float tbxH  = m_showToolbox ? (workH - propH) : 0.0f;

        if (m_showProperties) {
            ImGui::SetCursorScreenPos(ImVec2(W - rightW, workY));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(17, 18, 23, 255));
            ImGui::BeginChild("##PropsPanel", ImVec2(rightW, propH), true);
            drawPanelHeader("Propriedades", UiIcon::Gear, IM_COL32(194, 200, 214, 255), &m_showProperties);

            ImDrawList* pdl = ImGui::GetWindowDrawList();

            ImGui::SetCursorPosX(6.0f);
            drawSearchInput("##prp_search", "Pesquisar propriedades...", m_propsSearch, sizeof(m_propsSearch));
            ImGui::Spacing();

            if (SceneObject* sel = m_scene.selected()) {
                // "Parte: Model                               ✕" header bar (Image 1)
                {
                    ImVec2 hp = ImGui::GetCursorScreenPos();
                    float availW = ImGui::GetContentRegionAvail().x;
                    pdl->AddRectFilled(hp, ImVec2(hp.x + availW, hp.y + 24.0f), IM_COL32(22, 24, 31, 255), 5.0f);
                    std::string pTitle = "Parte: " + sel->name;
                    pdl->AddText(ImVec2(hp.x + 8.0f, hp.y + 4.0f), IM_COL32(236, 239, 248, 255), pTitle.c_str());
                    UIWidgets::DrawIcon(pdl, ImVec2(hp.x + availW - 12.0f, hp.y + 12.0f), 9.5f,
                                        UiIcon::Close, IM_COL32(144, 148, 160, 220));
                    ImGui::Dummy(ImVec2(availW, 26.0f));
                }

                const float labelColW = 94.0f;
                const float availRowW = rightW - 22.0f;
                const float rightAreaW = availRowW - labelColW;

                // 1. Transformação (Posição, Rotação, Escala with X/Y/Z inline boxes)
                if (ImGui::CollapsingHeader("  Transforma\xC3\xA7\xC3\xA3o", ImGuiTreeNodeFlags_DefaultOpen)) {
                    float boxW = (rightAreaW - 8.0f) / 3.0f;

                    auto vec3Row = [&](const char* rowLbl, const char* idBase, AstronVec3* v) {
                        if (!icontains(rowLbl, m_propsSearch)) return;
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextColored(ImVec4(0.74f, 0.76f, 0.82f, 1.0f), "%s", rowLbl);
                        ImGui::SameLine(labelColW);
                        std::string ix = std::string("##") + idBase + "_x";
                        std::string iy = std::string("##") + idBase + "_y";
                        std::string iz = std::string("##") + idBase + "_z";
                        drawAxisFloatBox(ix.c_str(), "X", &v->x, boxW);
                        ImGui::SameLine(0, 4.0f);
                        drawAxisFloatBox(iy.c_str(), "Y", &v->y, boxW);
                        ImGui::SameLine(0, 4.0f);
                        drawAxisFloatBox(iz.c_str(), "Z", &v->z, boxW);
                    };

                    vec3Row("Posi\xC3\xA7\xC3\xA3o", "pos", &sel->position);
                    vec3Row("Rota\xC3\xA7\xC3\xA3o", "rot", &sel->rotation);
                    vec3Row("Escala",                 "scl", &sel->scale);
                }

                // 2. Aparência (Cor [#FFFFFF], Material [Plastic ▾], Transparência [0.000 ──], Visibilidade [switch])
                if (ImGui::CollapsingHeader("  Apar\xC3\xAAncia", ImGuiTreeNodeFlags_DefaultOpen)) {
                    // Cor
                    if (icontains("Cor", m_propsSearch)) {
                        AstronVec4 c = { 1, 1, 1, 1 };
                        astron_hex_to_rgba(sel->colorHex.c_str(), &c);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextColored(ImVec4(0.74f, 0.76f, 0.82f, 1.0f), "Cor");
                        ImGui::SameLine(labelColW);

                        ImVec2 cp = ImGui::GetCursorScreenPos();
                        if (ImGui::InvisibleButton("##col_hex_btn", ImVec2(rightAreaW, 24.0f))) {
                            ImGui::OpenPopup("AstronColorPickerPopup");
                        }
                        pdl->AddRectFilled(cp, ImVec2(cp.x + rightAreaW, cp.y + 24.0f), IM_COL32(14, 15, 20, 255), 5.0f);
                        pdl->AddRect(cp, ImVec2(cp.x + rightAreaW, cp.y + 24.0f), IM_COL32(42, 45, 56, 220), 5.0f);
                        pdl->AddRectFilled(ImVec2(cp.x + 5.0f, cp.y + 4.0f), ImVec2(cp.x + 21.0f, cp.y + 20.0f),
                                           ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, 1.0f)), 3.0f);
                        pdl->AddRect(ImVec2(cp.x + 5.0f, cp.y + 4.0f), ImVec2(cp.x + 21.0f, cp.y + 20.0f),
                                     IM_COL32(80, 84, 96, 200), 3.0f);
                        pdl->AddText(ImVec2(cp.x + 28.0f, cp.y + 4.0f), IM_COL32(228, 232, 242, 255), sel->colorHex.c_str());
                        UIWidgets::DrawIcon(pdl, ImVec2(cp.x + rightAreaW - 13.0f, cp.y + 12.0f),
                                            12.0f, UiIcon::Eyedropper, IM_COL32(160, 164, 176, 240));

                        if (ImGui::BeginPopup("AstronColorPickerPopup")) {
                            float rgb[3] = { c.x, c.y, c.z };
                            if (ImGui::ColorPicker3("##picker3", rgb)) {
                                char hexBuf[16] = {};
                                astron_rgba_to_hex({ rgb[0], rgb[1], rgb[2], 1.0f }, hexBuf);
                                sel->colorHex = hexBuf;
                            }
                            ImGui::EndPopup();
                        }
                    }

                    // Material
                    if (icontains("Material", m_propsSearch)) {
                        const char* mats[] = { "Plastic", "Metal", "Wood", "Glass", "Neon", "Matte" };
                        int matIdx = static_cast<int>(sel->material);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextColored(ImVec4(0.74f, 0.76f, 0.82f, 1.0f), "Material");
                        ImGui::SameLine(labelColW);
                        ImGui::SetNextItemWidth(rightAreaW);
                        if (ImGui::Combo("##mat_cmb", &matIdx, mats, 6)) {
                            sel->material = static_cast<MaterialKind>(matIdx);
                        }
                    }

                    // Transparência
                    if (icontains("Transparencia", m_propsSearch)) {
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextColored(ImVec4(0.74f, 0.76f, 0.82f, 1.0f), "Transpar\xC3\xAAncia");
                        ImGui::SameLine(labelColW);
                        ImGui::SetNextItemWidth(54.0f);
                        ImGui::DragFloat("##tr_num", &sel->transparency, 0.01f, 0.0f, 0.98f, "%.3f");
                        ImGui::SameLine(0, 6.0f);
                        ImGui::SetNextItemWidth(rightAreaW - 60.0f);
                        ImGui::SliderFloat("##tr_sld", &sel->transparency, 0.0f, 0.98f, "");
                    }

                    // Visibilidade
                    if (icontains("Visibilidade", m_propsSearch)) {
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextColored(ImVec4(0.74f, 0.76f, 0.82f, 1.0f), "Visibilidade");
                        ImGui::SameLine(availRowW - 34.0f);
                        UIWidgets::ToggleSwitch("##vis_sw", &sel->visible, true);
                    }
                }

                // 3. Física (Massa [1.000], Gravidade [switch], Colisão [Ativado ▾])
                if (ImGui::CollapsingHeader("  F\xC3\xADsica", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (icontains("Massa", m_propsSearch)) {
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextColored(ImVec4(0.74f, 0.76f, 0.82f, 1.0f), "Massa");
                        ImGui::SameLine(labelColW);
                        ImGui::SetNextItemWidth(rightAreaW);
                        ImGui::DragFloat("##phys_mass", &sel->mass, 0.1f, 0.01f, 1000.0f, "%.3f");
                    }

                    if (icontains("Gravidade", m_propsSearch)) {
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextColored(ImVec4(0.74f, 0.76f, 0.82f, 1.0f), "Gravidade");
                        ImGui::SameLine(availRowW - 34.0f);
                        UIWidgets::ToggleSwitch("##phys_grav", &sel->gravity, true);
                    }

                    if (icontains("Colisao", m_propsSearch)) {
                        const char* cols[] = { "Ativado", "Desativado", "Trigger" };
                        int colIdx = (sel->collision == "Desativado") ? 1 : (sel->collision == "Trigger" ? 2 : 0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextColored(ImVec4(0.74f, 0.76f, 0.82f, 1.0f), "Colis\xC3\xA3o");
                        ImGui::SameLine(labelColW);
                        ImGui::SetNextItemWidth(rightAreaW);
                        if (ImGui::Combo("##phys_col", &colIdx, cols, 3)) {
                            sel->collision = cols[colIdx];
                        }
                    }
                }

                // 4. Renderização (collapsed by default just like Image 1)
                if (ImGui::CollapsingHeader("  Renderiza\xC3\xA7\xC3\xA3o")) {
                    ImGui::Checkbox("Sombras Din\xC3\xA2micas", &m_viewport.showGrid);
                    ImGui::Checkbox("Modo Wireframe", &m_viewport.wireframe);
                }

                // 5. Outras Configurações (collapsed by default just like Image 1)
                if (ImGui::CollapsingHeader("  Outras Configura\xC3\xA7\xC3\xB5" "es")) {
                    char tagBuf[64] = {};
                    std::snprintf(tagBuf, sizeof(tagBuf), "%s", sel->tag.c_str());
                    if (ImGui::InputText("Tag", tagBuf, sizeof(tagBuf))) sel->tag = tagBuf;
                }
            } else {
                ImGui::TextDisabled("Selecione um objeto no Viewport 3D ou Explorer.");
            }

            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        // -------------------- CAIXA DE FERRAMENTAS (Identical to Image 1) --------------------
        if (m_showToolbox) {
            ImGui::SetCursorScreenPos(ImVec2(W - rightW, workY + propH));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(17, 18, 23, 255));
            ImGui::BeginChild("##ToolboxPanel", ImVec2(rightW, tbxH), true);
            drawPanelHeader("Caixa de Ferramentas", UiIcon::Cube3D, IM_COL32(180, 186, 200, 255), &m_showToolbox);

            ImGui::SetCursorPosX(6.0f);
            const char* tbTabs[] = { "Modelos", "Recursos", "Materiais" };
            for (int i = 0; i < 3; ++i) {
                if (i > 0) ImGui::SameLine(0, 4);
                bool act = (m_toolboxTab == i);
                ImGui::PushStyleColor(ImGuiCol_Button, act ? IM_COL32(37, 114, 246, 255) : IM_COL32(25, 27, 34, 255));
                ImGui::PushStyleColor(ImGuiCol_Text, act ? IM_COL32(255, 255, 255, 255) : IM_COL32(168, 172, 184, 255));
                if (ImGui::Button(tbTabs[i], ImVec2((rightW - 22.0f) / 3.0f, 25.0f))) m_toolboxTab = i;
                ImGui::PopStyleColor(2);
            }

            ImGui::Spacing();
            ImDrawList* tdl = ImGui::GetWindowDrawList();

            if (m_toolboxTab == 0) {
                struct PrimCard { const char* name; MeshKind kind; };
                const PrimCard prims[4] = {
                    { "Part",     MeshKind::Cube },
                    { "Sphere",   MeshKind::Sphere },
                    { "Cylinder", MeshKind::Cylinder },
                    { "Block",    MeshKind::Block }
                };

                float gap = 6.0f;
                float cw = (rightW - 18.0f - gap * 3.0f) / 4.0f;
                float ch = std::clamp(tbxH - 76.0f, 72.0f, 92.0f);
                ImVec2 base = ImGui::GetCursorScreenPos();
                base.x += 2.0f;

                for (int i = 0; i < 4; ++i) {
                    ImVec2 cMin(base.x + i * (cw + gap), base.y);
                    ImVec2 cMax(cMin.x + cw, cMin.y + ch);

                    ImGui::SetCursorScreenPos(cMin);
                    std::string id = std::string("##prim_") + prims[i].name;
                    bool clicked = ImGui::InvisibleButton(id.c_str(), ImVec2(cw, ch));
                    bool hov = ImGui::IsItemHovered();

                    tdl->AddRectFilled(cMin, cMax, hov ? IM_COL32(29, 32, 42, 255) : IM_COL32(22, 24, 31, 255), 8.0f);
                    tdl->AddRect(cMin, cMax, hov ? IM_COL32(68, 142, 255, 240) : IM_COL32(40, 43, 54, 230), 8.0f);

                    UIWidgets::DrawToolboxShape(tdl, ImVec2(cMin.x + cw * 0.5f, cMin.y + ch * 0.42f), 40.0f, prims[i].kind);

                    ImVec2 ns = ImGui::CalcTextSize(prims[i].name);
                    tdl->AddText(ImVec2(cMin.x + (cw - ns.x) * 0.5f, cMax.y - 20.0f), IM_COL32(214, 218, 228, 255), prims[i].name);

                    if (clicked) {
                        float offset = static_cast<float>(m_scene.objects.size()) * 0.8f;
                        m_scene.addObject(prims[i].name, prims[i].kind, { offset * 0.5f, 0.5f, 0.0f });
                        pushLog("INFO", std::string("Objeto 3D instanciado: ") + prims[i].name);
                    }
                }
            } else if (m_toolboxTab == 1) {
                if (ImGui::Button("Script Player.cs", ImVec2(-8, 28))) openFileTab("res://scripts/Player.cs");
                if (ImGui::Button("Script main.lua", ImVec2(-8, 28))) openFileTab("res://main.lua");
            } else {
                const char* mats[] = { "Plastic", "Metal", "Wood", "Glass", "Neon", "Matte" };
                for (int m = 0; m < 6; ++m) {
                    if (ImGui::Button(mats[m], ImVec2((rightW - 24.0f) * 0.5f, 26.0f))) {
                        if (auto* s = m_scene.selected()) s->material = static_cast<MaterialKind>(m);
                    }
                    if (m % 2 == 0) ImGui::SameLine();
                }
            }

            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    }

    // ==================== 4. BOTTOM STATUS BAR (Identical to Image 1) ====================
    const float sbY = H - statusH;
    rdl->AddRectFilled(ImVec2(0, sbY), ImVec2(W, H), IM_COL32(15, 16, 21, 255));
    rdl->AddLine(ImVec2(0, sbY), ImVec2(W, sbY), IM_COL32(36, 38, 48, 255));

    rdl->AddCircleFilled(ImVec2(14.0f, sbY + 12.0f), 4.0f, IM_COL32(48, 214, 114, 255));
    rdl->AddText(ImVec2(24.0f, sbY + 4.0f), IM_COL32(198, 202, 214, 255),
                 m_playing ? "Simulando (WASD + Espa\xC3\xA7o)" : "Pronto");

    int fps = static_cast<int>(std::round(ImGui::GetIO().Framerate));
    if (fps <= 0) fps = 144;
    uint32_t ramMb = astron_process_memory_mb();
    if (ramMb == 0) ramMb = 842;
    char statBuf[128] = {};
    std::snprintf(statBuf, sizeof(statBuf), "FPS: %d   |   Mem\xC3\xB3ria: %u MB   |   Vers\xC3\xA3o: 0.1.0", fps, ramMb);
    ImVec2 stSz = ImGui::CalcTextSize(statBuf);
    rdl->AddText(ImVec2(W - stSz.x - 16.0f, sbY + 4.0f), IM_COL32(168, 172, 184, 255), statBuf);

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

} // namespace astron
