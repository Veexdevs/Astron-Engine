# 🪐 Astron Engine

**Astron Engine** é uma engine nativa multi-linguagem para criação e compilação de jogos 2D/3D, construída **100% de forma nativa** em **C++20**, **C11**, **GLSL 3.30 Core** e **C# (.NET 8)** — **sem nenhuma versão HTML/Web**.

---

## Arquitetura Multi-Linguagem

| Camada | Linguagem | Diretório | Responsabilidade |
| :--- | :--- | :--- | :--- |
| **Core Matemático & Plataforma** | **C11** | `engine/core_c/` | Matrizes 4×4 (`AstronMat4`), vetores, projeção perspectiva/ortográfica, raycast 3D AABB (`astron_ray_intersect_aabb`), loader dinâmico OpenGL 3.3 Core (`astron_gl.c`) e telemetria nativa de memória/processo (`astron_platform.c`). |
| **Editor Desktop, UI & Agente IA** | **C++20** | `engine/include/`, `engine/src/` | Aplicação desktop GLFW + OpenGL 3.3 + Dear ImGui, renderizador 3D PBR em Framebuffer (`Viewport3D`), gerenciador de projetos `res://` (`ProjectManager`), cliente HTTP nativo (`WinHTTP` / `libcurl`) e **Agente IA Autônomo** (`AIAgent`). |
| **Shaders GPU** | **GLSL 3.30** | `shaders/` | Céu procedural + grade infinita anti-aliased (`sky_grid.*`), iluminação PBR/Blinn-Phong multi-material (`pbr_lit.*`), gizmos 3D RGB (`gizmo.*`) e compilação ao vivo no **Editor de Shader**. |
| **Jogos & Runtime Multiplataforma** | **C# (.NET 8)** | `games/AstronGame/` | Jogos escritos em C# (`Astron.Engine` + `Astron.Game`) compilados nativamente para **Android (`.apk`)**, **iOS (`.ipa`)**, **Windows (`.exe`)** e **Linux (`x64`)**. |

---

## Telas Nativas da Engine

1. **Splash Screen** — Fundo espacial escuro, logotipo planetário da **Astron Engine**, barra de carregamento nativa e transição automática.
2. **Tela de Login** — Horizonte planetário, **Astron Engine** (`Crie. Desenvolva. Explore.`), formulário `Bem-vindo(a)!`, login GitHub e acesso às **Configurações** de API.
3. **Hub Inicial (`Início`)** — Navegação lateral (`Início`, `Meus Projetos`, `Bibioteca`, `Loja`, `Configurações`, `Sobre`), banner `+ Novo Projeto` e cartões de `Projetos Recentes` (`Meu Jogo`, `Projeto RP`, `Obby Teste`).
4. **Tela de Compilação (`Compilar`)** — Alvos exclusivamente nativos: **Gerar APK** (Android), **Gerar IPA** (iOS), **Gerar EXE** (Windows), **Build para Linux**, **Exportar ZIP** e **Outras Plataformas** (consoles/embarcados), com **Build Rápida** integrada ao GitHub Actions.
5. **Editor 3D Completo (`Astron Engine`)** — Barra superior com a logo oficial da **Astron Engine** e menu **Astron Engine**, barra de ferramentas (`Selecionar`, `Mover`, `Rotacionar`, `Escalar`, `Transformar`, `Criar`, `Câmera`, `Iluminação`, `Terreno`, `Componente`, `2D/3D`, `Jogar`, `Pausar`, `Parar`), `Explorer`, árvore `res://` em `Arquivos`, Viewport 3D OpenGL interativo com gizmo RGB, editor de código C#, `Propriedades`, `Caixa de Ferramentas`, `Console`, `Editor de Shader` GLSL e **Agente IA**.

---

## Agente IA Autônomo (Qualquer API até OpenRouter)

O painel **Agente IA** (`Ctrl+I` no Editor) conecta-se diretamente via C++ (`WinHTTP` no Windows / `libcurl` no Linux) a qualquer provedor de IA até o **OpenRouter**:

- **OpenRouter** (`https://openrouter.ai/api/v1` — Claude, GPT, Gemini, DeepSeek, Llama, Qwen, Mistral...)
- **OpenAI**, **Anthropic (Claude)**, **Groq**, **Mistral**, **DeepSeek**, **Google Gemini** ou qualquer endpoint compatível (`Custom`).
- **100% Autônomo**: ao pedir um jogo ou mecânica no chat, o Agente inspeciona o projeto (`list_files`, `read_file`), programa e grava os arquivos **C#** diretamente em `res://scripts/` e sincroniza automaticamente com `games/AstronGame/Core/` (`write_file`), além de posicionar objetos 3D na cena (`spawn_entity`) sem exigir nenhuma intervenção manual do usuário.

---

## Compilando Localmente (CMake)

### Linux (Ubuntu / Debian)
```bash
sudo apt-get install -y build-essential cmake ninja-build \
  libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libxi-dev libcurl4-openssl-dev

cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/AstronEngine
```

### Windows (Visual Studio 2022 / MSVC x64)
```powershell
cmake -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
.\build\Release\AstronEngine.exe
```

---

## Pipelines GitHub Actions

- **`.github/workflows/build-engine.yml`** (`🪐 Build Astron Engine`) — Compila o binário nativo da **Astron Engine** em C++20/C11/GLSL para **Windows (`AstronEngine.exe`)** e **Linux (`AstronEngine`)** e executa a suíte nativa de testes (`ctest`).
- **`.github/workflows/build-game.yml`** (`🎮 Build do Jogo`) — Compila os jogos em **C# (.NET 8)** gerando artefatos prontos para **Android (`.apk` assinado)**, **iOS (`.ipa`)**, **Windows (`.exe` single-file)** e **Linux (`.tar.gz`)**.
