#include "AIAgent.hpp"
#include <sstream>

namespace astron {

AIAgent::AIAgent(ProjectManager& projectManager)
    : m_pm(projectManager), m_history(Json::array()) {}

AIAgent::~AIAgent() {
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

void AIAgent::clearConversation() {
    if (m_running.load()) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_history = Json::array();
    m_eventQueue.clear();
}

std::vector<AgentEvent> AIAgent::pollEvents() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AgentEvent> out;
    out.swap(m_eventQueue);
    return out;
}

Json AIAgent::buildToolsSchema() {
    auto makeTool = [](const char* name, const char* desc, Json props, Json req) {
        Json fn = Json::object();
        fn["name"] = name;
        fn["description"] = desc;
        Json params = Json::object();
        params["type"] = "object";
        params["properties"] = std::move(props);
        params["required"] = std::move(req);
        fn["parameters"] = std::move(params);

        Json t = Json::object();
        t["type"] = "function";
        t["function"] = std::move(fn);
        return t;
    };

    Json tools = Json::array();

    tools.push_back(makeTool(
        "list_files",
        "Lista todos os arquivos atuais do projeto Astron Engine (caminhos res://).",
        Json::object(),
        Json::array()
    ));

    {
        Json props = Json::object();
        Json p = Json::object();
        p["type"] = "string";
        p["description"] = "Caminho res:// do arquivo, ex.: res://scripts/GameWorld.cs";
        props["path"] = std::move(p);
        Json req = Json::array();
        req.push_back("path");
        tools.push_back(makeTool("read_file", "Lê o conteúdo completo de um arquivo C#, .scene ou .glsl.", std::move(props), std::move(req)));
    }

    {
        Json props = Json::object();
        Json p = Json::object();
        p["type"] = "string";
        p["description"] = "Caminho res:// do arquivo a criar ou substituir, ex.: res://scripts/GameWorld.cs";
        props["path"] = std::move(p);
        Json c = Json::object();
        c["type"] = "string";
        c["description"] = "Conteúdo COMPLETO em C# (.cs), cena JSON (.scene) ou shader (.glsl)";
        props["content"] = std::move(c);
        Json req = Json::array();
        req.push_back("path");
        req.push_back("content");
        tools.push_back(makeTool(
            "write_file",
            "Cria ou substitui um arquivo inteiro no disco do projeto (sincronizando C# com os builds APK/IPA/EXE/Linux). Sempre envie o arquivo COMPLETO.",
            std::move(props),
            std::move(req)
        ));
    }

    {
        Json props = Json::object();
        auto strProp = [](const char* d) { Json o = Json::object(); o["type"] = "string"; o["description"] = d; return o; };
        auto numProp = [](const char* d) { Json o = Json::object(); o["type"] = "number"; o["description"] = d; return o; };
        props["name"] = strProp("Nome do objeto na cena 3D");
        props["mesh"] = strProp("Cube | Sphere | Cylinder | Block | Terrain");
        props["x"] = numProp("Posição X");
        props["y"] = numProp("Posição Y");
        props["z"] = numProp("Posição Z");
        props["sx"] = numProp("Escala X");
        props["sy"] = numProp("Escala Y");
        props["sz"] = numProp("Escala Z");
        props["color"] = strProp("Cor hex, ex.: #3B82F6");
        Json req = Json::array();
        req.push_back("name");
        req.push_back("mesh");
        tools.push_back(makeTool(
            "spawn_entity",
            "Adiciona um objeto 3D diretamente na cena ativa do editor OpenGL.",
            std::move(props),
            std::move(req)
        ));
    }

    {
        Json props = Json::object();
        Json s = Json::object();
        s["type"] = "string";
        s["description"] = "Resumo curto em português do que foi criado";
        props["summary"] = std::move(s);
        Json req = Json::array();
        req.push_back("summary");
        tools.push_back(makeTool(
            "finish",
            "Finaliza a implementação autônoma quando todo o código C# e objetos já tiverem sido gravados.",
            std::move(props),
            std::move(req)
        ));
    }

    return tools;
}

std::string AIAgent::buildSystemPrompt() const {
    auto files = m_pm.listFiles();
    std::ostringstream tree;
    if (files.empty()) {
        tree << "(nenhum arquivo)";
    } else {
        for (const auto& f : files) tree << "- " << f << "\n";
    }

    return
        "Você é o AGENTE IA AUTÔNOMO da ASTRON ENGINE (engine nativa em C++, C, GLSL e C#).\n"
        "Sua missão é construir o jogo INTEIRO pedido pelo usuário escrevendo os arquivos C# diretamente no projeto através das ferramentas, SEM pedir ajuda manual.\n\n"
        "REGRAS OBRIGATÓRIAS:\n"
        "1. Responda sempre em português do Brasil.\n"
        "2. Todos os jogos são programados em C# (.NET 8, compatível com netstandard2.0). Nunca use HTML, JS ou Lua.\n"
        "3. Use `list_files` e `read_file` para inspecionar o projeto antes de alterar arquivos existentes.\n"
        "4. Use `write_file` sempre com o CONTEÚDO COMPLETO do arquivo (sem '...' ou trechos truncados).\n"
        "5. Para alterar o jogo compilável multiplataforma (APK Android, IPA iOS, EXE Windows e Linux), atualize `res://scripts/GameWorld.cs` mantendo a assinatura pública:\n"
        "   - `namespace Astron.Game`\n"
        "   - `ColorRgba`, `GameInput` (`Left`, `Right`, `Jump`), `IRenderer` (`Clear`, `FillRect`, `FillCircle`, `DrawText`), `RectF`\n"
        "   - `public sealed class GameWorld` com `public const float Width = 1280f; public const float Height = 720f;`, `public void Update(GameInput input, float dt)` e `public void Render(IRenderer r)`.\n"
        "6. Você também pode criar componentes C# modulares em `res://scripts/*.cs`, shaders GLSL 3.30 em `res://shaders/*.glsl` e popular a cena 3D com `spawn_entity`.\n"
        "7. Termine chamando `finish` com um resumo objetivo do que foi implementado.\n\n"
        "Arquivos atuais em " + m_pm.currentProject().name + ":\n" + tree.str();
}

void AIAgent::startPromptAsync(const AIConfig& cfg, const std::string& userPrompt) {
    if (m_running.exchange(true)) return;

    if (m_worker.joinable()) {
        m_worker.join();
    }

    m_worker = std::thread([this, cfg, userPrompt]() {
        auto pushEvt = [this](const AgentEvent& e) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_eventQueue.push_back(e);
        };
        runTurnSync(cfg, userPrompt, pushEvt);
        m_running.store(false);
    });
}

AgentTurnResult AIAgent::runTurnSync(
    const AIConfig& cfg,
    const std::string& userPrompt,
    const std::function<void(const AgentEvent&)>& onEvent
) {
    AgentTurnResult out;
    auto emitSimple = [&](AgentEvent::Type t, const std::string& txt, const std::string& det = {}) {
        if (onEvent) onEvent(AgentEvent::make(t, txt, det));
    };

    {
        Json u = Json::object();
        u["role"] = "user";
        u["content"] = userPrompt;
        m_history.push_back(std::move(u));
    }

    const Json tools = buildToolsSchema();
    const int maxSteps = cfg.maxSteps > 0 ? cfg.maxSteps : 20;

    try {
        for (int step = 0; step < maxSteps; ++step) {
            emitSimple(AgentEvent::Type::Status, step == 0 ? "Pensando..." : ("Executando passo " + std::to_string(step + 1) + "..."));

            Json reqMsgs = Json::array();
            {
                Json sys = Json::object();
                sys["role"] = "system";
                sys["content"] = buildSystemPrompt();
                reqMsgs.push_back(std::move(sys));
            }
            for (const auto& m : m_history.asArray()) {
                reqMsgs.push_back(m);
            }

            ChatReply reply = AIProviders::chatComplete(cfg, reqMsgs, tools);

            if (!reply.content.empty()) {
                out.finalText = reply.content;
                emitSimple(AgentEvent::Type::AssistantText, reply.content);
            }

            if (reply.toolCalls.empty()) {
                if (!reply.content.empty()) {
                    Json a = Json::object();
                    a["role"] = "assistant";
                    a["content"] = reply.content;
                    m_history.push_back(std::move(a));
                }
                emitSimple(AgentEvent::Type::Done, out.finalText);
                return out;
            }

            // Record assistant tool_calls message in history
            {
                Json a = Json::object();
                a["role"] = "assistant";
                if (!reply.content.empty()) a["content"] = reply.content;
                else a["content"] = nullptr;

                Json tcs = Json::array();
                for (const auto& tc : reply.toolCalls) {
                    Json item = Json::object();
                    item["id"] = tc.id;
                    item["type"] = "function";
                    Json fn = Json::object();
                    fn["name"] = tc.name;
                    fn["arguments"] = tc.args.dump();
                    item["function"] = std::move(fn);
                    tcs.push_back(std::move(item));
                }
                a["tool_calls"] = std::move(tcs);
                m_history.push_back(std::move(a));
            }

            for (const auto& call : reply.toolCalls) {
                std::string toolResult;

                if (call.name == "list_files" || call.name == "list_project_files") {
                    emitSimple(AgentEvent::Type::ToolStart, "list_files");
                    auto list = m_pm.listFiles();
                    std::ostringstream oss;
                    for (const auto& p : list) oss << p << "\n";
                    toolResult = oss.str();
                    emitSimple(AgentEvent::Type::ToolDone, "list_files", std::to_string(list.size()) + " arquivos");
                } else if (call.name == "read_file" || call.name == "read_project_file") {
                    std::string path = m_pm.normalizeResPath(call.args["path"].str());
                    emitSimple(AgentEvent::Type::ToolStart, "read_file", path);
                    bool ok = false;
                    std::string content = m_pm.readFile(path, &ok);
                    toolResult = ok ? content : ("ERRO: arquivo não encontrado: " + path);
                    emitSimple(AgentEvent::Type::ToolDone, "read_file", path);
                } else if (call.name == "write_file" || call.name == "write_csharp_script") {
                    std::string path = m_pm.normalizeResPath(call.args["path"].str());
                    std::string content = call.args["content"].str();
                    emitSimple(AgentEvent::Type::ToolStart, "write_file", path);
                    if (path == "res://" || content.empty()) {
                        toolResult = "ERRO: write_file requer 'path' válido e 'content' não-vazio.";
                        emitSimple(AgentEvent::Type::ToolDone, "write_file", "erro de argumentos");
                    } else if (m_pm.writeFile(path, content)) {
                        out.filesTouched.push_back(path);
                        toolResult = "OK: gravado " + path + " (" + std::to_string(content.size()) + " bytes).";
                        emitSimple(AgentEvent::Type::FileWritten, path, std::to_string(content.size()) + " bytes");
                        emitSimple(AgentEvent::Type::ToolDone, "write_file", path);
                    } else {
                        toolResult = "ERRO ao gravar " + path;
                        emitSimple(AgentEvent::Type::ToolDone, "write_file", "falha de I/O");
                    }
                } else if (call.name == "spawn_entity" || call.name == "spawn_entity_3d") {
                    std::string ename = call.args["name"].strOr("Objeto");
                    std::string mstr = call.args["mesh"].strOr("Cube");
                    emitSimple(AgentEvent::Type::ToolStart, "spawn_entity", ename);

                    AgentEvent se;
                    se.type = AgentEvent::Type::SpawnEntity;
                    se.entityName = ename;
                    se.mesh = Scene::meshFromName(mstr);
                    se.pos = {
                        static_cast<float>(call.args["x"].num(0.0)),
                        static_cast<float>(call.args["y"].num(0.5)),
                        static_cast<float>(call.args["z"].num(0.0))
                    };
                    se.scale = {
                        static_cast<float>(call.args["sx"].num(1.0)),
                        static_cast<float>(call.args["sy"].num(1.0)),
                        static_cast<float>(call.args["sz"].num(1.0))
                    };
                    se.colorHex = call.args["color"].strOr("#FFFFFF");
                    if (onEvent) onEvent(se);

                    toolResult = "OK: entidade 3D '" + ename + "' adicionada à cena.";
                    emitSimple(AgentEvent::Type::ToolDone, "spawn_entity", ename);
                } else if (call.name == "finish") {
                    std::string summary = call.args["summary"].strOr(out.finalText);
                    out.finalText = summary;
                    emitSimple(AgentEvent::Type::ToolStart, "finish");
                    emitSimple(AgentEvent::Type::ToolDone, "finish", summary);

                    Json tm = Json::object();
                    tm["role"] = "tool";
                    tm["tool_call_id"] = call.id;
                    tm["content"] = "Concluído.";
                    m_history.push_back(std::move(tm));

                    emitSimple(AgentEvent::Type::Done, summary);
                    return out;
                } else {
                    toolResult = "ERRO: ferramenta desconhecida: " + call.name;
                }

                Json tm = Json::object();
                tm["role"] = "tool";
                tm["tool_call_id"] = call.id;
                tm["content"] = toolResult;
                m_history.push_back(std::move(tm));
            }
        }

        emitSimple(AgentEvent::Type::Done, out.finalText);
    } catch (const std::exception& ex) {
        out.ok = false;
        out.error = ex.what();
        emitSimple(AgentEvent::Type::Error, ex.what());
    }
    return out;
}

} // namespace astron
