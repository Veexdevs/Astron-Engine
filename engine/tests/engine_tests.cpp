#include "../core_c/astron_math.h"
#include "../core_c/astron_gl.h"
#include "../core_c/astron_platform.h"
#include "../core_c/astron_audio.h"
#include "../core_c/astron_simd.h"
#include "Json.hpp"
#include "Scene.hpp"
#include "ProjectManager.hpp"
#include "TextureManager.hpp"
#include "AIProviders.hpp"
#include "AIAgent.hpp"
#include "EngineApp.hpp"

#include <imgui.h>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

using namespace astron;

namespace {

void testC11Math() {
    std::printf("[TEST 1/5] C11 Math & 3D Raycast...\n");
    AstronMat4 proj = astron_mat4_perspective(50.0f * (3.14159265f / 180.0f), 16.0f / 9.0f, 0.1f, 200.0f);
    AstronMat4 view = astron_mat4_look_at({ 0.0f, 3.0f, 8.0f }, { 0.0f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f });
    AstronMat4 vp   = astron_mat4_mul(proj, view);
    AstronMat4 inv;
    bool ok = astron_mat4_invert(vp, &inv);
    assert(ok);

    AstronMat4 ident = astron_mat4_mul(vp, inv);
    assert(std::fabs(ident.m[0] - 1.0f) < 1e-3f);
    assert(std::fabs(ident.m[5] - 1.0f) < 1e-3f);

    float tHit = 0.0f;
    AstronRay ray = { { 0.0f, 0.5f, 5.0f }, { 0.0f, 0.0f, -1.0f } };
    AstronAABB box = { { -0.5f, 0.0f, -0.5f }, { 0.5f, 1.0f, 0.5f } };
    bool hit = astron_ray_intersect_aabb(ray, box, &tHit);
    assert(hit);
    assert(std::fabs(tHit - 4.5f) < 1e-4f);

    AstronVec4 col;
    assert(astron_hex_to_rgba("#3882F6", &col));
    char hexOut[8] = {};
    astron_rgba_to_hex(col, hexOut);
    assert(std::string(hexOut) == "#3882F6");

    // Verify hardware SIMD matrix multiply & vertex transform match scalar C11
    AstronMat4 simdVp = astron_simd_mat4_mul(proj, view);
    for (int i = 0; i < 16; ++i) {
        assert(std::fabs(simdVp.m[i] - vp.m[i]) < 1e-4f);
    }
    const float srcPts[6] = { 1.0f, 2.0f, 3.0f, -2.0f, 0.5f, 4.0f };
    float outPts[6] = {};
    AstronMat4 tr = astron_mat4_translate({ 10.0f, -5.0f, 2.0f });
    astron_simd_transform_points(&tr, srcPts, outPts, 2);
    assert(std::fabs(outPts[0] - 11.0f) < 1e-4f);
    assert(std::fabs(outPts[1] - (-3.0f)) < 1e-4f);
    assert(std::fabs(outPts[2] - 5.0f) < 1e-4f);
    AstronAABB bnd = astron_simd_compute_bounds(outPts, 2);
    assert(std::fabs(bnd.min.x - 8.0f) < 1e-4f);
    assert(std::fabs(bnd.max.x - 11.0f) < 1e-4f);
    assert(std::strlen(astron_simd_backend_name()) > 0);
}

void testC11AudioAndBrandLogos(const std::filesystem::path& repoRoot, const std::filesystem::path& tmpWs) {
    std::printf("[TEST 2/6] C11 WAV Audio Synth & Official Astron Engine Logo Assets...\n");
    AstronAudioParams ap{};
    ap.preset = ASTRON_SFX_COIN;
    ap.sampleRate = 44100u;
    ap.durationSec = 0.25f;
    ap.baseFreqHz = 523.25f;
    ap.volume = 0.8f;

    float peaks[32] = {};
    auto wavPath = tmpWs / "test_coin.wav";
    assert(astron_audio_write_wav(wavPath.string().c_str(), &ap, peaks, 32));
    assert(std::filesystem::exists(wavPath));
    assert(std::filesystem::file_size(wavPath) > 44);

    // Check RIFF/WAVE magic bytes
    std::ifstream wf(wavPath, std::ios::binary);
    char magic[12] = {};
    wf.read(magic, 12);
    assert(std::memcmp(magic, "RIFF", 4) == 0);
    assert(std::memcmp(magic + 8, "WAVE", 4) == 0);

    // Verify official Astron Engine brand logos exist and decode cleanly
    const char* logos[] = {
        "assets/textures/astron-icon.png",
        "assets/textures/astron-mark.png",
        "assets/textures/astron-lockup.png",
        "assets/textures/astron-horizontal.png"
    };
    for (const char* rel : logos) {
        std::vector<unsigned char> px;
        int w = 0, h = 0;
        bool loaded = TextureManager::loadRgbaPixels(repoRoot / rel, px, w, h);
        assert(loaded && w > 0 && h > 0 && px.size() == static_cast<size_t>(w * h * 4));
    }
}

void testJsonAndScene() {
    std::printf("[TEST 2/5] JSON Parser & 3D Scene Physics...\n");
    Scene sc = Scene::createDefault();
    assert(!sc.objects.empty());

    sc.beginPlay();
    float initialY = sc.objects[0].position.y;
    sc.objects[0].position.y = 4.0f;
    for (int i = 0; i < 30; ++i) {
        sc.stepPhysics(1.0f / 60.0f, 1.0f, 0.0f, false);
    }
    assert(sc.objects[0].position.y < 4.0f);
    assert(sc.objects[0].position.x > 0.0f);
    sc.stopPlay();
    assert(std::fabs(sc.objects[0].position.y - initialY) < 1e-4f);

    std::string dumped = sc.toJson().dump(2);
    Scene restored = Scene::fromJson(Json::parse(dumped));
    assert(restored.objects.size() == sc.objects.size());
    assert(restored.objects[0].name == sc.objects[0].name);
}

void testProjectManagerSync(const std::filesystem::path& tmpWs) {
    std::printf("[TEST 3/5] ProjectManager C# res:// & AstronGame Sync...\n");
    std::filesystem::create_directories(tmpWs / "games" / "AstronGame" / "Core");
    {
        std::ofstream f(tmpWs / "games" / "AstronGame" / "Core" / "GameWorld.cs");
        f << "namespace Astron.Game { public sealed class GameWorld {} }\n";
    }

    ProjectManager pm(tmpWs);
    assert(pm.projects().size() == 3);
    assert(pm.projects()[0].name == "Meu Jogo");
    assert(pm.projects()[0].createdDate == "22/11/2026");
    assert(pm.projects()[1].name == "Projeto RP");
    assert(pm.projects()[2].name == "Obby Teste");

    std::string customWorld =
        "namespace Astron.Game {\n"
        "    public sealed class GameWorld { public int CustomScore = 777; }\n"
        "}\n";
    assert(pm.writeFile("res://scripts/GameWorld.cs", customWorld));

    std::ifstream synced(tmpWs / "games" / "AstronGame" / "Core" / "GameWorld.cs");
    std::string syncedStr((std::istreambuf_iterator<char>(synced)), std::istreambuf_iterator<char>());
    assert(syncedStr.find("CustomScore = 777") != std::string::npos);

    std::string helperCs =
        "using Astron.Engine;\n"
        "namespace Astron.Scripts { public sealed class Jetpack : Component {} }\n";
    assert(pm.writeFile("res://scripts/Jetpack.cs", helperCs));
    assert(std::filesystem::exists(tmpWs / "games" / "AstronGame" / "Core" / "Scripts" / "Jetpack.cs"));
}

#ifndef _WIN32
void testAutonomousOpenRouterAgent(const std::filesystem::path& tmpWs) {
    std::printf("[TEST 4/5] Autonomous AI Agent (OpenRouter Tool-Calling Loop)...\n");

    int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    assert(serverFd >= 0);
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0; // ephemeral port
    assert(::bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    socklen_t len = sizeof(addr);
    assert(::getsockname(serverFd, reinterpret_cast<sockaddr*>(&addr), &len) == 0);
    uint16_t port = ntohs(addr.sin_port);
    assert(::listen(serverFd, 4) == 0);

    std::atomic<int> callCount{0};
    std::thread serverThread([serverFd, &callCount]() {
        for (int turn = 0; turn < 2; ++turn) {
            int client = ::accept(serverFd, nullptr, nullptr);
            if (client < 0) break;

            std::string req;
            char buf[4096];
            while (true) {
                ssize_t n = ::recv(client, buf, sizeof(buf), 0);
                if (n <= 0) break;
                req.append(buf, static_cast<size_t>(n));
                auto hdrEnd = req.find("\r\n\r\n");
                if (hdrEnd != std::string::npos) {
                    size_t contentLen = 0;
                    auto clPos = req.find("Content-Length:");
                    if (clPos == std::string::npos) clPos = req.find("content-length:");
                    if (clPos != std::string::npos) {
                        contentLen = static_cast<size_t>(std::stoul(req.substr(clPos + 15)));
                    }
                    if (req.size() >= hdrEnd + 4 + contentLen) break;
                }
            }

            callCount++;
            Json resp = Json::object();
            Json choices = Json::array();
            Json c0 = Json::object();
            Json msg = Json::object();
            msg["role"] = "assistant";

            if (turn == 0) {
                msg["content"] = "Vou criar os scripts C# automaticamente no projeto.";
                Json tcs = Json::array();

                Json tc1 = Json::object();
                tc1["id"] = "call_write_cs_1";
                tc1["type"] = "function";
                Json fn1 = Json::object();
                fn1["name"] = "write_file";
                Json args1 = Json::object();
                args1["path"] = "res://scripts/EnemyDrone.cs";
                args1["content"] =
                    "using Astron.Engine;\n"
                    "namespace Astron.Scripts {\n"
                    "    public sealed class EnemyDrone : Component {\n"
                    "        public float Speed = 8.5f;\n"
                    "    }\n"
                    "}\n";
                fn1["arguments"] = args1.dump();
                tc1["function"] = fn1;
                tcs.push_back(tc1);

                Json tc2 = Json::object();
                tc2["id"] = "call_spawn_2";
                tc2["type"] = "function";
                Json fn2 = Json::object();
                fn2["name"] = "spawn_entity";
                Json args2 = Json::object();
                args2["name"] = "DroneInimigo";
                args2["mesh"] = "Sphere";
                args2["x"] = 2.0;
                args2["y"] = 1.5;
                args2["z"] = -1.0;
                args2["color"] = "#FF3B5C";
                fn2["arguments"] = args2.dump();
                tc2["function"] = fn2;
                tcs.push_back(tc2);

                msg["tool_calls"] = tcs;
            } else {
                // Verify tool result was fed back automatically
                assert(req.find("call_write_cs_1") != std::string::npos);
                msg["content"] = "Pronto! Gravei res://scripts/EnemyDrone.cs e adicionei DroneInimigo na cena 3D.";
            }

            c0["message"] = msg;
            choices.push_back(c0);
            resp["choices"] = choices;

            std::string body = resp.dump();
            std::string http =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: " + std::to_string(body.size()) + "\r\n"
                "Connection: close\r\n\r\n" + body;
            (void)::send(client, http.data(), http.size(), 0);
            ::close(client);
        }
        ::close(serverFd);
    });

    ProjectManager pm(tmpWs);
    AIAgent agent(pm);

    AIConfig cfg;
    cfg.provider = "openrouter";
    cfg.apiKey = "sk-or-v1-test-key";
    cfg.baseUrl = "http://127.0.0.1:" + std::to_string(port) + "/api/v1";
    cfg.model = "anthropic/claude-3.5-sonnet";

    int filesWritten = 0;
    int entitiesSpawned = 0;
    bool doneReceived = false;

    AgentTurnResult turnRes = agent.runTurnSync(cfg, "Crie um inimigo drone em C# e coloque na cena 3D", [&](const AgentEvent& ev) {
        if (ev.type == AgentEvent::Type::FileWritten) filesWritten++;
        if (ev.type == AgentEvent::Type::SpawnEntity) entitiesSpawned++;
        if (ev.type == AgentEvent::Type::Done) doneReceived = true;
    });
    assert(turnRes.ok);

    serverThread.join();

    assert(callCount.load() == 2);
    assert(filesWritten == 1);
    assert(entitiesSpawned == 1);
    assert(doneReceived);
    assert(pm.fileExists("res://scripts/EnemyDrone.cs"));
    assert(std::filesystem::exists(tmpWs / "games" / "AstronGame" / "Core" / "Scripts" / "EnemyDrone.cs"));
}
#endif

void testAllFiveNativeScreensHeadless(const std::filesystem::path& repoRoot, const std::filesystem::path& tmpWs) {
    std::printf("[TEST 5/5] Headless ImGui Render Verification of All 5 Screens...\n");

    // Copy fonts into tmpWs so loadFonts exercises real TTF loading
    std::filesystem::create_directories(tmpWs / "assets");
    std::error_code ec;
    std::filesystem::copy(repoRoot / "assets" / "fonts", tmpWs / "assets" / "fonts",
                          std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(1440.0f, 860.0f);

    EngineApp app(tmpWs);
    app.applyDarkTheme();
    app.loadFonts();

    unsigned char* pixels = nullptr;
    int texW = 0, texH = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &texW, &texH);
    assert(pixels != nullptr && texW > 0 && texH > 0);
    io.Fonts->SetTexID(static_cast<ImTextureID>(1));

    const ScreenId screens[5] = {
        ScreenId::Splash,
        ScreenId::Login,
        ScreenId::Home,
        ScreenId::Build,
        ScreenId::Editor
    };

    for (ScreenId sid : screens) {
        app.setScreen(sid);
        for (int frame = 0; frame < 3; ++frame) {
            io.DeltaTime = 1.0f / 60.0f;
            ImGui::NewFrame();
            app.updateAndRender(1.0f / 60.0f, 1440, 860);
            ImGui::Render();
            ImDrawData* dd = ImGui::GetDrawData();
            assert(dd != nullptr);
            assert(dd->CmdListsCount > 0);
        }
    }

    ImGui::DestroyContext();
}

} // namespace

int main() {
    std::filesystem::path repoRoot = std::filesystem::current_path();
    std::filesystem::path tmpWs = "/tmp/astron_engine_unit_ws";
    std::error_code ec;
    std::filesystem::remove_all(tmpWs, ec);
    std::filesystem::create_directories(tmpWs, ec);

    testC11Math();
    testC11AudioAndBrandLogos(repoRoot, tmpWs);
    testJsonAndScene();
    testProjectManagerSync(tmpWs);
#ifndef _WIN32
    testAutonomousOpenRouterAgent(tmpWs);
#endif
    testAllFiveNativeScreensHeadless(repoRoot, tmpWs);

    std::filesystem::remove_all(tmpWs, ec);
    std::printf("\n=== ALL ASTRON ENGINE NATIVE TESTS PASSED ===\n");
    return 0;
}
