#include "EngineApp.hpp"
#include "UIWidgets.hpp"
#include "../core_c/astron_platform.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace astron {

void EngineApp::renderSplash(float dt, float w, float h) {
    m_splashProgress += dt * 36.0f;
    if (m_splashProgress >= 100.0f) {
        m_splashProgress = 100.0f;
        m_screen = ScreenId::Login;
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 255));
    ImGui::Begin("##Splash", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Subtle dark atmospheric orbs matching Splash mockup
    dl->AddCircleFilled(ImVec2(-80.0f, -90.0f), 390.0f, IM_COL32(14, 14, 18, 255), 64);
    dl->AddCircle(ImVec2(-80.0f, -90.0f), 390.0f, IM_COL32(34, 34, 40, 110), 64, 2.0f);
    dl->AddCircleFilled(ImVec2(w + 110.0f, h + 130.0f), 440.0f, IM_COL32(15, 15, 19, 255), 64);
    dl->AddCircle(ImVec2(w + 110.0f, h + 130.0f), 440.0f, IM_COL32(36, 36, 42, 110), 64, 2.0f);

    UIWidgets::DrawWindowChrome(w, onMinimizeWindow, onMaximizeWindow, onCloseWindow);

    const float cx = w * 0.5f;
    const float cy = h * 0.5f;

    // Official Astron Engine Saturn-ring planet + wordmark lockup texture (with procedural fallback)
    GpuTexture lockupTex = m_textures.get("assets/textures/astron-lockup.png");
    if (lockupTex.id != 0) {
        const float lw = 380.0f;
        const float lh = lw * (static_cast<float>(lockupTex.height) / std::max(1, lockupTex.width));
        dl->AddImage(static_cast<ImTextureID>(static_cast<uintptr_t>(lockupTex.id)),
                     ImVec2(cx - lw * 0.5f, cy - lh + 68.0f),
                     ImVec2(cx + lw * 0.5f, cy + 68.0f));
    } else {
        UIWidgets::DrawPlanetMark(dl, ImVec2(cx, cy - 118.0f), 78.0f, IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255));
        if (fontLogoBig) ImGui::PushFont(fontLogoBig);
        const char* wordA = "Astron";
        ImVec2 szA = ImGui::CalcTextSize(wordA);
        dl->AddText(ImVec2(cx - szA.x * 0.5f, cy - 22.0f), IM_COL32(255, 255, 255, 255), wordA);
        if (fontLogoBig) ImGui::PopFont();

        if (fontLogoMed) ImGui::PushFont(fontLogoMed);
        const char* wordE = "E n g i n e";
        ImVec2 szE = ImGui::CalcTextSize(wordE);
        dl->AddText(ImVec2(cx - szE.x * 0.5f, cy + 44.0f), IM_COL32(236, 236, 240, 255), wordE);
        if (fontLogoMed) ImGui::PopFont();
    }

    // Progress pill bar
    const float barW = std::min(620.0f, w * 0.62f);
    const float barH = 22.0f;
    ImVec2 bMin(cx - barW * 0.5f, cy + 104.0f);
    ImVec2 bMax(cx + barW * 0.5f, cy + 104.0f + barH);
    dl->AddRect(bMin, bMax, IM_COL32(92, 92, 100, 255), 11.0f, 0, 1.6f);

    float fillFrac = std::clamp(m_splashProgress / 100.0f, 0.02f, 1.0f);
    ImVec2 fMin(bMin.x + 3.5f, bMin.y + 3.5f);
    ImVec2 fMax(bMin.x + 3.5f + (barW - 7.0f) * fillFrac, bMax.y - 3.5f);
    dl->AddRectFilled(fMin, fMax, IM_COL32(255, 255, 255, 255), 9.0f);

    const char* loadingTxt = "Carregando...";
    ImVec2 szL = ImGui::CalcTextSize(loadingTxt);
    dl->AddText(ImVec2(cx - szL.x * 0.5f, bMax.y + 18.0f), IM_COL32(220, 220, 226, 255), loadingTxt);

    ImGui::End();
    ImGui::PopStyleColor();
}

void EngineApp::renderLogin(float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(0, 0, 0, 255));
    ImGui::Begin("##Login", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Top-left dark orb + bottom-left planet horizon texture
    dl->AddCircleFilled(ImVec2(-60.0f, -80.0f), 280.0f, IM_COL32(16, 16, 20, 255), 64);
    GpuTexture planetTex = m_textures.get("assets/textures/login-planet.png");
    if (planetTex.id != 0) {
        float pw = w * 0.46f;
        float ph = pw * (static_cast<float>(planetTex.height) / std::max(1, planetTex.width));
        dl->AddImage(static_cast<ImTextureID>(static_cast<uintptr_t>(planetTex.id)),
                     ImVec2(-pw * 0.12f, h - ph * 0.78f),
                     ImVec2(pw * 0.88f, h + ph * 0.22f));
    }

    // Center vertical divider
    dl->AddLine(ImVec2(w * 0.5f, 88.0f), ImVec2(w * 0.5f, h - 48.0f), IM_COL32(42, 42, 48, 255), 1.0f);

    UIWidgets::DrawWindowChrome(w, onMinimizeWindow, onMaximizeWindow, [this]() { m_screen = ScreenId::Splash; m_splashProgress = 0.0f; });

    // Left Half: Official Astron Engine Brand Lockup + Tagline
    const float lx = w * 0.25f;
    const float ly = h * 0.46f;
    GpuTexture loginLockup = m_textures.get("assets/textures/astron-lockup.png");
    if (loginLockup.id != 0) {
        const float lw = 360.0f;
        const float lh = lw * (static_cast<float>(loginLockup.height) / std::max(1, loginLockup.width));
        dl->AddImage(static_cast<ImTextureID>(static_cast<uintptr_t>(loginLockup.id)),
                     ImVec2(lx - lw * 0.5f, ly - lh + 102.0f),
                     ImVec2(lx + lw * 0.5f, ly + 102.0f));
    } else {
        UIWidgets::DrawPlanetMark(dl, ImVec2(lx, ly - 95.0f), 84.0f, IM_COL32(255, 255, 255, 255), IM_COL32(0, 0, 0, 255));
        if (fontLogoBig) ImGui::PushFont(fontLogoBig);
        ImVec2 szA = ImGui::CalcTextSize("Astron");
        dl->AddText(ImVec2(lx - szA.x * 0.5f, ly + 8.0f), IM_COL32(255, 255, 255, 255), "Astron");
        if (fontLogoBig) ImGui::PopFont();

        if (fontLogoMed) ImGui::PushFont(fontLogoMed);
        ImVec2 szE = ImGui::CalcTextSize("E n g i n e");
        dl->AddText(ImVec2(lx - szE.x * 0.5f, ly + 74.0f), IM_COL32(236, 236, 240, 255), "E n g i n e");
        if (fontLogoMed) ImGui::PopFont();
    }

    const char* tag = "C r i e .   D e s e n v o l v a .   E x p l o r e .";
    ImVec2 szT = ImGui::CalcTextSize(tag);
    dl->AddText(ImVec2(lx - szT.x * 0.5f, ly + 122.0f), IM_COL32(210, 210, 218, 255), tag);

    // Right Half: Login Card
    const float cardW = std::min(520.0f, w * 0.40f);
    const float cardH = 510.0f;
    const float cardX = w * 0.75f - cardW * 0.5f;
    const float cardY = (h - cardH) * 0.5f;

    dl->AddRectFilled(ImVec2(cardX, cardY), ImVec2(cardX + cardW, cardY + cardH), IM_COL32(10, 10, 13, 235), 24.0f);
    dl->AddRect(ImVec2(cardX, cardY), ImVec2(cardX + cardW, cardY + cardH), IM_COL32(42, 43, 50, 255), 24.0f, 0, 1.0f);

    const float padX = 42.0f;
    const float innerW = cardW - padX * 2.0f;

    ImGui::SetCursorScreenPos(ImVec2(cardX + padX, cardY + 38.0f));
    ImGui::BeginGroup();

    if (fontTitle) ImGui::PushFont(fontTitle);
    ImGui::TextUnformatted("Bem-vindo(a)!");
    if (fontTitle) ImGui::PopFont();

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(158, 158, 168, 255));
    ImGui::TextUnformatted("Faça login na sua conta para continuar.");
    ImGui::PopStyleColor();

    ImGui::Dummy(ImVec2(0, 16));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14, 14));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(13, 14, 18, 255));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(46, 48, 56, 255));

    ImGui::SetNextItemWidth(innerW);
    ImGui::InputTextWithHint("##login_user", "Usuário ou e-mail", m_loginUser, sizeof(m_loginUser));

    ImGui::Dummy(ImVec2(0, 8));

    ImGui::SetNextItemWidth(innerW - 46.0f);
    ImGuiInputTextFlags pwFlags = m_loginShowPass ? 0 : ImGuiInputTextFlags_Password;
    bool enterPressed = ImGui::InputTextWithHint("##login_pass", "Senha", m_loginPass, sizeof(m_loginPass),
                                                 pwFlags | ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine(0, 6);
    if (UIWidgets::OutlineButton(m_loginShowPass ? "Ocultar" : "Ver", ImVec2(40, 44), 10.0f)) {
        m_loginShowPass = !m_loginShowPass;
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    if (!m_loginError.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 105, 110, 255));
        ImGui::TextWrapped("%s", m_loginError.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Dummy(ImVec2(0, 6));
    ImGui::Checkbox("Lembrar de mim", &m_loginRemember);
    ImGui::SameLine(innerW - 148.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(215, 216, 224, 255));
    if (ImGui::SmallButton("Esqueceu sua senha?")) {
        pushToast("Recuperação de senha enviada (modo local).");
    }
    ImGui::PopStyleColor(2);

    ImGui::Dummy(ImVec2(0, 10));

    auto tryLogin = [&]() {
        if (std::strlen(m_loginUser) == 0) {
            std::snprintf(m_loginUser, sizeof(m_loginUser), "dev@astron.engine");
        }
        m_loginError.clear();
        m_screen = ScreenId::Home;
    };

    if (UIWidgets::WhitePillButton("Entrar", ImVec2(innerW, 50.0f), 13.0f) || enterPressed) {
        tryLogin();
    }

    ImGui::Dummy(ImVec2(0, 10));
    {
        ImVec2 cur = ImGui::GetCursorScreenPos();
        dl->AddLine(ImVec2(cur.x, cur.y + 9), ImVec2(cur.x + innerW * 0.44f, cur.y + 9), IM_COL32(42, 43, 50, 255));
        dl->AddText(ImVec2(cur.x + innerW * 0.48f, cur.y), IM_COL32(142, 144, 154, 255), "ou");
        dl->AddLine(ImVec2(cur.x + innerW * 0.56f, cur.y + 9), ImVec2(cur.x + innerW, cur.y + 9), IM_COL32(42, 43, 50, 255));
        ImGui::Dummy(ImVec2(0, 22));
    }

    if (UIWidgets::OutlineButton("Entrar com GitHub", ImVec2(innerW, 48.0f), 13.0f)) {
        std::snprintf(m_loginUser, sizeof(m_loginUser), "github-dev");
        m_screen = ScreenId::Home;
    }

    ImGui::Dummy(ImVec2(0, 12));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (innerW - 220.0f) * 0.5f);
    ImGui::TextColored(ImVec4(0.62f, 0.63f, 0.67f, 1.f), "Não tem uma conta?");
    ImGui::SameLine();
    if (ImGui::SmallButton("Criar conta")) {
        tryLogin();
    }

    ImGui::EndGroup();

    // Bottom-left Configurações & bottom-right v1.0.0
    ImGui::SetCursorScreenPos(ImVec2(24.0f, h - 42.0f));
    if (UIWidgets::OutlineButton("⚙  Configurações", ImVec2(154.0f, 32.0f), 10.0f)) {
        m_settingsOpen = true;
    }
    dl->AddText(ImVec2(w - 72.0f, h - 34.0f), IM_COL32(142, 144, 154, 255), "v1.0.0");

    ImGui::End();
    ImGui::PopStyleColor();
}

void EngineApp::renderSettingsModal() {
    if (!m_settingsOpen) return;

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(640.0f, 520.0f), ImGuiCond_Appearing);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(13, 14, 18, 252));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(26, 22));

    if (ImGui::Begin("Configurações — Astron Engine & Agente IA", &m_settingsOpen, ImGuiWindowFlags_NoCollapse)) {
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
        if (ImGui::BeginCombo("##ai_model_preset", "Modelos rápidos...")) {
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
        ImGui::SliderInt("Passos autônomos máx.", &m_aiConfig.maxSteps, 4, 40);

        ImGui::Dummy(ImVec2(0, 14));
        if (UIWidgets::WhitePillButton("Salvar Configurações", ImVec2(210, 42), 11.0f)) {
            m_aiConfig.apiKey = m_apiKeyBuf;
            m_aiConfig.baseUrl = m_baseUrlBuf;
            m_aiConfig.model = m_modelBuf;
            saveSettings();
            m_settingsOpen = false;
            pushToast("Configurações de IA salvas ✓");
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
