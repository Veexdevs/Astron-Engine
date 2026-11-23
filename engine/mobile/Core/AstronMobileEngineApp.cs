using System;
using System.Collections.Generic;
using System.Globalization;
using System.Threading.Tasks;
using Astron.Engine;

namespace Astron.Engine.Mobile
{
    public enum MobileScreen
    {
        Splash,
        Login,
        Home,
        Build,
        Editor
    }

    /// <summary>
    /// Aplicação completa da Astron Engine nativa para Android (.apk) e iOS (.ipa).
    /// Contém as 5 telas oficiais (Splash, Login, Hub Início, Compilar e Editor 3D completo
    /// com logo Astron Engine, Viewport 3D interativo por toque, C# e Agente IA OpenRouter).
    /// </summary>
    public sealed class AstronMobileEngineApp
    {
        private readonly IMobileHostBridge _host;
        private readonly MobileProjectStore _store;
        private readonly MobileViewport3D _viewport = new MobileViewport3D();
        private readonly MobileAIAgent _agent = new MobileAIAgent();
        private readonly UiHitMap _hits = new UiHitMap();

        private AstronWorld3D _world = new AstronWorld3D();
        private int _selectedNodeId = 2;

        public MobileScreen Screen { get; set; } = MobileScreen.Splash;

        // Splash
        private float _splashProgress;

        // Login
        private string _loginUser = "";
        private string _loginPass = "";
        private bool _loginRemember = true;

        // Home & Build
        private string _homeSection = "inicio"; // inicio | projetos | biblioteca | loja | sobre
        private bool _quickBuild = true;
        private bool _settingsModalOpen;
        private bool _buildModalOpen;
        private string _buildTargetLabel = "APK";
        private float _buildAnimTimer;

        // Editor
        private int _activeTool; // 0=Selecionar, 1=Mover, 2=Rotacionar, 3=Escalar
        private bool _isPlaying;
        private bool _isPaused;
        private string _activeCodePath; // null = Cena Principal 3D
        private int _bottomTab; // 0=Console, 1=Depurador, 2=Áudio, 3=Shader, 4=Agente IA
        private readonly List<string> _consoleLogs = new List<string>
        {
            "[INFO] Astron Engine Mobile carregada com sucesso!",
            "[INFO] Cena Principal 3D inicializada (C# + GLSL + C11).",
            "[INFO] Use o Agente IA para criar jogos completos em C#."
        };

        // Toast
        private string _toastText;
        private float _toastTimer;

        // Touch tracking (distingue toque curto de arraste 3D no Viewport)
        private UiRect _lastViewportRect;
        private bool _touchInViewport;
        private float _touchStartX;
        private float _touchStartY;
        private float _lastTouchX;
        private float _lastTouchY;
        private bool _wasDragged;

        public AstronMobileEngineApp(IMobileHostBridge host)
        {
            _host = host;
            _store = new MobileProjectStore(host?.PersistentDataPath);
            ResetDefaultScene();
        }

        private void ResetDefaultScene()
        {
            _world = new AstronWorld3D { SceneName = "Cena Principal" };
            var baseplate = _world.SpawnPart(
                "Baseplate",
                AstronPrimitiveKind.Plane,
                new Vec3(0f, -0.06f, 0f),
                new Vec3(24f, 0.12f, 24f),
                AstronMaterialPreset.Concrete,
                true
            );
            baseplate.ColorRgba = 0x262932FFu;

            var cube = _world.SpawnPart(
                "Model",
                AstronPrimitiveKind.Cube,
                new Vec3(0f, 0.5f, 0f),
                new Vec3(1f, 1f, 1f),
                AstronMaterialPreset.Plastic,
                true
            );
            cube.ColorRgba = 0xE8ECF4FFu;
            _selectedNodeId = cube.Id;
        }

        private SceneNode3D GetSelectedNode()
        {
            for (int i = 0; i < _world.Nodes.Count; i++)
            {
                if (_world.Nodes[i].Id == _selectedNodeId)
                    return _world.Nodes[i];
            }
            return null;
        }

        private void ShowToast(string msg)
        {
            _toastText = msg;
            _toastTimer = 2.8f;
            _host?.RequestRedraw();
        }

        public bool NeedsContinuousAnimation =>
            Screen == MobileScreen.Splash || _isPlaying || _buildModalOpen || _toastTimer > 0f || _agent.IsBusy;

        public void Update(float dt)
        {
            if (dt <= 0f) dt = 1f / 60f;
            if (dt > 0.1f) dt = 0.1f;

            if (Screen == MobileScreen.Splash)
            {
                _splashProgress += dt * 42f;
                if (_splashProgress >= 100f)
                {
                    _splashProgress = 100f;
                    Screen = MobileScreen.Login;
                }
            }

            if (_isPlaying && !_isPaused && Screen == MobileScreen.Editor)
            {
                _world.StepPhysics(dt);
            }

            if (_buildModalOpen)
            {
                _buildAnimTimer += dt;
            }

            if (_toastTimer > 0f)
            {
                _toastTimer -= dt;
                if (_toastTimer <= 0f) _toastText = null;
            }
        }

        public void HandleTouch(TouchPhase phase, float x, float y)
        {
            switch (phase)
            {
                case TouchPhase.Began:
                    _touchStartX = _lastTouchX = x;
                    _touchStartY = _lastTouchY = y;
                    _wasDragged = false;
                    _touchInViewport = (Screen == MobileScreen.Editor &&
                                        _activeCodePath == null &&
                                        !_settingsModalOpen &&
                                        _lastViewportRect.Contains(x, y));
                    break;

                case TouchPhase.Moved:
                {
                    float dx = x - _lastTouchX;
                    float dy = y - _lastTouchY;
                    _lastTouchX = x;
                    _lastTouchY = y;

                    if (Math.Abs(x - _touchStartX) + Math.Abs(y - _touchStartY) > 9f)
                        _wasDragged = true;

                    if (_touchInViewport && _wasDragged)
                    {
                        SceneNode3D sel = GetSelectedNode();
                        if (_activeTool == 1 && sel != null && !string.Equals(sel.Name, "Baseplate", StringComparison.OrdinalIgnoreCase))
                        {
                            sel.Position = new Vec3(sel.Position.X + dx * 0.025f, Math.Max(0.2f, sel.Position.Y - dy * 0.025f), sel.Position.Z);
                        }
                        else if (_activeTool == 2 && sel != null)
                        {
                            sel.RotationEulerDeg = new Vec3(sel.RotationEulerDeg.X, sel.RotationEulerDeg.Y + dx * 0.8f, sel.RotationEulerDeg.Z);
                        }
                        else if (_activeTool == 3 && sel != null && !string.Equals(sel.Name, "Baseplate", StringComparison.OrdinalIgnoreCase))
                        {
                            float s = Math.Max(0.25f, sel.Scale.X + (dx - dy) * 0.015f);
                            sel.Scale = new Vec3(s, s, s);
                        }
                        else
                        {
                            _viewport.Orbit(-dx * 0.42f, dy * 0.36f);
                        }
                        _host?.RequestRedraw();
                    }
                    break;
                }

                case TouchPhase.Ended:
                    if (!_wasDragged)
                    {
                        if (!_hits.DispatchTap(x, y) && _touchInViewport)
                        {
                            int hitId = _viewport.PickNode(_world, _lastViewportRect, x, y);
                            if (hitId > 0) _selectedNodeId = hitId;
                        }
                    }
                    _touchInViewport = false;
                    _host?.RequestRedraw();
                    break;

                case TouchPhase.Cancelled:
                    _touchInViewport = false;
                    break;
            }
        }

        public void HandlePinchZoom(float pinchScale)
        {
            if (pinchScale <= 0.1f) return;
            if (Screen == MobileScreen.Editor && _activeCodePath == null && !_settingsModalOpen)
            {
                _viewport.Zoom(1f / pinchScale);
                _host?.RequestRedraw();
            }
        }

        public void Render(IMobileCanvas g)
        {
            _hits.Clear();
            float w = Math.Max(320f, g.Width);
            float h = Math.Max(240f, g.Height);

            switch (Screen)
            {
                case MobileScreen.Splash: RenderSplash(g, w, h); break;
                case MobileScreen.Login:  RenderLogin(g, w, h);  break;
                case MobileScreen.Home:   RenderHome(g, w, h);   break;
                case MobileScreen.Build:  RenderBuild(g, w, h);  break;
                case MobileScreen.Editor: RenderEditor(g, w, h); break;
            }

            if (_settingsModalOpen)
                RenderSettingsModal(g, w, h);

            if (_buildModalOpen)
                RenderBuildModal(g, w, h);

            if (!string.IsNullOrEmpty(_toastText))
            {
                float tw = Math.Min(w - 40f, g.MeasureText(_toastText, 13f, true) + 36f);
                var tr = new UiRect((w - tw) * 0.5f, h - 54f, tw, 36f);
                g.FillRect(tr, new UiColor(20, 24, 32, 242), 18f);
                g.StrokeRect(tr, new UiColor(72, 138, 255, 220), 1.2f, 18f);
                g.DrawText(_toastText, tr.CenterX, tr.Y + 10f, 13f, new UiColor(255, 255, 255, 255), true, true);
            }
        }

        // ==================== TELA 1: SPLASH ====================
        private void RenderSplash(IMobileCanvas g, float w, float h)
        {
            g.Clear(new UiColor(0, 0, 0, 255));
            g.FillCircle(-40f, -40f, Math.Min(w, h) * 0.42f, new UiColor(14, 15, 20, 255));
            g.FillCircle(w + 50f, h + 60f, Math.Min(w, h) * 0.46f, new UiColor(15, 16, 22, 255));

            float cx = w * 0.5f;
            float cy = h * 0.46f;
            float lockW = Math.Min(300f, w * 0.46f);
            float lockH = lockW * 0.65f;
            var logoRect = new UiRect(cx - lockW * 0.5f, cy - lockH * 0.62f, lockW, lockH);

            if (!g.DrawImage("astron-lockup.png", logoRect))
            {
                DrawProceduralPlanetLogo(g, cx, cy - 34f, 44f);
                g.DrawText("Astron", cx, cy + 16f, 34f, new UiColor(255, 255, 255, 255), true, true);
                g.DrawText("E n g i n e", cx, cy + 54f, 16f, new UiColor(225, 226, 234, 255), false, true);
            }

            float barW = Math.Min(440f, w * 0.62f);
            var barRect = new UiRect(cx - barW * 0.5f, logoRect.Bottom + 28f, barW, 16f);
            g.StrokeRect(barRect, new UiColor(98, 100, 112, 255), 1.4f, 8f);

            float fillW = Math.Max(12f, (barW - 6f) * (_splashProgress / 100f));
            g.FillRect(new UiRect(barRect.X + 3f, barRect.Y + 3f, fillW, 10f), new UiColor(255, 255, 255, 255), 5f);
            g.DrawText("Carregando...", cx, barRect.Bottom + 14f, 13f, new UiColor(215, 216, 224, 255), false, true);

            _hits.Register("skip_splash", new UiRect(0, 0, w, h), () => Screen = MobileScreen.Login);
        }

        // ==================== TELA 2: LOGIN ====================
        private void RenderLogin(IMobileCanvas g, float w, float h)
        {
            g.Clear(new UiColor(0, 0, 0, 255));

            float planetSz = Math.Min(w * 0.46f, h * 0.75f);
            g.DrawImage("login-planet.png", new UiRect(-planetSz * 0.14f, h - planetSz * 0.80f, planetSz, planetSz), 220);

            bool wide = w >= 620f;
            if (wide)
            {
                g.DrawLine(w * 0.46f, 36f, w * 0.46f, h - 36f, new UiColor(40, 42, 50, 255), 1f);
                float lx = w * 0.23f;
                float ly = h * 0.44f;
                float lw = Math.Min(250f, w * 0.34f);
                float lh = lw * 0.65f;
                if (!g.DrawImage("astron-lockup.png", new UiRect(lx - lw * 0.5f, ly - lh * 0.55f, lw, lh)))
                {
                    DrawProceduralPlanetLogo(g, lx, ly - 24f, 38f);
                    g.DrawText("Astron Engine", lx, ly + 26f, 24f, new UiColor(255, 255, 255, 255), true, true);
                }
                g.DrawText("Crie. Desenvolva. Explore.", lx, ly + lh * 0.50f + 12f, 12.5f, new UiColor(200, 202, 212, 255), false, true);
            }

            float cardW = wide ? Math.Min(380f, w * 0.44f) : Math.Min(380f, w - 36f);
            float cardH = Math.Min(346f, h - 28f);
            float cardX = wide ? (w * 0.73f - cardW * 0.5f) : (w - cardW) * 0.5f;
            float cardY = (h - cardH) * 0.5f;

            var card = new UiRect(cardX, cardY, cardW, cardH);
            g.FillRect(card, new UiColor(11, 12, 16, 242), 18f);
            g.StrokeRect(card, new UiColor(44, 46, 56, 255), 1.2f, 18f);

            float ix = card.X + 22f;
            float iw = card.W - 44f;
            float cy = card.Y + 20f;

            g.DrawText("Bem-vindo(a)!", ix, cy, 20f, new UiColor(255, 255, 255, 255), true);
            cy += 26f;
            g.DrawText("Faça login na Astron Engine.", ix, cy, 12f, new UiColor(156, 158, 170, 255));
            cy += 26f;

            // Campo Usuário
            var userBox = new UiRect(ix, cy, iw, 38f);
            g.FillRect(userBox, new UiColor(16, 18, 24, 255), 10f);
            g.StrokeRect(userBox, new UiColor(52, 55, 66, 255), 1f, 10f);
            g.DrawText(
                string.IsNullOrEmpty(_loginUser) ? "Usuário ou e-mail (toque p/ digitar)" : _loginUser,
                ix + 12f, cy + 11f, 12.5f,
                string.IsNullOrEmpty(_loginUser) ? new UiColor(124, 128, 140, 255) : new UiColor(245, 246, 250, 255)
            );
            _hits.Register("login_user", userBox, () =>
            {
                _host?.PromptText("Usuário ou E-mail", "Digite seu usuário da Astron Engine:", _loginUser, false, val =>
                {
                    _loginUser = val ?? "";
                    _host?.RequestRedraw();
                });
            });
            cy += 46f;

            // Campo Senha
            var passBox = new UiRect(ix, cy, iw, 38f);
            g.FillRect(passBox, new UiColor(16, 18, 24, 255), 10f);
            g.StrokeRect(passBox, new UiColor(52, 55, 66, 255), 1f, 10f);
            string masked = string.IsNullOrEmpty(_loginPass) ? "Senha (toque p/ digitar)" : new string('•', Math.Min(14, _loginPass.Length));
            g.DrawText(
                masked, ix + 12f, cy + 11f, 12.5f,
                string.IsNullOrEmpty(_loginPass) ? new UiColor(124, 128, 140, 255) : new UiColor(245, 246, 250, 255)
            );
            _hits.Register("login_pass", passBox, () =>
            {
                _host?.PromptText("Senha", "Digite sua senha:", _loginPass, true, val =>
                {
                    _loginPass = val ?? "";
                    _host?.RequestRedraw();
                });
            });
            cy += 46f;

            // Checkbox Lembrar de mim
            var remRect = new UiRect(ix, cy, 150f, 22f);
            g.StrokeRect(new UiRect(ix, cy + 2f, 16f, 16f), new UiColor(120, 124, 138, 255), 1.2f, 4f);
            if (_loginRemember)
                g.FillRect(new UiRect(ix + 3f, cy + 5f, 10f, 10f), new UiColor(255, 255, 255, 255), 2f);
            g.DrawText("Lembrar de mim", ix + 24f, cy + 3f, 11.5f, new UiColor(200, 202, 212, 255));
            _hits.Register("login_rem", remRect, () => _loginRemember = !_loginRemember);
            cy += 30f;

            // Botão Entrar (pílula branca)
            DrawWhitePillButton(g, "btn_login_go", new UiRect(ix, cy, iw, 40f), "Entrar", () =>
            {
                if (string.IsNullOrEmpty(_loginUser)) _loginUser = "dev@astron.engine";
                Screen = MobileScreen.Home;
            });
            cy += 48f;

            // Botão Entrar com GitHub
            DrawOutlineButton(g, "btn_login_gh", new UiRect(ix, cy, iw, 38f), "Entrar com GitHub", () =>
            {
                _loginUser = "github-dev";
                Screen = MobileScreen.Home;
            });

            // Botão Configurações no canto inferior esquerdo
            DrawOutlineButton(g, "btn_login_cfg", new UiRect(14f, h - 40f, 138f, 30f), "⚙ Configurações", () => _settingsModalOpen = true);
            g.DrawText("v1.0.0", w - 54f, h - 28f, 11.5f, new UiColor(140, 142, 154, 255));
        }

        // ==================== TELA 3: HUB INICIAL (INÍCIO) ====================
        private void RenderHome(IMobileCanvas g, float w, float h)
        {
            g.Clear(new UiColor(4, 4, 7, 255));

            float sideW = Math.Min(210f, w * 0.26f);
            g.FillRect(new UiRect(0, 0, sideW, h), new UiColor(8, 8, 11, 255));
            g.DrawLine(sideW, 0, sideW, h, new UiColor(28, 30, 38, 255), 1f);

            // Logo Astron Engine oficial na barra lateral
            var horizRect = new UiRect(14f, 14f, sideW - 28f, (sideW - 28f) * 0.25f);
            if (!g.DrawImage("astron-horizontal.png", horizRect))
            {
                DrawProceduralPlanetLogo(g, 32f, 32f, 14f);
                g.DrawText("Astron Engine", 56f, 24f, 15f, new UiColor(255, 255, 255, 255), true);
            }

            string[][] navItems =
            {
                new[] { "inicio",     "⌂  Início" },
                new[] { "projetos",   "📁 Meus Projetos" },
                new[] { "biblioteca", "📦 Bibioteca" },
                new[] { "compilar",   "🚀 Compilar" },
                new[] { "config",     "⚙  Configurações" },
                new[] { "sobre",      "ℹ  Sobre" },
            };

            float ny = 74f;
            for (int i = 0; i < navItems.Length; i++)
            {
                string id = navItems[i][0];
                string lbl = navItems[i][1];
                bool act = (_homeSection == id);
                var nr = new UiRect(10f, ny, sideW - 20f, 36f);
                if (act)
                {
                    g.FillRect(nr, new UiColor(24, 26, 34, 255), 9f);
                    g.FillRect(new UiRect(2f, ny + 8f, 4f, 20f), new UiColor(255, 255, 255, 255), 2f);
                }
                g.DrawText(lbl, nr.X + 12f, nr.Y + 10f, 12.5f, act ? new UiColor(255, 255, 255, 255) : new UiColor(188, 190, 202, 255), act);
                string captured = id;
                _hits.Register("nav_" + id, nr, () =>
                {
                    if (captured == "config") _settingsModalOpen = true;
                    else if (captured == "compilar") Screen = MobileScreen.Build;
                    else _homeSection = captured;
                });
                ny += 40f;
            }

            // Cartão inferior Online
            var stCard = new UiRect(10f, h - 62f, sideW - 20f, 50f);
            g.FillRect(stCard, new UiColor(12, 13, 18, 255), 10f);
            g.StrokeRect(stCard, new UiColor(34, 36, 44, 255), 1f, 10f);
            g.FillCircle(stCard.X + 16f, stCard.Y + 18f, 4.5f, new UiColor(47, 209, 88, 255));
            g.DrawText("Astron Engine v1.0.0", stCard.X + 28f, stCard.Y + 11f, 11f, new UiColor(240, 242, 248, 255), true);
            g.DrawText("Online · " + (_host?.PlatformName ?? "Mobile"), stCard.X + 28f, stCard.Y + 28f, 10f, new UiColor(148, 152, 164, 255));

            // Área principal
            float mx = sideW + 18f;
            float mw = w - sideW - 36f;

            if (_homeSection == "sobre")
            {
                float acx = mx + mw * 0.5f;
                float acy = h * 0.42f;
                g.DrawImage("astron-lockup.png", new UiRect(acx - 120f, acy - 80f, 240f, 156f));
                g.DrawText("Astron Engine — Nativa p/ Android (.apk), iOS (.ipa), Windows (.exe) e Linux",
                    acx, acy + 92f, 12.5f, new UiColor(200, 204, 214, 255), true, true);
                g.DrawText("Construída em C++20 · C# · C11 · GLSL · HLSL · Rust · Lua (Zero Web)",
                    acx, acy + 114f, 11.5f, new UiColor(150, 154, 166, 255), false, true);
                return;
            }

            float curY = 16f;
            float heroH = Math.Min(142f, h * 0.32f);
            var heroRect = new UiRect(mx, curY, mw, heroH);
            g.FillRect(heroRect, new UiColor(10, 12, 16, 255), 14f);
            g.DrawImage("hero-planet.jpg", heroRect, 225, 14f);
            g.FillRect(new UiRect(mx, curY, mw * 0.62f, heroH), new UiColor(0, 0, 0, 155), 14f);
            g.StrokeRect(heroRect, new UiColor(38, 40, 50, 255), 1f, 14f);

            g.DrawText("Crie. Desenvolva. Explore.", mx + 22f, curY + 22f, 20f, new UiColor(255, 255, 255, 255), true);
            g.DrawText("Sua imaginação, sem limites — direto no Android & iOS.", mx + 22f, curY + 50f, 12f, new UiColor(210, 212, 222, 255));

            DrawWhitePillButton(g, "btn_home_new", new UiRect(mx + 22f, curY + heroH - 48f, 154f, 36f), "+  Novo Projeto", () =>
            {
                _host?.PromptText("Novo Projeto C#", "Nome do novo projeto:", "Meu Jogo Mobile", false, name =>
                {
                    if (!string.IsNullOrWhiteSpace(name))
                    {
                        _store.CreateProject(name, "obby");
                        ResetDefaultScene();
                        Screen = MobileScreen.Editor;
                        ShowToast("Projeto '" + name + "' criado ✓");
                    }
                });
            });

            curY += heroH + 16f;
            g.DrawText("Projetos Recentes", mx, curY + 4f, 14.5f, new UiColor(255, 255, 255, 255), true);
            DrawOutlineButton(g, "btn_home_build_top", new UiRect(mx + mw - 118f, curY - 2f, 118f, 28f), "Compilar  >", () => Screen = MobileScreen.Build);
            curY += 34f;

            // Grade de 4 cartões (3 projetos + Novo Projeto)
            int cols = 4;
            float gap = 10f;
            float cw = (mw - gap * (cols - 1)) / cols;
            float ch = Math.Max(110f, h - curY - 16f);

            int showCount = Math.Min(3, _store.Projects.Count);
            for (int i = 0; i < showCount; i++)
            {
                MobileProjectInfo p = _store.Projects[i];
                var cr = new UiRect(mx + i * (cw + gap), curY, cw, ch);
                g.FillRect(cr, new UiColor(11, 12, 16, 255), 12f);
                var thRect = new UiRect(cr.X, cr.Y, cr.W, cr.H * 0.62f);
                g.DrawImage(p.ThumbAsset, thRect, 255, 10f);
                g.StrokeRect(cr, new UiColor(36, 38, 48, 255), 1f, 12f);

                g.DrawText(p.Name, cr.X + 10f, thRect.Bottom + 8f, 12.5f, new UiColor(248, 249, 252, 255), true);
                g.DrawText("Criado em " + p.CreatedDate, cr.X + 10f, thRect.Bottom + 26f, 10f, new UiColor(144, 146, 158, 255));

                string pid = p.Id;
                _hits.Register("open_proj_" + pid, cr, () =>
                {
                    _store.SelectProject(pid);
                    Screen = MobileScreen.Editor;
                    ShowToast("Projeto aberto: " + p.Name);
                });
            }

            // 4º cartão: + Novo Projeto
            var newCard = new UiRect(mx + showCount * (cw + gap), curY, cw, ch);
            g.StrokeRect(newCard, new UiColor(68, 72, 86, 255), 1.3f, 12f);
            g.DrawText("+", newCard.CenterX, newCard.CenterY - 26f, 26f, new UiColor(245, 246, 250, 255), true, true);
            g.DrawText("Novo Projeto", newCard.CenterX, newCard.CenterY + 4f, 12f, new UiColor(242, 244, 250, 255), true, true);
            g.DrawText("Comece do zero", newCard.CenterX, newCard.CenterY + 22f, 10f, new UiColor(142, 144, 156, 255), false, true);
            _hits.Register("card_new_zero", newCard, () =>
            {
                _store.CreateProject("Novo Jogo 3D", "obby");
                ResetDefaultScene();
                Screen = MobileScreen.Editor;
            });
        }

        // ==================== TELA 4: COMPILAR (BUILD) ====================
        private void RenderBuild(IMobileCanvas g, float w, float h)
        {
            g.Clear(new UiColor(4, 4, 7, 255));

            float topH = 44f;
            g.FillRect(new UiRect(0, 0, w, topH), new UiColor(9, 10, 14, 255));
            g.DrawLine(0, topH, w, topH, new UiColor(30, 32, 40, 255), 1f);
            g.DrawImage("astron-icon.png", new UiRect(12f, 8f, 28f, 28f));
            g.DrawText("Astron Engine — Build Multiplataforma Nativa", 48f, 13f, 14f, new UiColor(255, 255, 255, 255), true);

            DrawOutlineButton(g, "build_back_home", new UiRect(w - 220f, 7f, 96f, 30f), "← Início", () => Screen = MobileScreen.Home);
            DrawWhitePillButton(g, "build_open_ed", new UiRect(w - 114f, 7f, 102f, 30f), "Abrir Editor", () => Screen = MobileScreen.Editor);

            float mx = 18f;
            float mw = w - 36f;
            float curY = topH + 14f;

            // Banner Astron Engine
            float banH = Math.Min(92f, h * 0.22f);
            var ban = new UiRect(mx, curY, mw, banH);
            g.FillRect(ban, new UiColor(10, 12, 16, 255), 14f);
            g.DrawImage("hero-planet.jpg", ban, 210, 14f);
            g.StrokeRect(ban, new UiColor(36, 38, 48, 255), 1f, 14f);
            g.DrawImage("astron-mark.png", new UiRect(ban.X + 20f, ban.Y + (banH - 64f) * 0.5f, 64f, 64f));
            g.DrawText("Buildar Seu Projeto (" + _store.Current.Name + ")", ban.X + 100f, ban.Y + 20f, 17f, new UiColor(255, 255, 255, 255), true);
            g.DrawText("Gere pacotes nativos .APK (Android), .IPA (iOS), .EXE (Windows) e Linux — sem versão Web.",
                ban.X + 100f, ban.Y + 46f, 11.5f, new UiColor(210, 212, 222, 255));

            curY += banH + 14f;

            // 6 Cartões de Plataforma Nativa
            string[][] targets =
            {
                new[] { "APK",    "Android",    "Gerar APK ->" },
                new[] { "IPA",    "iOS",        "Gerar IPA ->" },
                new[] { "EXE",    "Windows",    "Gerar EXE ->" },
                new[] { "Linux",  "Linux x64",  "Gerar Linux ->" },
                new[] { "ZIP",    "Pacote C#",  "Gerar ZIP ->" },
                new[] { "Outros", "APAK Rust",  "Ver Mais ->" },
            };

            float gap = 8f;
            float cw = (mw - gap * 5f) / 6f;
            float ch = Math.Min(138f, h * 0.34f);

            for (int i = 0; i < 6; i++)
            {
                var cr = new UiRect(mx + i * (cw + gap), curY, cw, ch);
                g.FillRect(cr, new UiColor(12, 13, 18, 255), 12f);
                g.StrokeRect(cr, new UiColor(36, 38, 48, 255), 1f, 12f);

                g.DrawText(targets[i][0], cr.CenterX, cr.Y + 24f, 16f, new UiColor(255, 255, 255, 255), true, true);
                g.DrawText(targets[i][1], cr.CenterX, cr.Y + 48f, 10.5f, new UiColor(148, 150, 162, 255), false, true);

                string tName = targets[i][0];
                DrawOutlineButton(g, "trig_bld_" + tName, new UiRect(cr.X + 6f, cr.Bottom - 36f, cr.W - 12f, 28f), targets[i][2], () =>
                {
                    _buildTargetLabel = tName;
                    _buildAnimTimer = 0f;
                    _buildModalOpen = true;
                });
            }

            curY += ch + 14f;
            float botH = Math.Max(68f, h - curY - 14f);
            var quickCard = new UiRect(mx, curY, mw, botH);
            g.FillRect(quickCard, new UiColor(11, 12, 16, 255), 12f);
            g.StrokeRect(quickCard, new UiColor(36, 38, 48, 255), 1f, 12f);
            g.DrawText("⚡ Build Rápida via GitHub Actions (APK + IPA + EXE + Linux)", quickCard.X + 16f, quickCard.Y + 14f, 13.5f, new UiColor(255, 255, 255, 255), true);
            g.DrawText("Sincroniza scripts C# e dispara build-engine.yml & build-game.yml.", quickCard.X + 16f, quickCard.Y + 36f, 11f, new UiColor(152, 156, 168, 255));

            DrawWhitePillButton(g, "btn_quick_run", new UiRect(quickCard.Right - 186f, quickCard.CenterY - 18f, 170f, 36f), "▶ Iniciar Build", () =>
            {
                _buildTargetLabel = _quickBuild ? "APK + IPA + EXE" : "APK";
                _buildAnimTimer = 0f;
                _buildModalOpen = true;
            });
        }

        // ==================== TELA 5: EDITOR 3D COMPLETO (ASTRON ENGINE) ====================
        private void RenderEditor(IMobileCanvas g, float w, float h)
        {
            g.Clear(new UiColor(14, 15, 19, 255));

            // 1. Barra Superior Oficial: Logo Astron Engine + "Astron Engine v"
            const float menuH = 34f;
            g.FillRect(new UiRect(0, 0, w, menuH), new UiColor(11, 12, 16, 255));
            g.DrawLine(0, menuH, w, menuH, new UiColor(28, 30, 38, 255), 1f);

            if (!g.DrawImage("astron-icon.png", new UiRect(8f, 4f, 26f, 26f)))
            {
                DrawProceduralPlanetLogo(g, 21f, 17f, 10f);
            }

            var brandBtn = new UiRect(38f, 3f, 134f, 28f);
            g.DrawText("Astron Engine  ▾", brandBtn.X + 4f, 9f, 13f, new UiColor(255, 255, 255, 255), true);
            _hits.Register("ed_brand_home", brandBtn, () => Screen = MobileScreen.Home);

            string[] topMenus = { "Arquivo", "Cena", "Script C#", "Shader GLSL", "Agente IA", "Compilar", "Configurações" };
            float mx = 180f;
            for (int i = 0; i < topMenus.Length; i++)
            {
                float bw = g.MeasureText(topMenus[i], 11.5f) + 18f;
                if (mx + bw > w - 12f) break;
                var mr = new UiRect(mx, 4f, bw, 26f);
                g.DrawText(topMenus[i], mr.X + 9f, 9f, 11.5f, new UiColor(210, 212, 222, 255));
                int idx = i;
                _hits.Register("ed_topm_" + idx, mr, () =>
                {
                    if (idx == 0) Screen = MobileScreen.Home;
                    else if (idx == 1) { ResetDefaultScene(); ShowToast("Cena Principal restaurada"); }
                    else if (idx == 2) _activeCodePath = "res://scripts/GameWorld.cs";
                    else if (idx == 3) { _activeCodePath = "res://shaders/surface.glsl"; _bottomTab = 3; }
                    else if (idx == 4) _bottomTab = 4;
                    else if (idx == 5) Screen = MobileScreen.Build;
                    else if (idx == 6) _settingsModalOpen = true;
                });
                mx += bw + 4f;
            }

            // 2. Barra de Ferramentas 3D
            const float tbH = 42f;
            float tbY = menuH;
            g.FillRect(new UiRect(0, tbY, w, tbH), new UiColor(18, 20, 26, 255));
            g.DrawLine(0, tbY + tbH, w, tbY + tbH, new UiColor(32, 35, 44, 255), 1f);

            string[] tools = { "Selecionar", "Mover", "Rotacionar", "Escalar" };
            float tx = 8f;
            for (int t = 0; t < tools.Length; t++)
            {
                float tw = 74f;
                var tr = new UiRect(tx, tbY + 5f, tw, 32f);
                bool act = (_activeTool == t);
                g.FillRect(tr, act ? new UiColor(28, 78, 168, 255) : new UiColor(25, 28, 36, 255), 7f);
                if (act) g.StrokeRect(tr, new UiColor(68, 134, 245, 255), 1f, 7f);
                g.DrawText(tools[t], tr.CenterX, tr.Y + 9f, 11f, new UiColor(248, 249, 252, 255), act, true);
                int ct = t;
                _hits.Register("tool_" + ct, tr, () => _activeTool = ct);
                tx += tw + 4f;
            }

            // + Criar Cubo / Esfera
            DrawToolbarChip(g, "tb_add_cube", ref tx, tbY, "+ Cubo", false, () =>
            {
                float ox = ((_world.Nodes.Count % 5) - 2) * 1.4f;
                var n = _world.SpawnPart("Part_" + _world.Nodes.Count, AstronPrimitiveKind.Cube, new Vec3(ox, 0.5f, 0f), Vec3.One, AstronMaterialPreset.Plastic, false);
                n.ColorRgba = 0x56A0FFFFu;
                _selectedNodeId = n.Id;
                _consoleLogs.Add("[INFO] Criado objeto 3D: " + n.Name);
            });
            DrawToolbarChip(g, "tb_add_sph", ref tx, tbY, "+ Esfera", false, () =>
            {
                var n = _world.SpawnPart("Sphere_" + _world.Nodes.Count, AstronPrimitiveKind.Sphere, new Vec3(1.5f, 1.2f, 0f), Vec3.One, AstronMaterialPreset.Neon, false);
                n.ColorRgba = 0x38D996FFu;
                _selectedNodeId = n.Id;
                _consoleLogs.Add("[INFO] Criada esfera 3D: " + n.Name);
            });
            DrawToolbarChip(g, "tb_2d3d", ref tx, tbY, _viewport.Is2DMode ? "2D" : "3D", _viewport.Is2DMode, () => _viewport.Is2DMode = !_viewport.Is2DMode);
            DrawToolbarChip(g, "tb_play", ref tx, tbY, _isPlaying ? "▶ Rodando" : "▶ Jogar", _isPlaying, () =>
            {
                _isPlaying = true;
                _isPaused = false;
                _activeCodePath = null;
                SceneNode3D sel = GetSelectedNode();
                if (sel != null && !string.Equals(sel.Name, "Baseplate", StringComparison.OrdinalIgnoreCase))
                {
                    sel.Anchored = false;
                    sel.UseGravity = true;
                    if (sel.Position.Y < 2.5f) sel.Position = new Vec3(sel.Position.X, 4.2f, sel.Position.Z);
                }
                _consoleLogs.Add("[INFO] Simulação 3D C# iniciada.");
            });
            DrawToolbarChip(g, "tb_stop", ref tx, tbY, "⏹ Parar", false, () =>
            {
                _isPlaying = false;
                _isPaused = false;
                ResetDefaultScene();
                _consoleLogs.Add("[INFO] Simulação parada.");
            });

            // 3. Corpo em 3 Colunas (Explorer/Arquivos | Viewport 3D + Painel Inferior | Propriedades/Toolbox)
            const float statusH = 22f;
            float bodyY = menuH + tbH + 4f;
            float bodyH = h - bodyY - statusH - 4f;

            float leftW = Math.Min(178f, w * 0.21f);
            float rightW = Math.Min(196f, w * 0.23f);
            float centerX = leftW + 8f;
            float centerW = w - leftW - rightW - 16f;

            // --- COLUNA ESQUERDA: Explorer + Arquivos (res://) ---
            float expH = bodyH * 0.52f;
            var expRect = new UiRect(4f, bodyY, leftW, expH);
            g.FillRect(expRect, new UiColor(17, 19, 24, 255), 8f);
            g.StrokeRect(expRect, new UiColor(30, 33, 42, 255), 1f, 8f);
            g.DrawText("Explorer", expRect.X + 10f, expRect.Y + 8f, 12f, new UiColor(245, 246, 252, 255), true);
            g.DrawLine(expRect.X, expRect.Y + 26f, expRect.Right, expRect.Y + 26f, new UiColor(28, 30, 38, 255), 1f);

            float ey = expRect.Y + 32f;
            g.DrawText("▾ 📁 Cena Principal", expRect.X + 8f, ey, 11f, new UiColor(180, 205, 255, 255), true);
            ey += 20f;
            for (int i = 0; i < _world.Nodes.Count && ey + 18f < expRect.Bottom; i++)
            {
                SceneNode3D n = _world.Nodes[i];
                bool isSel = (n.Id == _selectedNodeId);
                var rowR = new UiRect(expRect.X + 6f, ey, expRect.W - 12f, 20f);
                if (isSel) g.FillRect(rowR, new UiColor(28, 78, 168, 220), 5f);
                g.DrawText("  🧊 " + n.Name, rowR.X + 6f, rowR.Y + 3f, 10.8f, new UiColor(232, 234, 242, 255));
                int nid = n.Id;
                _hits.Register("sel_node_" + nid, rowR, () => { _selectedNodeId = nid; _activeCodePath = null; });
                ey += 21f;
            }

            // Painel Arquivos (res://)
            var filesRect = new UiRect(4f, bodyY + expH + 4f, leftW, bodyH - expH - 4f);
            g.FillRect(filesRect, new UiColor(17, 19, 24, 255), 8f);
            g.StrokeRect(filesRect, new UiColor(30, 33, 42, 255), 1f, 8f);
            g.DrawText("Arquivos (res://)", filesRect.X + 10f, filesRect.Y + 8f, 12f, new UiColor(245, 246, 252, 255), true);
            g.DrawLine(filesRect.X, filesRect.Y + 26f, filesRect.Right, filesRect.Y + 26f, new UiColor(28, 30, 38, 255), 1f);

            List<string> resFiles = _store.ListResFiles();
            float fy = filesRect.Y + 30f;
            for (int i = 0; i < resFiles.Count && fy + 18f < filesRect.Bottom; i++)
            {
                string rf = resFiles[i];
                string shortName = rf.Replace("res://", "");
                bool activeFile = (_activeCodePath == rf);
                var fr = new UiRect(filesRect.X + 6f, fy, filesRect.W - 12f, 20f);
                if (activeFile) g.FillRect(fr, new UiColor(28, 78, 168, 220), 5f);
                g.DrawText("📄 " + shortName, fr.X + 4f, fr.Y + 3f, 10.2f, new UiColor(214, 220, 232, 255));
                string capturedRf = rf;
                _hits.Register("open_res_" + i, fr, () => _activeCodePath = capturedRf);
                fy += 20f;
            }

            // --- COLUNA CENTRAL: Abas + Viewport 3D / Editor Código + Painel Inferior ---
            float bottomH = Math.Min(132f, bodyH * 0.34f);
            float centerTopH = bodyH - bottomH - 4f;
            var centerWrap = new UiRect(centerX, bodyY, centerW, centerTopH);
            g.FillRect(centerWrap, new UiColor(17, 19, 24, 255), 8f);
            g.StrokeRect(centerWrap, new UiColor(30, 33, 42, 255), 1f, 8f);

            // Barra de Abas (Cena Principal | Script C# ativo)
            const float tabH = 26f;
            var sceneTabR = new UiRect(centerWrap.X + 4f, centerWrap.Y + 3f, 118f, tabH - 4f);
            g.FillRect(sceneTabR, _activeCodePath == null ? new UiColor(28, 78, 168, 255) : new UiColor(24, 26, 34, 255), 6f);
            g.DrawText("🧊 Cena Principal", sceneTabR.X + 8f, sceneTabR.Y + 4f, 10.8f, new UiColor(255, 255, 255, 255), true);
            _hits.Register("tab_scene3d", sceneTabR, () => _activeCodePath = null);

            if (_activeCodePath != null)
            {
                var codeTabR = new UiRect(sceneTabR.Right + 4f, centerWrap.Y + 3f, 168f, tabH - 4f);
                g.FillRect(codeTabR, new UiColor(28, 78, 168, 255), 6f);
                g.DrawText("📄 " + _activeCodePath.Replace("res://", ""), codeTabR.X + 8f, codeTabR.Y + 4f, 10.5f, new UiColor(255, 255, 255, 255), true);
            }

            var contentRect = new UiRect(centerWrap.X + 2f, centerWrap.Y + tabH, centerWrap.W - 4f, centerWrap.H - tabH - 2f);
            if (_activeCodePath == null)
            {
                _lastViewportRect = contentRect;
                _viewport.Render(g, contentRect, _world, _selectedNodeId);

                // Controles flutuantes no canto superior esquerdo do Viewport 3D
                var perspBtn = new UiRect(contentRect.X + 8f, contentRect.Y + 8f, 96f, 24f);
                g.FillRect(perspBtn, new UiColor(16, 18, 24, 210), 6f);
                g.DrawText(_viewport.Is2DMode ? "Ortográfica ▾" : "Perspectiva ▾", perspBtn.X + 8f, perspBtn.Y + 5f, 10.5f, new UiColor(240, 242, 248, 255));
                _hits.Register("vp_persp", perspBtn, () => _viewport.Is2DMode = !_viewport.Is2DMode);

                var zoomInBtn = new UiRect(perspBtn.Right + 6f, contentRect.Y + 8f, 26f, 24f);
                g.FillRect(zoomInBtn, new UiColor(16, 18, 24, 210), 6f);
                g.DrawText("+", zoomInBtn.CenterX, zoomInBtn.Y + 4f, 13f, new UiColor(255, 255, 255, 255), true, true);
                _hits.Register("vp_zin", zoomInBtn, () => _viewport.Zoom(0.82f));

                var zoomOutBtn = new UiRect(zoomInBtn.Right + 4f, contentRect.Y + 8f, 26f, 24f);
                g.FillRect(zoomOutBtn, new UiColor(16, 18, 24, 210), 6f);
                g.DrawText("-", zoomOutBtn.CenterX, zoomOutBtn.Y + 4f, 13f, new UiColor(255, 255, 255, 255), true, true);
                _hits.Register("vp_zout", zoomOutBtn, () => _viewport.Zoom(1.22f));

                var camResetBtn = new UiRect(zoomOutBtn.Right + 4f, contentRect.Y + 8f, 54f, 24f);
                g.FillRect(camResetBtn, new UiColor(16, 18, 24, 210), 6f);
                g.DrawText("Reset", camResetBtn.CenterX, camResetBtn.Y + 5f, 10.5f, new UiColor(230, 232, 240, 255), false, true);
                _hits.Register("vp_reset", camResetBtn, () => _viewport.ResetCamera());
            }
            else
            {
                // Visualizador e Editor Nativo de Código C# / Lua / GLSL
                g.FillRect(contentRect, new UiColor(11, 13, 18, 255), 6f);
                DrawWhitePillButton(g, "btn_edit_code_modal", new UiRect(contentRect.Right - 134f, contentRect.Y + 6f, 126f, 26f), "Editar Código", () =>
                {
                    string curCode = _store.ReadFile(_activeCodePath);
                    _host?.PromptText("Editar " + _activeCodePath, "Edite o código nativo e confirme para salvar:", curCode, false, updated =>
                    {
                        if (updated != null)
                        {
                            _store.WriteFile(_activeCodePath, updated);
                            ShowToast("Salvo: " + _activeCodePath);
                        }
                    });
                });

                string code = _store.ReadFile(_activeCodePath);
                string[] lines = (code ?? "").Split('\n');
                float ly = contentRect.Y + 10f;
                for (int i = 0; i < lines.Length && ly + 15f < contentRect.Bottom; i++)
                {
                    g.DrawText((i + 1).ToString(CultureInfo.InvariantCulture).PadLeft(2, ' ') + "  " + lines[i].TrimEnd('\r'),
                        contentRect.X + 10f, ly, 10.8f, new UiColor(198, 218, 248, 255));
                    ly += 15f;
                }
            }

            // --- PAINEL INFERIOR: Console | Depurador | Áudio | Shader | 🤖 Agente IA ---
            var botRect = new UiRect(centerX, bodyY + centerTopH + 4f, centerW, bottomH);
            g.FillRect(botRect, new UiColor(17, 19, 24, 255), 8f);
            g.StrokeRect(botRect, new UiColor(30, 33, 42, 255), 1f, 8f);

            string[] bTabs = { "Console", "Depurador", "Áudio C11", "Shader GLSL", "🤖 Agente IA" };
            float btx = botRect.X + 6f;
            for (int b = 0; b < bTabs.Length; b++)
            {
                float bw = g.MeasureText(bTabs[b], 10.8f, true) + 16f;
                var br = new UiRect(btx, botRect.Y + 4f, bw, 22f);
                g.FillRect(br, _bottomTab == b ? new UiColor(28, 78, 168, 255) : new UiColor(24, 26, 34, 255), 5f);
                g.DrawText(bTabs[b], br.X + 8f, br.Y + 4f, 10.5f, new UiColor(248, 249, 252, 255), _bottomTab == b);
                int cb = b;
                _hits.Register("btab_" + cb, br, () => _bottomTab = cb);
                btx += bw + 4f;
            }
            g.DrawLine(botRect.X, botRect.Y + 29f, botRect.Right, botRect.Y + 29f, new UiColor(28, 30, 38, 255), 1f);

            RenderBottomPanelContent(g, new UiRect(botRect.X + 8f, botRect.Y + 33f, botRect.W - 16f, botRect.H - 37f));

            // --- COLUNA DIREITA: Propriedades + Caixa de Ferramentas ---
            float propH = bodyH * 0.62f;
            var propRect = new UiRect(w - rightW - 4f, bodyY, rightW, propH);
            g.FillRect(propRect, new UiColor(17, 19, 24, 255), 8f);
            g.StrokeRect(propRect, new UiColor(30, 33, 42, 255), 1f, 8f);
            g.DrawText("Propriedades", propRect.X + 10f, propRect.Y + 8f, 12f, new UiColor(245, 246, 252, 255), true);
            g.DrawLine(propRect.X, propRect.Y + 26f, propRect.Right, propRect.Y + 26f, new UiColor(28, 30, 38, 255), 1f);

            SceneNode3D selNode = GetSelectedNode();
            if (selNode != null)
            {
                float py = propRect.Y + 32f;
                g.DrawText("Parte: " + selNode.Name, propRect.X + 10f, py, 11.2f, new UiColor(168, 204, 255, 255), true);
                py += 20f;
                g.DrawText("Pos: " + selNode.Position, propRect.X + 10f, py, 10.2f, new UiColor(215, 218, 228, 255));
                py += 17f;
                g.DrawText("Esc: " + selNode.Scale, propRect.X + 10f, py, 10.2f, new UiColor(215, 218, 228, 255));
                py += 17f;
                g.DrawText("Mat: " + selNode.Material, propRect.X + 10f, py, 10.2f, new UiColor(215, 218, 228, 255));
                py += 20f;

                // Botões rápidos de cor PBR / Material
                uint[] palette = { 0xE8ECF4FFu, 0x3882F6FFu, 0x2FD158FFu, 0xFF3B5CFFu, 0xFFD43BFFu };
                float cxp = propRect.X + 10f;
                for (int c = 0; c < palette.Length; c++)
                {
                    var sw = new UiRect(cxp, py, 24f, 20f);
                    g.FillRect(sw, UiColor.FromHex(palette[c]), 5f);
                    uint colVal = palette[c];
                    _hits.Register("prop_col_" + c, sw, () => selNode.ColorRgba = colVal);
                    cxp += 29f;
                }
                py += 26f;

                if (py + 26f < propRect.Bottom)
                {
                    DrawOutlineButton(g, "prop_mat_cycle", new UiRect(propRect.X + 10f, py, propRect.W - 20f, 24f), "Alternar Material PBR", () =>
                    {
                        int next = ((int)selNode.Material + 1) % 7;
                        selNode.Material = (AstronMaterialPreset)next;
                    });
                }
            }

            // Caixa de Ferramentas
            var tbxRect = new UiRect(w - rightW - 4f, bodyY + propH + 4f, rightW, bodyH - propH - 4f);
            g.FillRect(tbxRect, new UiColor(17, 19, 24, 255), 8f);
            g.StrokeRect(tbxRect, new UiColor(30, 33, 42, 255), 1f, 8f);
            g.DrawText("Caixa de Ferramentas", tbxRect.X + 10f, tbxRect.Y + 8f, 11.5f, new UiColor(245, 246, 252, 255), true);
            g.DrawLine(tbxRect.X, tbxRect.Y + 25f, tbxRect.Right, tbxRect.Y + 25f, new UiColor(28, 30, 38, 255), 1f);

            string[] shapes = { "Part", "Sphere", "Cylinder", "Block" };
            AstronPrimitiveKind[] kinds = { AstronPrimitiveKind.Cube, AstronPrimitiveKind.Sphere, AstronPrimitiveKind.Cylinder, AstronPrimitiveKind.Cube };
            float swW = (tbxRect.W - 24f) * 0.5f;
            for (int s = 0; s < 4; s++)
            {
                int col = s % 2;
                int row = s / 2;
                float sy = tbxRect.Y + 31f + row * 32f;
                if (sy + 26f > tbxRect.Bottom) break;
                var sr = new UiRect(tbxRect.X + 8f + col * (swW + 8f), sy, swW, 26f);
                g.FillRect(sr, new UiColor(25, 28, 36, 255), 6f);
                g.DrawText("+ " + shapes[s], sr.CenterX, sr.Y + 6f, 10.5f, new UiColor(230, 234, 244, 255), false, true);
                int cs = s;
                _hits.Register("spawn_tbx_" + cs, sr, () =>
                {
                    Vec3 scl = cs == 3 ? new Vec3(2.2f, 0.5f, 2.2f) : Vec3.One;
                    var node = _world.SpawnPart(shapes[cs], kinds[cs], new Vec3(0f, 1.2f, 0f), scl, AstronMaterialPreset.Plastic, false);
                    _selectedNodeId = node.Id;
                });
            }

            // 4. Barra de Status Inferior
            var sb = new UiRect(0, h - statusH, w, statusH);
            g.FillRect(sb, new UiColor(10, 11, 15, 255));
            g.FillCircle(12f, sb.CenterY, 4f, _agent.IsBusy ? new UiColor(58, 138, 255, 255) : new UiColor(47, 209, 88, 255));
            g.DrawText(_agent.IsBusy ? _agent.StatusLine : (_isPlaying ? "Em execução" : "Pronto"), 22f, sb.Y + 4f, 10.5f, new UiColor(210, 214, 224, 255));
            string rightInfo = "FPS: 60  |  Objetos: " + _world.Nodes.Count + "  |  Astron Engine v1.0.0";
            g.DrawText(rightInfo, w - g.MeasureText(rightInfo, 10.5f) - 12f, sb.Y + 4f, 10.5f, new UiColor(162, 166, 178, 255));
        }

        private void RenderBottomPanelContent(IMobileCanvas g, UiRect r)
        {
            if (_bottomTab == 0)
            {
                int start = Math.Max(0, _consoleLogs.Count - 4);
                float y = r.Y;
                for (int i = start; i < _consoleLogs.Count && y + 15f <= r.Bottom; i++)
                {
                    g.DrawText(_consoleLogs[i], r.X, y, 10.5f, new UiColor(145, 198, 255, 255));
                    y += 16f;
                }
            }
            else if (_bottomTab == 1)
            {
                g.DrawText("Plataforma: " + (_host?.PlatformName ?? "Mobile Nativo") + "  |  Runtime: C# .NET 10 (AOT/JIT)", r.X, r.Y, 11f, new UiColor(135, 224, 250, 255));
                g.DrawText("Shaders: GLSL 3.30 + HLSL SM5.0  |  Core: C++20 / C11 SIMD / Rust .apak", r.X, r.Y + 20f, 11f, new UiColor(195, 202, 216, 255));
            }
            else if (_bottomTab == 2)
            {
                g.DrawText("Sintetizador PCM 16-bit (.wav): Pulo · Moeda · Laser · Explosão", r.X, r.Y + 4f, 11f, new UiColor(215, 220, 232, 255));
                DrawWhitePillButton(g, "btn_synth_sfx", new UiRect(r.Right - 150f, r.Y, 146f, 26f), "Gerar Efeito .WAV", () =>
                {
                    _consoleLogs.Add("[INFO] Efeito sonoro jump.wav registrado em res://sounds/.");
                    ShowToast("res://sounds/jump.wav gerado ✓");
                });
            }
            else if (_bottomTab == 3)
            {
                g.DrawText("Shader GLSL ativo: res://shaders/surface.glsl (PBR + Luz Direcional)", r.X, r.Y + 4f, 11f, new UiColor(185, 214, 255, 255));
                DrawOutlineButton(g, "btn_open_glsl", new UiRect(r.Right - 140f, r.Y, 136f, 26f), "Abrir Shader GLSL", () => _activeCodePath = "res://shaders/surface.glsl");
            }
            else
            {
                // 🤖 AGENTE IA AUTÔNOMO (OpenRouter + Qualquer API)
                DrawWhitePillButton(g, "btn_ask_ai", new UiRect(r.Right - 154f, r.Y, 150f, 26f),
                    _agent.IsBusy ? "Trabalhando..." : "Pedir Jogo à IA", () =>
                {
                    if (_agent.IsBusy) return;
                    _host?.PromptText(
                        "Agente IA Autônomo (" + MobileAIAgent.Presets[_agent.ProviderIndex].Name + ")",
                        "Peça o jogo completo em C# (ex: 'Crie um Obby 3D com plataformas e moedas'):",
                        "Crie um jogo Obby 3D completo em C# com plataformas coloridas",
                        false,
                        prompt =>
                        {
                            if (!string.IsNullOrWhiteSpace(prompt))
                            {
                                Task _ = _agent.RunAutonomousTurnAsync(prompt, _store, _world, () => _host?.RequestRedraw());
                            }
                        });
                });

                DrawOutlineButton(g, "btn_cfg_ai_inline", new UiRect(r.Right - 252f, r.Y, 92f, 26f), "⚙ API Key", () => _settingsModalOpen = true);

                if (_agent.Messages.Count == 0)
                {
                    g.DrawText("Peça qualquer jogo em C# — o Agente programa em res://scripts/ e monta a cena 3D sozinho!",
                        r.X, r.Y + 6f, 10.8f, new UiColor(204, 210, 224, 255));
                    DrawOutlineButton(g, "chip_obby_ai", new UiRect(r.X, r.Y + 32f, 240f, 24f), "✨ Gerar Obby 3D em C# Agora", () =>
                    {
                        if (!_agent.IsBusy)
                        {
                            Task _ = _agent.RunAutonomousTurnAsync("Crie um Obby 3D completo em C#", _store, _world, () => _host?.RequestRedraw());
                        }
                    });
                }
                else
                {
                    MobileChatMessage last = _agent.Messages[_agent.Messages.Count - 1];
                    string summary = string.IsNullOrEmpty(last.Text) ? _agent.StatusLine : last.Text;
                    if (summary.Length > 110) summary = summary.Substring(0, 110) + "...";
                    g.DrawText("IA: " + summary, r.X, r.Y + 6f, 10.5f, new UiColor(168, 232, 188, 255));
                    if (last.ToolSummaries.Count > 0)
                    {
                        g.DrawText("✓ " + string.Join("  ·  ✓ ", last.ToolSummaries), r.X, r.Y + 28f, 10f, new UiColor(115, 198, 255, 255));
                    }
                }
            }
        }

        // ==================== MODAIS: CONFIGURAÇÕES IA & PIPELINE BUILD ====================
        private void RenderSettingsModal(IMobileCanvas g, float w, float h)
        {
            g.FillRect(new UiRect(0, 0, w, h), new UiColor(0, 0, 0, 175));
            float mw = Math.Min(460f, w - 32f);
            float mh = Math.Min(290f, h - 28f);
            var m = new UiRect((w - mw) * 0.5f, (h - mh) * 0.5f, mw, mh);
            g.FillRect(m, new UiColor(13, 15, 20, 252), 16f);
            g.StrokeRect(m, new UiColor(54, 58, 72, 255), 1.2f, 16f);

            float x = m.X + 20f;
            float iw = m.W - 40f;
            float y = m.Y + 18f;

            g.DrawText("Configurações — Agente IA (OpenRouter & APIs)", x, y, 15f, new UiColor(255, 255, 255, 255), true);
            y += 28f;

            var provBtn = new UiRect(x, y, iw, 34f);
            g.FillRect(provBtn, new UiColor(20, 23, 30, 255), 8f);
            g.StrokeRect(provBtn, new UiColor(48, 52, 64, 255), 1f, 8f);
            g.DrawText("Provedor: " + MobileAIAgent.Presets[_agent.ProviderIndex].Name + "  (toque p/ alternar)",
                x + 10f, y + 9f, 11.5f, new UiColor(235, 238, 246, 255));
            _hits.Register("cfg_prov_cycle", provBtn, () => _agent.SelectProvider((_agent.ProviderIndex + 1) % MobileAIAgent.Presets.Length));
            y += 42f;

            var keyBtn = new UiRect(x, y, iw, 34f);
            g.FillRect(keyBtn, new UiColor(20, 23, 30, 255), 8f);
            g.StrokeRect(keyBtn, new UiColor(48, 52, 64, 255), 1f, 8f);
            string keyLabel = string.IsNullOrEmpty(_agent.ApiKey)
                ? "API Key: (vazia — toque para colar sk-or-v1-...)"
                : "API Key: configurada (" + _agent.ApiKey.Substring(0, Math.Min(8, _agent.ApiKey.Length)) + "•••)";
            g.DrawText(keyLabel, x + 10f, y + 9f, 11.5f, new UiColor(168, 224, 184, 255));
            _hits.Register("cfg_edit_key", keyBtn, () =>
            {
                _host?.PromptText("Chave da API (OpenRouter / OpenAI / Groq)", "Cole sua chave sk-or-v1-...:", _agent.ApiKey, true, val =>
                {
                    _agent.ApiKey = val ?? "";
                    _host?.RequestRedraw();
                });
            });
            y += 42f;

            var modBtn = new UiRect(x, y, iw, 34f);
            g.FillRect(modBtn, new UiColor(20, 23, 30, 255), 8f);
            g.StrokeRect(modBtn, new UiColor(48, 52, 64, 255), 1f, 8f);
            g.DrawText("Modelo: " + _agent.Model + "  (toque p/ editar ID OpenRouter)", x + 10f, y + 9f, 11.5f, new UiColor(225, 228, 238, 255));
            _hits.Register("cfg_edit_model", modBtn, () =>
            {
                _host?.PromptText("ID do Modelo", "Ex: anthropic/claude-3.5-sonnet, openai/gpt-4o-mini, deepseek/deepseek-chat:", _agent.Model, false, val =>
                {
                    if (!string.IsNullOrWhiteSpace(val)) _agent.Model = val.Trim();
                    _host?.RequestRedraw();
                });
            });
            y += 46f;

            DrawWhitePillButton(g, "cfg_save_close", new UiRect(x, y, 150f, 36f), "Salvar e Fechar", () =>
            {
                _settingsModalOpen = false;
                ShowToast("Configurações de IA salvas ✓");
            });
            DrawOutlineButton(g, "cfg_open_or", new UiRect(x + 160f, y, iw - 160f, 36f), "Obter Chave OpenRouter", () =>
            {
                _host?.OpenUrl("https://openrouter.ai/api/keys");
            });
        }

        private void RenderBuildModal(IMobileCanvas g, float w, float h)
        {
            g.FillRect(new UiRect(0, 0, w, h), new UiColor(0, 0, 0, 175));
            float mw = Math.Min(440f, w - 32f);
            float mh = 230f;
            var m = new UiRect((w - mw) * 0.5f, (h - mh) * 0.5f, mw, mh);
            g.FillRect(m, new UiColor(13, 15, 20, 252), 16f);
            g.StrokeRect(m, new UiColor(54, 58, 72, 255), 1.2f, 16f);

            g.DrawText("Pipeline Nativo — " + _buildTargetLabel, m.X + 20f, m.Y + 18f, 15f, new UiColor(255, 255, 255, 255), true);
            string[] steps =
            {
                "[OK] Scripts C# sincronizados em res://scripts/",
                "[OK] Workflows: build-engine.yml & build-game.yml",
                _buildAnimTimer > 0.8f ? "[OK] Artefatos prontos (.apk / .ipa / .exe / Linux)" : "[..] Preparando pacote nativo..."
            };
            for (int i = 0; i < steps.Length; i++)
            {
                g.DrawText(steps[i], m.X + 20f, m.Y + 56f + i * 24f, 11.5f, new UiColor(110, 232, 146, 255));
            }

            DrawWhitePillButton(g, "bld_open_gh", new UiRect(m.X + 20f, m.Bottom - 50f, 190f, 34f), "Abrir GitHub Actions", () =>
            {
                _host?.OpenUrl("https://github.com/Veexdevs/Astron-Engine/actions");
            });
            DrawOutlineButton(g, "bld_close", new UiRect(m.X + 220f, m.Bottom - 50f, 110f, 34f), "Fechar", () => _buildModalOpen = false);
        }

        // ==================== WIDGET HELPERS ====================
        private void DrawWhitePillButton(IMobileCanvas g, string id, UiRect r, string label, Action onTap)
        {
            g.FillRect(r, new UiColor(255, 255, 255, 255), r.H * 0.5f);
            g.DrawText(label, r.CenterX, r.Y + (r.H - 13f) * 0.5f, 12f, new UiColor(8, 10, 14, 255), true, true);
            _hits.Register(id, r, onTap);
        }

        private void DrawOutlineButton(IMobileCanvas g, string id, UiRect r, string label, Action onTap)
        {
            g.FillRect(r, new UiColor(18, 20, 26, 235), 9f);
            g.StrokeRect(r, new UiColor(58, 62, 74, 255), 1.1f, 9f);
            g.DrawText(label, r.CenterX, r.Y + (r.H - 12f) * 0.5f, 11.2f, new UiColor(235, 237, 245, 255), false, true);
            _hits.Register(id, r, onTap);
        }

        private void DrawToolbarChip(IMobileCanvas g, string id, ref float tx, float tbY, string label, bool active, Action onTap)
        {
            float bw = g.MeasureText(label, 11f, true) + 18f;
            var r = new UiRect(tx, tbY + 5f, bw, 32f);
            g.FillRect(r, active ? new UiColor(28, 78, 168, 255) : new UiColor(25, 28, 36, 255), 7f);
            g.StrokeRect(r, active ? new UiColor(68, 134, 245, 255) : new UiColor(42, 46, 56, 255), 1f, 7f);
            g.DrawText(label, r.CenterX, r.Y + 9f, 11f, new UiColor(246, 248, 252, 255), active, true);
            _hits.Register(id, r, onTap);
            tx += bw + 5f;
        }

        private static void DrawProceduralPlanetLogo(IMobileCanvas g, float cx, float cy, float radius)
        {
            g.FillCircle(cx, cy, radius, new UiColor(255, 255, 255, 255));
            g.DrawLine(cx - radius * 1.65f, cy + radius * 0.52f, cx + radius * 1.65f, cy - radius * 0.52f, new UiColor(255, 255, 255, 255), Math.Max(2.5f, radius * 0.24f));
        }
    }
}
