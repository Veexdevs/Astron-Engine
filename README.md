# 🪐 Astron Engine

> **Crie. Desenvolva. Explore.**
> Engine de criação de jogos com editor 3D, **Agente IA** que escreve o código **C#** por você e builds automáticos de **APK · IPA · EXE · Linux** via **GitHub Actions**.

---

## 📸 Telas (fiéis ao design)

| Tela | Descrição |
|------|-----------|
| **Splash** | Logo Astron + barra `Carregando...` |
| **Login** | `Bem-vindo(a)!` — usuário/senha, Lembrar de mim, GitHub, v1.0.0 |
| **Home** | Início · Meus Projetos · Bibioteca · Loja · Configurações · Sobre + Projetos Recentes |
| **Compilar** | APK · IPA · EXE · Web · ZIP · Outros + Projeto Atual + Build Rápida |
| **Editor** | Menu, Toolbar, Explorer, Arquivos `res://`, Viewport 3D, Propriedades, Caixa de Ferramentas, Console e **Agente IA** |

---

## 🚀 Rodando o editor

```bash
npm install
npm run dev      # http://localhost:5173
npm run build    # build de produção (dist/)
```

---

## 🤖 Agente IA (chat no editor)

Abra o editor → aba **`Agente IA`** no painel inferior (ou `Ctrl+I` / menu **Script → Agente IA**).

- **Funciona com qualquer API**: OpenRouter, OpenAI, Anthropic, Groq, Mistral, DeepSeek, Google Gemini (compatível OpenAI) e endpoints locais (Ollama, LM Studio...).
- Configure em **⚙ Configurações → Agente IA**: provedor, chave, modelo, URL base, temperatura e transporte (proxy do editor recomendado — sem CORS).
- O Agente usa **function calling** com as ferramentas `list_files`, `read_file`, `write_file` e `finish` — ele **planeja, cria os arquivos `.cs` e coloca o código dentro do projeto sozinho**, sem sua ajuda.
- Cada `write_file` aparece no chat, no **Console** e nas **Abas de código**, e o arquivo entra automaticamente na árvore `res://`.
- A chave fica **apenas no seu navegador** (localStorage).

> Dica: no OpenRouter, pegue a chave em <https://openrouter.ai/api/keys> e cole em Configurações.

---

## 🎮 Jogos em C#

Os jogos são programados em **C# (.NET 8)** na pasta `games/AstronGame/`:

```
games/AstronGame/
├── Core/       # AstronGame.Core — lógica compartilhada (GameWorld, IRenderer, GameInput)
├── Desktop/    # EXE Windows + Linux (OpenTK / OpenGL)
├── Android/    # APK  (net8.0-android, View + Canvas)
└── iOS/        # IPA  (net8.0-ios, UIView + CoreGraphics)
```

A mesma `GameWorld` (plataforma com pulo, moedas, física e HUD) roda em todas as plataformas; cada *head* só implementa o `IRenderer` e a entrada.

---

## ⚙️ GitHub Actions (builds)

Workflow **`.github/workflows/build-game.yml`**:

| Job | Runner | Saída |
|-----|--------|-------|
| `windows` | `windows-latest` | `AstronGame.exe` (self-contained single-file) |
| `linux` | `ubuntu-latest` | `AstronGame-linux-x64.tar.gz` |
| `android` | `windows-latest` (JDK 17 + workload android) | `AstronGame-release.apk` **assinado** |
| `ios` | `macos-latest` (workload ios) | `AstronGame-ios.ipa` |
| `release` | tags `v*` | Publica tudo na Release do GitHub |

Dispare manualmente: **Actions → 🎮 Build do Jogo → Run workflow** escolhendo `all | android | ios | windows | linux`.

O CI do editor (`.github/workflows/editor-ci.yml`) roda `npm ci && npm run build` a cada push/PR.

---

## 📁 Estrutura do repositório

```
├── public/                 # assets (planeta hero, thumbs, etc.)
├── src/
│   ├── screens/            # Splash, Login, Home, Build, Editor, Settings
│   ├── editor/             # Menu, Toolbar, Explorer, Viewport, Propriedades,
│   │                       # Caixa de Ferramentas, Console, CodeEditor, AIAgent
│   ├── ai/                 # providers.js (qualquer API) + agent.js (loop autônomo)
│   ├── components/         # Logo, Toggle, WindowChrome, modais
│   └── styles/             # global.css, screens.css, editor.css
├── games/AstronGame/       # jogo C# (.NET 8) multiplataforma
└── .github/workflows/      # build-game.yml + editor-ci.yml
```

---

## 🛠 Stack

- **Editor:** React 18 · Vite 6 · Three.js · lucide-react · fontes Inter/Michroma/JetBrains Mono
- **Jogos:** C# · .NET 8 · OpenTK (desktop) · Android View · UIKit/CoreGraphics (iOS)
- **CI/CD:** GitHub Actions

---

Feito com 🪐 para a comunidade de dev de jogos.
