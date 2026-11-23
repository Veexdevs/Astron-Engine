#include "EngineApp.hpp"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace astron {

EngineApp::EngineApp(std::filesystem::path workspaceRoot)
    : m_root(std::move(workspaceRoot)),
      m_projects(m_root),
      m_textures(m_root),
      m_viewport(m_root),
      m_agent(m_projects),
      m_scene(Scene::createDefault()) {

    m_tabs.push_back({ "scene:main", "Cena Principal", true, "", false });

    bool ok = false;
    std::string scJson = m_projects.readFile("res://default.scene", &ok);
    if (ok && !scJson.empty()) {
        m_scene = Scene::fromJson(Json::parse(scJson));
    }

    m_shaderSource = m_projects.readFile("res://shaders/surface.glsl", &ok);
    if (!ok || m_shaderSource.empty()) {
        m_shaderSource =
            "#version 330 core\n"
            "in vec3 vWorldPos;\n"
            "in vec3 vNormal;\n"
            "out vec4 FragColor;\n"
            "uniform vec4 uColor;\n"
            "uniform float uTime;\n"
            "void main() {\n"
            "    float l = max(dot(normalize(vNormal), normalize(vec3(0.6, 0.9, 0.5))), 0.2);\n"
            "    FragColor = vec4(uColor.rgb * l, uColor.a);\n"
            "}\n";
    }

    m_logs = {
        { "INFO",    "Projeto carregado com sucesso!" },
        { "WARNING", "Nenhum objeto selecionado." },
        { "ERROR",   "Erro ao carregar o script principal." }
    };

    loadSettings();
}

void EngineApp::initRenderResources() {
    m_viewport.init();
}

void EngineApp::loadFonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();

    static const ImWchar kGlyphRanges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin-1 Supplement (Portuguese accents)
        0x0100, 0x017F, // Latin Extended-A
        0x2000, 0x206F, // General Punctuation (•, —, …)
        0x2190, 0x21FF, // Arrows (←, →, ↖, ⤢)
        0x2200, 0x22FF, // Mathematical Operators
        0x2300, 0x23FF, // Misc Technical (⌂, ⏸)
        0x25A0, 0x25FF, // Geometric Shapes (■, ▶, ◇)
        0x2600, 0x26FF, // Misc Symbols (⚙, ⚡, ⚠)
        0x2700, 0x27BF, // Dingbats (✓, ✨)
        0,
    };

    auto tryLoad = [&](const char* rel, float sz) -> ImFont* {
        std::filesystem::path p = m_root / rel;
        std::error_code ec;
        if (std::filesystem::exists(p, ec)) {
            return io.Fonts->AddFontFromFileTTF(p.string().c_str(), sz, nullptr, kGlyphRanges);
        }
        return nullptr;
    };

    fontUi        = tryLoad("assets/fonts/AstronUI-Regular.ttf", 15.5f);
    fontUiBold    = tryLoad("assets/fonts/AstronUI-Bold.ttf", 16.0f);
    fontTitle     = tryLoad("assets/fonts/AstronUI-Bold.ttf", 32.0f);
    fontLogoBig   = tryLoad("assets/fonts/Michroma-Regular.ttf", 52.0f);
    fontLogoMed   = tryLoad("assets/fonts/Michroma-Regular.ttf", 21.0f);
    fontLogoSmall = tryLoad("assets/fonts/Michroma-Regular.ttf", 11.5f);
    fontMono      = tryLoad("assets/fonts/AstronCode-Mono.ttf", 14.5f);

    if (!fontUi) io.Fonts->AddFontDefault();
}

void EngineApp::applyDarkTheme() {
    ImGui::StyleColorsDark();
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowBorderSize = 1.0f;
    st.ChildBorderSize  = 1.0f;
    st.PopupBorderSize  = 1.0f;
    st.FrameRounding    = 6.0f;
    st.PopupRounding    = 10.0f;
    st.GrabRounding     = 6.0f;
    st.ScrollbarSize    = 10.0f;

    ImVec4* c = st.Colors;
    c[ImGuiCol_WindowBg]         = ImVec4(0.05f, 0.055f, 0.068f, 1.00f);
    c[ImGuiCol_ChildBg]          = ImVec4(0.067f, 0.075f, 0.094f, 1.00f);
    c[ImGuiCol_PopupBg]          = ImVec4(0.075f, 0.082f, 0.102f, 0.98f);
    c[ImGuiCol_Border]           = ImVec4(0.16f, 0.17f, 0.21f, 1.00f);
    c[ImGuiCol_FrameBg]          = ImVec4(0.085f, 0.092f, 0.115f, 1.00f);
    c[ImGuiCol_FrameBgHovered]   = ImVec4(0.12f, 0.13f, 0.16f, 1.00f);
    c[ImGuiCol_FrameBgActive]    = ImVec4(0.15f, 0.17f, 0.22f, 1.00f);
    c[ImGuiCol_TitleBg]          = ImVec4(0.06f, 0.065f, 0.08f, 1.00f);
    c[ImGuiCol_TitleBgActive]    = ImVec4(0.08f, 0.09f, 0.12f, 1.00f);
    c[ImGuiCol_Header]           = ImVec4(0.11f, 0.31f, 0.66f, 0.85f);
    c[ImGuiCol_HeaderHovered]    = ImVec4(0.15f, 0.38f, 0.78f, 0.90f);
    c[ImGuiCol_HeaderActive]     = ImVec4(0.18f, 0.44f, 0.88f, 1.00f);
    c[ImGuiCol_Button]           = ImVec4(0.12f, 0.13f, 0.17f, 1.00f);
    c[ImGuiCol_ButtonHovered]    = ImVec4(0.18f, 0.20f, 0.26f, 1.00f);
    c[ImGuiCol_ButtonActive]     = ImVec4(0.14f, 0.34f, 0.72f, 1.00f);
    c[ImGuiCol_CheckMark]        = ImVec4(0.24f, 0.56f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrab]       = ImVec4(0.24f, 0.56f, 1.00f, 1.00f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.36f, 0.66f, 1.00f, 1.00f);
}

void EngineApp::pushLog(const std::string& level, const std::string& msg) {
    m_logs.push_back({ level, msg });
    if (m_logs.size() > 300) {
        m_logs.erase(m_logs.begin());
    }
}

void EngineApp::pushToast(const std::string& msg) {
    m_toasts.push_back({ msg, 3.2f });
}

void EngineApp::openFileTab(const std::string& resPath) {
    std::string norm = m_projects.normalizeResPath(resPath);
    for (size_t i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs[i].id == norm) {
            bool ok = false;
            std::string diskContent = m_projects.readFile(norm, &ok);
            if (ok && !m_tabs[i].dirty) m_tabs[i].buffer = std::move(diskContent);
            m_activeTabIdx = static_cast<int>(i);
            return;
        }
    }

    bool ok = false;
    std::string content = m_projects.readFile(norm, &ok);
    std::string fname = std::filesystem::path(norm.substr(6)).filename().string();
    m_tabs.push_back({ norm, fname.empty() ? norm : fname, false, content, false });
    m_activeTabIdx = static_cast<int>(m_tabs.size()) - 1;
}

void EngineApp::saveActiveScene() {
    std::string dump = m_scene.toJson().dump(2);
    m_projects.writeFile("res://default.scene", dump);
    pushLog("INFO", "Cena salva em res://default.scene");
    pushToast("Cena salva ✓");
}

void EngineApp::loadSettings() {
    if (const char* envOr = std::getenv("OPENROUTER_API_KEY")) {
        m_aiConfig.apiKey = envOr;
        m_aiConfig.provider = "openrouter";
    } else if (const char* envOa = std::getenv("OPENAI_API_KEY")) {
        m_aiConfig.apiKey = envOa;
        m_aiConfig.provider = "openai";
        m_aiConfig.baseUrl = "https://api.openai.com/v1";
        m_aiConfig.model = "gpt-4o-mini";
    }

    std::ifstream f(m_root / "astron_settings.json", std::ios::binary);
    if (f) {
        std::ostringstream ss;
        ss << f.rdbuf();
        Json j = Json::parse(ss.str());
        if (j.isObject()) {
            m_aiConfig.provider    = j["provider"].strOr(m_aiConfig.provider);
            m_aiConfig.apiKey      = j["apiKey"].strOr(m_aiConfig.apiKey);
            m_aiConfig.baseUrl     = j["baseUrl"].strOr(m_aiConfig.baseUrl);
            m_aiConfig.model       = j["model"].strOr(m_aiConfig.model);
            m_aiConfig.temperature = static_cast<float>(j["temperature"].num(m_aiConfig.temperature));
            m_aiConfig.maxSteps    = j["maxSteps"].integer(m_aiConfig.maxSteps);
        }
    }

    std::snprintf(m_apiKeyBuf, sizeof(m_apiKeyBuf), "%s", m_aiConfig.apiKey.c_str());
    std::snprintf(m_baseUrlBuf, sizeof(m_baseUrlBuf), "%s", m_aiConfig.baseUrl.c_str());
    std::snprintf(m_modelBuf, sizeof(m_modelBuf), "%s", m_aiConfig.model.c_str());
}

void EngineApp::saveSettings() const {
    Json j = Json::object();
    j["provider"] = m_aiConfig.provider;
    j["apiKey"] = m_aiConfig.apiKey;
    j["baseUrl"] = m_aiConfig.baseUrl;
    j["model"] = m_aiConfig.model;
    j["temperature"] = static_cast<double>(m_aiConfig.temperature);
    j["maxSteps"] = m_aiConfig.maxSteps;

    std::ofstream f(m_root / "astron_settings.json", std::ios::binary | std::ios::trunc);
    if (f) {
        std::string s = j.dump(2);
        f.write(s.data(), static_cast<std::streamsize>(s.size()));
    }
}

void EngineApp::submitAgentPrompt(const std::string& prompt) {
    if (prompt.empty() || m_agent.isRunning()) return;

    m_chatMsgs.push_back({ true, prompt, {}, {}, false, {} });
    m_chatMsgs.push_back({ false, {}, "Conectando à API de IA...", {}, true, {} });
    m_agent.startPromptAsync(m_aiConfig, prompt);
}

void EngineApp::pollAgentEvents() {
    auto evts = m_agent.pollEvents();
    if (evts.empty()) return;

    for (const auto& e : evts) {
        if (m_chatMsgs.empty() || m_chatMsgs.back().isUser) {
            m_chatMsgs.push_back({ false, {}, {}, {}, true, {} });
        }
        auto& cur = m_chatMsgs.back();

        switch (e.type) {
            case AgentEvent::Type::Status:
                cur.status = e.text;
                break;
            case AgentEvent::Type::AssistantText:
                if (!cur.text.empty()) cur.text += "\n";
                cur.text += e.text;
                break;
            case AgentEvent::Type::ToolStart:
                cur.status = "Executando " + e.text + (e.detail.empty() ? "" : (" · " + e.detail));
                break;
            case AgentEvent::Type::ToolDone:
                cur.tools.push_back({ e.text, e.detail });
                cur.status.clear();
                break;
            case AgentEvent::Type::FileWritten:
                pushLog("INFO", "Agente IA gravou " + e.text + " (" + e.detail + ")");
                openFileTab(e.text);
                break;
            case AgentEvent::Type::SpawnEntity: {
                int idx = m_scene.addObject(e.entityName, e.mesh, e.pos);
                if (idx >= 0 && idx < static_cast<int>(m_scene.objects.size())) {
                    m_scene.objects[static_cast<size_t>(idx)].scale = e.scale;
                    m_scene.objects[static_cast<size_t>(idx)].colorHex = e.colorHex;
                }
                pushLog("INFO", "Agente IA adicionou objeto 3D: " + e.entityName);
                break;
            }
            case AgentEvent::Type::Error:
                cur.error = e.text;
                cur.pending = false;
                cur.status.clear();
                pushLog("ERROR", std::string("Agente IA: ") + e.text);
                break;
            case AgentEvent::Type::Done:
                if (cur.text.empty() && !e.text.empty()) cur.text = e.text;
                cur.pending = false;
                cur.status.clear();
                pushToast("Agente IA concluiu a implementação C# ✓");
                break;
        }
    }
}

void EngineApp::renderToasts(float dt, float w, float h) {
    float y = h - 54.0f;
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    for (auto it = m_toasts.begin(); it != m_toasts.end();) {
        it->ttl -= dt;
        if (it->ttl <= 0.0f) {
            it = m_toasts.erase(it);
            continue;
        }
        ImVec2 ts = ImGui::CalcTextSize(it->text.c_str());
        float tw = ts.x + 32.0f;
        float th = ts.y + 18.0f;
        ImVec2 mn(w - tw - 22.0f, y - th);
        ImVec2 mx(w - 22.0f, y);
        fg->AddRectFilled(mn, mx, IM_COL32(20, 22, 28, 242), 10.0f);
        fg->AddRect(mn, mx, IM_COL32(58, 122, 235, 220), 10.0f);
        fg->AddText(ImVec2(mn.x + 16.0f, mn.y + 9.0f), IM_COL32(245, 246, 250, 255), it->text.c_str());
        y -= (th + 8.0f);
        ++it;
    }
}

void EngineApp::updateAndRender(float dt, int displayW, int displayH) {
    m_timeSec += dt;
    pollAgentEvents();

    const float w = static_cast<float>(std::max(640, displayW));
    const float h = static_cast<float>(std::max(400, displayH));

    switch (m_screen) {
        case ScreenId::Splash: renderSplash(dt, w, h); break;
        case ScreenId::Login:  renderLogin(w, h);      break;
        case ScreenId::Home:   renderHome(w, h);       break;
        case ScreenId::Build:  renderBuild(dt, w, h);  break;
        case ScreenId::Editor: renderEditor(dt, w, h); break;
    }

    renderSettingsModal();
    renderNewProjectModal();
    renderBuildPipelineModal(dt);
    renderToasts(dt, w, h);
}

} // namespace astron
