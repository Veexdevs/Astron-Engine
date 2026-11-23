#pragma once
#include "Json.hpp"
#include <string>
#include <vector>

namespace astron {

struct ProviderPreset {
    std::string id;
    std::string label;
    std::string kind;   // "openai" | "anthropic"
    std::string baseUrl;
    std::vector<std::string> models;
    std::string hint;
};

struct AIConfig {
    std::string provider = "openrouter";
    std::string apiKey;
    std::string baseUrl = "https://openrouter.ai/api/v1";
    std::string model = "openai/gpt-4o-mini";
    float temperature = 0.35f;
    int maxSteps = 24;
};

struct ToolCall {
    std::string id;
    std::string name;
    Json args;
};

struct ChatReply {
    std::string content;
    std::vector<ToolCall> toolCalls;
};

class AIProviders {
public:
    static const std::vector<ProviderPreset>& presets();
    static const ProviderPreset& presetById(const std::string& id);

    // Calls OpenRouter, OpenAI, Anthropic, Groq, Mistral, DeepSeek, Gemini, or any custom endpoint.
    static ChatReply chatComplete(const AIConfig& cfg, const Json& messages, const Json& tools);
};

} // namespace astron
