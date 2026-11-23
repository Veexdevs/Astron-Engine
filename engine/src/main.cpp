#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "../core_c/astron_gl.h"
#include "EngineApp.hpp"

#include <cstdio>
#include <filesystem>

namespace {

std::filesystem::path locateWorkspaceRoot(const char* argv0) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path cwd = fs::current_path(ec);
    if (fs::exists(cwd / "assets", ec)) return cwd;

    if (argv0) {
        fs::path exeDir = fs::absolute(fs::path(argv0), ec).parent_path();
        if (fs::exists(exeDir / "assets", ec)) return exeDir;
        if (fs::exists(exeDir.parent_path() / "assets", ec)) return exeDir.parent_path();
    }
    return cwd;
}

} // namespace

int main(int argc, char** argv) {
    if (!glfwInit()) {
        std::fprintf(stderr, "Falha ao inicializar GLFW.\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1440, 860, "Astron Engine", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Falha ao criar janela OpenGL 3.3 Core.\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!AstronGLLoad(reinterpret_cast<AstronGLGetProc>(glfwGetProcAddress))) {
        std::fprintf(stderr, "Aviso: nem todas as funções OpenGL 3.3 foram carregadas.\n");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    std::filesystem::path root = locateWorkspaceRoot(argc > 0 ? argv[0] : nullptr);

    // Define o ícone oficial da Astron Engine na barra de título / barra de tarefas do SO
    {
        std::vector<unsigned char> iconRgba;
        int iconW = 0, iconH = 0;
        if (astron::TextureManager::loadRgbaPixels(root / "assets/textures/astron-icon.png", iconRgba, iconW, iconH)) {
            GLFWimage img{};
            img.width = iconW;
            img.height = iconH;
            img.pixels = iconRgba.data();
            glfwSetWindowIcon(window, 1, &img);
        }
    }

    astron::EngineApp app(root);
    app.applyDarkTheme();
    app.loadFonts();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    app.initRenderResources();

    app.onMinimizeWindow = [window]() { glfwIconifyWindow(window); };
    app.onMaximizeWindow = [window]() {
        if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED)) glfwRestoreWindow(window);
        else glfwMaximizeWindow(window);
    };
    app.onCloseWindow = [window]() { glfwSetWindowShouldClose(window, GLFW_TRUE); };

    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double now = glfwGetTime();
        float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        int fbw = 0, fbh = 0;
        glfwGetFramebufferSize(window, &fbw, &fbh);
        int winW = 0, winH = 0;
        glfwGetWindowSize(window, &winW, &winH);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        app.updateAndRender(dt, winW, winH);

        ImGui::Render();
        aglBindFramebuffer(GL_FRAMEBUFFER, 0);
        aglViewport(0, 0, fbw, fbh);
        aglClearColor(0.02f, 0.02f, 0.03f, 1.0f);
        aglClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
