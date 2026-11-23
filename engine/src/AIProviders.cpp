#include "AIProviders.hpp"
#include "HttpClient.hpp"
#include <stdexcept>

namespace astron {

const std::vector<ProviderPreset>& AIProviders::presets() {
    static const std::vector<ProviderPreset> kPresets = {
        {
            "openrouter",
            "OpenRouter (qualquer modelo)",
            "openai",
            "https://openrouter.ai/api/v1",
            {
                "openai/gpt-4o-mini",
                "openai/gpt-4o",
                "anthropic/claude-3.7-sonnet",
                "anthropic/claude-3.5-sonnet",
                "google/gemini-2.0-flash-001",
                "qwen/qwen-2.5-coder-32b-instruct",
                "deepseek/deepseek-chat",
                "meta-llama/llama-3.3-70b-instruct",
                "mistralai/mistral-large"
            },
            "openrouter.ai/api/keys — centenas de modelos com uma só chave"
        },
        {
            "openai",
            "OpenAI",
            "openai",
            "https://api.openai.com/v1",
            { "gpt-4o-mini", "gpt-4o", "o3-mini", "gpt-4.1-mini" },
            "platform.openai.com/api-keys"
        },
        {
            "anthropic",
            "Anthropic (Claude)",
            "anthropic",
            "https://api.anthropic.com",
            { "claude-sonnet-4-5", "claude-opus-4-1", "claude-3-5-haiku-latest" },
            "console.anthropic.com"
        },
        {
            "groq",
            "Groq",
            "openai",
            "https://api.groq.com/openai/v1",
            { "llama-3.3-70b-versatile", "qwen-2.5-coder-32b", "llama-3.1-8b-instant" },
            "console.groq.com/keys"
        },
        {
            "mistral",
            "Mistral / Codestral",
            "openai",
            "https://api.mistral.ai/v1",
            { "codestral-latest", "mistral-large-latest", "mistral-small-latest" },
            "console.mistral.ai/api-keys"
        },
        {
            "deepseek",
            "DeepSeek",
            "openai",
            "https://api.deepseek.com/v1",
            { "deepseek-chat", "deepseek-reasoner" },
            "platform.deepseek.com/api_keys"
        },
        {
            "gemini",
            "Google Gemini",
            "openai",
            "https://generativelanguage.googleapis.com/v1beta/openai",
            { "gemini-2.0-flash", "gemini-2.5-flash", "gemini-1.5-pro" },
            "aistudio.google.com/apikey"
        },
        {
            "custom",
            "Personalizada / Local (OpenAI API)",
            "openai",
            "http://localhost:11434/v1",
            { "qwen2.5-coder", "llama3.2", "deepseek-coder-v2" },
            "Qualquer endpoint compatível com /chat/completions (Ollama, LM Studio, vLLM...)"
        }
    };
    return kPresets;
}

const ProviderPreset& AIProviders::presetById(const std::string& id) {
    for (const auto& p : presets()) {
        if (p.id == id) return p;
    }
    return presets().back();
}

namespace {

std::string trimSlashes(std::string s) {
    while (!s.empty() && s.back() == '/') s.pop_back();
    return s;
}

void checkHttpError(const HttpResponse& res) {
    if (!res.error.empty()) {
        throw std::runtime_error(res.error);
    }
    if (res.status >= 200 && res.status < 300) return;

    std::string detail = res.body.substr(0, 400);
    Json parsed = Json::parse(res.body);
    if (parsed.isObject()) {
        if (parsed["error"].isObject() && parsed["error"]["message"].isString()) {
            detail = parsed["error"]["message"].str();
        } else if (parsed["message"].isString()) {
            detail = parsed["message"].str();
        }
    }

    if (res.status == 401 || res.status == 403) {
        throw std::runtime_error("Chave de API recusada (" + std::to_string(res.status) +
                                 "). Verifique em Configurações → Agente IA. " + detail);
    }
    if (res.status == 429) {
        throw std::runtime_error("Limite de requisições atingido (429): " + detail);
    }
    throw std::runtime_error("Erro HTTP " + std::to_string(res.status) + " na API de IA: " + detail);
}

ChatReply callOpenAICompat(const AIConfig& cfg, const Json& messages, const Json& tools) {
    std::string url = trimSlashes(cfg.baseUrl) + "/chat/completions";

    Json payload = Json::object();
    payload["model"] = cfg.model;
    payload["messages"] = messages;
    if (tools.isArray() && tools.size() > 0) {
        payload["tools"] = tools;
    }
    payload["temperature"] = static_cast<double>(cfg.temperature);
    payload["max_tokens"] = 4096;

    HttpClient::HeaderList headers = {
        { "Content-Type", "application/json" },
        { "Authorization", "Bearer " + cfg.apiKey }
    };
    if (cfg.provider == "openrouter" || cfg.baseUrl.find("openrouter.ai") != std::string::npos) {
        headers.push_back({ "HTTP-Referer", "https://github.com/Veexdevs/Astron-Engine" });
        headers.push_back({ "X-Title", "Astron Engine" });
    }

    HttpResponse res = HttpClient::postJson(url, headers, payload.dump(), 120);
    checkHttpError(res);

    std::string parseErr;
    Json data = Json::parse(res.body, &parseErr);
    if (!parseErr.empty()) {
        throw std::runtime_error("Resposta JSON inválida da API: " + parseErr);
    }

    ChatReply reply;
    const Json& msg = data["choices"].at(0)["message"];
    if (msg["content"].isString()) {
        reply.content = msg["content"].str();
    }

    const Json& tcs = msg["tool_calls"];
    if (tcs.isArray()) {
        int idx = 0;
        for (const auto& tc : tcs.asArray()) {
            ToolCall call;
            call.id = tc["id"].strOr("call_" + std::to_string(++idx));
            call.name = tc["function"]["name"].str();
            const Json& rawArgs = tc["function"]["arguments"];
            if (rawArgs.isString()) {
                call.args = Json::parse(rawArgs.str());
                if (!call.args.isObject()) call.args = Json::object();
            } else if (rawArgs.isObject()) {
                call.args = rawArgs;
            } else {
                call.args = Json::object();
            }
            reply.toolCalls.push_back(std::move(call));
        }
    }
    return reply;
}

ChatReply callAnthropic(const AIConfig& cfg, const Json& messages, const Json& tools) {
    std::string url = trimSlashes(cfg.baseUrl) + "/v1/messages";

    std::string systemPrompt;
    Json rest = Json::array();

    if (messages.isArray()) {
        for (const auto& m : messages.asArray()) {
            std::string role = m["role"].str();
            if (role == "system") {
                systemPrompt = m["content"].str();
                continue;
            }
            if (role == "tool") {
                Json block = Json::object();
                block["type"] = "tool_result";
                block["tool_use_id"] = m["tool_call_id"].str();
                block["content"] = m["content"].str();

                auto& arr = rest.asArray();
                if (!arr.empty() && arr.back()["role"].str() == "user" && arr.back()["content"].isArray()) {
                    arr.back()["content"].push_back(std::move(block));
                } else {
                    Json u = Json::object();
                    u["role"] = "user";
                    Json blocks = Json::array();
                    blocks.push_back(std::move(block));
                    u["content"] = std::move(blocks);
                    rest.push_back(std::move(u));
                }
                continue;
            }
            if (role == "assistant" && m["tool_calls"].isArray()) {
                Json blocks = Json::array();
                if (m["content"].isString() && !m["content"].str().empty()) {
                    Json tb = Json::object();
                    tb["type"] = "text";
                    tb["text"] = m["content"].str();
                    blocks.push_back(std::move(tb));
                }
                for (const auto& tc : m["tool_calls"].asArray()) {
                    Json tu = Json::object();
                    tu["type"] = "tool_use";
                    tu["id"] = tc["id"].str();
                    tu["name"] = tc["function"]["name"].str();
                    Json parsedArgs = Json::parse(tc["function"]["arguments"].str());
                    tu["input"] = parsedArgs.isObject() ? parsedArgs : Json::object();
                    blocks.push_back(std::move(tu));
                }
                Json a = Json::object();
                a["role"] = "assistant";
                a["content"] = std::move(blocks);
                rest.push_back(std::move(a));
                continue;
            }
            Json item = Json::object();
            item["role"] = role;
            item["content"] = m["content"].str();
            rest.push_back(std::move(item));
        }
    }

    Json anthTools = Json::array();
    if (tools.isArray()) {
        for (const auto& t : tools.asArray()) {
            Json ft = Json::object();
            ft["name"] = t["function"]["name"].str();
            ft["description"] = t["function"]["description"].str();
            ft["input_schema"] = t["function"]["parameters"];
            anthTools.push_back(std::move(ft));
        }
    }

    Json payload = Json::object();
    payload["model"] = cfg.model;
    payload["max_tokens"] = 4096;
    payload["temperature"] = static_cast<double>(cfg.temperature);
    if (!systemPrompt.empty()) payload["system"] = systemPrompt;
    payload["messages"] = std::move(rest);
    if (anthTools.size() > 0) payload["tools"] = std::move(anthTools);

    HttpClient::HeaderList headers = {
        { "Content-Type", "application/json" },
        { "x-api-key", cfg.apiKey },
        { "anthropic-version", "2023-06-01" }
    };

    HttpResponse res = HttpClient::postJson(url, headers, payload.dump(), 120);
    checkHttpError(res);

    std::string parseErr;
    Json data = Json::parse(res.body, &parseErr);
    if (!parseErr.empty()) {
        throw std::runtime_error("Resposta JSON inválida da Anthropic: " + parseErr);
    }

    ChatReply reply;
    const Json& content = data["content"];
    if (content.isArray()) {
        for (const auto& b : content.asArray()) {
            std::string btype = b["type"].str();
            if (btype == "text") {
                if (!reply.content.empty()) reply.content += "\n";
                reply.content += b["text"].str();
            } else if (btype == "tool_use") {
                ToolCall call;
                call.id = b["id"].str();
                call.name = b["name"].str();
                call.args = b["input"].isObject() ? b["input"] : Json::object();
                reply.toolCalls.push_back(std::move(call));
            }
        }
    }
    return reply;
}

} // namespace

ChatReply AIProviders::chatComplete(const AIConfig& cfg, const Json& messages, const Json& tools) {
    if (cfg.apiKey.empty() && cfg.provider != "custom") {
        throw std::runtime_error(
            "Nenhuma chave de API configurada. Abra Configurações → Agente IA e cole sua chave (ex.: OpenRouter)."
        );
    }
    const auto& preset = presetById(cfg.provider);
    if (preset.kind == "anthropic") {
        return callAnthropic(cfg, messages, tools);
    }
    return callOpenAICompat(cfg, messages, tools);
}

} // namespace astron
