using System;
using System.Collections.Generic;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Text;
using System.Threading.Tasks;
using Astron.Engine;

namespace Astron.Engine.Mobile
{
    public sealed class MobileAIProviderPreset
    {
        public string Id;
        public string Name;
        public string BaseUrl;
        public string DefaultModel;
    }

    public sealed class MobileChatMessage
    {
        public bool IsUser;
        public string Text;
        public List<string> ToolSummaries = new List<string>();
    }

    /// <summary>
    /// Agente IA Autônomo nativo em C# para Android (.apk) e iOS (.ipa).
    /// Suporta OpenRouter, OpenAI, Anthropic, Groq, Mistral, DeepSeek, Gemini e endpoints customizados,
    /// executando tool-calling autônomo (list_files, read_file, write_file, spawn_entity).
    /// </summary>
    public sealed class MobileAIAgent
    {
        private static readonly HttpClient SharedHttp = new HttpClient { Timeout = TimeSpan.FromSeconds(75) };

        public static readonly MobileAIProviderPreset[] Presets = new[]
        {
            new MobileAIProviderPreset { Id = "openrouter", Name = "OpenRouter (Todos os Modelos)", BaseUrl = "https://openrouter.ai/api/v1", DefaultModel = "openai/gpt-4o-mini" },
            new MobileAIProviderPreset { Id = "openai",     Name = "OpenAI",                        BaseUrl = "https://api.openai.com/v1",     DefaultModel = "gpt-4o-mini" },
            new MobileAIProviderPreset { Id = "groq",       Name = "Groq (Ultra Rápido)",           BaseUrl = "https://api.groq.com/openai/v1", DefaultModel = "llama-3.3-70b-versatile" },
            new MobileAIProviderPreset { Id = "deepseek",   Name = "DeepSeek",                      BaseUrl = "https://api.deepseek.com/v1",   DefaultModel = "deepseek-chat" },
            new MobileAIProviderPreset { Id = "gemini",     Name = "Google Gemini (OpenAI Compat)", BaseUrl = "https://generativelanguage.googleapis.com/v1beta/openai", DefaultModel = "gemini-2.0-flash" },
        };

        public int ProviderIndex { get; set; }
        public string ApiKey { get; set; } = string.Empty;
        public string BaseUrl { get; set; } = "https://openrouter.ai/api/v1";
        public string Model { get; set; } = "openai/gpt-4o-mini";
        public bool IsBusy { get; private set; }
        public string StatusLine { get; private set; } = "Pronto";

        public readonly List<MobileChatMessage> Messages = new List<MobileChatMessage>();

        public void SelectProvider(int index)
        {
            if (index < 0 || index >= Presets.Length) return;
            ProviderIndex = index;
            BaseUrl = Presets[index].BaseUrl;
            Model = Presets[index].DefaultModel;
        }

        public async Task RunAutonomousTurnAsync(
            string userPrompt,
            MobileProjectStore store,
            AstronWorld3D world,
            Action onUiChanged)
        {
            if (IsBusy || string.IsNullOrWhiteSpace(userPrompt)) return;
            IsBusy = true;
            Messages.Add(new MobileChatMessage { IsUser = true, Text = userPrompt.Trim() });

            var replyMsg = new MobileChatMessage { IsUser = false, Text = string.Empty };
            Messages.Add(replyMsg);
            StatusLine = "Conectando à API de IA...";
            onUiChanged?.Invoke();

            try
            {
                if (string.IsNullOrWhiteSpace(ApiKey))
                {
                    // Modo assistente offline local instantâneo se o usuário ainda não inseriu chave da API,
                    // gerando código C# real e objetos na cena para teste imediato no celular!
                    RunOfflineSmartBuilder(userPrompt, store, world, replyMsg);
                    StatusLine = "Concluído (modo local — adicione API Key p/ OpenRouter)";
                    IsBusy = false;
                    onUiChanged?.Invoke();
                    return;
                }

                var conv = JsonVal.NewArray();
                var sys = JsonVal.NewObject();
                sys["role"] = JsonVal.FromStr("system");
                sys["content"] = JsonVal.FromStr(
                    "Você é o Agente IA Autônomo oficial da Astron Engine (100% nativa C++, C#, C, GLSL). " +
                    "Use as ferramentas list_files, read_file, write_file e spawn_entity para criar o jogo completo em C# " +
                    "em res://scripts/ e montar objetos 3D na cena sem pedir ao usuário para programar manualmente. Responda em português."
                );
                conv.Add(sys);

                var usr = JsonVal.NewObject();
                usr["role"] = JsonVal.FromStr("user");
                usr["content"] = JsonVal.FromStr(userPrompt);
                conv.Add(usr);

                JsonVal tools = BuildToolDefinitions();

                for (int step = 0; step < 8; step++)
                {
                    StatusLine = "Agente IA trabalhando (passo " + (step + 1) + ")...";
                    onUiChanged?.Invoke();

                    var reqBody = JsonVal.NewObject();
                    reqBody["model"] = JsonVal.FromStr(Model);
                    reqBody["messages"] = conv;
                    reqBody["tools"] = tools;
                    reqBody["temperature"] = JsonVal.FromNum(0.25);

                    string endpoint = BaseUrl.TrimEnd('/') + "/chat/completions";
                    using (var req = new HttpRequestMessage(HttpMethod.Post, endpoint))
                    {
                        req.Headers.Authorization = new AuthenticationHeaderValue("Bearer", ApiKey.Trim());
                        req.Headers.TryAddWithoutValidation("HTTP-Referer", "https://github.com/Veexdevs/Astron-Engine");
                        req.Headers.TryAddWithoutValidation("X-Title", "Astron Engine Mobile");
                        req.Content = new StringContent(reqBody.Dump(), Encoding.UTF8, "application/json");

                        using (HttpResponseMessage resp = await SharedHttp.SendAsync(req).ConfigureAwait(false))
                        {
                            string raw = await resp.Content.ReadAsStringAsync().ConfigureAwait(false);
                            if (!resp.IsSuccessStatusCode)
                            {
                                replyMsg.Text = "Erro HTTP " + (int)resp.StatusCode + ": " + Truncate(raw, 200);
                                break;
                            }

                            JsonVal parsed = JsonVal.Parse(raw);
                            JsonVal choices = parsed["choices"];
                            if (choices.Type != JsonVal.Kind.Array || choices.ArrVal.Count == 0)
                            {
                                replyMsg.Text = "Resposta inesperada da API.";
                                break;
                            }

                            JsonVal message = choices.ArrVal[0]["message"];
                            conv.Add(message);

                            string contentText = message["content"].AsString();
                            if (!string.IsNullOrWhiteSpace(contentText))
                                replyMsg.Text = contentText;

                            JsonVal toolCalls = message["tool_calls"];
                            if (toolCalls.Type != JsonVal.Kind.Array || toolCalls.ArrVal.Count == 0)
                                break;

                            for (int t = 0; t < toolCalls.ArrVal.Count; t++)
                            {
                                JsonVal tc = toolCalls.ArrVal[t];
                                string callId = tc["id"].AsString("call_" + t);
                                string fnName = tc["function"]["name"].AsString();
                                JsonVal fnArgs = JsonVal.Parse(tc["function"]["arguments"].AsString("{}"));

                                string toolResult = ExecuteTool(fnName, fnArgs, store, world, replyMsg);
                                onUiChanged?.Invoke();

                                var toolMsg = JsonVal.NewObject();
                                toolMsg["role"] = JsonVal.FromStr("tool");
                                toolMsg["tool_call_id"] = JsonVal.FromStr(callId);
                                toolMsg["content"] = JsonVal.FromStr(toolResult);
                                conv.Add(toolMsg);
                            }
                        }
                    }
                }

                if (string.IsNullOrWhiteSpace(replyMsg.Text))
                    replyMsg.Text = "Arquivos C# e objetos 3D atualizados com sucesso no projeto.";
                StatusLine = "Pronto";
            }
            catch (Exception ex)
            {
                replyMsg.Text = "Falha na requisição: " + ex.Message;
                StatusLine = "Erro de conexão";
            }
            finally
            {
                IsBusy = false;
                onUiChanged?.Invoke();
            }
        }

        private static string ExecuteTool(
            string fnName,
            JsonVal args,
            MobileProjectStore store,
            AstronWorld3D world,
            MobileChatMessage uiMsg)
        {
            switch (fnName)
            {
                case "list_files":
                {
                    var arr = JsonVal.NewArray();
                    foreach (string f in store.ListResFiles()) arr.Add(JsonVal.FromStr(f));
                    uiMsg.ToolSummaries.Add("list_files (" + arr.ArrVal.Count + " arquivos)");
                    return arr.Dump();
                }
                case "read_file":
                {
                    string path = args["path"].AsString("res://scripts/GameWorld.cs");
                    uiMsg.ToolSummaries.Add("read_file " + path);
                    return store.ReadFile(path);
                }
                case "write_file":
                {
                    string path = args["path"].AsString("res://scripts/AutoGame.cs");
                    string content = args["content"].AsString();
                    bool ok = store.WriteFile(path, content);
                    uiMsg.ToolSummaries.Add("write_file " + path);
                    return ok ? "{\"ok\":true}" : "{\"ok\":false}";
                }
                case "spawn_entity":
                {
                    string name = args["name"].AsString("ObjetoIA");
                    string mesh = args["mesh"].AsString("Cube");
                    float x = args["x"].AsFloat(0f);
                    float y = args["y"].AsFloat(1f);
                    float z = args["z"].AsFloat(0f);
                    string hex = args["color"].AsString("#3882F6");

                    AstronPrimitiveKind kind = AstronPrimitiveKind.Cube;
                    if (mesh.IndexOf("Sphere", StringComparison.OrdinalIgnoreCase) >= 0) kind = AstronPrimitiveKind.Sphere;
                    else if (mesh.IndexOf("Cylinder", StringComparison.OrdinalIgnoreCase) >= 0) kind = AstronPrimitiveKind.Cylinder;

                    UiColor c = UiColor.ParseHtml(hex, new UiColor(56, 130, 246, 255));
                    uint packed = ((uint)c.R << 24) | ((uint)c.G << 16) | ((uint)c.B << 8) | 0xFFu;

                    var node = world.SpawnPart(name, kind, new Vec3(x, y, z), Vec3.One, AstronMaterialPreset.SmoothPlastic, false);
                    node.ColorRgba = packed;
                    uiMsg.ToolSummaries.Add("spawn_entity " + name);
                    return "{\"ok\":true,\"id\":" + node.Id + "}";
                }
                default:
                    return "{\"error\":\"unknown_tool\"}";
            }
        }

        private static void RunOfflineSmartBuilder(
            string prompt,
            MobileProjectStore store,
            AstronWorld3D world,
            MobileChatMessage replyMsg)
        {
            string lower = prompt.ToLowerInvariant();
            if (lower.Contains("obby") || lower.Contains("plataforma"))
            {
                for (int i = 0; i < 5; i++)
                {
                    var plat = world.SpawnPart(
                        "PlataformaObby_" + (i + 1),
                        AstronPrimitiveKind.Cube,
                        new Vec3(i * 2.4f - 4.0f, 0.8f + i * 0.65f, -i * 1.2f),
                        new Vec3(1.8f, 0.35f, 1.8f),
                        AstronMaterialPreset.Neon,
                        true
                    );
                    plat.ColorRgba = (i % 2 == 0) ? 0x3882F6FFu : 0x2FD158FFu;
                }
                var orb = world.SpawnPart("TrofeuFinal", AstronPrimitiveKind.Sphere, new Vec3(6.2f, 4.5f, -5.0f), Vec3.One, AstronMaterialPreset.Neon, true);
                orb.ColorRgba = 0xFFD43BFFu;

                store.WriteFile("res://scripts/ObbyController.cs",
                    "using Astron.Engine;\n\n" +
                    "namespace Astron.Scripts\n{\n" +
                    "    public sealed class ObbyController\n    {\n" +
                    "        public int Checkpoints = 5;\n" +
                    "        public float JumpBoost = 12.5f;\n" +
                    "    }\n}\n");

                replyMsg.ToolSummaries.Add("write_file res://scripts/ObbyController.cs");
                replyMsg.ToolSummaries.Add("spawn_entity 5x PlataformaObby + TrofeuFinal");
                replyMsg.Text = "Criei automaticamente o percurso Obby 3D na cena e gravei res://scripts/ObbyController.cs em C#! (Configure sua API Key em ⚙ para conectar ao OpenRouter.)";
            }
            else
            {
                var hero = world.SpawnPart("NaveJogador", AstronPrimitiveKind.Cube, new Vec3(0f, 1.4f, 0f), new Vec3(1.4f, 0.7f, 1.8f), AstronMaterialPreset.Metal, false);
                hero.ColorRgba = 0x4EA1FFFFu;
                var enemy = world.SpawnPart("DroneInimigo", AstronPrimitiveKind.Sphere, new Vec3(2.8f, 2.0f, -2.2f), Vec3.One, AstronMaterialPreset.Neon, true);
                enemy.ColorRgba = 0xFF3B5CFFu;

                store.WriteFile("res://scripts/AutoGeneratedGame.cs",
                    "using Astron.Engine;\n\n" +
                    "namespace Astron.Scripts\n{\n" +
                    "    public sealed class AutoGeneratedGame\n    {\n" +
                    "        public string Prompt = \"" + prompt.Replace("\"", "'") + "\";\n" +
                    "        public void Tick(AstronWorld3D world, float dt) => world.StepPhysics(dt);\n" +
                    "    }\n}\n");

                replyMsg.ToolSummaries.Add("write_file res://scripts/AutoGeneratedGame.cs");
                replyMsg.ToolSummaries.Add("spawn_entity NaveJogador + DroneInimigo");
                replyMsg.Text = "Gerei res://scripts/AutoGeneratedGame.cs em C# e instanciei NaveJogador e DroneInimigo na cena 3D! (Adicione sua API Key em ⚙ Configurações para usar qualquer modelo do OpenRouter ao vivo.)";
            }
        }

        private static JsonVal BuildToolDefinitions()
        {
            var arr = JsonVal.NewArray();
            arr.Add(MakeTool("list_files", "Lista todos os arquivos em res:// do projeto atual.", new string[0]));
            arr.Add(MakeTool("read_file", "Lê um arquivo de res://.", new[] { "path" }));
            arr.Add(MakeTool("write_file", "Grava código C#, Lua ou shader em res://.", new[] { "path", "content" }));
            arr.Add(MakeTool("spawn_entity", "Cria um objeto 3D na cena ativa.", new[] { "name", "mesh", "color" }));
            return arr;
        }

        private static JsonVal MakeTool(string name, string desc, string[] stringProps)
        {
            var props = JsonVal.NewObject();
            var req = JsonVal.NewArray();
            for (int i = 0; i < stringProps.Length; i++)
            {
                var p = JsonVal.NewObject();
                p["type"] = JsonVal.FromStr("string");
                props[stringProps[i]] = p;
                req.Add(JsonVal.FromStr(stringProps[i]));
            }
            if (name == "spawn_entity")
            {
                foreach (string axis in new[] { "x", "y", "z" })
                {
                    var np = JsonVal.NewObject();
                    np["type"] = JsonVal.FromStr("number");
                    props[axis] = np;
                }
            }

            var parameters = JsonVal.NewObject();
            parameters["type"] = JsonVal.FromStr("object");
            parameters["properties"] = props;
            parameters["required"] = req;

            var fn = JsonVal.NewObject();
            fn["name"] = JsonVal.FromStr(name);
            fn["description"] = JsonVal.FromStr(desc);
            fn["parameters"] = parameters;

            var wrap = JsonVal.NewObject();
            wrap["type"] = JsonVal.FromStr("function");
            wrap["function"] = fn;
            return wrap;
        }

        private static string Truncate(string s, int max) =>
            string.IsNullOrEmpty(s) || s.Length <= max ? s : s.Substring(0, max) + "...";
    }
}
