#pragma once
#include "AIAgent.hpp"
#include "ProjectManager.hpp"
#include "Scene.hpp"
#include "TextureManager.hpp"
#include "Viewport3D.hpp"
#include <imgui.h>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace astron {

enum class ScreenId {
    Splash,
    Login,
    Home,
    Build,
    Editor
};

struct ConsoleEntry {
    std::string level; // "INFO" | "WARNING" | "ERROR"
    std::string message;
};

struct EditorTab {
    std::string id;      // "scene:main" or "res://..."
    std::string label;
    bool isScene = false;
    std::string buffer;
    bool dirty = false;
};

struct ChatMessageUI {
    bool isUser = false;
    std::string text;
    std::string status;
    std::string error;
    bool pending = false;
    std::vector<std::pair<std::string, std::string>> tools; // (name, detail)
};

struct ToastItem {
    std::string text;
    float ttl = 3.2f;
};

class EngineApp {
public:
    explicit EngineApp(std::filesystem::path workspaceRoot = ".");

    void initRenderResources();
    void loadFonts();
    void applyDarkTheme();

    void updateAndRender(float dt, int displayW, int displayH);

    void setScreen(ScreenId s) { m_screen = s; }
    ScreenId screen() const { return m_screen; }

    // Window callbacks wired by main.cpp
    std::function<void()> onMinimizeWindow;
    std::function<void()> onMaximizeWindow;
    std::function<void()> onCloseWindow;

    void pushLog(const std::string& level, const std::string& msg);
    void pushToast(const std::string& msg);
    void openFileTab(const std::string& resPath);
    void saveActiveScene();
    void loadSettings();
    void saveSettings() const;

    // Fonts
    ImFont* fontUi = nullptr;
    ImFont* fontUiBold = nullptr;
    ImFont* fontTitle = nullptr;
    ImFont* fontLogoBig = nullptr;
    ImFont* fontLogoMed = nullptr;
    ImFont* fontLogoSmall = nullptr;
    ImFont* fontMono = nullptr;

private:
    void renderSplash(float dt, float w, float h);
    void renderLogin(float w, float h);
    void renderHome(float w, float h);
    void renderBuild(float dt, float w, float h);
    void renderEditor(float dt, float w, float h);

    void renderSettingsModal();
    void renderNewProjectModal();
    void renderBuildPipelineModal(float dt);
    void renderToasts(float dt, float w, float h);

    void pollAgentEvents();
    void submitAgentPrompt(const std::string& prompt);
    void startBuildPipeline(const std::string& targetId);

    std::filesystem::path m_root;
    ProjectManager m_projects;
    TextureManager m_textures;
    Viewport3D m_viewport;
    AIAgent m_agent;
    AIConfig m_aiConfig;
    Scene m_scene;

    ScreenId m_screen = ScreenId::Splash;
    float m_timeSec = 0.0f;

    // Splash state
    float m_splashProgress = 0.0f;

    // Login state
    char m_loginUser[128] = "";
    char m_loginPass[128] = "";
    bool m_loginRemember = true;
    bool m_loginShowPass = false;
    std::string m_loginError;

    // Home & Build navigation
    std::string m_homePage = "inicio";     // inicio | projetos | biblioteca | loja | config | sobre
    std::string m_buildPage = "compilar";  // compilar | config | sobre
    bool m_quickBuild = false;

    // Modals
    bool m_settingsOpen = false;
    bool m_newProjOpen = false;
    char m_newProjName[128] = "Novo Jogo C#";
    int  m_newProjTemplate = 1;

    bool m_buildModalOpen = false;
    std::string m_buildTargetId = "exe";
    std::vector<std::string> m_buildSteps;
    float m_buildStepTimer = 0.0f;
    int m_buildCurrentStep = 0;
    std::string m_lastExportedBundle;

    // Editor state
    int  m_activeTool = 0; // 0=Selecionar, 1=Mover, 2=Rotacionar, 3=Escalar, 4=Transformar
    bool m_playing = false;
    bool m_paused = false;
    bool m_showExplorer = true;
    bool m_showFiles = true;
    bool m_showProperties = true;
    bool m_showToolbox = true;
    bool m_showBottom = true;

    char m_explorerSearch[128] = "";
    char m_filesSearch[128] = "";
    char m_propsSearch[128] = "";
    char m_consoleInput[256] = "";
    char m_agentInput[2048] = "";

    int m_toolboxTab = 0; // 0=Modelos, 1=Recursos, 2=Materiais
    int m_bottomTab = 0;  // 0=Console, 1=Saída, 2=Depurador, 3=Áudio, 4=Animação, 5=Editor de Shader, 6=Agente IA

    std::vector<EditorTab> m_tabs;
    int m_activeTabIdx = 0;
    std::string m_shaderSource;

    // C11 Audio Synthesizer state (🔊 Áudio panel)
    int   m_audioPreset = 0;
    float m_audioFreqHz = 440.0f;
    float m_audioDurationSec = 0.45f;
    float m_audioVolume = 0.80f;
    float m_audioPeaks[48] = {};
    std::string m_lastSynthWav;

    std::vector<ConsoleEntry> m_logs;
    std::vector<ChatMessageUI> m_chatMsgs;
    std::vector<ToastItem> m_toasts;

    // Settings input buffers
    char m_apiKeyBuf[512] = "";
    char m_baseUrlBuf[512] = "https://openrouter.ai/api/v1";
    char m_modelBuf[256] = "openai/gpt-4o-mini";
};

} // namespace astron
