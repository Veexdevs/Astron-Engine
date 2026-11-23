#include "EngineApp.hpp"
#include "UIWidgets.hpp"
#include "../core_c/astron_platform.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace astron {

namespace {

int InputTextStdStringCallback(ImGuiInputTextCallbackData* data) {
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
        auto* str = static_cast<std::string*>(data->UserData);
        str->resize(static_cast<size_t>(data->BufTextLen));
        data->Buf = str->data();
    }
    return 0;
}

bool InputTextMultilineString(const char* label, std::string* str, const ImVec2& size) {
    if (str->capacity() < 256) str->reserve(256);
    return ImGui::InputTextMultiline(
        label,
        str->data(),
        str->capacity() + 1,
        size,
        ImGuiInputTextFlags_CallbackResize | ImGuiInputTextFlags_AllowTabInput,
        InputTextStdStringCallback,
        str
    );
}

bool ToolbarButton(const char* id, const char* iconGlyph, const char* label, bool active, float w = 72.0f) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const float h = 48.0f;

    bool clicked = ImGui::InvisibleButton(id, ImVec2(w, h));
    bool hovered = ImGui::IsItemHovered();

    if (active) {
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), IM_COL32(28, 78, 168, 255), 8.0f);
        dl->AddRect(p, ImVec2(p.x + w, p.y + h), IM_COL32(58, 122, 235, 255), 8.0f);
    } else if (hovered) {
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), IM_COL32(30, 33, 40, 255), 8.0f);
    }

    ImVec2 gi = ImGui::CalcTextSize(iconGlyph);
    dl->AddText(ImVec2(p.x + (w - gi.x) * 0.5f, p.y + 7.0f), IM_COL32(240, 242, 248, 255), iconGlyph);

    ImVec2 tl = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(p.x + (w - tl.x) * 0.5f, p.y + 27.0f),
                active ? IM_COL32(255, 255, 255, 255) : IM_COL32(204, 208, 218, 255), label);
    return clicked;
}

} // namespace

void EngineApp::renderEditor(float dt, float w, float h) {
    // Keyboard shortcuts inside Editor
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
        if (m_activeTabIdx >= 0 && m_activeTabIdx < static_cast<int>(m_tabs.size()) && !m_tabs[static_cast<size_t>(m_activeTabIdx)].isScene) {
            auto& tab = m_tabs[static_cast<size_t>(m_activeTabIdx)];
            m_projects.writeFile(tab.id, tab.buffer);
            tab.dirty = false;
            pushLog("INFO", "Arquivo salvo: " + tab.id);
            pushToast("Arquivo salvo ✓");
        } else {
            saveActiveScene();
        }
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_B, false)) {
        m_screen = ScreenId::Build;
    }
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_I, false)) {
        m_showBottom = true;
        m_bottomTab = 6; // Agente IA
    }

    // Play-mode physics step
    if (m_playing && !m_paused) {
        float mx = (ImGui::IsKeyDown(ImGuiKey_D) || ImGui::IsKeyDown(ImGuiKey_RightArrow) ? 1.f : 0.f) -
                   (ImGui::IsKeyDown(ImGuiKey_A) || ImGui::IsKeyDown(ImGuiKey_LeftArrow)  ? 1.f : 0.f);
        float mz = (ImGui::IsKeyDown(ImGuiKey_S) || ImGui::IsKeyDown(ImGuiKey_DownArrow)  ? 1.f : 0.f) -
                   (ImGui::IsKeyDown(ImGuiKey_W) || ImGui::IsKeyDown(ImGuiKey_UpArrow)    ? 1.f : 0.f);
        bool jp  = ImGui::IsKeyDown(ImGuiKey_Space);
        m_scene.stepPhysics(dt, mx, mz, jp);
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(14, 15, 18, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##AstronEditorRoot", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // ==================== 1. TOP MENU BAR (Astron Engine logo + "Astron Engine") ====================
    const float menuH = 36.0f;
    dl->AddRectFilled(ImVec2(0, 0), ImVec2(w, menuH), IM_COL32(12, 13, 16, 255));
    dl->AddLine(ImVec2(0, menuH), ImVec2(w, menuH), IM_COL32(28, 30, 36, 255));

    // Official Astron Engine Saturn-ring planet logo (replacing old "N")
    UIWidgets::DrawPlanetMark(dl, ImVec2(24.0f, menuH * 0.5f), 11.5f,
                              IM_COL32(255, 255, 255, 255), IM_COL32(12, 13, 16, 255));

    // "Astron Engine v" dropdown menu in the top-left header bar
    ImGui::SetCursorScreenPos(ImVec2(44.0f, 4.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(28, 30, 38, 255));
    if (fontUiBold) ImGui::PushFont(fontUiBold);
    if (ImGui::Button("Astron Engine  v", ImVec2(142.0f, 28.0f))) {
        ImGui::OpenPopup("astron_engine_brand_menu");
    }
    if (fontUiBold) ImGui::PopFont();

    if (ImGui::BeginPopup("astron_engine_brand_menu")) {
        if (ImGui::MenuItem("Voltar ao Hub Inicial")) m_screen = ScreenId::Home;
        if (ImGui::MenuItem("Compilar Projeto (.apk / .ipa / .exe / Linux)")) m_screen = ScreenId::Build;
        ImGui::Separator();
        if (ImGui::MenuItem("Configurações & Agente IA")) m_settingsOpen = true;
        if (ImGui::MenuItem("Sair para Login")) m_screen = ScreenId::Login;
        ImGui::EndPopup();
    }

    // Menu items matching the Editor design
    ImGui::SetCursorScreenPos(ImVec2(194.0f, 4.0f));
    const char* menuNames[] = {
        "Arquivo", "Editar", "Visualizar", "Projeto", "Cena",
        "Jogo", "Script", "Biblioteca de Recursos", "Configurações", "Ajuda"
    };
    for (int m = 0; m < 10; ++m) {
        std::string popId = std::string("ed_menu_") + std::to_string(m);
        if (ImGui::Button(menuNames[m], ImVec2(0, 28.0f))) {
            ImGui::OpenPopup(popId.c_str());
        }
        ImGui::SameLine(0, 6.0f);

        if (ImGui::BeginPopup(popId.c_str())) {
            if (m == 0) { // Arquivo
                if (ImGui::MenuItem("Novo Projeto C#...")) m_newProjOpen = true;
                if (ImGui::MenuItem("Abrir Projeto...")) { m_homePage = "projetos"; m_screen = ScreenId::Home; }
                ImGui::Separator();
                if (ImGui::MenuItem("Salvar Cena", "Ctrl+S")) saveActiveScene();
                if (ImGui::MenuItem("Compilar Projeto...", "Ctrl+B")) m_screen = ScreenId::Build;
                ImGui::Separator();
                if (ImGui::MenuItem("Voltar ao Hub")) m_screen = ScreenId::Home;
            } else if (m == 1) { // Editar
                if (ImGui::MenuItem("Duplicar Objeto")) {
                    if (const auto* sel = m_scene.selected()) {
                        m_scene.addObject(sel->name, sel->mesh, { sel->position.x + 1.2f, sel->position.y, sel->position.z });
                    }
                }
                if (ImGui::MenuItem("Remover Objeto Selecionado", "Del")) m_scene.removeSelected();
            } else if (m == 2) { // Visualizar
                ImGui::MenuItem("Explorer", nullptr, &m_showExplorer);
                ImGui::MenuItem("Arquivos (res://)", nullptr, &m_showFiles);
                ImGui::MenuItem("Propriedades", nullptr, &m_showProperties);
                ImGui::MenuItem("Caixa de Ferramentas", nullptr, &m_showToolbox);
                ImGui::MenuItem("Painel Inferior", nullptr, &m_showBottom);
                ImGui::Separator();
                ImGui::MenuItem("Grade 3D", nullptr, &m_viewport.showGrid);
                ImGui::MenuItem("Wireframe", nullptr, &m_viewport.wireframe);
            } else if (m == 3) { // Projeto
                if (ImGui::MenuItem("Compilar APK / IPA / EXE / Linux", "Ctrl+B")) m_screen = ScreenId::Build;
                if (ImGui::MenuItem("Exportar Pacote C#")) {
                    auto p = m_projects.exportProjectBundle("editor");
                    pushToast("Exportado: " + p.filename().string());
                }
            } else if (m == 4) { // Cena
                if (ImGui::MenuItem("Adicionar Cubo")) m_scene.addObject("Part", MeshKind::Cube, { 0.f, 0.5f, 0.f });
                if (ImGui::MenuItem("Adicionar Esfera")) m_scene.addObject("Sphere", MeshKind::Sphere, { 1.5f, 0.5f, 0.f });
                if (ImGui::MenuItem("Adicionar Cilindro")) m_scene.addObject("Cylinder", MeshKind::Cylinder, { -1.5f, 0.5f, 0.f });
                if (ImGui::MenuItem("Resetar Câmera")) m_viewport.resetCamera();
            } else if (m == 5) { // Jogo
                if (ImGui::MenuItem("Jogar", "F5")) { m_playing = true; m_paused = false; m_scene.beginPlay(); pushLog("INFO", "Modo de jogo iniciado."); }
                if (ImGui::MenuItem("Parar", "F8")) { m_playing = false; m_paused = false; m_scene.stopPlay(); pushLog("INFO", "Jogo parado."); }
            } else if (m == 6) { // Script
                if (ImGui::MenuItem("Abrir Agente IA Autônomo", "Ctrl+I")) { m_showBottom = true; m_bottomTab = 6; }
                if (ImGui::MenuItem("Editar GameWorld.cs (C#)")) openFileTab("res://scripts/GameWorld.cs");
                if (ImGui::MenuItem("Editar Player.cs (C#)")) openFileTab("res://scripts/Player.cs");
                if (ImGui::MenuItem("Novo Script C#")) {
                    std::string path = "res://scripts/NovoComponente.cs";
                    if (!m_projects.fileExists(path)) {
                        m_projects.writeFile(path,
                            "using Astron.Engine;\n\nnamespace Astron.Scripts\n{\n    public sealed class NovoComponente : Component\n    {\n        public override void OnUpdate(float dt)\n        {\n        }\n    }\n}\n");
                    }
                    openFileTab(path);
                }
            } else if (m == 7) { // Biblioteca
                if (ImGui::MenuItem("Editor de Shader GLSL")) { m_showBottom = true; m_bottomTab = 5; }
                if (ImGui::MenuItem("Abrir Bibioteca")) { m_homePage = "biblioteca"; m_screen = ScreenId::Home; }
            } else if (m == 8) { // Configurações
                if (ImGui::MenuItem("Configurar API de IA (OpenRouter, OpenAI, Claude...)")) m_settingsOpen = true;
            } else if (m == 9) { // Ajuda
                if (ImGui::MenuItem("Sobre a Astron Engine")) pushToast("Astron Engine v1.0.0 — C++20 / C11 / GLSL / C# (.NET 8)");
            }
            ImGui::EndPopup();
        }
    }
    ImGui::PopStyleColor(2);

    UIWidgets::DrawWindowChrome(w, onMinimizeWindow, onMaximizeWindow, [this]() { m_screen = ScreenId::Home; });

    // ==================== 2. TOOLBAR ====================
    const float tbY = menuH;
    const float tbH = 60.0f;
    dl->AddRectFilled(ImVec2(0, tbY), ImVec2(w, tbY + tbH), IM_COL32(18, 20, 25, 255));
    dl->AddLine(ImVec2(0, tbY + tbH), ImVec2(w, tbY + tbH), IM_COL32(32, 35, 42, 255));

    ImGui::SetCursorScreenPos(ImVec2(10.0f, tbY + 6.0f));
    const char* toolLabels[5] = { "Selecionar", "Mover", "Rotacionar", "Escalar", "Transformar" };
    const char* toolGlyphs[5] = { "↖", "✥", "↻", "⤢", "⊞" };
    for (int t = 0; t < 5; ++t) {
        std::string tid = std::string("##tb_tool_") + std::to_string(t);
        if (ToolbarButton(tid.c_str(), toolGlyphs[t], toolLabels[t], m_activeTool == t, 76.0f)) {
            m_activeTool = t;
        }
        ImGui::SameLine(0, 4.0f);
    }

    // Separator
    {
        ImVec2 c = ImGui::GetCursorScreenPos();
        dl->AddLine(ImVec2(c.x + 4, tbY + 12), ImVec2(c.x + 4, tbY + tbH - 12), IM_COL32(42, 45, 54, 255));
        ImGui::Dummy(ImVec2(12, tbH - 12));
        ImGui::SameLine();
    }

    if (ToolbarButton("##tb_criar", "📦", "Criar v", false, 66.0f)) ImGui::OpenPopup("tb_create_popup");
    if (ImGui::BeginPopup("tb_create_popup")) {
        if (ImGui::MenuItem("Part (Cubo)")) m_scene.addObject("Part", MeshKind::Cube, { 0.f, 0.5f, 0.f });
        if (ImGui::MenuItem("Sphere (Esfera)")) m_scene.addObject("Sphere", MeshKind::Sphere, { 1.6f, 0.5f, 0.f });
        if (ImGui::MenuItem("Cylinder (Cilindro)")) m_scene.addObject("Cylinder", MeshKind::Cylinder, { -1.6f, 0.5f, 0.f });
        if (ImGui::MenuItem("Block (Bloco)")) m_scene.addObject("Block", MeshKind::Block, { 0.f, 0.25f, 2.0f });
        ImGui::EndPopup();
    }
    ImGui::SameLine(0, 4.0f);
    if (ToolbarButton("##tb_cam", "📷", "Câmera", false, 66.0f)) { m_viewport.resetCamera(); pushLog("INFO", "Câmera reposicionada."); }
    ImGui::SameLine(0, 4.0f);
    if (ToolbarButton("##tb_light", "💡", "Iluminação", false, 78.0f)) { m_scene.addObject("Luz", MeshKind::Light, { 2.f, 4.f, 2.f }); pushLog("INFO", "Luz adicionada."); }
    ImGui::SameLine(0, 4.0f);
    if (ToolbarButton("##tb_terr", "⛰", "Terreno", false, 66.0f)) { m_scene.addObject("Terreno", MeshKind::Terrain); pushLog("INFO", "Terreno 3D adicionado."); }
    ImGui::SameLine(0, 4.0f);
    if (ToolbarButton("##tb_comp", "🧩", "Componente", false, 82.0f)) { openFileTab("res://scripts/Player.cs"); }
    ImGui::SameLine(0, 10.0f);

    // 2D | 3D segmented pill
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        dl->AddRectFilled(ImVec2(p.x, p.y + 8), ImVec2(p.x + 84, p.y + 40), IM_COL32(12, 13, 17, 255), 8.0f);
        dl->AddRect(ImVec2(p.x, p.y + 8), ImVec2(p.x + 84, p.y + 40), IM_COL32(40, 43, 52, 255), 8.0f);
        if (m_viewport.is2DMode) {
            dl->AddRectFilled(ImVec2(p.x + 3, p.y + 11), ImVec2(p.x + 41, p.y + 37), IM_COL32(28, 78, 168, 255), 6.0f);
        } else {
            dl->AddRectFilled(ImVec2(p.x + 43, p.y + 11), ImVec2(p.x + 81, p.y + 37), IM_COL32(28, 78, 168, 255), 6.0f);
        }
        dl->AddText(ImVec2(p.x + 13, p.y + 16), IM_COL32(240, 242, 248, 255), "2D");
        dl->AddText(ImVec2(p.x + 53, p.y + 16), IM_COL32(240, 242, 248, 255), "3D");
        if (ImGui::InvisibleButton("##mode_2d3d", ImVec2(84, 48))) {
            float relX = io.MousePos.x - p.x;
            m_viewport.is2DMode = (relX < 42.0f);
        }
        ImGui::SameLine(0, 10.0f);
    }

    // Play / Pause / Stop
    if (ToolbarButton("##tb_play", "▶", "Jogar", m_playing && !m_paused, 60.0f)) {
        if (!m_playing) m_scene.beginPlay();
        m_playing = true;
        m_paused = false;
        pushLog("INFO", "▶ Simulação iniciada (use WASD + Espaço).");
    }
    ImGui::SameLine(0, 4.0f);
    if (ToolbarButton("##tb_pause", "⏸", "Pausar", m_playing && m_paused, 60.0f)) {
        if (m_playing) m_paused = !m_paused;
    }
    ImGui::SameLine(0, 4.0f);
    if (ToolbarButton("##tb_stop", "■", "Parar", false, 60.0f)) {
        if (m_playing) {
            m_playing = false;
            m_paused = false;
            m_scene.stopPlay();
            pushLog("INFO", "■ Simulação parada.");
        }
    }

    // Right-aligned Visualização + Configurações
    ImGui::SetCursorScreenPos(ImVec2(w - 188.0f, tbY + 6.0f));
    if (ToolbarButton("##tb_vis", "🖥", "Visualização v", m_viewport.wireframe, 92.0f)) {
        ImGui::OpenPopup("tb_vis_popup");
    }
    if (ImGui::BeginPopup("tb_vis_popup")) {
        if (ImGui::MenuItem("Perspectiva 3D", nullptr, !m_viewport.is2DMode)) m_viewport.is2DMode = false;
        if (ImGui::MenuItem("Ortográfica 2D", nullptr, m_viewport.is2DMode)) m_viewport.is2DMode = true;
        ImGui::Separator();
        ImGui::MenuItem("Wireframe", nullptr, &m_viewport.wireframe);
        ImGui::MenuItem("Grade 3D", nullptr, &m_viewport.showGrid);
        ImGui::MenuItem("Gizmos RGB", nullptr, &m_viewport.showGizmo);
        ImGui::EndPopup();
    }
    ImGui::SameLine(0, 4.0f);
    if (ToolbarButton("##tb_cfg", "⚙", "Configurações", false, 84.0f)) {
        m_settingsOpen = true;
    }

    // ==================== 3. MAIN WORKSPACE PANELS ====================
    const float statusH = 26.0f;
    const float bodyY = tbY + tbH + 4.0f;
    const float bodyH = h - bodyY - statusH - 4.0f;

    const float leftW  = (m_showExplorer || m_showFiles) ? 256.0f : 0.0f;
    const float rightW = (m_showProperties || m_showToolbox) ? 296.0f : 0.0f;
    const float centerX = (leftW > 0.0f) ? (leftW + 8.0f) : 4.0f;
    const float centerW = w - centerX - ((rightW > 0.0f) ? (rightW + 8.0f) : 4.0f);

    // ---------- LEFT COLUMN: Explorer + Arquivos ----------
    if (leftW > 0.0f) {
        const float expH = (m_showExplorer && m_showFiles) ? (bodyH * 0.58f - 3.0f) : bodyH;
        if (m_showExplorer) {
            ImGui::SetNextWindowPos(ImVec2(4.0f, bodyY));
            ImGui::SetNextWindowSize(ImVec2(leftW, expH));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(17, 19, 24, 255));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
            ImGui::BeginChild("##ExplorerPanel", ImVec2(leftW, expH), true);

            ImGui::TextUnformatted("🗂  Explorer");
            ImGui::SameLine(leftW - 28.0f);
            if (ImGui::SmallButton("×##close_exp")) m_showExplorer = false;

            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##exp_search", "🔍 Pesquisar no Explorer...", m_explorerSearch, sizeof(m_explorerSearch));
            ImGui::Spacing();

            if (ImGui::TreeNodeEx("📦 Projeto", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::TreeNodeEx("📁 Cenas", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGuiTreeNodeFlags fMain = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth |
                                               (m_activeTabIdx == 0 ? ImGuiTreeNodeFlags_Selected : 0);
                    if (ImGui::TreeNodeEx("🧊 Cena Principal", fMain)) {
                        if (ImGui::IsItemClicked()) m_activeTabIdx = 0;
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNodeEx("🧊 Cena Secundária", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth)) {
                        ImGui::TreePop();
                    }
                    ImGui::TreePop();
                }
                if (ImGui::TreeNodeEx("📁 Mundo 3D", ImGuiTreeNodeFlags_DefaultOpen)) {
                    if (ImGui::TreeNodeEx("🗂 Objetos", ImGuiTreeNodeFlags_DefaultOpen)) {
                        for (size_t i = 0; i < m_scene.objects.size(); ++i) {
                            const auto& obj = m_scene.objects[i];
                            if (m_explorerSearch[0] != '\0' && obj.name.find(m_explorerSearch) == std::string::npos) continue;
                            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanAvailWidth |
                                                       (m_scene.selectedIndex == static_cast<int>(i) ? ImGuiTreeNodeFlags_Selected : 0);
                            std::string lbl = "🔹 " + obj.name + "##scobj_" + std::to_string(i);
                            if (ImGui::TreeNodeEx(lbl.c_str(), flags)) {
                                if (ImGui::IsItemClicked()) m_scene.selectedIndex = static_cast<int>(i);
                                ImGui::TreePop();
                            }
                        }
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNodeEx("⛰ Terreno", ImGuiTreeNodeFlags_Leaf)) ImGui::TreePop();
                    if (ImGui::TreeNodeEx("💡 Iluminação", ImGuiTreeNodeFlags_Leaf)) ImGui::TreePop();
                    ImGui::TreePop();
                }
                const char* extraNodes[] = {
                    "📷 Câmeras", "👤 Personagens", "⚙ Física", "🎵 Áudio",
                    "🖥 Interface", "</> Scripts", "📁 Recursos", "🔵 Materiais"
                };
                for (const char* en : extraNodes) {
                    if (ImGui::TreeNodeEx(en, ImGuiTreeNodeFlags_SpanAvailWidth)) {
                        if (std::strstr(en, "Scripts")) {
                            if (ImGui::Selectable("Player.cs")) openFileTab("res://scripts/Player.cs");
                            if (ImGui::Selectable("GameWorld.cs")) openFileTab("res://scripts/GameWorld.cs");
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }

            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();
        }

        if (m_showFiles) {
            const float filesY = m_showExplorer ? (bodyY + expH + 6.0f) : bodyY;
            const float filesH = m_showExplorer ? (bodyH - expH - 6.0f) : bodyH;

            ImGui::SetNextWindowPos(ImVec2(4.0f, filesY));
            ImGui::SetNextWindowSize(ImVec2(leftW, filesH));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(17, 19, 24, 255));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
            ImGui::BeginChild("##FilesPanel", ImVec2(leftW, filesH), true);

            ImGui::TextUnformatted("📁  Arquivos");
            ImGui::SameLine(leftW - 28.0f);
            if (ImGui::SmallButton("×##close_files")) m_showFiles = false;

            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##files_search", "🔍 Pesquisar arquivos...", m_filesSearch, sizeof(m_filesSearch));
            ImGui::Spacing();

            ResTreeEntry tree = m_projects.buildResTree();
            auto drawEntry = [&](auto& self, const ResTreeEntry& e, bool rootOpen) -> void {
                if (e.isDir) {
                    ImGuiTreeNodeFlags f = rootOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0;
                    std::string lbl = "📁 " + e.name + "##dir_" + e.resPath;
                    if (ImGui::TreeNodeEx(lbl.c_str(), f)) {
                        for (const auto& c : e.children) self(self, c, false);
                        ImGui::TreePop();
                    }
                } else {
                    if (m_filesSearch[0] != '\0' && e.name.find(m_filesSearch) == std::string::npos) return;
                    std::string icon = (e.name.find(".cs") != std::string::npos) ? "📄 " :
                                       (e.name.find(".scene") != std::string::npos) ? "🧊 " : "✨ ";
                    std::string lbl = icon + e.name + "##file_" + e.resPath;
                    bool sel = (m_activeTabIdx >= 0 && m_activeTabIdx < static_cast<int>(m_tabs.size()) &&
                                m_tabs[static_cast<size_t>(m_activeTabIdx)].id == e.resPath);
                    if (ImGui::Selectable(lbl.c_str(), sel)) {
                        openFileTab(e.resPath);
                    }
                }
            };
            drawEntry(drawEntry, tree, true);

            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();
        }
    }

    // ---------- CENTER COLUMN: Tabs + 3D Viewport / Code Editor + Bottom Panel ----------
    const float bottomH = m_showBottom ? std::min(228.0f, bodyH * 0.34f) : 0.0f;
    const float vpWrapH = bodyH - (m_showBottom ? (bottomH + 6.0f) : 0.0f);

    {
        ImGui::SetNextWindowPos(ImVec2(centerX, bodyY));
        ImGui::SetNextWindowSize(ImVec2(centerW, vpWrapH));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(17, 19, 24, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::BeginChild("##CenterViewportWrap", ImVec2(centerW, vpWrapH), true, ImGuiWindowFlags_NoScrollbar);

        // Editor Tab Bar
        const float tabBarH = 32.0f;
        ImVec2 cwPos = ImGui::GetWindowPos();
        dl->AddRectFilled(cwPos, ImVec2(cwPos.x + centerW, cwPos.y + tabBarH), IM_COL32(13, 14, 18, 255));

        ImGui::SetCursorPos(ImVec2(6.0f, 4.0f));
        for (size_t i = 0; i < m_tabs.size(); ++i) {
            bool active = (static_cast<int>(i) == m_activeTabIdx);
            ImGui::PushStyleColor(ImGuiCol_Button, active ? IM_COL32(26, 29, 36, 255) : IM_COL32(15, 16, 20, 255));
            std::string tlabel = (m_tabs[i].isScene ? "🧊 " : "📄 ") + m_tabs[i].label + (m_tabs[i].dirty ? " *" : "") + "##tab_" + std::to_string(i);
            if (ImGui::Button(tlabel.c_str(), ImVec2(0, 25.0f))) {
                m_activeTabIdx = static_cast<int>(i);
            }
            ImGui::PopStyleColor();
            if (!m_tabs[i].isScene) {
                ImGui::SameLine(0, 2.0f);
                std::string xid = "×##close_tab_" + std::to_string(i);
                if (ImGui::SmallButton(xid.c_str())) {
                    m_tabs.erase(m_tabs.begin() + static_cast<ptrdiff_t>(i));
                    if (m_activeTabIdx >= static_cast<int>(m_tabs.size())) {
                        m_activeTabIdx = std::max(0, static_cast<int>(m_tabs.size()) - 1);
                    }
                    break;
                }
            }
            ImGui::SameLine(0, 6.0f);
        }
        if (ImGui::SmallButton("+##new_cs_tab")) {
            openFileTab("res://scripts/GameWorld.cs");
        }

        // Active Tab Content
        const float contentH = std::max(32.0f, vpWrapH - tabBarH);
        EditorTab* curTab = (m_activeTabIdx >= 0 && m_activeTabIdx < static_cast<int>(m_tabs.size()))
            ? &m_tabs[static_cast<size_t>(m_activeTabIdx)]
            : nullptr;

        if (!curTab || curTab->isScene) {
            // Render live 3D OpenGL Scene to FBO and display
            int fbW = std::max(32, static_cast<int>(centerW));
            int fbH = std::max(32, static_cast<int>(contentH));
            m_viewport.render(m_scene, fbW, fbH, m_timeSec);

            ImVec2 vpMin(cwPos.x, cwPos.y + tabBarH);
            ImVec2 vpMax(cwPos.x + centerW, cwPos.y + tabBarH + contentH);

            if (m_viewport.colorTexture() != 0) {
                dl->AddImage(
                    static_cast<ImTextureID>(static_cast<uintptr_t>(m_viewport.colorTexture())),
                    vpMin, vpMax,
                    ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f)
                );
            }

            // Mouse interaction inside 3D Viewport
            ImGui::SetCursorScreenPos(vpMin);
            ImGui::InvisibleButton("##vp3d_canvas", ImVec2(centerW, contentH),
                                   ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
            if (ImGui::IsItemHovered()) {
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 1.0f)) {
                    m_viewport.orbit(-io.MouseDelta.x * 0.35f, io.MouseDelta.y * 0.35f);
                } else if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 1.0f)) {
                    m_viewport.pan(io.MouseDelta.x, io.MouseDelta.y);
                }
                if (std::fabs(io.MouseWheel) > 0.001f) {
                    m_viewport.zoom(io.MouseWheel);
                }
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    int hit = m_viewport.pickObject(m_scene, io.MousePos.x - vpMin.x, io.MousePos.y - vpMin.y, fbW, fbH);
                    if (hit >= 0) m_scene.selectedIndex = hit;
                }
            }

            // Top-left Viewport Overlay: [Perspectiva v] + camera/grid/gizmo icons
            ImGui::SetCursorScreenPos(ImVec2(vpMin.x + 12.0f, vpMin.y + 10.0f));
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(20, 22, 28, 215));
            if (ImGui::Button(m_viewport.is2DMode ? "Ortográfica  v" : "Perspectiva  v", ImVec2(128, 28))) {
                m_viewport.is2DMode = !m_viewport.is2DMode;
            }
            ImGui::SameLine(0, 6);
            if (ImGui::Button("📷", ImVec2(30, 28))) m_viewport.resetCamera();
            ImGui::SameLine(0, 4);
            if (ImGui::Button("⊞", ImVec2(30, 28))) m_viewport.showGrid = !m_viewport.showGrid;
            ImGui::SameLine(0, 4);
            if (ImGui::Button("✥", ImVec2(30, 28))) m_viewport.showGizmo = !m_viewport.showGizmo;
            ImGui::SameLine(0, 4);
            if (ImGui::Button("◇", ImVec2(30, 28))) m_viewport.wireframe = !m_viewport.wireframe;
            ImGui::PopStyleColor();

            // Top-right 3D RGB X/Y/Z Orientation Gizmo
            UIWidgets::DrawOrientationGizmo(dl, ImVec2(vpMax.x - 48.0f, vpMin.y + 46.0f), 28.0f, m_viewport.computeViewMatrix());
        } else {
            // Native C# / Scene / GLSL Code Editor Tab
            ImGui::SetCursorPos(ImVec2(10.0f, tabBarH + 6.0f));
            ImGui::TextColored(ImVec4(0.55f, 0.78f, 1.0f, 1.f), "%s", curTab->id.c_str());
            ImGui::SameLine(centerW - 165.0f);
            if (UIWidgets::WhitePillButton("Salvar (Ctrl+S)", ImVec2(150.0f, 26.0f), 7.0f)) {
                m_projects.writeFile(curTab->id, curTab->buffer);
                curTab->dirty = false;
                pushLog("INFO", "Gravado em disco: " + curTab->id);
                pushToast("Salvo em disco ✓");
            }

            ImGui::SetCursorPos(ImVec2(8.0f, tabBarH + 38.0f));
            if (fontMono) ImGui::PushFont(fontMono);
            if (InputTextMultilineString("##code_editor_buf", &curTab->buffer, ImVec2(centerW - 16.0f, contentH - 46.0f))) {
                curTab->dirty = true;
            }
            if (fontMono) ImGui::PopFont();
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    // ---------- BOTTOM PANEL: Console / Saída / Depurador / Áudio / Animação / Editor de Shader / Agente IA ----------
    if (m_showBottom) {
        ImGui::SetNextWindowPos(ImVec2(centerX, bodyY + vpWrapH + 6.0f));
        ImGui::SetNextWindowSize(ImVec2(centerW, bottomH));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(17, 19, 24, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 6));
        ImGui::BeginChild("##BottomPanel", ImVec2(centerW, bottomH), true);

        const char* btabs[7] = {
            "🖥 Console", "📄 Saída", "🐞 Depurador", "🔊 Áudio",
            "🎞 Animação", "✨ Editor de Shader", "🤖 Agente IA"
        };
        for (int b = 0; b < 7; ++b) {
            bool act = (m_bottomTab == b);
            ImGui::PushStyleColor(ImGuiCol_Button, act ? IM_COL32(28, 78, 168, 255) : IM_COL32(22, 24, 30, 255));
            if (ImGui::Button(btabs[b], ImVec2(0, 26.0f))) m_bottomTab = b;
            ImGui::PopStyleColor();
            ImGui::SameLine(0, 5.0f);
        }

        ImGui::SameLine(centerW - 30.0f);
        if (ImGui::SmallButton("×##close_bot")) m_showBottom = false;
        ImGui::Separator();

        if (m_bottomTab == 0) {
            // Console matching Mockup 1 ([INFO], [WARNING], [ERROR] + input bar)
            ImGui::BeginChild("##console_scroll", ImVec2(0, bottomH - 76.0f), false);
            if (fontMono) ImGui::PushFont(fontMono);
            for (const auto& entry : m_logs) {
                ImVec4 col(0.75f, 0.78f, 0.84f, 1.f);
                if (entry.level == "INFO")    col = ImVec4(0.32f, 0.62f, 1.0f, 1.f);
                if (entry.level == "WARNING") col = ImVec4(0.96f, 0.74f, 0.22f, 1.f);
                if (entry.level == "ERROR")   col = ImVec4(0.94f, 0.34f, 0.36f, 1.f);
                ImGui::TextColored(col, "[%s]", entry.level.c_str());
                ImGui::SameLine();
                ImGui::TextUnformatted(entry.message.c_str());
            }
            if (fontMono) ImGui::PopFont();
            ImGui::EndChild();

            ImGui::SetNextItemWidth(centerW - 96.0f);
            if (ImGui::InputTextWithHint("##console_cmd", "Digite uma mensagem...", m_consoleInput, sizeof(m_consoleInput),
                                         ImGuiInputTextFlags_EnterReturnsTrue)) {
                if (m_consoleInput[0] != '\0') {
                    pushLog("INFO", std::string("> ") + m_consoleInput);
                    m_consoleInput[0] = '\0';
                }
            }
            ImGui::SameLine();
            if (UIWidgets::OutlineButton("Enviar", ImVec2(68.0f, 28.0f), 7.0f) && m_consoleInput[0] != '\0') {
                pushLog("INFO", std::string("> ") + m_consoleInput);
                m_consoleInput[0] = '\0';
            }
        } else if (m_bottomTab == 5) {
            // Real GLSL 3.30 Shader Editor compiled directly onto OpenGL GPU!
            ImGui::TextColored(ImVec4(0.65f, 0.80f, 1.0f, 1.f), "Fragment Shader GLSL 3.30 Core (ao vivo na GPU):");
            ImGui::SameLine(centerW - 320.0f);
            if (UIWidgets::WhitePillButton("Compilar GLSL na GPU", ImVec2(175.0f, 25.0f), 7.0f)) {
                std::string logOut;
                if (m_viewport.compileCustomSurfaceShader(m_shaderSource, logOut)) {
                    m_projects.writeFile("res://shaders/surface.glsl", m_shaderSource);
                    pushLog("INFO", logOut);
                    pushToast("Shader GLSL compilado na GPU ✓");
                } else {
                    pushLog("ERROR", logOut);
                    pushToast("Erro de compilação GLSL (veja Console)");
                }
            }
            ImGui::SameLine();
            if (UIWidgets::OutlineButton("Padrão PBR", ImVec2(115.0f, 25.0f), 7.0f)) {
                m_viewport.useDefaultSurfaceShader();
                pushToast("Shader PBR padrão restaurado");
            }

            if (fontMono) ImGui::PushFont(fontMono);
            InputTextMultilineString("##glsl_live_editor", &m_shaderSource, ImVec2(centerW - 22.0f, bottomH - 76.0f));
            if (fontMono) ImGui::PopFont();
        } else if (m_bottomTab == 6) {
            // ==================== AUTONOMOUS AI AGENT CHAT ====================
            if (m_aiConfig.apiKey.empty() && m_aiConfig.provider != "custom") {
                ImGui::TextColored(ImVec4(0.96f, 0.76f, 0.28f, 1.f),
                                   "⚠ Configure sua API Key (OpenRouter, OpenAI, Claude, Groq, Gemini...) para o Agente programar o jogo C# sozinho.");
                ImGui::SameLine();
                if (ImGui::SmallButton("Configurar API")) m_settingsOpen = true;
            }

            ImGui::BeginChild("##ia_chat_scroll", ImVec2(0, bottomH - (m_aiConfig.apiKey.empty() ? 98.0f : 76.0f)), false);
            if (m_chatMsgs.empty()) {
                ImGui::TextColored(ImVec4(0.75f, 0.78f, 0.86f, 1.f),
                                   "Agente IA Autônomo (%s · %s) — peça o jogo inteiro em C# e ele grava os arquivos no projeto sem sua ajuda:",
                                   m_aiConfig.provider.c_str(), m_aiConfig.model.c_str());
                const char* chips[] = {
                    "Faça um jogo de plataforma completo em C# com moedas e inimigos",
                    "Crie um jogo espacial shooter em C# no GameWorld.cs",
                    "Monte um cenário Obby 3D e gere os scripts C#"
                };
                for (const char* ch : chips) {
                    if (UIWidgets::OutlineButton(ch, ImVec2(0, 26.0f), 13.0f) && !m_agent.isRunning()) {
                        submitAgentPrompt(ch);
                    }
                    ImGui::SameLine(0, 6.0f);
                }
                ImGui::NewLine();
            } else {
                for (const auto& cm : m_chatMsgs) {
                    if (cm.isUser) {
                        ImGui::TextColored(ImVec4(0.45f, 0.72f, 1.0f, 1.f), "Você: %s", cm.text.c_str());
                    } else {
                        if (!cm.error.empty()) {
                            ImGui::TextColored(ImVec4(0.96f, 0.38f, 0.40f, 1.f), "Agente IA [Erro]: %s", cm.error.c_str());
                        }
                        if (!cm.text.empty()) {
                            ImGui::TextWrapped("Agente IA: %s", cm.text.c_str());
                        }
                        for (const auto& tl : cm.tools) {
                            ImGui::TextColored(ImVec4(0.35f, 0.88f, 0.52f, 1.f), "   ✓ %s  %s", tl.first.c_str(), tl.second.c_str());
                        }
                        if (cm.pending && !cm.status.empty()) {
                            ImGui::TextColored(ImVec4(0.65f, 0.70f, 0.80f, 1.f), "   ⏳ %s", cm.status.c_str());
                        }
                    }
                    ImGui::Spacing();
                }
            }
            ImGui::EndChild();

            ImGui::SetNextItemWidth(centerW - 210.0f);
            bool sendNow = ImGui::InputTextWithHint(
                "##agent_prompt_in",
                "Peça o jogo completo em C#... (qualquer API até OpenRouter)",
                m_agentInput, sizeof(m_agentInput),
                ImGuiInputTextFlags_EnterReturnsTrue
            );
            ImGui::SameLine();
            if ((UIWidgets::WhitePillButton(m_agent.isRunning() ? "Trabalhando..." : "Enviar ao Agente", ImVec2(142.0f, 28.0f), 8.0f) || sendNow) &&
                !m_agent.isRunning() && m_agentInput[0] != '\0') {
                std::string q = m_agentInput;
                m_agentInput[0] = '\0';
                submitAgentPrompt(q);
            }
            ImGui::SameLine();
            if (UIWidgets::OutlineButton("⚙", ImVec2(36.0f, 28.0f), 8.0f)) {
                m_settingsOpen = true;
            }
        } else {
            ImGui::TextColored(ImVec4(0.62f, 0.64f, 0.70f, 1.f),
                               "Painel pronto — execute o projeto C# ou inspecione os recursos ativos.");
        }

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    // ---------- RIGHT COLUMN: Propriedades + Caixa de Ferramentas ----------
    if (rightW > 0.0f) {
        const float rightX = w - rightW - 4.0f;
        const float propH  = (m_showProperties && m_showToolbox) ? (bodyH * 0.68f - 3.0f) : bodyH;

        if (m_showProperties) {
            ImGui::SetNextWindowPos(ImVec2(rightX, bodyY));
            ImGui::SetNextWindowSize(ImVec2(rightW, propH));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(17, 19, 24, 255));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
            ImGui::BeginChild("##PropertiesPanel", ImVec2(rightW, propH), true);

            ImGui::TextUnformatted("📋  Propriedades");
            ImGui::SameLine(rightW - 28.0f);
            if (ImGui::SmallButton("×##close_props")) m_showProperties = false;

            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##props_search", "🔍 Pesquisar propriedades...", m_propsSearch, sizeof(m_propsSearch));
            ImGui::Spacing();

            if (SceneObject* sel = m_scene.selected()) {
                ImGui::Text("Parte: %s", sel->name.c_str());
                ImGui::Separator();

                if (ImGui::CollapsingHeader("⚙ Transformação", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::DragFloat3("Posição", &sel->position.x, 0.05f, -500.f, 500.f, "%.3f");
                    ImGui::DragFloat3("Rotação", &sel->rotation.x, 0.5f, -360.f, 360.f, "%.3f");
                    ImGui::DragFloat3("Escala",  &sel->scale.x,    0.05f, 0.05f, 200.f, "%.3f");
                }

                if (ImGui::CollapsingHeader("Aparência", ImGuiTreeNodeFlags_DefaultOpen)) {
                    AstronVec4 rgba = { 1.f, 1.f, 1.f, 1.f };
                    astron_hex_to_rgba(sel->colorHex.c_str(), &rgba);
                    float col3[3] = { rgba.x, rgba.y, rgba.z };
                    if (ImGui::ColorEdit3("Cor", col3)) {
                        char hex[8] = {};
                        astron_rgba_to_hex({ col3[0], col3[1], col3[2], 1.f }, hex);
                        sel->colorHex = hex;
                    }

                    const char* mats[] = { "Plastic", "Metal", "Wood", "Glass", "Neon", "Matte" };
                    int matIdx = static_cast<int>(sel->material);
                    if (ImGui::Combo("Material", &matIdx, mats, 6)) {
                        sel->material = static_cast<MaterialKind>(matIdx);
                    }

                    ImGui::SliderFloat("Transparência", &sel->transparency, 0.0f, 0.95f, "%.3f");
                    ImGui::TextUnformatted("Visibilidade");
                    ImGui::SameLine(rightW - 60.0f);
                    UIWidgets::ToggleSwitch("##prop_vis_sw", &sel->visible, true);
                }

                if (ImGui::CollapsingHeader("Física", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::DragFloat("Massa", &sel->mass, 0.05f, 0.01f, 1000.f, "%.3f");
                    ImGui::TextUnformatted("Gravidade");
                    ImGui::SameLine(rightW - 60.0f);
                    UIWidgets::ToggleSwitch("##prop_grav_sw", &sel->gravity, true);

                    const char* cols[] = { "Ativado", "Desativado", "Trigger" };
                    int colIdx = (sel->collision == "Desativado") ? 1 : (sel->collision == "Trigger" ? 2 : 0);
                    if (ImGui::Combo("Colisão", &colIdx, cols, 3)) {
                        sel->collision = cols[colIdx];
                    }
                }

                if (ImGui::CollapsingHeader("Renderização")) {
                    ImGui::Checkbox("Modo Wireframe Global", &m_viewport.wireframe);
                }
                if (ImGui::CollapsingHeader("Outras Configurações")) {
                    ImGui::Text("Script C#: %s", sel->scriptPath.c_str());
                    if (ImGui::SmallButton("Abrir Script no Editor")) openFileTab(sel->scriptPath);
                }
            } else {
                ImGui::TextDisabled("Nenhum objeto selecionado.");
            }

            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();
        }

        if (m_showToolbox) {
            const float tbPanelY = m_showProperties ? (bodyY + propH + 6.0f) : bodyY;
            const float tbPanelH = m_showProperties ? (bodyH - propH - 6.0f) : bodyH;

            ImGui::SetNextWindowPos(ImVec2(rightX, tbPanelY));
            ImGui::SetNextWindowSize(ImVec2(rightW, tbPanelH));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(17, 19, 24, 255));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
            ImGui::BeginChild("##ToolboxPanel", ImVec2(rightW, tbPanelH), true);

            ImGui::TextUnformatted("🧰  Caixa de Ferramentas");
            ImGui::SameLine(rightW - 28.0f);
            if (ImGui::SmallButton("×##close_tbx")) m_showToolbox = false;

            const char* tbxTabs[3] = { "Modelos", "Recursos", "Materiais" };
            for (int t = 0; t < 3; ++t) {
                ImGui::PushStyleColor(ImGuiCol_Button, m_toolboxTab == t ? IM_COL32(28, 78, 168, 255) : IM_COL32(24, 26, 32, 255));
                if (ImGui::Button(tbxTabs[t], ImVec2(84.0f, 24.0f))) m_toolboxTab = t;
                ImGui::PopStyleColor();
                if (t < 2) ImGui::SameLine(0, 6.0f);
            }
            ImGui::Spacing();

            if (m_toolboxTab == 0) {
                struct ShapeCard { const char* name; MeshKind kind; };
                const ShapeCard shapes[4] = {
                    { "Part",     MeshKind::Cube },
                    { "Sphere",   MeshKind::Sphere },
                    { "Cylinder", MeshKind::Cylinder },
                    { "Block",    MeshKind::Block }
                };
                const float itemW = (rightW - 38.0f) / 4.0f;
                for (int i = 0; i < 4; ++i) {
                    ImVec2 p = ImGui::GetCursorScreenPos();
                    ImDrawList* cdl = ImGui::GetWindowDrawList();
                    cdl->AddRectFilled(p, ImVec2(p.x + itemW, p.y + 78.0f), IM_COL32(24, 26, 33, 255), 8.0f);
                    UIWidgets::DrawToolboxShape(cdl, ImVec2(p.x + itemW * 0.5f, p.y + 32.0f), 40.0f, shapes[i].kind);
                    ImVec2 ns = ImGui::CalcTextSize(shapes[i].name);
                    cdl->AddText(ImVec2(p.x + (itemW - ns.x) * 0.5f, p.y + 57.0f), IM_COL32(215, 218, 226, 255), shapes[i].name);

                    std::string sid = std::string("##spawn_shape_") + shapes[i].name;
                    if (ImGui::InvisibleButton(sid.c_str(), ImVec2(itemW, 78.0f))) {
                        float rx = static_cast<float>((m_scene.objects.size() % 5)) * 1.4f - 2.0f;
                        m_scene.addObject(shapes[i].name, shapes[i].kind, { rx, 0.5f, 0.0f });
                        pushLog("INFO", std::string("Objeto 3D adicionado: ") + shapes[i].name);
                    }
                    if (i < 3) ImGui::SameLine(0, 6.0f);
                }
            } else if (m_toolboxTab == 1) {
                if (ImGui::Button("Player.cs (Script C#)", ImVec2(-1, 28))) openFileTab("res://scripts/Player.cs");
                if (ImGui::Button("GameWorld.cs (Jogo C#)", ImVec2(-1, 28))) openFileTab("res://scripts/GameWorld.cs");
                if (ImGui::Button("surface.glsl (Shader GLSL)", ImVec2(-1, 28))) { m_bottomTab = 5; }
            } else {
                const char* mats[] = { "Plastic", "Metal", "Wood", "Glass", "Neon", "Matte" };
                for (int m = 0; m < 6; ++m) {
                    if (ImGui::SmallButton(mats[m])) {
                        if (auto* sel = m_scene.selected()) sel->material = static_cast<MaterialKind>(m);
                    }
                    if (m % 3 != 2) ImGui::SameLine();
                }
            }

            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor();
        }
    }

    // ==================== 4. STATUS BAR ====================
    const float sbY = h - statusH;
    dl->AddRectFilled(ImVec2(0, sbY), ImVec2(w, h), IM_COL32(11, 12, 15, 255));
    dl->AddLine(ImVec2(0, sbY), ImVec2(w, sbY), IM_COL32(28, 30, 36, 255));

    dl->AddCircleFilled(ImVec2(14.0f, sbY + statusH * 0.5f), 4.5f,
                        m_agent.isRunning() ? IM_COL32(58, 138, 255, 255) : IM_COL32(47, 209, 88, 255));
    dl->AddText(ImVec2(26.0f, sbY + 5.0f), IM_COL32(210, 212, 220, 255),
                m_agent.isRunning() ? "Agente IA escrevendo código C#..." : (m_playing ? "Em execução" : "Pronto"));

    int fps = static_cast<int>(std::round(io.Framerate > 1.0f ? io.Framerate : 144.0f));
    uint32_t memMb = astron_process_memory_mb();
    char rightStat[160];
    std::snprintf(rightStat, sizeof(rightStat), "FPS: %d  |  Memória: %u MB  |  Versão: 0.1.0", fps, memMb);
    ImVec2 rsSz = ImGui::CalcTextSize(rightStat);
    dl->AddText(ImVec2(w - rsSz.x - 14.0f, sbY + 5.0f), IM_COL32(168, 170, 180, 255), rightStat);

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

} // namespace astron
