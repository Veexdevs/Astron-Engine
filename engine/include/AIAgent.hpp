#pragma once
#include "AIProviders.hpp"
#include "ProjectManager.hpp"
#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace astron {

struct AgentEvent {
    enum class Type {
        Status,
        AssistantText,
        ToolStart,
        ToolDone,
        SpawnEntity,
        FileWritten,
        Error,
        Done
    };

    Type type = Type::Status;
    std::string text;
    std::string detail;

    // Used when Type::SpawnEntity
    std::string entityName;
    MeshKind mesh = MeshKind::Cube;
    AstronVec3 pos = { 0.0f, 0.5f, 0.0f };
    AstronVec3 scale = { 1.0f, 1.0f, 1.0f };
    std::string colorHex = "#FFFFFF";

    static AgentEvent make(Type t, std::string txt = {}, std::string det = {}) {
        AgentEvent e;
        e.type = t;
        e.text = std::move(txt);
        e.detail = std::move(det);
        return e;
    }
};

struct AgentTurnResult {
    std::string finalText;
    std::vector<std::string> filesTouched;
    bool ok = true;
    std::string error;
};

class AIAgent {
public:
    explicit AIAgent(ProjectManager& projectManager);
    ~AIAgent();

    bool isRunning() const { return m_running.load(); }

    // Starts an autonomous multi-step turn on a background C++ worker thread.
    void startPromptAsync(const AIConfig& cfg, const std::string& userPrompt);

    // Synchronous execution (used internally by worker thread and by C++ unit tests).
    AgentTurnResult runTurnSync(
        const AIConfig& cfg,
        const std::string& userPrompt,
        const std::function<void(const AgentEvent&)>& onEvent = nullptr
    );

    // Drain queued events on the UI/OpenGL main thread.
    std::vector<AgentEvent> pollEvents();

    void clearConversation();

    static Json buildToolsSchema();

private:
    std::string buildSystemPrompt() const;

    ProjectManager& m_pm;
    Json m_history;
    mutable std::mutex m_mutex;
    std::vector<AgentEvent> m_eventQueue;
    std::atomic<bool> m_running{ false };
    std::thread m_worker;
};

} // namespace astron
