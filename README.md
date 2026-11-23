![Astron Engine](assets/textures/astron-lockup.png)

# 🪐 Astron Engine

**Engine nativa multi-linguagem 2D/3D construída em C++20 · C# · C11 · GLSL · HLSL · Rust · Lua · CMake**  
*100% Nativa para Desktop (**`.exe`** / **Linux x64**) e Mobile (**`.apk`** Android / **`.ipa`** iOS & iPadOS) — zero HTML, zero CSS, zero JavaScript, sem versão Web.*

---

## Plataformas Nativas da Astron Engine (Editor Completo)

| Plataforma | Formato Oficial | Tecnologia Nativa | Artefato CI |
| :--- | :--- | :--- | :--- |
| **Windows x64** | **`.exe`** (`AstronEngine.exe`) | C++20 + C11 + OpenGL 3.3 + WinHTTP + Rust `astron-pak.exe` | `AstronEngine-windows-x64` |
| **Linux x64** | **Binário ELF x64** (`AstronEngine`) | C++20 + C11 + OpenGL 3.3 + libcurl + Rust `astron-pak` | `AstronEngine-linux-x64` |
| **Android** | **`.apk` Assinado** (`AstronEngine-release.apk`) | C# `.NET 10 Android` (`Android.Graphics.Canvas` + Touch 3D + HTTP Nativo) | `AstronEngine-android-apk` |
| **iOS / iPadOS** | **`.ipa` arm64** (`AstronEngine-ios.ipa`) | C# `.NET 10 iOS` (`CoreGraphics` + `UIKit` + Gestos Multi-Touch + HTTP Nativo) | `AstronEngine-ios-ipa` |

---

## Arquitetura Nativa Multi-Linguagem

| Subsistema | Linguagem | Diretório | Responsabilidade |
| :--- | :--- | :--- | :--- |
| **Editor Desktop, Renderer 3D & Agente IA** | **C++20** | `engine/include/`, `engine/src/` | Aplicação desktop nativa GLFW + OpenGL 3.3 Core + Dear ImGui, Viewport 3D PBR em Framebuffer (`Viewport3D`), gerenciador `res://` (`ProjectManager`), cliente HTTP nativo (`WinHTTP` / `libcurl`) e **Agente IA Autônomo** (`AIAgent`). |
| **Editor Mobile Nativo (`.apk` & `.ipa`)** | **C# (.NET 10)** | `engine/mobile/` | **Astron Engine completa para Android e iOS/iPadOS** (`Core/`, `Android/`, `iOS/`) sem WebView: todas as 5 telas oficiais, Viewport 3D interativo com órbita/pinça/raycast, sistema `res://` persistente, editor de código nativo e **Agente IA OpenRouter**. |
| **SDK de Jogos & Runtime Multiplataforma** | **C# (.NET)** | `games/AstronGame/` | SDK `Astron.Engine` (`AstronWorld3D`, `SceneNode3D`, `Vec3`, física AABB 2D/3D) compartilhado entre o Editor Mobile e os jogos compilados para **Android (`.apk`)**, **iOS (`.ipa`)**, **Windows (`.exe`)** e **Linux (`x64`)**. |
| **Core Matemático, SIMD & Sintetizador DSP** | **C11** | `engine/core_c/` | Matrizes 4×4 (`astron_math.c`), aceleração por hardware **SSE2 / ARM64 NEON** (`astron_simd.c`), sintetizador de áudio PCM 16-bit `.wav` (`astron_audio.c`), loader dinâmico OpenGL (`astron_gl.c`) e telemetria de SO (`astron_platform.c`). |
| **Shaders GPU OpenGL 3.3 Core** | **GLSL** | `shaders/*.{vert,frag,glsl}` | Céu procedural + grade anti-aliased (`sky_grid`), Cook-Torrance GGX PBR (`pbr_metallic`), holograma sci-fi (`hologram_cyber`), oceano Gerstner (`water_waves`), cel-shading (`toon_cel`) e pós-processamento ACES + Bloom (`post_aces_bloom`). |
| **Shaders DirectX 11 / 12 (Windows)** | **HLSL** | `shaders/hlsl/*.hlsl` | Pipeline Direct3D Shader Model 5.0 PBR (`AstronStandardPBR.hlsl`) e simulação de partículas em Compute Shader (`AstronParticlesCS.hlsl`). |
| **Empacotador Binário `.apak` & FFI** | **Rust** | `engine/tools/astron_pak_rs/` | Ferramenta nativa `astron-pak` e biblioteca estática FFI C para empacotamento, compressão RLE e verificação de integridade FNV-1a 64-bit de arquivos `.apak`. |
| **Scripting Rápido de Comportamentos** | **Lua 5.4** | `engine/scripting_lua/` | Scripts reativos leves (`main.lua` / `astron_behaviors.lua`) para plataformas móveis, coletáveis e ciclos de iluminação. |
| **Sistema de Build Nativo** | **CMake** | `CMakeLists.txt` | Orquestração de compilação C11 + C++20 multiplataforma (Linux GCC/Ninja e Windows MSVC x64) e suíte de testes `ctest`. |

---

## Identidade Oficial & Telas Nativas (Desktop & Mobile)

1. **Splash Screen** — Fundo espacial escuro, logotipo planetário oficial da **Astron Engine** (`assets/textures/astron-lockup.png`), barra de progresso e transição automática.
2. **Tela de Login** — Horizonte planetário, **Astron Engine** (`Crie. Desenvolva. Explore.`), cartão `Bem-vindo(a)!`, autenticação GitHub e **Configurações** de IA.
3. **Hub Inicial (`Início`)** — Barra lateral com a logo horizontal oficial da **Astron Engine**, seções `Início`, `Meus Projetos`, `Bibioteca`, `Loja`, `Configurações` e `Sobre`, banner `+ Novo Projeto` e `Projetos Recentes` (`Meu Jogo`, `Projeto RP`, `Obby Teste`).
4. **Tela de Compilação (`Compilar`)** — Exportação exclusivamente nativa: **APK** (Android), **IPA** (iOS), **EXE** (Windows), **Linux x64**, **ZIP** e **Outros** (macOS / C++), além de **Build Rápida**.
5. **Editor 3D Completo (`Astron Engine`)** — Cabeçalho superior com o ícone planetário oficial da **Astron Engine** (`astron-icon.png`) e menu **`Astron Engine ▾`**, ferramentas 3D (`Selecionar`, `Mover`, `Rotacionar`, `Escalar`, `Criar`, `Câmera`, `2D/3D`, `Jogar`, `Pausar`, `Parar`), `Explorer`, `Arquivos (res://)`, Viewport 3D com gizmo RGB, editor C#/Lua/GLSL, `Propriedades`, `Caixa de Ferramentas`, `Console`, `Depurador`, `Áudio`, `Shader` e **Agente IA**.

---

## Agente IA Autônomo (Qualquer API até OpenRouter)

Disponível tanto no Desktop (`C++20`) quanto no Mobile **`.apk` / `.ipa`** (`C# HttpClient`), conectando-se diretamente a qualquer provedor compatível até o **OpenRouter**:

- **OpenRouter** (`https://openrouter.ai/api/v1` — Claude, GPT, Gemini, DeepSeek, Llama, Qwen, Mistral...)
- **OpenAI**, **Anthropic**, **Groq**, **Mistral**, **DeepSeek**, **Google Gemini** ou endpoint customizado.
- **Execução 100% Autônoma**: o Agente lê a estrutura do projeto (`list_files`, `read_file`), escreve scripts **C#** diretamente em `res://scripts/` (`write_file`) e instancia entidades 3D na cena (`spawn_entity`) automaticamente.

---

## Compilando Localmente

### Linux x64 (GCC + Ninja + Cargo)
```bash
sudo apt-get install -y build-essential cmake ninja-build \
  libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libxi-dev libcurl4-openssl-dev

cargo build --release --manifest-path engine/tools/astron_pak_rs/Cargo.toml
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/AstronEngine
```

### Windows x64 (Visual Studio 2022 / MSVC + Cargo)
```powershell
cargo build --release --manifest-path engine/tools/astron_pak_rs/Cargo.toml
cmake -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
.\build\Release\AstronEngine.exe
```

### Android (`.apk` — .NET 10 + JDK 21)
```bash
dotnet workload install android
dotnet publish engine/mobile/Android/AstronEngine.Android.csproj -c Release -f net10.0-android -p:AndroidPackageFormat=apk
```

### iOS / iPadOS (`.ipa` — macOS + Xcode + .NET 10)
```bash
dotnet workload install ios
dotnet build engine/mobile/iOS/AstronEngine.iOS.csproj -c Release -f net10.0-ios -r ios-arm64
```

---

## Pipelines GitHub Actions

- **`.github/workflows/build-engine.yml`** (`🪐 Build Astron Engine`) — Compila e empacota a **Astron Engine** completa para **Windows x64 (`.exe`)**, **Linux x64**, **Android (`.apk` assinado)** e **iOS/iPadOS (`.ipa` arm64)**, além de publicar todos os pacotes automaticamente no GitHub Releases em tags `v*`.
- **`.github/workflows/build-game.yml`** (`🎮 Build do Jogo`) — Compila os jogos criados em **C#** gerando pacotes nativos para **Android (`.apk` assinado)**, **iOS (`.ipa`)**, **Windows (`.exe`)** e **Linux (`.tar.gz`)**.
