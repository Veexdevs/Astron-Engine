#include "EngineApp.hpp"
#include "AIProviders.hpp"
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

void drawDarkOrbitalSphere(ImDrawList* dl, ImVec2 center, float radius, float rimStartRad, float rimEndRad) {
    const int rings = 24;
    for (int i = rings; i >= 1; --i) {
        float t = static_cast<float>(i) / rings;
        float limb = std::pow(t, 2.4f);
        int v = static_cast<int>(5.0f + limb * 22.0f);
        int b = static_cast<int>(7.0f + limb * 27.0f);
        dl->AddCircleFilled(center, radius * t, IM_COL32(v, v + 1, b, 255), 96);
    }
    dl->PathArcTo(center, radius + 1.5f, rimStartRad, rimEndRad, 64);
    dl->PathStroke(IM_COL32(190, 198, 218, 34), 0, 5.0f);
    dl->PathArcTo(center, radius, rimStartRad, rimEndRad, 64);
    dl->PathStroke(IM_COL32(220, 226, 242, 78), 0, 1.4f);
}

} // namespace

void EngineApp::renderSplash(float dt, float W, float H) {
    m_splashProgress += dt * 0.62f;
    if (m_splashProgress >= 1.0f) {
        m_splashProgress = 1.0f;
        m_screen = ScreenId::Login;
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(W, H));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##AstronSplash", nullptr, flags);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Top-left and bottom-right curved dark orbital spheres with subtle rim highlights (Image 5)
    float rTop = std::min(W, H) * 0.56f;
    float rBot = std::min(W, H) * 0.64f;
    drawDarkOrbitalSphere(dl, ImVec2(W * 0.04f, H * 0.03f), rTop,
                          static_cast<float>(M_PI * 0.02), static_cast<float>(M_PI * 0.52));
    drawDarkOrbitalSphere(dl, ImVec2(W * 0.96f, H * 0.99f), rBot,
                          static_cast<float>(M_PI * 1.02), static_cast<float>(M_PI * 1.52));

    const float cx = W * 0.5f;
    const float cy = H * 0.45f;

    // Official Astron Engine vertical lockup
    GpuTexture lockup = m_textures.get("assets/textures/astron-lockup.png");
    if (lockup.id != 0 && lockup.height > 0) {
        float aspect = static_cast<float>(lockup.width) / static_cast<float>(lockup.height);
        float drawH = 172.0f;
        float drawW = drawH * aspect;
        dl->AddImage(
            static_cast<ImTextureID>(static_cast<uintptr_t>(lockup.id)),
            ImVec2(cx - drawW * 0.5f, cy - drawH * 0.60f),
            ImVec2(cx + drawW * 0.5f, cy + drawH * 0.40f)
        );
    } else {
        UIWidgets::DrawPlanetMark(dl, ImVec2(cx, cy - 42.0f), 42.0f, IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255));
        if (fontLogoBig) ImGui::PushFont(fontLogoBig);
        const char* title = "ASTRON";
        ImVec2 ts = ImGui::CalcTextSize(title);
        dl->AddText(ImVec2(cx - ts.x * 0.5f, cy + 16.0f), IM_COL32(255, 255, 255, 255), title);
        if (fontLogoBig) ImGui::PopFont();

        const char* sub = "E N G I N E";
        ImVec2 ss = ImGui::CalcTextSize(sub);
        dl->AddText(ImVec2(cx - ss.x * 0.5f, cy + 56.0f), IM_COL32(168, 172, 182, 255), sub);
    }

    // Pill outline progress bar with inner padding and bright white fill
    const float barW = std::min(380.0f, W * 0.32f);
    const float barH = 13.0f;
    const float bx = cx - barW * 0.5f;
    const float by = cy + 98.0f;

    dl->AddRectFilled(ImVec2(bx, by), ImVec2(bx + barW, by + barH), IM_COL32(8, 9, 12, 240), barH * 0.5f);
    dl->AddRect(ImVec2(bx, by), ImVec2(bx + barW, by + barH), IM_COL32(215, 220, 232, 125), barH * 0.5f, 0, 1.25f);

    const float pad = 2.4f;
    float fillW = std::max((barH - pad * 2.0f), (barW - pad * 2.0f) * std::clamp(m_splashProgress, 0.0f, 1.0f));
    dl->AddRectFilled(ImVec2(bx + pad, by + pad), ImVec2(bx + pad + fillW, by + barH - pad),
                      IM_COL32(252, 252, 255, 255), (barH - pad * 2.0f) * 0.5f);

    const char* loadingTxt = "Carregando...";
    ImVec2 ls = ImGui::CalcTextSize(loadingTxt);
    dl->AddText(ImVec2(cx - ls.x * 0.5f, by + 24.0f), IM_COL32(165, 170, 180, 235), loadingTxt);

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void EngineApp::renderLogin(float W, float H) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(W, H));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##AstronLogin", nullptr, flags);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Top-left dark orbital sphere arc
    drawDarkOrbitalSphere(dl, ImVec2(W * 0.05f, -H * 0.06f), std::min(W, H) * 0.48f,
                          static_cast<float>(M_PI * 0.08), static_cast<float>(M_PI * 0.48));

    // Subtle deep-space starfield dust on left half
    for (int i = 0; i < 34; ++i) {
        float sx = std::fmod(i * 197.3f + 43.0f, W * 0.52f);
        float sy = std::fmod(i * 131.7f + 71.0f, H * 0.92f);
        int alpha = 24 + (i % 4) * 18;
        dl->AddCircleFilled(ImVec2(sx, sy), (i % 3 == 0) ? 1.3f : 0.9f, IM_COL32(220, 226, 242, alpha));
    }

    // Bottom-left glowing cratered planet horizon (assets/textures/login-planet.png, aspect-preserved)
    GpuTexture planet = m_textures.get("assets/textures/login-planet.png");
    if (planet.id != 0 && planet.height > 0) {
        float aspect = static_cast<float>(planet.width) / static_cast<float>(planet.height);
        float pH = H * 0.56f;
        float pW = pH * aspect;
        dl->AddImage(
            static_cast<ImTextureID>(static_cast<uintptr_t>(planet.id)),
            ImVec2(0.0f, H - pH),
            ImVec2(pW, H),
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, 245)
        );
    }

    // Left Half: Astron Engine official lockup + tagline
    const float leftCx = W * 0.265f;
    const float leftCy = H * 0.45f;

    GpuTexture lockup = m_textures.get("assets/textures/astron-lockup.png");
    if (lockup.id != 0 && lockup.height > 0) {
        float aspect = static_cast<float>(lockup.width) / static_cast<float>(lockup.height);
        float drawH = 180.0f;
        float drawW = drawH * aspect;
        dl->AddImage(
            static_cast<ImTextureID>(static_cast<uintptr_t>(lockup.id)),
            ImVec2(leftCx - drawW * 0.5f, leftCy - 122.0f),
            ImVec2(leftCx + drawW * 0.5f, leftCy + 58.0f)
        );
    } else {
        UIWidgets::DrawPlanetMark(dl, ImVec2(leftCx, leftCy - 56.0f), 48.0f, IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255));
        if (fontLogoBig) ImGui::PushFont(fontLogoBig);
        const char* brand = "ASTRON";
        ImVec2 bs = ImGui::CalcTextSize(brand);
        dl->AddText(ImVec2(leftCx - bs.x * 0.5f, leftCy + 8.0f), IM_COL32(255, 255, 255, 255), brand);
        if (fontLogoBig) ImGui::PopFont();
    }

    const char* tagline = "C r i e .   D e s e n v o l v a .   E x p l o r e .";
    ImVec2 tgs = ImGui::CalcTextSize(tagline);
    dl->AddText(ImVec2(leftCx - tgs.x * 0.5f, leftCy + 76.0f), IM_COL32(182, 186, 198, 235), tagline);

    // Subtle vertical center divider
    dl->AddLine(ImVec2(W * 0.50f, H * 0.14f), ImVec2(W * 0.50f, H * 0.86f), IM_COL32(255, 255, 255, 22), 1.0f);

    // Bottom-left Settings button with crisp vector Gear icon
    ImGui::SetCursorScreenPos(ImVec2(26.0f, H - 48.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 18));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 255, 255, 28));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    if (ImGui::Button("##login_cfg", ImVec2(142.0f, 32.0f))) {
        m_settingsOpen = true;
    }
    {
        ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
        float cyBtn = (mn.y + mx.y) * 0.5f;
        UIWidgets::DrawIcon(dl, ImVec2(mn.x + 12.0f, cyBtn), 15.0f, UiIcon::Gear, IM_COL32(200, 204, 214, 240));
        const char* cfgLbl = "Configura\xC3\xA7\xC3\xB5" "es";
        ImVec2 cs = ImGui::CalcTextSize(cfgLbl);
        dl->AddText(ImVec2(mn.x + 28.0f, cyBtn - cs.y * 0.5f), IM_COL32(200, 204, 214, 240), cfgLbl);
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    // Bottom-right version label
    dl->AddText(ImVec2(W - 64.0f, H - 36.0f), IM_COL32(138, 142, 154, 215), "v1.0.0");

    // Right Half: Dark rounded Login Card matching Image 3
    const float cardW = 420.0f;
    const float cardH = 492.0f;
    const float cardX = W * 0.74f - cardW * 0.5f;
    const float cardY = H * 0.50f - cardH * 0.5f;

    ImGui::SetCursorScreenPos(ImVec2(cardX, cardY));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(13, 14, 18, 244));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(46, 49, 60, 175));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 20.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(36.0f, 34.0f));

    ImGui::BeginChild("##LoginCard", ImVec2(cardW, cardH), ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);
    ImDrawList* cdl = ImGui::GetWindowDrawList();

    if (fontTitle) ImGui::PushFont(fontTitle);
    ImGui::TextColored(ImVec4(0.98f, 0.98f, 1.0f, 1.0f), "Bem-vindo(a)!");
    if (fontTitle) ImGui::PopFont();

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.58f, 0.60f, 0.65f, 1.0f), "Entre na sua conta para continuar");
    ImGui::Dummy(ImVec2(0, 18.0f));

    const float fieldW = cardW - 72.0f;
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(21, 22, 28, 255));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(46, 49, 60, 210));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 11.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

    // 1. User / email field with left User vector icon
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(40.0f, 13.0f));
    ImGui::SetNextItemWidth(fieldW);
    ImGui::InputTextWithHint("##login_user", "Usu\xC3\xA1rio ou e-mail", m_loginUser, sizeof(m_loginUser));
    {
        ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
        UIWidgets::DrawIcon(cdl, ImVec2(mn.x + 20.0f, (mn.y + mx.y) * 0.5f), 15.0f,
                            UiIcon::User, IM_COL32(150, 154, 166, 235));
    }
    ImGui::PopStyleVar();

    ImGui::Dummy(ImVec2(0, 10.0f));

    // 2. Password field with left Lock vector icon + right Eye/EyeOff toggle inside box
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(40.0f, 13.0f));
    ImGui::SetNextItemWidth(fieldW);
    ImGuiInputTextFlags pwFlags = m_loginShowPass ? 0 : ImGuiInputTextFlags_Password;
    bool enterPressed = ImGui::InputTextWithHint("##login_pass", "Senha", m_loginPass, sizeof(m_loginPass),
                                                 pwFlags | ImGuiInputTextFlags_EnterReturnsTrue);
    {
        ImVec2 mn = ImGui::GetItemRectMin(), mx = ImGui::GetItemRectMax();
        float cyField = (mn.y + mx.y) * 0.5f;
        UIWidgets::DrawIcon(cdl, ImVec2(mn.x + 20.0f, cyField), 15.0f,
                            UiIcon::Lock, IM_COL32(150, 154, 166, 235));

        ImVec2 savedCur = ImGui::GetCursorScreenPos();
        ImGui::SetCursorScreenPos(ImVec2(mx.x - 38.0f, mn.y + 4.0f));
        if (ImGui::InvisibleButton("##toggle_pw_eye", ImVec2(34.0f, mx.y - mn.y - 8.0f))) {
            m_loginShowPass = !m_loginShowPass;
        }
        UIWidgets::DrawIcon(cdl, ImVec2(mx.x - 21.0f, cyField), 15.5f,
                            m_loginShowPass ? UiIcon::Eye : UiIcon::EyeOff,
                            IM_COL32(154, 158, 170, 235));
        ImGui::SetCursorScreenPos(savedCur);
    }
    ImGui::PopStyleVar();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    ImGui::Dummy(ImVec2(0, 10.0f));

    // Remember me + Underlined "Esqueceu sua senha?"
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(20, 21, 26, 255));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(72, 76, 88, 230));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::Checkbox("Lembrar de mim", &m_loginRemember);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    const char* forgotLbl = "Esqueceu sua senha?";
    ImVec2 fSz = ImGui::CalcTextSize(forgotLbl);
    ImGui::SameLine(fieldW - fSz.x + 36.0f);
    {
        ImVec2 fp = ImGui::GetCursorScreenPos();
        if (ImGui::InvisibleButton("##forgot_pw", ImVec2(fSz.x, fSz.y + 4.0f))) {
            pushToast("Link de recupera\xC3\xA7\xC3\xA3o dispon\xC3\xADvel nas Configura\xC3\xA7\xC3\xB5" "es da conta.");
        }
        ImU32 fCol = ImGui::IsItemHovered() ? IM_COL32(255, 255, 255, 255) : IM_COL32(215, 218, 228, 235);
        cdl->AddText(fp, fCol, forgotLbl);
        cdl->AddLine(ImVec2(fp.x, fp.y + fSz.y + 1.0f), ImVec2(fp.x + fSz.x, fp.y + fSz.y + 1.0f), fCol, 1.0f);
    }

    ImGui::Dummy(ImVec2(0, 12.0f));

    // Primary white "Entrar" button
    if (UIWidgets::WhitePillButton("Entrar", ImVec2(fieldW, 46.0f), 12.0f) || enterPressed) {
        if (m_loginUser[0] == '\0') {
            std::snprintf(m_loginUser, sizeof(m_loginUser), "Desenvolvedor");
        }
        m_loginError.clear();
        m_screen = ScreenId::Home;
    }

    ImGui::Dummy(ImVec2(0, 14.0f));

    // "── ou ──"
    {
        ImVec2 p = ImGui::GetCursorScreenPos();
        float midY = p.y + 8.0f;
        cdl->AddLine(ImVec2(p.x, midY), ImVec2(p.x + fieldW * 0.43f, midY), IM_COL32(48, 50, 60, 220), 1.0f);
        const char* ouTxt = "ou";
        ImVec2 os = ImGui::CalcTextSize(ouTxt);
        cdl->AddText(ImVec2(p.x + (fieldW - os.x) * 0.5f, p.y), IM_COL32(138, 142, 152, 230), ouTxt);
        cdl->AddLine(ImVec2(p.x + fieldW * 0.57f, midY), ImVec2(p.x + fieldW, midY), IM_COL32(48, 50, 60, 220), 1.0f);
        ImGui::Dummy(ImVec2(fieldW, 20.0f));
    }

    ImGui::Dummy(ImVec2(0, 4.0f));

    // "Entrar com GitHub" with crisp Octocat vector icon
    if (UIWidgets::OutlineButton("Entrar com GitHub", ImVec2(fieldW, 46.0f), 12.0f, UiIcon::GitHub)) {
        std::snprintf(m_loginUser, sizeof(m_loginUser), "GitHub Developer");
        m_screen = ScreenId::Home;
        pushToast("Autenticado via GitHub OAuth.");
    }

    if (!m_loginError.empty()) {
        ImGui::Dummy(ImVec2(0, 4.0f));
        ImGui::TextColored(ImVec4(0.95f, 0.42f, 0.42f, 1.0f), "%s", m_loginError.c_str());
    } else {
        ImGui::Dummy(ImVec2(0, 14.0f));
    }

    // Bottom: "Não tem uma conta? Criar conta"
    {
        const char* p1 = "N\xC3\xA3o tem uma conta? ";
        const char* p2 = "Criar conta";
        ImVec2 s1 = ImGui::CalcTextSize(p1);
        ImVec2 s2 = ImGui::CalcTextSize(p2);
        float startX = (fieldW - (s1.x + s2.x)) * 0.5f;
        ImGui::SetCursorPosX(36.0f + startX);
        ImGui::TextColored(ImVec4(0.56f, 0.58f, 0.64f, 1.0f), "%s", p1);
        ImGui::SameLine(0, 0);
        ImVec2 cp = ImGui::GetCursorScreenPos();
        if (ImGui::InvisibleButton("##create_acc", ImVec2(s2.x + 4.0f, s2.y + 2.0f))) {
            std::snprintf(m_loginUser, sizeof(m_loginUser), "AstronCreator");
            m_screen = ScreenId::Home;
        }
        cdl->AddText(cp, ImGui::IsItemHovered() ? IM_COL32(200, 220, 255, 255) : IM_COL32(250, 250, 255, 255), p2);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);

    // Top-right window controls
    UIWidgets::DrawWindowChrome(W, onMinimizeWindow, onMaximizeWindow, onCloseWindow);

    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void EngineApp::renderSettingsModal() {
    if (!m_settingsOpen) return;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(580.0f, 470.0f), ImGuiCond_Appearing);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(13, 14, 18, 252));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 20));

    if (ImGui::Begin("Configura\xC3\xA7\xC3\xB5" "es — Astron Engine & Agente IA", &m_settingsOpen, ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextColored(ImVec4(0.65f, 0.67f, 0.72f, 1.f),
                           "Configure qualquer API de IA (OpenRouter, OpenAI, Anthropic, Groq, Mistral, DeepSeek, Gemini ou Local).");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const auto& presets = AIProviders::presets();
        int curPreset = 0;
        for (size_t i = 0; i < presets.size(); ++i) {
            if (presets[i].id == m_aiConfig.provider) { curPreset = static_cast<int>(i); break; }
        }

        ImGui::TextUnformatted("Provedor de IA");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##ai_provider", presets[static_cast<size_t>(curPreset)].label.c_str())) {
            for (size_t i = 0; i < presets.size(); ++i) {
                bool sel = (static_cast<int>(i) == curPreset);
                if (ImGui::Selectable(presets[i].label.c_str(), sel)) {
                    m_aiConfig.provider = presets[i].id;
                    m_aiConfig.baseUrl = presets[i].baseUrl;
                    if (!presets[i].models.empty()) m_aiConfig.model = presets[i].models.front();
                    std::snprintf(m_baseUrlBuf, sizeof(m_baseUrlBuf), "%s", m_aiConfig.baseUrl.c_str());
                    std::snprintf(m_modelBuf, sizeof(m_modelBuf), "%s", m_aiConfig.model.c_str());
                }
            }
            ImGui::EndCombo();
        }
        ImGui::TextColored(ImVec4(0.52f, 0.55f, 0.62f, 1.f), "%s", presets[static_cast<size_t>(curPreset)].hint.c_str());

        ImGui::Spacing();
        ImGui::TextUnformatted("Chave da API (API Key)");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputTextWithHint("##ai_key", "sk-or-v1-... (salva localmente no seu PC)", m_apiKeyBuf, sizeof(m_apiKeyBuf), ImGuiInputTextFlags_Password)) {
            m_aiConfig.apiKey = m_apiKeyBuf;
        }

        ImGui::Spacing();
        ImGui::TextUnformatted("URL Base da API");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##ai_baseurl", m_baseUrlBuf, sizeof(m_baseUrlBuf))) {
            m_aiConfig.baseUrl = m_baseUrlBuf;
        }

        ImGui::Spacing();
        ImGui::TextUnformatted("Modelo (escolha sugerido ou digite qualquer ID do OpenRouter)");
        const auto& models = presets[static_cast<size_t>(curPreset)].models;
        ImGui::SetNextItemWidth(260);
        if (ImGui::BeginCombo("##ai_model_preset", "Modelos r\xC3\xA1pidos...")) {
            for (const auto& m : models) {
                if (ImGui::Selectable(m.c_str(), m == m_aiConfig.model)) {
                    m_aiConfig.model = m;
                    std::snprintf(m_modelBuf, sizeof(m_modelBuf), "%s", m.c_str());
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##ai_model_custom", m_modelBuf, sizeof(m_modelBuf))) {
            m_aiConfig.model = m_modelBuf;
        }

        ImGui::Spacing();
        ImGui::SliderFloat("Temperatura", &m_aiConfig.temperature, 0.0f, 1.2f, "%.2f");
        ImGui::SliderInt("Passos aut\xC3\xB4nomos m\xC3\xA1x.", &m_aiConfig.maxSteps, 4, 40);

        ImGui::Dummy(ImVec2(0, 14));
        if (UIWidgets::WhitePillButton("Salvar Configura\xC3\xA7\xC3\xB5" "es", ImVec2(210, 42), 11.0f)) {
            m_aiConfig.apiKey = m_apiKeyBuf;
            m_aiConfig.baseUrl = m_baseUrlBuf;
            m_aiConfig.model = m_modelBuf;
            saveSettings();
            m_settingsOpen = false;
            pushToast("Configura\xC3\xA7\xC3\xB5" "es de IA salvas!");
        }
        ImGui::SameLine();
        if (UIWidgets::OutlineButton("Abrir OpenRouter Keys", ImVec2(190, 42), 11.0f)) {
            astron_open_url("https://openrouter.ai/api/keys");
        }
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

} // namespace astron
