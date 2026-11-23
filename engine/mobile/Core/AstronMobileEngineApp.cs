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

    public sealed class MobileConsoleEntry
    {
        public string Level;
        public string Message;

        public MobileConsoleEntry(string level, string message)
        {
            Level = level;
            Message = message;
        }
    }

    /// <summary>
    /// Aplicação completa da Astron Engine nativa para Android (.apk) e iOS (.ipa).
    /// Renderiza as 5 telas (Splash, Login, Hub Início, Compilar e Editor 3D completo)
    /// com fidelidade visual idêntica às referências oficiais e interatividade real 3D/C#/IA.
    /// </summary>
    public sealed class AstronMobileEngineApp
    {
        private readonly IMobileHostBridge _host;
        private readonly MobileProjectStore _store;
        private readonly MobileViewport3D _viewport = new MobileViewport3D();
        private readonly MobileAIAgent _agent = new MobileAIAgent();
        private readonly UiHitMap _hits = new UiHitMap();

        private AstronWorld3D _world = new AstronWorld3D();
        private int _selectedNodeId = 1;

        public MobileScreen Screen { get; set; } = MobileScreen.Splash;

        // Splash
        private float _splashProgress;

        // Login
        private string _loginUser = "";
        private string _loginPass = "";
        private bool _loginRemember = true;
        private bool _loginShowPass;

        // Home & Build
        private string _homeSection = "inicio"; // inicio | projetos | biblioteca | loja | sobre
        private bool _quickBuild;
        private bool _settingsModalOpen;
        private bool _buildModalOpen;
        private string _buildTargetLabel = "APK";
        private float _buildAnimTimer;

        // Editor
        private int _activeTool; // 0=Selecionar, 1=Mover, 2=Rotacionar, 3=Escalar, 4=Transformar
        private bool _isPlaying;
        private bool _isPaused;
        private string _activeCodePath; // null = Cena Principal 3D
        private int _bottomTab; // 0=Console, 1=Saída, 2=Depurador, 3=Áudio, 4=Animação, 5=Editor de Shader, 6=Agente IA
        private int _toolboxTab; // 0=Modelos, 1=Recursos, 2=Materiais
        private bool _showExplorer = true;
        private bool _showFiles = true;
        private bool _showProperties = true;
        private bool _showToolbox = true;
        private bool _showBottom = true;
        private bool _expObjsOpen;

        private readonly List<MobileConsoleEntry> _consoleLogs = new List<MobileConsoleEntry>
        {
            new MobileConsoleEntry("INFO", "Projeto carregado com sucesso!"),
            new MobileConsoleEntry("WARNING", "Nenhum objeto selecionado."),
            new MobileConsoleEntry("ERROR", "Erro ao carregar o script principal.")
        };

        // Toast
        private string _toastText;
        private float _toastTimer;

        // Touch tracking
        private UiRect _lastViewportRect;
        private bool _touchInViewport;
        private float _touchStartX;
        private float _touchStartY;
        private float _lastTouchX;
        private float _lastTouchY;
        private bool _wasDragged;

        // Exact texture aspect ratios measured from assets/textures/
        private const float AspectLockup = 1.4977f;     // astron-lockup.png (647x432)
        private const float AspectHorizontal = 4.0500f; // astron-horizontal.png (405x100)
        private const float AspectHero = 2.5006f;       // hero-planet.jpg (1983x793)

        public AstronMobileEngineApp(IMobileHostBridge host)
        {
            _host = host;
            _store = new MobileProjectStore(host != null ? host.PersistentDataPath : null);
            ResetDefaultScene();
        }

        private void ResetDefaultScene()
        {
            _world = new AstronWorld3D { SceneName = "Cena Principal" };
            var model = _world.SpawnPart(
                "Model",
                AstronPrimitiveKind.Cube,
                new Vec3(0f, 0f, 0f),
                new Vec3(1f, 1f, 1f),
                AstronMaterialPreset.Plastic,
                true
            );
            model.ColorRgba = 0xFFFFFFFFu;
            model.UseGravity = true;
            model.Mass = 1.0f;
            model.CanCollide = true;
            _selectedNodeId = model.Id;
        }

        private SceneNode3D GetSelectedNode()
        {
            for (int i = 0; i < _world.Nodes.Count; i++)
            {
                if (_world.Nodes[i].Id == _selectedNodeId)
                    return _world.Nodes[i];
            }
            return _world.Nodes.Count > 0 ? _world.Nodes[0] : null;
        }

        private void PushLog(string level, string msg)
        {
            _consoleLogs.Add(new MobileConsoleEntry(level, msg));
            if (_consoleLogs.Count > 120)
                _consoleLogs.RemoveAt(0);
        }

        private void ShowToast(string msg)
        {
            _toastText = msg;
            _toastTimer = 2.8f;
            if (_host != null) _host.RequestRedraw();
        }

        public bool NeedsContinuousAnimation =>
            Screen == MobileScreen.Splash || _isPlaying || _buildModalOpen || _toastTimer > 0f || _agent.IsBusy;

        public void Update(float dt)
        {
            if (dt <= 0f) dt = 1f / 60f;
            if (dt > 0.1f) dt = 0.1f;

            if (Screen == MobileScreen.Splash)
            {
                _splashProgress += dt * 52f;
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
                                        !_buildModalOpen &&
                                        _lastViewportRect.Contains(x, y));
                    break;

                case TouchPhase.Moved:
                {
                    float dx = x - _lastTouchX;
                    float dy = y - _lastTouchY;
                    _lastTouchX = x;
                    _lastTouchY = y;

                    if (Math.Abs(x - _touchStartX) + Math.Abs(y - _touchStartY) > 8f)
                        _wasDragged = true;

                    if (_touchInViewport && _wasDragged)
                    {
                        SceneNode3D sel = GetSelectedNode();
                        if ((_activeTool == 1 || _activeTool == 4) && sel != null)
                        {
                            sel.Position = new Vec3(sel.Position.X + dx * 0.025f, sel.Position.Y - dy * 0.025f, sel.Position.Z);
                        }
                        else if (_activeTool == 2 && sel != null)
                        {
                            sel.RotationEulerDeg = new Vec3(sel.RotationEulerDeg.X, sel.RotationEulerDeg.Y + dx * 0.8f, sel.RotationEulerDeg.Z);
                        }
                        else if (_activeTool == 3 && sel != null)
                        {
                            float s = Math.Max(0.20f, sel.Scale.X + (dx - dy) * 0.015f);
                            sel.Scale = new Vec3(s, s, s);
                        }
                        else
                        {
                            _viewport.Orbit(-dx * 0.42f, dy * 0.36f);
                        }
                        if (_host != null) _host.RequestRedraw();
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
                    if (_host != null) _host.RequestRedraw();
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
                if (_host != null) _host.RequestRedraw();
            }
        }

        public void Render(IMobileCanvas g)
        {
            _hits.Clear();
            float w = Math.Max(480f, g.Width);
            float h = Math.Max(320f, g.Height);

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
                RenderBuildPipelineModal(g, w, h);

            if (!string.IsNullOrEmpty(_toastText))
                RenderToast(g, w, h);
        }

        // =========================================================================
        // SHARED WIDGET HELPERS
        // =========================================================================
        private void DrawWindowControls(IMobileCanvas g, float w)
        {
            float bx = w - 106f;
            float by = 5f;
            UiColor ic = new UiColor(208, 212, 222, 235);
            g.DrawLine(bx + 10f, by + 11f, bx + 20f, by + 11f, ic, 1.4f);
            g.StrokeRect(new UiRect(bx + 42f, by + 6f, 10f, 10f), ic, 1.3f, 1.5f);
            g.DrawLine(bx + 74f, by + 6f, bx + 84f, by + 16f, ic, 1.5f);
            g.DrawLine(bx + 84f, by + 6f, bx + 74f, by + 16f, ic, 1.5f);
        }

        private void DrawToggleSwitch(IMobileCanvas g, string hitId, float x, float y, bool value, bool blueActive, Action onToggle)
        {
            UiRect r = new UiRect(x, y, 36f, 19f);
            UiColor bg = value
                ? (blueActive ? new UiColor(37, 114, 246, 255) : new UiColor(242, 244, 250, 255))
                : new UiColor(48, 50, 58, 255);
            UiColor knob = (value && !blueActive) ? new UiColor(14, 15, 19, 255) : new UiColor(255, 255, 255, 255);
            g.FillRect(r, bg, 9.5f);
            float kx = value ? (r.Right - 9.5f) : (r.X + 9.5f);
            g.FillCircle(kx, r.CenterY, 7.1f, knob);
            _hits.Register(hitId, new UiRect(x - 4f, y - 4f, 44f, 27f), onToggle);
        }

        private void DrawWhitePillBtn(IMobileCanvas g, string hitId, UiRect rect, string label, float radius,
                                      MobileIcon leftIcon, Action onTap)
        {
            g.FillRect(rect, new UiColor(252, 252, 255, 255), radius);
            UiColor fg = new UiColor(10, 11, 15, 255);
            float tw = g.MeasureText(label, 13.5f, true);
            float iconSpace = leftIcon != MobileIcon.None ? 20f : 0f;
            float sx = rect.CenterX - (tw + iconSpace) * 0.5f;
            if (leftIcon != MobileIcon.None)
            {
                MobileVectorIcons.DrawIcon(g, sx + 6f, rect.CenterY, 13.5f, leftIcon, fg);
                sx += iconSpace;
            }
            g.DrawText(label, sx, rect.CenterY - 7f, 13.5f, fg, true, false);
            _hits.Register(hitId, rect, onTap);
        }

        private void DrawOutlineBtn(IMobileCanvas g, string hitId, UiRect rect, string label, float radius,
                                    MobileIcon leftIcon, MobileIcon rightIcon, Action onTap)
        {
            g.FillRect(rect, new UiColor(16, 17, 22, 255), radius);
            g.StrokeRect(rect, new UiColor(52, 55, 66, 255), 1.1f, radius);
            UiColor fg = new UiColor(236, 238, 244, 255);
            float tw = g.MeasureText(label, 12.5f, false);
            float lw = leftIcon != MobileIcon.None ? 20f : 0f;
            float rw = rightIcon != MobileIcon.None ? 18f : 0f;
            float sx = rect.CenterX - (tw + lw + rw) * 0.5f;
            if (leftIcon != MobileIcon.None)
            {
                MobileVectorIcons.DrawIcon(g, sx + 6f, rect.CenterY, 13.5f, leftIcon, fg);
                sx += lw;
            }
            g.DrawText(label, sx, rect.CenterY - 6.5f, 12.5f, fg, false, false);
            if (rightIcon != MobileIcon.None)
            {
                MobileVectorIcons.DrawIcon(g, sx + tw + 11f, rect.CenterY, 12f, rightIcon, fg);
            }
            _hits.Register(hitId, rect, onTap);
        }

        private void DrawSidebarNavItem(IMobileCanvas g, string hitId, UiRect rect, MobileIcon icon,
                                        string label, bool active, Action onTap)
        {
            if (active)
            {
                g.FillRect(rect, new UiColor(28, 30, 37, 255), 10f);
                g.StrokeRect(rect, new UiColor(52, 55, 66, 185), 1f, 10f);
                g.FillRect(new UiRect(rect.X + 1f, rect.Y + 9f, 3f, rect.H - 18f), new UiColor(255, 255, 255, 255), 1.5f);
            }
            UiColor fg = active ? new UiColor(255, 255, 255, 255) : new UiColor(154, 158, 170, 255);
            MobileVectorIcons.DrawIcon(g, rect.X + 21f, rect.CenterY, 15f, icon, fg);
            g.DrawText(label, rect.X + 40f, rect.CenterY - 7f, 13f, fg, active, false);
            _hits.Register(hitId, rect, onTap);
        }

        private void PromptAgentOrCommand(string initialText = "")
        {
            if (_host == null) return;
            _host.PromptText(
                "Astron Engine — Console / Agente IA",
                "Digite um comando ou descreva o jogo/sistema C# para o Agente IA autônomo:",
                initialText,
                false,
                txt =>
                {
                    if (string.IsNullOrWhiteSpace(txt)) return;
                    string cmd = txt.Trim();
                    PushLog("INFO", "> " + cmd);
                    if (string.Equals(cmd, "clear", StringComparison.OrdinalIgnoreCase))
                    {
                        _consoleLogs.Clear();
                        return;
                    }
                    _bottomTab = 6;
                    _showBottom = true;
                    Task.Run(() => _agent.RunAutonomousTurnAsync(cmd, _store, _world, () =>
                    {
                        if (_host != null) _host.RequestRedraw();
                    }));
                }
            );
        }

        // =========================================================================
        // TELA 1: SPLASH SCREEN (Identical to Image 5)
        // =========================================================================
        private void RenderSplash(IMobileCanvas g, float w, float h)
        {
            g.Clear(new UiColor(0, 0, 0, 255));

            float minDim = Math.Min(w, h);
            MobileVectorIcons.DrawDarkOrbitalSphere(g, w * 0.04f, h * 0.03f, minDim * 0.56f,
                (float)(Math.PI * 0.02), (float)(Math.PI * 0.52));
            MobileVectorIcons.DrawDarkOrbitalSphere(g, w * 0.96f, h * 0.99f, minDim * 0.64f,
                (float)(Math.PI * 1.02), (float)(Math.PI * 1.52));

            float cx = w * 0.5f;
            float cy = h * 0.45f;

            float lh = 166f;
            float lw = lh * AspectLockup;
            if (!g.DrawImage("astron-lockup.png", new UiRect(cx - lw * 0.5f, cy - lh * 0.60f, lw, lh)))
            {
                g.DrawText("ASTRON", cx, cy - 12f, 34f, new UiColor(255, 255, 255, 255), true, true);
                g.DrawText("E N G I N E", cx, cy + 28f, 12f, new UiColor(168, 172, 182, 255), false, true);
            }

            float barW = Math.Min(360f, w * 0.32f);
            float barH = 13f;
            UiRect track = new UiRect(cx - barW * 0.5f, cy + 94f, barW, barH);
            g.FillRect(track, new UiColor(8, 9, 12, 240), barH * 0.5f);
            g.StrokeRect(track, new UiColor(215, 220, 232, 125), 1.25f, barH * 0.5f);

            const float pad = 2.4f;
            float fillW = Math.Max(barH - pad * 2f, (barW - pad * 2f) * (_splashProgress / 100f));
            g.FillRect(new UiRect(track.X + pad, track.Y + pad, fillW, barH - pad * 2f),
                       new UiColor(252, 252, 255, 255), (barH - pad * 2f) * 0.5f);

            g.DrawText("Carregando...", cx, track.Bottom + 12f, 12.5f, new UiColor(165, 170, 180, 235), false, true);

            _hits.Register("splash_skip", new UiRect(0, 0, w, h), () => Screen = MobileScreen.Login);
        }

        // =========================================================================
        // TELA 2: LOGIN (Identical to Image 3)
        // =========================================================================
        private void RenderLogin(IMobileCanvas g, float w, float h)
        {
            g.Clear(new UiColor(0, 0, 0, 255));

            MobileVectorIcons.DrawDarkOrbitalSphere(g, w * 0.05f, -h * 0.06f, Math.Min(w, h) * 0.48f,
                (float)(Math.PI * 0.08), (float)(Math.PI * 0.48));

            for (int i = 0; i < 30; i++)
            {
                float sx = (i * 197.3f + 43f) % (w * 0.52f);
                float sy = (i * 131.7f + 71f) % (h * 0.92f);
                byte al = (byte)(24 + (i % 4) * 18);
                g.FillCircle(sx, sy, (i % 3 == 0) ? 1.2f : 0.9f, new UiColor(220, 226, 242, al));
            }

            // Bottom-left cratered planet (login-planet.png is 1254x1254, 1:1 aspect preserved!)
            float planetSz = h * 0.56f;
            g.DrawImage("login-planet.png", new UiRect(0f, h - planetSz, planetSz, planetSz), 245);

            // Left Half: Astron Engine lockup + tagline
            float leftCx = w * 0.265f;
            float leftCy = h * 0.45f;
            float lh = 172f;
            float lw = lh * AspectLockup;
            if (!g.DrawImage("astron-lockup.png", new UiRect(leftCx - lw * 0.5f, leftCy - 118f, lw, lh)))
            {
                g.DrawText("ASTRON", leftCx, leftCy - 16f, 34f, new UiColor(255, 255, 255, 255), true, true);
            }
            g.DrawText("C r i e .   D e s e n v o l v a .   E x p l o r e .",
                       leftCx, leftCy + 70f, 12.5f, new UiColor(182, 186, 198, 235), false, true);

            // Center vertical divider
            g.DrawLine(w * 0.50f, h * 0.14f, w * 0.50f, h * 0.86f, new UiColor(255, 255, 255, 22), 1f);

            // Bottom-left Settings button with Gear icon
            UiRect cfgBtn = new UiRect(24f, h - 46f, 138f, 30f);
            MobileVectorIcons.DrawIcon(g, cfgBtn.X + 12f, cfgBtn.CenterY, 14.5f, MobileIcon.Gear, new UiColor(200, 204, 214, 240));
            g.DrawText("Configurações", cfgBtn.X + 27f, cfgBtn.CenterY - 7f, 12.5f, new UiColor(200, 204, 214, 240));
            _hits.Register("login_cfg", cfgBtn, () => _settingsModalOpen = true);

            // Bottom-right version
            g.DrawText("v1.0.0", w - 60f, h - 34f, 12f, new UiColor(138, 142, 154, 215));

            // Right Half: Dark rounded Login Card (Image 3)
            float cardW = Math.Min(406f, w * 0.43f);
            float cardH = Math.Min(476f, h - 48f);
            UiRect card = new UiRect(w * 0.74f - cardW * 0.5f, (h - cardH) * 0.5f, cardW, cardH);
            g.FillRect(card, new UiColor(13, 14, 18, 244), 20f);
            g.StrokeRect(card, new UiColor(46, 49, 60, 175), 1.1f, 20f);

            float pad = 32f;
            float fx = card.X + pad;
            float fw = card.W - pad * 2f;
            float fy = card.Y + 30f;

            g.DrawText("Bem-vindo(a)!", fx, fy, 25f, new UiColor(250, 250, 255, 255), true);
            fy += 34f;
            g.DrawText("Entre na sua conta para continuar", fx, fy, 12.5f, new UiColor(148, 152, 165, 255));
            fy += 32f;

            // Field 1: Usuário ou e-mail
            UiRect userBox = new UiRect(fx, fy, fw, 44f);
            g.FillRect(userBox, new UiColor(21, 22, 28, 255), 11f);
            g.StrokeRect(userBox, new UiColor(46, 49, 60, 210), 1f, 11f);
            MobileVectorIcons.DrawIcon(g, userBox.X + 19f, userBox.CenterY, 14.5f, MobileIcon.User, new UiColor(150, 154, 166, 235));
            string uDisp = string.IsNullOrEmpty(_loginUser) ? "Usuário ou e-mail" : _loginUser;
            UiColor uCol = string.IsNullOrEmpty(_loginUser) ? new UiColor(126, 130, 142, 255) : new UiColor(242, 244, 250, 255);
            g.DrawText(uDisp, userBox.X + 38f, userBox.CenterY - 6.5f, 12.5f, uCol);
            _hits.Register("login_user", userBox, () =>
            {
                if (_host != null)
                    _host.PromptText("Usuário ou e-mail", "Digite seu nome de usuário ou e-mail:", _loginUser, false, v => _loginUser = v);
            });
            fy += 54f;

            // Field 2: Senha + Eye/EyeOff toggle
            UiRect passBox = new UiRect(fx, fy, fw, 44f);
            g.FillRect(passBox, new UiColor(21, 22, 28, 255), 11f);
            g.StrokeRect(passBox, new UiColor(46, 49, 60, 210), 1f, 11f);
            MobileVectorIcons.DrawIcon(g, passBox.X + 19f, passBox.CenterY, 14.5f, MobileIcon.Lock, new UiColor(150, 154, 166, 235));
            string pDisp = string.IsNullOrEmpty(_loginPass)
                ? "Senha"
                : (_loginShowPass ? _loginPass : new string('•', Math.Min(16, _loginPass.Length)));
            UiColor pCol = string.IsNullOrEmpty(_loginPass) ? new UiColor(126, 130, 142, 255) : new UiColor(242, 244, 250, 255);
            g.DrawText(pDisp, passBox.X + 38f, passBox.CenterY - 6.5f, 12.5f, pCol);

            UiRect eyeHit = new UiRect(passBox.Right - 38f, passBox.Y, 38f, passBox.H);
            MobileVectorIcons.DrawIcon(g, passBox.Right - 19f, passBox.CenterY, 15f,
                _loginShowPass ? MobileIcon.Eye : MobileIcon.EyeOff, new UiColor(154, 158, 170, 235));
            _hits.Register("login_pass", new UiRect(passBox.X, passBox.Y, passBox.W - 40f, passBox.H), () =>
            {
                if (_host != null)
                    _host.PromptText("Senha", "Digite sua senha:", _loginPass, true, v => _loginPass = v);
            });
            _hits.Register("login_eye", eyeHit, () => _loginShowPass = !_loginShowPass);
            fy += 54f;

            // Checkbox "Lembrar de mim" + underlined "Esqueceu sua senha?"
            UiRect chk = new UiRect(fx, fy + 1f, 15f, 15f);
            g.FillRect(chk, _loginRemember ? new UiColor(37, 114, 246, 255) : new UiColor(20, 21, 26, 255), 3.5f);
            g.StrokeRect(chk, new UiColor(72, 76, 88, 230), 1f, 3.5f);
            if (_loginRemember)
            {
                g.DrawLine(chk.X + 3.5f, chk.Y + 8f, chk.X + 6.5f, chk.Y + 11f, new UiColor(255, 255, 255, 255), 1.6f);
                g.DrawLine(chk.X + 6.5f, chk.Y + 11f, chk.X + 12f, chk.Y + 4.5f, new UiColor(255, 255, 255, 255), 1.6f);
            }
            g.DrawText("Lembrar de mim", fx + 22f, fy + 1f, 12f, new UiColor(178, 182, 194, 255));
            _hits.Register("login_rem", new UiRect(fx, fy - 2f, 130f, 22f), () => _loginRemember = !_loginRemember);

            const string forgotTxt = "Esqueceu sua senha?";
            float fgW = g.MeasureText(forgotTxt, 12f, false);
            float fgX = fx + fw - fgW;
            g.DrawText(forgotTxt, fgX, fy + 1f, 12f, new UiColor(220, 224, 234, 240));
            g.DrawLine(fgX, fy + 16f, fgX + fgW, fy + 16f, new UiColor(220, 224, 234, 210), 1f);
            _hits.Register("login_forgot", new UiRect(fgX - 4f, fy - 2f, fgW + 8f, 22f),
                () => ShowToast("Recuperação disponível nas Configurações."));
            fy += 32f;

            // Primary white "Entrar" button
            DrawWhitePillBtn(g, "login_submit", new UiRect(fx, fy, fw, 44f), "Entrar", 12f, MobileIcon.None, () =>
            {
                if (string.IsNullOrEmpty(_loginUser)) _loginUser = "Desenvolvedor";
                Screen = MobileScreen.Home;
            });
            fy += 58f;

            // "── ou ──"
            g.DrawLine(fx, fy + 7f, fx + fw * 0.42f, fy + 7f, new UiColor(48, 50, 60, 220), 1f);
            g.DrawText("ou", fx + fw * 0.5f, fy, 12f, new UiColor(138, 142, 152, 230), false, true);
            g.DrawLine(fx + fw * 0.58f, fy + 7f, fx + fw, fy + 7f, new UiColor(48, 50, 60, 220), 1f);
            fy += 24f;

            // "Entrar com GitHub" with Octocat icon
            DrawOutlineBtn(g, "login_gh", new UiRect(fx, fy, fw, 44f), "Entrar com GitHub", 12f,
                MobileIcon.GitHub, MobileIcon.None, () =>
                {
                    _loginUser = "GitHub Developer";
                    Screen = MobileScreen.Home;
                    ShowToast("Autenticado via GitHub OAuth.");
                });
            fy += 58f;

            // "Não tem uma conta? Criar conta"
            string p1 = "Não tem uma conta? ";
            string p2 = "Criar conta";
            float s1 = g.MeasureText(p1, 12f, false);
            float s2 = g.MeasureText(p2, 12f, true);
            float sxAcc = fx + (fw - (s1 + s2)) * 0.5f;
            g.DrawText(p1, sxAcc, fy, 12f, new UiColor(144, 148, 160, 255));
            g.DrawText(p2, sxAcc + s1, fy, 12f, new UiColor(250, 250, 255, 255), true);
            _hits.Register("login_create", new UiRect(sxAcc, fy - 4f, s1 + s2, 22f), () =>
            {
                _loginUser = "AstronCreator";
                Screen = MobileScreen.Home;
            });

            DrawWindowControls(g, w);
        }

        // =========================================================================
        // TELA 3: HUB INÍCIO (Identical to Image 2)
        // =========================================================================
        private void RenderHome(IMobileCanvas g, float w, float h)
        {
            g.Clear(new UiColor(7, 8, 11, 255));

            float sideW = 228f;
            g.FillRect(new UiRect(0, 0, sideW, h), new UiColor(9, 10, 13, 255));
            g.DrawLine(sideW, 0, sideW, h, new UiColor(34, 36, 44, 255), 1f);

            // Official horizontal Astron Engine lockup
            float logoW = 166f;
            float logoH = logoW / AspectHorizontal;
            if (!g.DrawImage("astron-horizontal.png", new UiRect(18f, 20f, logoW, logoH)))
            {
                g.DrawText("ASTRON ENGINE", 20f, 24f, 15f, new UiColor(255, 255, 255, 255), true);
            }

            float ny = 82f;
            float nw = sideW - 26f;
            DrawSidebarNavItem(g, "hm_ini",  new UiRect(13f, ny, nw, 38f), MobileIcon.Home,       "Início",        _homeSection == "inicio",     () => _homeSection = "inicio"); ny += 42f;
            DrawSidebarNavItem(g, "hm_prj",  new UiRect(13f, ny, nw, 38f), MobileIcon.Folder,     "Meus Projetos", _homeSection == "projetos",   () => _homeSection = "projetos"); ny += 42f;
            DrawSidebarNavItem(g, "hm_bib",  new UiRect(13f, ny, nw, 38f), MobileIcon.Cube3D,     "Bibioteca",     _homeSection == "biblioteca", () => _homeSection = "biblioteca"); ny += 42f;
            DrawSidebarNavItem(g, "hm_loja", new UiRect(13f, ny, nw, 38f), MobileIcon.Bag,        "Loja",          _homeSection == "loja",       () => _homeSection = "loja"); ny += 42f;
            DrawSidebarNavItem(g, "hm_cfg",  new UiRect(13f, ny, nw, 38f), MobileIcon.Gear,       "Configurações", false,                        () => _settingsModalOpen = true); ny += 42f;
            DrawSidebarNavItem(g, "hm_sob",  new UiRect(13f, ny, nw, 38f), MobileIcon.InfoCircle, "Sobre",         _homeSection == "sobre",      () => _homeSection = "sobre");

            // Bottom Engine Status Card ("⚡ Astron Engine / v1.0.0 | ● Online")
            UiRect stBox = new UiRect(13f, h - 74f, nw, 58f);
            g.FillRect(stBox, new UiColor(15, 16, 21, 255), 12f);
            g.StrokeRect(stBox, new UiColor(36, 38, 48, 255), 1f, 12f);
            MobileVectorIcons.DrawIcon(g, stBox.X + 15f, stBox.Y + 19f, 13.5f, MobileIcon.Bolt, new UiColor(238, 240, 248, 255));
            g.DrawText("Astron Engine", stBox.X + 28f, stBox.Y + 11f, 12.5f, new UiColor(240, 242, 248, 255), true);
            g.DrawText("v1.0.0", stBox.X + 12f, stBox.Y + 34f, 11.5f, new UiColor(126, 130, 142, 255));
            g.FillCircle(stBox.Right - 58f, stBox.Y + 41f, 3.8f, new UiColor(46, 214, 114, 255));
            g.DrawText("Online", stBox.Right - 49f, stBox.Y + 34f, 11.5f, new UiColor(178, 182, 194, 255));

            // Main Hub Content
            float padX = 28f;
            float contentW = (w - sideW) - padX * 2f;

            // Top-right circular Gear & User buttons (Image 2)
            float topCy = 34f;
            float uCx = w - padX - 18f;
            float gCx = uCx - 44f;
            g.FillCircle(gCx, topCy, 17.5f, new UiColor(22, 23, 29, 255));
            g.StrokeCircle(gCx, topCy, 17.5f, new UiColor(46, 49, 60, 210), 1f);
            MobileVectorIcons.DrawIcon(g, gCx, topCy, 15.5f, MobileIcon.Gear, new UiColor(214, 218, 228, 255));
            _hits.Register("hm_top_gear", new UiRect(gCx - 18f, topCy - 18f, 36f, 36f), () => _settingsModalOpen = true);

            g.FillCircle(uCx, topCy, 17.5f, new UiColor(22, 23, 29, 255));
            g.StrokeCircle(uCx, topCy, 17.5f, new UiColor(46, 49, 60, 210), 1f);
            MobileVectorIcons.DrawIcon(g, uCx, topCy, 15.5f, MobileIcon.User, new UiColor(214, 218, 228, 255));
            _hits.Register("hm_top_user", new UiRect(uCx - 18f, topCy - 18f, 36f, 36f),
                () => ShowToast("Conta: " + (string.IsNullOrEmpty(_loginUser) ? "Desenvolvedor" : _loginUser)));

            // Hero Planet Banner (hero-planet.jpg cover-cropped)
            UiRect banner = new UiRect(sideW + padX, 62f, contentW, 214f);
            if (!MobileVectorIcons.DrawImageCover(g, "hero-planet.jpg", AspectHero, banner, 255, 18f))
            {
                g.FillRect(banner, new UiColor(18, 20, 28, 255), 18f);
            }
            g.FillRect(new UiRect(banner.X, banner.Y, banner.W * 0.48f, banner.H), new UiColor(6, 7, 10, 128), 18f);
            g.StrokeRect(banner, new UiColor(52, 56, 68, 175), 1f, 18f);

            g.DrawText("Crie. Desenvolva. Explore.", banner.X + 34f, banner.Y + 44f, 26f, new UiColor(255, 255, 255, 255), true);
            g.DrawText("Sua imaginação, sem limites.", banner.X + 34f, banner.Y + 82f, 13.5f, new UiColor(194, 198, 210, 255));

            DrawWhitePillBtn(g, "hm_new_hero", new UiRect(banner.X + 34f, banner.Y + 128f, 162f, 42f),
                "Novo Projeto", 21f, MobileIcon.Plus, () => PromptCreateNewProject());

            // Section Header: Clock icon + "Projetos Recentes" + "Ver Todos  >"
            float secY = banner.Bottom + 24f;
            MobileVectorIcons.DrawIcon(g, sideW + padX + 9f, secY + 9f, 16f, MobileIcon.Clock, new UiColor(210, 214, 226, 255));
            g.DrawText(_homeSection == "projetos" ? "Todos os Projetos" : "Projetos Recentes",
                       sideW + padX + 25f, secY + 1f, 15.5f, new UiColor(244, 246, 252, 255), true);

            const string seeAll = "Ver Todos  >";
            float saW = g.MeasureText(seeAll, 12.5f, false);
            UiRect saRect = new UiRect(sideW + padX + contentW - saW - 6f, secY, saW + 6f, 22f);
            g.DrawText(seeAll, saRect.X, secY + 2f, 12.5f, new UiColor(168, 172, 184, 255));
            _hits.Register("hm_see_all", saRect, () => _homeSection = _homeSection == "projetos" ? "inicio" : "projetos");

            // 4-Column Project Grid (Meu Jogo, Projeto RP, Obby Teste, + Novo Projeto)
            float gridY = secY + 34f;
            float gap = 16f;
            int cols = 4;
            float cardW = (contentW - gap * (cols - 1)) / cols;
            float cardH = Math.Max(184f, Math.Min(218f, h - gridY - 22f));
            float thumbH = cardH - 64f;

            int projCount = Math.Min(3, _store.Projects.Count);
            for (int i = 0; i <= projCount; i++)
            {
                float cx = sideW + padX + i * (cardW + gap);
                UiRect cRect = new UiRect(cx, gridY, cardW, cardH);

                if (i < projCount)
                {
                    MobileProjectInfo pr = _store.Projects[i];
                    g.FillRect(cRect, new UiColor(14, 15, 20, 255), 14f);

                    float thAspect = pr.ThumbAsset == "thumb-city.jpg" ? 1.7511f : 2.0f;
                    MobileVectorIcons.DrawImageCover(g, pr.ThumbAsset, thAspect,
                        new UiRect(cRect.X, cRect.Y, cRect.W, thumbH), 255, 14f);
                    // Cover bottom of rounded thumbnail cleanly into card footer
                    g.FillRect(new UiRect(cRect.X, cRect.Y + thumbH - 6f, cRect.W, cRect.H - thumbH + 6f),
                               new UiColor(14, 15, 20, 255), 14f);
                    g.FillRect(new UiRect(cRect.X, cRect.Y + thumbH - 6f, cRect.W, 12f), new UiColor(14, 15, 20, 255));
                    g.StrokeRect(cRect, new UiColor(36, 38, 48, 225), 1f, 14f);

                    // Top-right circular "•••" button
                    float dCx = cRect.Right - 20f;
                    float dCy = cRect.Y + 19f;
                    g.FillCircle(dCx, dCy, 12.5f, new UiColor(12, 13, 18, 205));
                    g.StrokeCircle(dCx, dCy, 12.5f, new UiColor(70, 74, 88, 150), 1f);
                    MobileVectorIcons.DrawIcon(g, dCx, dCy, 12.5f, MobileIcon.DotsH, new UiColor(226, 228, 236, 255));

                    g.DrawText(pr.Name, cRect.X + 13f, cRect.Y + thumbH + 11f, 14f, new UiColor(246, 247, 252, 255), true);
                    MobileVectorIcons.DrawIcon(g, cRect.X + 19f, cRect.Y + thumbH + 43f, 11.5f, MobileIcon.Clock, new UiColor(134, 138, 150, 255));
                    g.DrawText("Criado em " + pr.CreatedDate, cRect.X + 29f, cRect.Y + thumbH + 36f, 11.5f, new UiColor(138, 142, 154, 255));

                    string pid = pr.Id;
                    string pname = pr.Name;
                    _hits.Register("hm_card_" + pid, new UiRect(cRect.X, cRect.Y, cRect.W - 38f, cRect.H), () =>
                    {
                        _store.SelectProject(pid);
                        Screen = MobileScreen.Editor;
                        ShowToast("Projeto aberto: " + pname);
                    });
                    _hits.Register("hm_dots_" + pid, new UiRect(dCx - 16f, dCy - 16f, 32f, 32f), () =>
                    {
                        _store.SelectProject(pid);
                        Screen = MobileScreen.Build;
                    });
                }
                else
                {
                    // 4th Card: Dashed "+ Novo Projeto / Comece do zero" (Image 2)
                    g.FillRect(cRect, new UiColor(11, 12, 16, 220), 14f);
                    MobileVectorIcons.DrawDashedRoundedRect(g, cRect, 14f, new UiColor(58, 62, 74, 210), 1.3f);

                    float pCx = cRect.CenterX;
                    float pCy = cRect.Y + cRect.H * 0.37f;
                    g.FillCircle(pCx, pCy, 21f, new UiColor(24, 26, 33, 255));
                    g.StrokeCircle(pCx, pCy, 21f, new UiColor(48, 52, 64, 220), 1f);
                    MobileVectorIcons.DrawIcon(g, pCx, pCy, 15f, MobileIcon.Plus, new UiColor(236, 238, 245, 255));

                    g.DrawText("Novo Projeto", pCx, pCy + 32f, 13.5f, new UiColor(244, 245, 250, 255), true, true);
                    g.DrawText("Comece do zero", pCx, pCy + 52f, 11.5f, new UiColor(132, 136, 148, 255), false, true);

                    _hits.Register("hm_card_new", cRect, () => PromptCreateNewProject());
                }
            }

            DrawWindowControls(g, w);
        }

        private void PromptCreateNewProject()
        {
            if (_host == null) return;
            _host.PromptText("Novo Projeto C# — Astron Engine", "Digite o nome do novo projeto 3D:", "Novo Jogo C#", false, name =>
            {
                if (string.IsNullOrWhiteSpace(name)) return;
                var info = _store.CreateProject(name, "obby");
                ResetDefaultScene();
                Screen = MobileScreen.Editor;
                ShowToast("Projeto criado: " + info.Name);
            });
        }

        // =========================================================================
        // TELA 4: COMPILAR / BUILD (Identical to Image 4)
        // =========================================================================
        private void RenderBuild(IMobileCanvas g, float w, float h)
        {
            g.Clear(new UiColor(7, 8, 11, 255));

            float sideW = 228f;
            UiRect sideRect = new UiRect(0, 0, sideW, h);
            g.FillRect(sideRect, new UiColor(9, 10, 13, 255));

            // Cratered planet glowing behind bottom-left of sidebar (Image 4)
            g.PushClip(sideRect);
            g.DrawImage("login-planet.png", new UiRect(-24f, h - 248f, 256f, 256f), 215);
            g.PopClip();

            g.DrawLine(sideW, 0, sideW, h, new UiColor(34, 36, 44, 255), 1f);

            float logoW = 166f;
            float logoH = logoW / AspectHorizontal;
            if (!g.DrawImage("astron-horizontal.png", new UiRect(18f, 20f, logoW, logoH)))
            {
                g.DrawText("ASTRON ENGINE", 20f, 24f, 15f, new UiColor(255, 255, 255, 255), true);
            }

            float ny = 82f;
            float nw = sideW - 26f;
            DrawSidebarNavItem(g, "bd_comp", new UiRect(13f, ny, nw, 38f), MobileIcon.Home,       "Compilar",      true,  () => { }); ny += 42f;
            DrawSidebarNavItem(g, "bd_proj", new UiRect(13f, ny, nw, 38f), MobileIcon.Folder,     "Projetos",      false, () => Screen = MobileScreen.Home); ny += 42f;
            DrawSidebarNavItem(g, "bd_cfg",  new UiRect(13f, ny, nw, 38f), MobileIcon.Gear,       "Configurações", false, () => _settingsModalOpen = true); ny += 42f;
            DrawSidebarNavItem(g, "bd_sob",  new UiRect(13f, ny, nw, 38f), MobileIcon.InfoCircle, "Sobre",         false, () => ShowToast("Astron Build Pipeline v1.0.0 — APK, EXE, IPA, Linux e ZIP."));

            // Frosted "Pronto para compilar / v1.0.0" box at bottom of sidebar (Image 4)
            UiRect stBox = new UiRect(13f, h - 80f, nw, 64f);
            g.FillRect(stBox, new UiColor(14, 15, 20, 224), 12f);
            g.StrokeRect(stBox, new UiColor(54, 58, 70, 195), 1f, 12f);
            g.FillCircle(stBox.X + 14f, stBox.Y + 21f, 3.8f, new UiColor(242, 244, 250, 255));
            g.DrawText("Pronto para compilar", stBox.X + 25f, stBox.Y + 14f, 12f, new UiColor(236, 238, 245, 255));
            g.DrawText("v1.0.0", stBox.X + 12f, stBox.Y + 39f, 11.5f, new UiColor(138, 142, 154, 255));

            // Main Build Content
            float padX = 28f;
            float contentW = (w - sideW) - padX * 2f;

            // Top Banner with official mark + hero-planet.jpg (Image 4)
            UiRect ban = new UiRect(sideW + padX, 28f, contentW, 118f);
            if (!MobileVectorIcons.DrawImageCover(g, "hero-planet.jpg", AspectHero, ban, 255, 16f))
            {
                g.FillRect(ban, new UiColor(16, 18, 24, 255), 16f);
            }
            g.FillRect(ban, new UiColor(8, 9, 13, 135), 16f);
            g.StrokeRect(ban, new UiColor(48, 52, 64, 185), 1f, 16f);

            g.DrawImage("astron-mark.png", new UiRect(ban.X + 26f, ban.CenterY - 25f, 50f, 50f));
            g.DrawText("Astron Engine", ban.X + 94f, ban.Y + 30f, 24f, new UiColor(255, 255, 255, 255), true);
            g.DrawText("Crie. Desenvolva. Explore.", ban.X + 94f, ban.Y + 64f, 13f, new UiColor(192, 196, 208, 255));

            // Rocket icon + "Buildar Seu Projeto"
            float headY = ban.Bottom + 20f;
            MobileVectorIcons.DrawIcon(g, sideW + padX + 10f, headY + 11f, 18f, MobileIcon.Rocket, new UiColor(244, 246, 252, 255));
            g.DrawText("Buildar Seu Projeto", sideW + padX + 27f, headY, 22f, new UiColor(248, 249, 252, 255), true);
            g.DrawText("Escolha a plataforma e gere seu arquivo de build com apenas um clique.",
                       sideW + padX, headY + 29f, 12.5f, new UiColor(150, 154, 166, 255));

            // 6 Platform Cards in 1 Horizontal Row (APK, EXE, IPA, Linux, ZIP, Outros)
            string[] titles = { "APK", "EXE", "IPA", "Linux", "ZIP", "Outros" };
            string[] subs   = { "Android", "Windows", "iOS", "Nativo x64", "Projeto Compactado", "Linux, Mac e mais" };
            string[] btns   = { "Gerar APK", "Gerar EXE", "Gerar IPA", "Gerar Linux", "Gerar ZIP", "Ver Mais" };
            MobilePlatformKind[] pKinds =
            {
                MobilePlatformKind.APK, MobilePlatformKind.EXE, MobilePlatformKind.IPA,
                MobilePlatformKind.Linux, MobilePlatformKind.ZIP, MobilePlatformKind.Outros
            };

            float rowY = headY + 58f;
            float gap = 12f;
            float cardW = (contentW - gap * 5f) / 6f;
            float cardH = 192f;

            for (int i = 0; i < 6; i++)
            {
                float cx = sideW + padX + i * (cardW + gap);
                UiRect cRect = new UiRect(cx, rowY, cardW, cardH);
                bool sel = (_buildTargetLabel == titles[i]);
                g.FillRect(cRect, sel ? new UiColor(20, 22, 29, 255) : new UiColor(13, 14, 19, 255), 16f);
                g.StrokeRect(cRect, sel ? new UiColor(102, 108, 128, 240) : new UiColor(38, 40, 50, 225), 1f, 16f);

                MobileVectorIcons.DrawPlatformIcon(g, cRect.CenterX, cRect.Y + 48f, 42f, pKinds[i]);
                g.DrawText(titles[i], cRect.CenterX, cRect.Y + 92f, 16f, new UiColor(248, 249, 252, 255), true, true);
                g.DrawText(subs[i], cRect.CenterX, cRect.Y + 115f, 11.5f, new UiColor(138, 142, 154, 255), false, true);

                string targetName = titles[i];
                DrawOutlineBtn(g, "bd_plt_" + i, new UiRect(cRect.X + 10f, cRect.Bottom - 44f, cRect.W - 20f, 32f),
                    btns[i], 16f, MobileIcon.None, MobileIcon.ArrowRight, () =>
                    {
                        _buildTargetLabel = targetName;
                        _buildAnimTimer = 0f;
                        _buildModalOpen = true;
                    });
            }

            // Bottom 2-Column Section: "Projeto Atual" & "Build Rápida" (Image 4)
            float botY = rowY + cardH + 16f;
            float botH = Math.Max(128f, h - botY - 18f);
            float halfW = (contentW - 16f) * 0.5f;

            // Left Card: Projeto Atual
            UiRect pCard = new UiRect(sideW + padX, botY, halfW, botH);
            g.FillRect(pCard, new UiColor(13, 14, 19, 255), 16f);
            g.StrokeRect(pCard, new UiColor(38, 40, 50, 225), 1f, 16f);
            MobileVectorIcons.DrawIcon(g, pCard.X + 26f, pCard.Y + 24f, 15f, MobileIcon.Folder, new UiColor(228, 232, 242, 255));
            g.DrawText("Projeto Atual", pCard.X + 41f, pCard.Y + 16f, 14f, new UiColor(244, 245, 250, 255), true);

            UiRect pIn = new UiRect(pCard.X + 16f, pCard.Y + 44f, pCard.W - 32f, pCard.H - 58f);
            g.FillRect(pIn, new UiColor(18, 19, 25, 255), 12f);
            g.StrokeRect(pIn, new UiColor(42, 45, 56, 210), 1f, 12f);
            g.FillRect(new UiRect(pIn.X + 10f, pIn.CenterY - 20f, 40f, 40f), new UiColor(25, 27, 35, 255), 10f);
            MobileVectorIcons.DrawToolboxShape(g, pIn.X + 30f, pIn.CenterY, 28f, 0);

            g.DrawText(_store.Current.Name, pIn.X + 60f, pIn.CenterY - 16f, 13.5f, new UiColor(248, 249, 252, 255), true);
            g.DrawText("Última modificação: " + _store.Current.LastModified, pIn.X + 60f, pIn.CenterY + 4f, 11.5f, new UiColor(140, 144, 156, 255));
            DrawOutlineBtn(g, "bd_open_ed", new UiRect(pIn.Right - 134f, pIn.CenterY - 18f, 122f, 36f),
                "Abrir Projeto", 10f, MobileIcon.Folder, MobileIcon.None, () => Screen = MobileScreen.Editor);

            // Right Card: Build Rápida
            UiRect qCard = new UiRect(pCard.Right + 16f, botY, halfW, botH);
            g.FillRect(qCard, new UiColor(13, 14, 19, 255), 16f);
            g.StrokeRect(qCard, new UiColor(38, 40, 50, 225), 1f, 16f);
            MobileVectorIcons.DrawIcon(g, qCard.X + 26f, qCard.Y + 24f, 15f, MobileIcon.Bolt, new UiColor(238, 240, 248, 255));
            g.DrawText("Build Rápida", qCard.X + 41f, qCard.Y + 16f, 14f, new UiColor(244, 245, 250, 255), true);

            UiRect qIn = new UiRect(qCard.X + 16f, qCard.Y + 44f, qCard.W - 182f, qCard.H - 58f);
            g.FillRect(qIn, new UiColor(18, 19, 25, 255), 12f);
            g.StrokeRect(qIn, new UiColor(42, 45, 56, 210), 1f, 12f);
            MobileVectorIcons.DrawIcon(g, qIn.X + 19f, qIn.CenterY, 15f, MobileIcon.Bolt, new UiColor(226, 230, 240, 255));
            g.DrawText("Compilação Rápida", qIn.X + 34f, qIn.CenterY - 15f, 12.5f, new UiColor(244, 245, 250, 255), true);
            g.DrawText("Build direta com configurações padrão", qIn.X + 34f, qIn.CenterY + 4f, 10.5f, new UiColor(138, 142, 154, 255));

            DrawToggleSwitch(g, "bd_quick_sw", qIn.Right - 46f, qIn.CenterY - 9.5f, _quickBuild, false, () => _quickBuild = !_quickBuild);
            DrawWhitePillBtn(g, "bd_start_now", new UiRect(qCard.Right - 156f, qIn.CenterY - 20f, 140f, 40f),
                "Iniciar Build", 11f, MobileIcon.Play, () =>
                {
                    _buildAnimTimer = 0f;
                    _buildModalOpen = true;
                });

            DrawWindowControls(g, w);
        }

        // =========================================================================
        // TELA 5: EDITOR 3D COMPLETO + AGENTE IA (Identical to Image 1)
        // =========================================================================
        private void RenderEditor(IMobileCanvas g, float w, float h)
        {
            g.Clear(new UiColor(14, 15, 19, 255));

            // -------------------- 1. TOP MENU BAR (h = 28px) --------------------
            const float menuH = 28f;
            g.FillRect(new UiRect(0, 0, w, menuH), new UiColor(16, 17, 22, 255));
            g.DrawLine(0, menuH, w, menuH, new UiColor(36, 38, 46, 255), 1f);

            g.DrawImage("astron-icon.png", new UiRect(9f, 4f, 20f, 20f));
            g.DrawText("Astron Engine ▾", 34f, 7f, 12.5f, new UiColor(246, 248, 252, 255), true);
            _hits.Register("ed_brand_hub", new UiRect(4f, 0f, 142f, menuH), () => Screen = MobileScreen.Home);

            string[] menus =
            {
                "Arquivo", "Editar", "Visualizar", "Projeto", "Cena",
                "Jogo", "Script", "Biblioteca de Recursos", "Configurações", "Ajuda"
            };
            float mx = 152f;
            for (int i = 0; i < menus.Length; i++)
            {
                float mw = g.MeasureText(menus[i], 11.5f, false) + 14f;
                if (mx + mw > w - 115f) break;
                UiRect mr = new UiRect(mx, 2f, mw, menuH - 4f);
                g.DrawText(menus[i], mx + 7f, 7.5f, 11.5f, new UiColor(198, 202, 214, 255));
                int idx = i;
                _hits.Register("ed_mnu_" + idx, mr, () =>
                {
                    if (idx == 0) Screen = MobileScreen.Home;
                    else if (idx == 2) { _showExplorer = true; _showFiles = true; _showProperties = true; _showToolbox = true; _showBottom = true; }
                    else if (idx == 3 || idx == 5) Screen = MobileScreen.Build;
                    else if (idx == 4) _viewport.ResetCamera();
                    else if (idx == 6) OpenCodeFile("res://scripts/Player.cs");
                    else if (idx == 7) { _showBottom = true; _bottomTab = 6; }
                    else if (idx == 8) _settingsModalOpen = true;
                    else ShowToast("Astron Engine v1.0.0 — Toque e arraste no Viewport para orbitar.");
                });
                mx += mw + 1f;
            }
            DrawWindowControls(g, w);

            // -------------------- 2. MAIN RIBBON TOOLBAR (h = 48px) --------------------
            const float toolY = menuH;
            const float toolH = 48f;
            g.FillRect(new UiRect(0, toolY, w, toolH), new UiColor(21, 22, 28, 255));
            g.DrawLine(0, toolY + toolH, w, toolY + toolH, new UiColor(38, 40, 48, 255), 1f);

            float tx = 6f;
            DrawRibbonBtn(g, "rb_sel", ref tx, toolY, 64f, MobileIcon.CursorSelect,   "Selecionar",  _activeTool == 0, false, () => _activeTool = 0);
            DrawRibbonBtn(g, "rb_mov", ref tx, toolY, 50f, MobileIcon.Move4Way,       "Mover",       _activeTool == 1, false, () => _activeTool = 1);
            DrawRibbonBtn(g, "rb_rot", ref tx, toolY, 64f, MobileIcon.RotateCw,       "Rotacionar",  _activeTool == 2, false, () => _activeTool = 2);
            DrawRibbonBtn(g, "rb_scl", ref tx, toolY, 52f, MobileIcon.ScaleDiag,      "Escalar",     _activeTool == 3, false, () => _activeTool = 3);
            DrawRibbonBtn(g, "rb_trn", ref tx, toolY, 70f, MobileIcon.TransformGizmo, "Transformar", _activeTool == 4, false, () => _activeTool = 4);

            DrawRibbonSep(g, ref tx, toolY, toolH);

            DrawRibbonBtn(g, "rb_cri", ref tx, toolY, 52f, MobileIcon.Cube3D,    "Criar",       false, true, () => SpawnPrimitive("Part", AstronPrimitiveKind.Cube));
            DrawRibbonBtn(g, "rb_cam", ref tx, toolY, 54f, MobileIcon.Camera,    "Câmera",      false, false, () => _viewport.ResetCamera());
            DrawRibbonBtn(g, "rb_lum", ref tx, toolY, 64f, MobileIcon.Lightbulb, "Iluminação",  false, false, () => ShowToast("Iluminação direcional PBR ativa."));
            DrawRibbonBtn(g, "rb_ter", ref tx, toolY, 54f, MobileIcon.Mountain,  "Terreno",     false, false, () => SpawnPrimitive("Terreno", AstronPrimitiveKind.Plane));
            DrawRibbonBtn(g, "rb_cmp", ref tx, toolY, 70f, MobileIcon.Puzzle,    "Componente",  false, false, () => OpenCodeFile("res://scripts/Player.cs"));

            DrawRibbonSep(g, ref tx, toolY, toolH);

            // [ 2D | 3D ] segmented pill
            UiRect seg = new UiRect(tx, toolY + 11f, 68f, 26f);
            g.FillRect(seg, new UiColor(13, 14, 18, 255), 7f);
            g.StrokeRect(seg, new UiColor(42, 45, 56, 255), 1f, 7f);
            UiRect b2d = new UiRect(seg.X + 2f, seg.Y + 2f, 31f, 22f);
            UiRect b3d = new UiRect(seg.X + 35f, seg.Y + 2f, 31f, 22f);
            if (_viewport.Is2DMode) g.FillRect(b2d, new UiColor(37, 114, 246, 255), 5f);
            else g.FillRect(b3d, new UiColor(37, 114, 246, 255), 5f);
            g.DrawText("2D", b2d.CenterX, b2d.CenterY - 6f, 11f, _viewport.Is2DMode ? new UiColor(255, 255, 255, 255) : new UiColor(168, 172, 184, 255), true, true);
            g.DrawText("3D", b3d.CenterX, b3d.CenterY - 6f, 11f, !_viewport.Is2DMode ? new UiColor(255, 255, 255, 255) : new UiColor(168, 172, 184, 255), true, true);
            _hits.Register("rb_2d", b2d, () => _viewport.Is2DMode = true);
            _hits.Register("rb_3d", b3d, () => _viewport.Is2DMode = false);
            tx += 74f;

            DrawRibbonSep(g, ref tx, toolY, toolH);

            DrawRibbonBtn(g, "rb_ply", ref tx, toolY, 48f, MobileIcon.Play,  "Jogar",  _isPlaying && !_isPaused, false, () =>
            {
                _isPlaying = true;
                _isPaused = false;
                PushLog("INFO", "Simulação 3D iniciada.");
            });
            DrawRibbonBtn(g, "rb_pau", ref tx, toolY, 50f, MobileIcon.Pause, "Pausar", _isPlaying && _isPaused, false, () =>
            {
                if (_isPlaying) _isPaused = !_isPaused;
            });
            DrawRibbonBtn(g, "rb_stp", ref tx, toolY, 46f, MobileIcon.Stop,  "Parar",  false, false, () =>
            {
                _isPlaying = false;
                _isPaused = false;
                ResetDefaultScene();
                PushLog("INFO", "Simulação encerrada.");
            });

            if (w > tx + 168f)
            {
                float rx = w - 166f;
                DrawRibbonBtn(g, "rb_vis", ref rx, toolY, 78f, MobileIcon.Monitor, "Visualização", false, true, () => _viewport.Wireframe = !_viewport.Wireframe);
                DrawRibbonSep(g, ref rx, toolY, toolH);
                DrawRibbonBtn(g, "rb_cfg", ref rx, toolY, 76f, MobileIcon.Gear, "Configurações", false, false, () => _settingsModalOpen = true);
            }

            // -------------------- 3. DOCKED WORKSPACE --------------------
            const float statusH = 22f;
            float workY = menuH + toolH;
            float workH = h - workY - statusH;

            float leftW = (_showExplorer || _showFiles) ? 222f : 0f;
            float rightW = (_showProperties || _showToolbox) ? 278f : 0f;
            float centerW = Math.Max(220f, w - leftW - rightW);
            float bottomH = _showBottom ? 196f : 0f;
            float viewAreaH = Math.Max(140f, workH - bottomH);

            // LEFT COLUMN: Explorer + Arquivos
            if (leftW > 0f)
            {
                float expH = _showExplorer ? (_showFiles ? workH * 0.57f : workH) : 0f;
                float filH = _showFiles ? (workH - expH) : 0f;

                if (_showExplorer)
                    RenderExplorerPanel(g, new UiRect(0, workY, leftW, expH));
                if (_showFiles)
                    RenderFilesPanel(g, new UiRect(0, workY + expH, leftW, filH));
            }

            // CENTER AREA: Scene / Script Tab Bar + 3D Viewport
            RenderCenterViewport(g, new UiRect(leftW, workY, centerW, viewAreaH));

            // BOTTOM DOCK: Console / Saída / Depurador / Áudio / Animação / Editor de Shader / Agente IA
            if (_showBottom)
                RenderBottomDock(g, new UiRect(leftW, workY + viewAreaH, centerW, bottomH));

            // RIGHT COLUMN: Propriedades + Caixa de Ferramentas
            if (rightW > 0f)
            {
                float propH = _showProperties ? (_showToolbox ? workH * 0.71f : workH) : 0f;
                float tbxH = _showToolbox ? (workH - propH) : 0f;

                if (_showProperties)
                    RenderPropertiesPanel(g, new UiRect(w - rightW, workY, rightW, propH));
                if (_showToolbox)
                    RenderToolboxPanel(g, new UiRect(w - rightW, workY + propH, rightW, tbxH));
            }

            // -------------------- 4. BOTTOM STATUS BAR --------------------
            float sbY = h - statusH;
            g.FillRect(new UiRect(0, sbY, w, statusH), new UiColor(15, 16, 21, 255));
            g.DrawLine(0, sbY, w, sbY, new UiColor(36, 38, 48, 255), 1f);
            g.FillCircle(13f, sbY + 11f, 3.8f, _agent.IsBusy ? new UiColor(58, 138, 255, 255) : new UiColor(48, 214, 114, 255));
            g.DrawText(_agent.IsBusy ? "Agente IA gerando C#..." : (_isPlaying ? "Simulando" : "Pronto"),
                       23f, sbY + 4.5f, 11f, new UiColor(198, 202, 214, 255));

            const string rightStat = "FPS: 144   |   Memória: 842 MB   |   Versão: 0.1.0";
            float rsW = g.MeasureText(rightStat, 11f, false);
            g.DrawText(rightStat, w - rsW - 14f, sbY + 4.5f, 11f, new UiColor(168, 172, 184, 255));
        }

        private void DrawRibbonBtn(IMobileCanvas g, string id, ref float tx, float toolY, float bw,
                                   MobileIcon icon, string label, bool active, bool arrow, Action onTap)
        {
            UiRect r = new UiRect(tx, toolY + 4f, bw, 40f);
            if (active)
            {
                g.FillRect(r, new UiColor(28, 72, 148, 95), 6.5f);
                g.StrokeRect(r, new UiColor(58, 128, 252, 220), 1f, 6.5f);
            }
            UiColor icCol = active ? new UiColor(84, 160, 255, 255) : new UiColor(214, 218, 228, 255);
            UiColor lbCol = active ? new UiColor(118, 182, 255, 255) : new UiColor(198, 202, 212, 255);

            MobileVectorIcons.DrawIcon(g, r.CenterX, r.Y + 12.5f, 14.5f, icon, icCol);
            string txt = arrow ? (label + " ▾") : label;
            g.DrawText(txt, r.CenterX, r.Y + 23.5f, 10f, lbCol, false, true);
            _hits.Register(id, r, onTap);
            tx += bw + 2f;
        }

        private static void DrawRibbonSep(IMobileCanvas g, ref float tx, float toolY, float toolH)
        {
            g.DrawLine(tx + 3f, toolY + 9f, tx + 3f, toolY + toolH - 9f, new UiColor(44, 47, 58, 255), 1f);
            tx += 9f;
        }

        private void DrawDockHeader(IMobileCanvas g, UiRect panel, string title, MobileIcon icon, UiColor iconCol, string closeHitId, Action onClose)
        {
            UiRect hdr = new UiRect(panel.X, panel.Y, panel.W, 25f);
            g.FillRect(hdr, new UiColor(20, 22, 28, 255));
            g.DrawLine(hdr.X, hdr.Bottom, hdr.Right, hdr.Bottom, new UiColor(38, 41, 50, 255), 1f);
            MobileVectorIcons.DrawIcon(g, hdr.X + 13f, hdr.CenterY, 12.5f, icon, iconCol);
            g.DrawText(title, hdr.X + 25f, hdr.Y + 6f, 11.5f, new UiColor(234, 237, 245, 255), true);
            MobileVectorIcons.DrawIcon(g, hdr.Right - 28f, hdr.CenterY, 10.5f, MobileIcon.Pin, new UiColor(138, 142, 154, 220));
            MobileVectorIcons.DrawIcon(g, hdr.Right - 11f, hdr.CenterY, 9.5f, MobileIcon.Close, new UiColor(148, 152, 164, 230));
            if (onClose != null)
                _hits.Register(closeHitId, new UiRect(hdr.Right - 20f, hdr.Y, 20f, hdr.H), onClose);
        }

        private static void DrawSearchBox(IMobileCanvas g, UiRect box, string placeholder)
        {
            g.FillRect(box, new UiColor(13, 14, 18, 255), 5.5f);
            g.StrokeRect(box, new UiColor(38, 41, 50, 220), 1f, 5.5f);
            MobileVectorIcons.DrawIcon(g, box.X + 12f, box.CenterY, 11.5f, MobileIcon.Search, new UiColor(138, 142, 154, 230));
            g.DrawText(placeholder, box.X + 24f, box.CenterY - 5.5f, 10.5f, new UiColor(126, 130, 142, 255));
        }

        private void RenderExplorerPanel(IMobileCanvas g, UiRect r)
        {
            g.FillRect(r, new UiColor(17, 18, 23, 255));
            g.StrokeRect(r, new UiColor(36, 38, 48, 255), 1f);
            DrawDockHeader(g, r, "Explorer", MobileIcon.Cube3D, new UiColor(84, 162, 255, 255), "cls_exp", () => _showExplorer = false);
            DrawSearchBox(g, new UiRect(r.X + 6f, r.Y + 29f, r.W - 12f, 22f), "Pesquisar no Explorer...");

            g.PushClip(new UiRect(r.X + 1f, r.Y + 54f, r.W - 2f, r.H - 55f));
            float y = r.Y + 56f;
            UiColor cAmber = new UiColor(234, 170, 58, 255);
            UiColor cBlue  = new UiColor(74, 162, 255, 255);
            UiColor cCyan  = new UiColor(96, 186, 255, 255);

            DrawTreeItem(g, "tr_prj", r.X, ref y, r.W, 0, "▾", MobileIcon.Cube3D, cBlue, "Projeto", false, null);
            DrawTreeItem(g, "tr_cen", r.X, ref y, r.W, 1, "▾", MobileIcon.Folder, cAmber, "Cenas", false, null);
            DrawTreeItem(g, "tr_cm1", r.X, ref y, r.W, 2, "",  MobileIcon.Cube3D, cCyan, "Cena Principal", _activeCodePath == null && _world.SceneName == "Cena Principal", () => { _world.SceneName = "Cena Principal"; _activeCodePath = null; });
            DrawTreeItem(g, "tr_cm2", r.X, ref y, r.W, 2, "",  MobileIcon.Cube3D, cCyan, "Cena Secundária", _world.SceneName == "Cena Secundária", () => { _world.SceneName = "Cena Secundária"; _activeCodePath = null; });
            DrawTreeItem(g, "tr_m3d", r.X, ref y, r.W, 1, "▾", MobileIcon.Folder, cAmber, "Mundo 3D", false, null);
            DrawTreeItem(g, "tr_obj", r.X, ref y, r.W, 2, _expObjsOpen ? "▾" : "▸", MobileIcon.Folder, cBlue, "Objetos", false, () => _expObjsOpen = !_expObjsOpen);
            if (_expObjsOpen)
            {
                for (int i = 0; i < _world.Nodes.Count; i++)
                {
                    SceneNode3D nd = _world.Nodes[i];
                    int nid = nd.Id;
                    DrawTreeItem(g, "tr_nd_" + nid, r.X, ref y, r.W, 3, "", MobileIcon.Cube3D, cCyan, nd.Name, _selectedNodeId == nid, () => _selectedNodeId = nid);
                }
            }
            DrawTreeItem(g, "tr_ter", r.X, ref y, r.W, 2, "",  MobileIcon.Mountain,   new UiColor(92, 204, 128, 255), "Terreno", false, () => SpawnPrimitive("Terreno", AstronPrimitiveKind.Plane));
            DrawTreeItem(g, "tr_ilu", r.X, ref y, r.W, 2, "",  MobileIcon.Lightbulb,  new UiColor(246, 204, 72, 255), "Iluminação", false, null);
            DrawTreeItem(g, "tr_cam", r.X, ref y, r.W, 1, "▸", MobileIcon.Camera,     cBlue, "Câmeras", false, () => _viewport.ResetCamera());
            DrawTreeItem(g, "tr_per", r.X, ref y, r.W, 1, "▸", MobileIcon.User,       cBlue, "Personagens", false, null);
            DrawTreeItem(g, "tr_fis", r.X, ref y, r.W, 1, "▸", MobileIcon.Gear,       new UiColor(174, 182, 198, 255), "Física", false, null);
            DrawTreeItem(g, "tr_aud", r.X, ref y, r.W, 1, "▸", MobileIcon.MusicNote,  new UiColor(184, 118, 255, 255), "Áudio", false, () => { _showBottom = true; _bottomTab = 3; });
            DrawTreeItem(g, "tr_ui",  r.X, ref y, r.W, 1, "▸", MobileIcon.Monitor,    cCyan, "Interface", false, null);
            DrawTreeItem(g, "tr_scr", r.X, ref y, r.W, 1, "▸", MobileIcon.CodeTags,   new UiColor(238, 194, 76, 255), "Scripts", false, () => OpenCodeFile("res://scripts/Player.cs"));
            DrawTreeItem(g, "tr_rec", r.X, ref y, r.W, 1, "▸", MobileIcon.Folder,     cAmber, "Recursos", false, null);
            DrawTreeItem(g, "tr_mat", r.X, ref y, r.W, 1, "▸", MobileIcon.SphereBall, cBlue, "Materiais", false, () => _toolboxTab = 2);
            g.PopClip();
        }

        private void RenderFilesPanel(IMobileCanvas g, UiRect r)
        {
            g.FillRect(r, new UiColor(17, 18, 23, 255));
            g.StrokeRect(r, new UiColor(36, 38, 48, 255), 1f);
            DrawDockHeader(g, r, "Arquivos", MobileIcon.Folder, new UiColor(84, 162, 255, 255), "cls_fil", () => _showFiles = false);
            DrawSearchBox(g, new UiRect(r.X + 6f, r.Y + 29f, r.W - 12f, 22f), "Pesquisar arquivos...");

            g.PushClip(new UiRect(r.X + 1f, r.Y + 54f, r.W - 2f, r.H - 55f));
            float y = r.Y + 56f;
            UiColor cAmber = new UiColor(234, 170, 58, 255);
            UiColor cBlue  = new UiColor(78, 158, 255, 255);
            DrawTreeItem(g, "fl_res", r.X, ref y, r.W, 0, "▾", MobileIcon.Folder,   cBlue,  "res://", false, null);
            DrawTreeItem(g, "fl_mdl", r.X, ref y, r.W, 1, "▸", MobileIcon.Folder,   cAmber, "models", false, null);
            DrawTreeItem(g, "fl_tex", r.X, ref y, r.W, 1, "▸", MobileIcon.Folder,   cAmber, "textures", false, null);
            DrawTreeItem(g, "fl_scr", r.X, ref y, r.W, 1, "▸", MobileIcon.Folder,   cAmber, "scripts", false, () => OpenCodeFile("res://scripts/Player.cs"));
            DrawTreeItem(g, "fl_snd", r.X, ref y, r.W, 1, "▸", MobileIcon.Folder,   cAmber, "sounds", false, null);
            DrawTreeItem(g, "fl_ui",  r.X, ref y, r.W, 1, "▸", MobileIcon.Folder,   cAmber, "ui", false, null);
            DrawTreeItem(g, "fl_scn", r.X, ref y, r.W, 1, "",  MobileIcon.FileDoc,  new UiColor(190, 198, 214, 255), "default.scene", false, () => _activeCodePath = null);
            DrawTreeItem(g, "fl_lua", r.X, ref y, r.W, 1, "",  MobileIcon.FileCode, new UiColor(96, 172, 255, 255), "main.lua", _activeCodePath == "res://main.lua", () => OpenCodeFile("res://main.lua"));
            g.PopClip();
        }

        private void DrawTreeItem(IMobileCanvas g, string hitId, float panelX, ref float y, float panelW,
                                  int depth, string arrow, MobileIcon icon, UiColor iconCol, string label, bool selected, Action onTap)
        {
            const float rowH = 18.5f;
            UiRect row = new UiRect(panelX + 2f, y, panelW - 4f, rowH);
            if (selected)
                g.FillRect(row, new UiColor(34, 88, 184, 165), 4f);
            float x = panelX + 8f + depth * 12f;
            if (!string.IsNullOrEmpty(arrow))
                g.DrawText(arrow, x, y + 3f, 9.5f, new UiColor(154, 158, 170, 255));
            MobileVectorIcons.DrawIcon(g, x + 16f, y + rowH * 0.5f, 12f, icon, iconCol);
            g.DrawText(label, x + 26f, y + 3f, 10.5f, selected ? new UiColor(255, 255, 255, 255) : new UiColor(214, 218, 228, 255));
            if (onTap != null)
                _hits.Register(hitId, row, onTap);
            y += rowH;
        }

        private void RenderCenterViewport(IMobileCanvas g, UiRect host)
        {
            g.FillRect(host, new UiColor(14, 15, 19, 255));
            g.StrokeRect(host, new UiColor(36, 38, 48, 255), 1f);

            // Scene & Script Tab Bar: [🧊 Cena Principal ✕] [+]
            const float tabH = 27f;
            g.FillRect(new UiRect(host.X, host.Y, host.W, tabH), new UiColor(18, 19, 24, 255));
            g.DrawLine(host.X, host.Y + tabH, host.Right, host.Y + tabH, new UiColor(38, 40, 50, 255), 1f);

            UiRect sceneTab = new UiRect(host.X + 5f, host.Y + 3f, 134f, tabH - 3f);
            bool sceneActive = (_activeCodePath == null);
            g.FillRect(sceneTab, sceneActive ? new UiColor(28, 30, 38, 255) : new UiColor(20, 21, 27, 255), 5f);
            if (sceneActive)
                g.DrawLine(sceneTab.X + 2f, sceneTab.Y, sceneTab.Right - 2f, sceneTab.Y, new UiColor(54, 130, 252, 255), 2f);
            MobileVectorIcons.DrawIcon(g, sceneTab.X + 13f, sceneTab.CenterY, 12f, MobileIcon.Cube3D, new UiColor(84, 164, 255, 255));
            g.DrawText(_world.SceneName, sceneTab.X + 24f, sceneTab.Y + 6f, 10.5f, new UiColor(244, 246, 252, 255));
            MobileVectorIcons.DrawIcon(g, sceneTab.Right - 11f, sceneTab.CenterY, 8.5f, MobileIcon.Close, new UiColor(150, 154, 166, 220));
            _hits.Register("tab_scene", sceneTab, () => _activeCodePath = null);

            float plusX = sceneTab.Right + 6f;
            if (_activeCodePath != null)
            {
                UiRect codeTab = new UiRect(plusX, host.Y + 3f, 138f, tabH - 3f);
                g.FillRect(codeTab, new UiColor(28, 30, 38, 255), 5f);
                g.DrawLine(codeTab.X + 2f, codeTab.Y, codeTab.Right - 2f, codeTab.Y, new UiColor(54, 130, 252, 255), 2f);
                MobileVectorIcons.DrawIcon(g, codeTab.X + 13f, codeTab.CenterY, 12f, MobileIcon.FileCode, new UiColor(118, 214, 136, 255));
                string shortName = _activeCodePath.Substring(_activeCodePath.LastIndexOf('/') + 1);
                g.DrawText(shortName, codeTab.X + 24f, codeTab.Y + 6f, 10.5f, new UiColor(244, 246, 252, 255));
                MobileVectorIcons.DrawIcon(g, codeTab.Right - 11f, codeTab.CenterY, 8.5f, MobileIcon.Close, new UiColor(150, 154, 166, 220));
                _hits.Register("tab_code_cls", new UiRect(codeTab.Right - 22f, codeTab.Y, 22f, codeTab.H), () => _activeCodePath = null);
                plusX = codeTab.Right + 6f;
            }

            MobileVectorIcons.DrawIcon(g, plusX + 10f, host.Y + 14.5f, 11f, MobileIcon.Plus, new UiColor(188, 192, 204, 240));
            _hits.Register("tab_plus", new UiRect(plusX, host.Y + 3f, 22f, 22f), () => OpenCodeFile("res://scripts/Player.cs"));

            UiRect vpRect = new UiRect(host.X + 1f, host.Y + tabH + 1f, host.W - 2f, host.H - tabH - 2f);

            if (_activeCodePath == null)
            {
                _lastViewportRect = vpRect;
                _viewport.Render(g, vpRect, _world, _selectedNodeId);

                // Top-left floating translucent viewport pill bar (Image 1)
                UiRect ov = new UiRect(vpRect.X + 9f, vpRect.Y + 8f, 272f, 28f);
                g.FillRect(ov, new UiColor(18, 20, 26, 212), 7f);
                g.StrokeRect(ov, new UiColor(56, 60, 74, 175), 1f, 7f);

                MobileVectorIcons.DrawIcon(g, ov.X + 15f, ov.CenterY, 12.5f, MobileIcon.FocusCube, new UiColor(212, 216, 228, 240));
                _hits.Register("vp_foc", new UiRect(ov.X, ov.Y, 28f, ov.H), () => _viewport.ResetCamera());
                g.DrawLine(ov.X + 29f, ov.Y + 6f, ov.X + 29f, ov.Bottom - 6f, new UiColor(58, 62, 74, 180), 1f);

                string pMode = _viewport.Is2DMode ? "Ortográfica ▾" : "Perspectiva ▾";
                g.DrawText(pMode, ov.X + 36f, ov.Y + 7.5f, 10.5f, new UiColor(232, 235, 244, 255));
                _hits.Register("vp_persp", new UiRect(ov.X + 30f, ov.Y, 88f, ov.H), () => _viewport.Is2DMode = !_viewport.Is2DMode);
                g.DrawLine(ov.X + 119f, ov.Y + 6f, ov.X + 119f, ov.Bottom - 6f, new UiColor(58, 62, 74, 180), 1f);

                float vx = ov.X + 123f;
                DrawVpMiniIcon(g, "vp_cam", ref vx, ov.Y, MobileIcon.Camera,        true,               () => _viewport.ResetCamera());
                DrawVpMiniIcon(g, "vp_grd", ref vx, ov.Y, MobileIcon.GridIcon,      _viewport.ShowGrid, () => _viewport.ShowGrid = !_viewport.ShowGrid);
                DrawVpMiniIcon(g, "vp_exp", ref vx, ov.Y, MobileIcon.ExpandCorners, false,              () => _showBottom = !_showBottom);
                DrawVpMiniIcon(g, "vp_snp", ref vx, ov.Y, MobileIcon.SnapTarget,    _viewport.ShowGizmo,() => _viewport.ShowGizmo = !_viewport.ShowGizmo);
                DrawVpMiniIcon(g, "vp_shd", ref vx, ov.Y, MobileIcon.ShieldLight,   _viewport.Wireframe,() => _viewport.Wireframe = !_viewport.Wireframe);
                DrawVpMiniIcon(g, "vp_mor", ref vx, ov.Y, MobileIcon.DotsV,         false,              () => _viewport.ResetCamera());
            }
            else
            {
                _lastViewportRect = new UiRect(0, 0, 0, 0);
                g.FillRect(vpRect, new UiColor(13, 14, 19, 255));
                g.DrawText(_activeCodePath, vpRect.X + 12f, vpRect.Y + 10f, 11.5f, new UiColor(118, 178, 255, 255), true);
                DrawWhitePillBtn(g, "ed_edit_code", new UiRect(vpRect.Right - 144f, vpRect.Y + 6f, 132f, 24f),
                    "Editar Código", 6f, MobileIcon.None, () =>
                    {
                        string cur = _store.ReadFile(_activeCodePath);
                        if (_host != null)
                        {
                            _host.PromptText(_activeCodePath, "Edite o código nativo e confirme para salvar:", cur, false, updated =>
                            {
                                _store.WriteFile(_activeCodePath, updated);
                                ShowToast("Salvo: " + _activeCodePath);
                            });
                        }
                    });

                string src = _store.ReadFile(_activeCodePath);
                string[] lines = src.Split('\n');
                float ly = vpRect.Y + 38f;
                for (int i = 0; i < lines.Length && ly < vpRect.Bottom - 16f; i++)
                {
                    g.DrawText((i + 1).ToString(CultureInfo.InvariantCulture), vpRect.X + 10f, ly, 10.5f, new UiColor(96, 102, 118, 255));
                    g.DrawText(lines[i].TrimEnd('\r'), vpRect.X + 36f, ly, 10.5f, new UiColor(220, 226, 238, 255));
                    ly += 15f;
                }
            }
        }

        private void DrawVpMiniIcon(IMobileCanvas g, string id, ref float vx, float ovY, MobileIcon ic, bool active, Action onTap)
        {
            UiRect r = new UiRect(vx, ovY + 3f, 23f, 22f);
            if (active)
                g.FillRect(r, new UiColor(37, 114, 246, 210), 5f);
            MobileVectorIcons.DrawIcon(g, r.CenterX, r.CenterY, 12.5f, ic,
                active ? new UiColor(255, 255, 255, 255) : new UiColor(212, 216, 228, 240));
            _hits.Register(id, r, onTap);
            vx += 24.5f;
        }

        private void RenderBottomDock(IMobileCanvas g, UiRect r)
        {
            g.FillRect(r, new UiColor(16, 17, 22, 255));
            g.StrokeRect(r, new UiColor(36, 38, 48, 255), 1f);

            const float stripH = 27f;
            g.FillRect(new UiRect(r.X, r.Y, r.W, stripH), new UiColor(20, 21, 27, 255));
            g.DrawLine(r.X, r.Y + stripH, r.Right, r.Y + stripH, new UiColor(38, 40, 50, 255), 1f);

            string[] tNames = { "Console", "Saída", "Depurador", "Áudio", "Animação", "Editor de Shader", "Agente IA" };
            MobileIcon[] tIcons =
            {
                MobileIcon.Terminal, MobileIcon.FileDoc, MobileIcon.Gear,
                MobileIcon.Speaker, MobileIcon.Keyframe, MobileIcon.ShaderEdit, MobileIcon.SparklesAI
            };

            float bx = r.X + 6f;
            for (int i = 0; i < 7; i++)
            {
                float tw = g.MeasureText(tNames[i], 10.5f, false) + 28f;
                if (bx + tw > r.Right - 42f) break;
                UiRect tr = new UiRect(bx, r.Y + 2f, tw, stripH - 2f);
                bool act = (_bottomTab == i);
                if (act)
                {
                    g.FillRect(tr, new UiColor(27, 29, 37, 255), 5f);
                    g.DrawLine(tr.X + 2f, tr.Bottom, tr.Right - 2f, tr.Bottom, new UiColor(58, 134, 255, 255), 2f);
                }
                UiColor icCol = i == 6 ? new UiColor(98, 178, 255, 255) : (act ? new UiColor(236, 238, 246, 255) : new UiColor(148, 152, 164, 255));
                MobileVectorIcons.DrawIcon(g, tr.X + 11f, tr.CenterY, 11.5f, tIcons[i], icCol);
                g.DrawText(tNames[i], tr.X + 21f, tr.Y + 6f, 10.5f, act ? new UiColor(244, 246, 252, 255) : new UiColor(158, 162, 174, 255));
                int idx = i;
                _hits.Register("btab_" + idx, tr, () => _bottomTab = idx);
                bx += tw + 2f;
            }

            MobileVectorIcons.DrawIcon(g, r.Right - 28f, r.Y + 13.5f, 10.5f, MobileIcon.Pin, new UiColor(142, 146, 158, 220));
            MobileVectorIcons.DrawIcon(g, r.Right - 11f, r.Y + 13.5f, 9.5f, MobileIcon.Close, new UiColor(152, 156, 168, 230));
            _hits.Register("cls_bot", new UiRect(r.Right - 22f, r.Y, 22f, stripH), () => _showBottom = false);

            if (_bottomTab == 0)
            {
                // Filter badges on right: (✕ 1) (⚠ 1) (ℹ 1) [Funnel] [Trash]
                int nInfo = 0, nWarn = 0, nErr = 0;
                for (int i = 0; i < _consoleLogs.Count; i++)
                {
                    if (_consoleLogs[i].Level == "ERROR") nErr++;
                    else if (_consoleLogs[i].Level == "WARNING") nWarn++;
                    else nInfo++;
                }

                float py = r.Y + stripH + 5f;
                float px = r.Right - 166f;
                DrawCountBadge(g, ref px, py, MobileIcon.ErrorCircle,  new UiColor(236, 72, 76, 255), nErr);
                DrawCountBadge(g, ref px, py, MobileIcon.WarnTriangle, new UiColor(242, 184, 54, 255), nWarn);
                DrawCountBadge(g, ref px, py, MobileIcon.InfoCircle,   new UiColor(72, 162, 255, 255), nInfo);
                MobileVectorIcons.DrawIcon(g, px + 10f, py + 9f, 11.5f, MobileIcon.FilterFunnel, new UiColor(164, 168, 180, 240));
                MobileVectorIcons.DrawIcon(g, px + 31f, py + 9f, 12f, MobileIcon.Trash, new UiColor(164, 168, 180, 240));
                _hits.Register("con_clr", new UiRect(px + 20f, py - 2f, 22f, 22f), () => _consoleLogs.Clear());

                // Colored log rows matching Image 1
                float ly = r.Y + stripH + 24f;
                for (int i = 0; i < _consoleLogs.Count && ly < r.Bottom - 36f; i++)
                {
                    MobileConsoleEntry e = _consoleLogs[i];
                    MobileIcon ic = MobileIcon.InfoCircle;
                    UiColor col = new UiColor(78, 168, 255, 255);
                    string tag = "[INFO]";
                    if (e.Level == "WARNING") { ic = MobileIcon.WarnTriangle; col = new UiColor(242, 186, 56, 255); tag = "[WARNING]"; }
                    else if (e.Level == "ERROR") { ic = MobileIcon.ErrorCircle; col = new UiColor(240, 76, 80, 255); tag = "[ERROR]"; }

                    MobileVectorIcons.DrawIcon(g, r.X + 16f, ly + 6.5f, 12f, ic, col);
                    g.DrawText(tag, r.X + 27f, ly, 11f, col, true);
                    float tagW = g.MeasureText(tag, 11f, true);
                    g.DrawText(e.Message, r.X + 34f + tagW, ly, 11f, new UiColor(216, 220, 230, 255));
                    ly += 19f;
                }

                // Bottom input bar: "Digite uma mensagem..." + paper-plane send icon
                UiRect inp = new UiRect(r.X + 8f, r.Bottom - 30f, r.W - 44f, 24f);
                g.FillRect(inp, new UiColor(12, 13, 17, 255), 6f);
                g.StrokeRect(inp, new UiColor(38, 41, 52, 230), 1f, 6f);
                g.DrawText("Digite uma mensagem...", inp.X + 10f, inp.Y + 6f, 10.5f, new UiColor(128, 132, 144, 255));
                _hits.Register("con_input", inp, () => PromptAgentOrCommand());

                UiRect sendR = new UiRect(inp.Right + 4f, inp.Y, 28f, 24f);
                MobileVectorIcons.DrawIcon(g, sendR.CenterX, sendR.CenterY, 14f, MobileIcon.SendPlane, new UiColor(196, 202, 216, 255));
                _hits.Register("con_send", sendR, () => PromptAgentOrCommand());
            }
            else if (_bottomTab == 6)
            {
                // Agente IA OpenRouter
                bool hasKey = !string.IsNullOrEmpty(_agent.ApiKey);
                g.DrawText(hasKey ? "OpenRouter Conectado (" + _agent.Model + ")" : "Configure sua OpenRouter API Key para geração autônoma C#",
                           r.X + 12f, r.Y + stripH + 8f, 11f,
                           hasKey ? new UiColor(72, 220, 128, 255) : new UiColor(242, 186, 56, 255), true);
                DrawOutlineBtn(g, "ai_cfg_key", new UiRect(r.Right - 144f, r.Y + stripH + 4f, 134f, 22f),
                    "API Key", 6f, MobileIcon.Gear, MobileIcon.None, () => _settingsModalOpen = true);

                float my = r.Y + stripH + 32f;
                if (_agent.Messages.Count == 0)
                {
                    g.DrawText("Peça qualquer jogo ou sistema em C# / 3D — o Agente IA cria objetos 3D e escreve scripts C# reais.",
                               r.X + 12f, my, 10.5f, new UiColor(156, 160, 174, 255));
                }
                else
                {
                    int start = Math.Max(0, _agent.Messages.Count - 4);
                    for (int i = start; i < _agent.Messages.Count && my < r.Bottom - 34f; i++)
                    {
                        var m = _agent.Messages[i];
                        string prefix = m.IsUser ? "[Você]: " : "[Astron IA]: ";
                        UiColor pc = m.IsUser ? new UiColor(118, 184, 255, 255) : new UiColor(92, 228, 158, 255);
                        string line = prefix + (m.Text ?? _agent.StatusLine);
                        if (line.Length > 95) line = line.Substring(0, 95) + "...";
                        g.DrawText(line, r.X + 12f, my, 10.5f, pc);
                        my += 17f;
                    }
                }

                UiRect aiInp = new UiRect(r.X + 8f, r.Bottom - 30f, r.W - 96f, 24f);
                g.FillRect(aiInp, new UiColor(12, 13, 17, 255), 6f);
                g.StrokeRect(aiInp, new UiColor(38, 41, 52, 230), 1f, 6f);
                g.DrawText("Descreva o jogo, mecânica C# ou cena 3D para o Agente IA...", aiInp.X + 10f, aiInp.Y + 6f, 10.5f, new UiColor(128, 132, 144, 255));
                _hits.Register("ai_prompt_box", aiInp, () => PromptAgentOrCommand());
                DrawWhitePillBtn(g, "ai_send_btn", new UiRect(aiInp.Right + 6f, aiInp.Y, 74f, 24f),
                    _agent.IsBusy ? "..." : "Enviar", 6f, MobileIcon.None, () => PromptAgentOrCommand());
            }
            else
            {
                g.DrawText("Astron Native Runtime — C++20 / C11 / C# / Rust / GLSL / HLSL / Lua ativos.",
                           r.X + 12f, r.Y + stripH + 14f, 11.5f, new UiColor(118, 214, 148, 255));
                g.DrawText("Projeto: " + _store.Current.Name + "  |  Entidades 3D: " + _world.Nodes.Count,
                           r.X + 12f, r.Y + stripH + 36f, 11f, new UiColor(168, 172, 184, 255));
            }
        }

        private static void DrawCountBadge(IMobileCanvas g, ref float px, float py, MobileIcon ic, UiColor icCol, int count)
        {
            string s = count.ToString(CultureInfo.InvariantCulture);
            float tw = g.MeasureText(s, 10.5f, false);
            UiRect b = new UiRect(px, py, 24f + tw, 18f);
            g.FillRect(b, new UiColor(24, 26, 33, 255), 4.5f);
            g.StrokeRect(b, new UiColor(42, 45, 56, 220), 1f, 4.5f);
            MobileVectorIcons.DrawIcon(g, b.X + 9f, b.CenterY, 10.5f, ic, icCol);
            g.DrawText(s, b.X + 17f, b.Y + 3f, 10.5f, new UiColor(216, 220, 230, 255));
            px += b.W + 4f;
        }

        private void RenderPropertiesPanel(IMobileCanvas g, UiRect r)
        {
            g.FillRect(r, new UiColor(17, 18, 23, 255));
            g.StrokeRect(r, new UiColor(36, 38, 48, 255), 1f);
            DrawDockHeader(g, r, "Propriedades", MobileIcon.Gear, new UiColor(194, 200, 214, 255), "cls_prp", () => _showProperties = false);
            DrawSearchBox(g, new UiRect(r.X + 6f, r.Y + 29f, r.W - 12f, 22f), "Pesquisar propriedades...");

            SceneNode3D sel = GetSelectedNode();
            if (sel == null) return;

            g.PushClip(new UiRect(r.X + 1f, r.Y + 54f, r.W - 2f, r.H - 55f));
            float y = r.Y + 56f;
            float lx = r.X + 8f;
            float vx = r.X + 92f;
            float vw = r.Right - 8f - vx;

            // "Parte: Model                               ✕"
            UiRect partHdr = new UiRect(lx, y, r.W - 16f, 21f);
            g.FillRect(partHdr, new UiColor(22, 24, 31, 255), 4.5f);
            g.DrawText("Parte: " + sel.Name, partHdr.X + 7f, partHdr.Y + 4.5f, 10.5f, new UiColor(236, 239, 248, 255), true);
            MobileVectorIcons.DrawIcon(g, partHdr.Right - 10f, partHdr.CenterY, 8.5f, MobileIcon.Close, new UiColor(144, 148, 160, 220));
            y += 25f;

            // ▾ Transformação
            DrawSectionRow(g, lx, ref y, r.W - 16f, "▾  Transformação");
            DrawVec3PropRow(g, "prp_pos", lx, vx, vw, ref y, "Posição", sel.Position, v => sel.Position = v);
            DrawVec3PropRow(g, "prp_rot", lx, vx, vw, ref y, "Rotação", sel.RotationEulerDeg, v => sel.RotationEulerDeg = v);
            DrawVec3PropRow(g, "prp_scl", lx, vx, vw, ref y, "Escala",  sel.Scale, v => sel.Scale = v);

            // ▾ Aparência
            DrawSectionRow(g, lx, ref y, r.W - 16f, "▾  Aparência");

            // Cor [#FFFFFF]
            g.DrawText("Cor", lx + 4f, y + 4f, 10.5f, new UiColor(188, 192, 204, 255));
            UiRect colBox = new UiRect(vx, y, vw, 20f);
            g.FillRect(colBox, new UiColor(14, 15, 20, 255), 4.5f);
            g.StrokeRect(colBox, new UiColor(42, 45, 56, 220), 1f, 4.5f);
            UiColor swatch = UiColor.FromHex(sel.ColorRgba);
            g.FillRect(new UiRect(colBox.X + 4f, colBox.Y + 3f, 14f, 14f), swatch, 3f);
            g.StrokeRect(new UiRect(colBox.X + 4f, colBox.Y + 3f, 14f, 14f), new UiColor(80, 84, 96, 200), 1f, 3f);
            string hexTxt = string.Format(CultureInfo.InvariantCulture, "#{0:X2}{1:X2}{2:X2}", swatch.R, swatch.G, swatch.B);
            g.DrawText(hexTxt, colBox.X + 24f, colBox.Y + 4f, 10.5f, new UiColor(228, 232, 242, 255));
            MobileVectorIcons.DrawIcon(g, colBox.Right - 11f, colBox.CenterY, 11f, MobileIcon.Eyedropper, new UiColor(160, 164, 176, 240));
            _hits.Register("prp_col", colBox, () =>
            {
                uint[] cycle = { 0xFFFFFFFFu, 0x3B82F6FFu, 0x22C55EFFu, 0xEF4444FFu, 0xF59E0BFFu, 0xA855F7FFu };
                int idx = Array.IndexOf(cycle, sel.ColorRgba);
                sel.ColorRgba = cycle[(idx + 1) % cycle.Length];
            });
            y += 23f;

            // Material [Plastic ▾]
            g.DrawText("Material", lx + 4f, y + 4f, 10.5f, new UiColor(188, 192, 204, 255));
            UiRect matBox = new UiRect(vx, y, vw, 20f);
            g.FillRect(matBox, new UiColor(14, 15, 20, 255), 4.5f);
            g.StrokeRect(matBox, new UiColor(42, 45, 56, 220), 1f, 4.5f);
            g.DrawText(sel.Material.ToString(), matBox.X + 7f, matBox.Y + 4f, 10.5f, new UiColor(228, 232, 242, 255));
            g.DrawText("▾", matBox.Right - 13f, matBox.Y + 4f, 10f, new UiColor(160, 164, 176, 255));
            _hits.Register("prp_mat", matBox, () =>
            {
                int nxt = ((int)sel.Material + 1) % 7;
                sel.Material = (AstronMaterialPreset)nxt;
            });
            y += 23f;

            // Transparência [0.000 ──●──]
            g.DrawText("Transparência", lx + 4f, y + 4f, 10.5f, new UiColor(188, 192, 204, 255));
            UiRect trNum = new UiRect(vx, y, 44f, 20f);
            g.FillRect(trNum, new UiColor(14, 15, 20, 255), 4.5f);
            g.StrokeRect(trNum, new UiColor(42, 45, 56, 220), 1f, 4.5f);
            g.DrawText(sel.Transparency.ToString("F3", CultureInfo.InvariantCulture), trNum.X + 5f, trNum.Y + 4f, 10f, new UiColor(224, 228, 238, 255));
            float slX = vx + 50f;
            float slW = vw - 54f;
            float slY = y + 10f;
            g.DrawLine(slX, slY, slX + slW, slY, new UiColor(44, 48, 60, 255), 3f);
            float knobX = slX + sel.Transparency * slW;
            g.DrawLine(slX, slY, knobX, slY, new UiColor(48, 130, 252, 255), 3f);
            g.FillCircle(knobX, slY, 5.5f, new UiColor(244, 246, 252, 255));
            _hits.Register("prp_tr", new UiRect(vx, y, vw, 20f), () =>
            {
                sel.Transparency = sel.Transparency >= 0.75f ? 0f : (sel.Transparency + 0.25f);
            });
            y += 23f;

            // Visibilidade [switch]
            g.DrawText("Visibilidade", lx + 4f, y + 3f, 10.5f, new UiColor(188, 192, 204, 255));
            DrawToggleSwitch(g, "prp_vis", r.Right - 46f, y, sel.Visible, true, () => sel.Visible = !sel.Visible);
            y += 23f;

            // ▾ Física
            DrawSectionRow(g, lx, ref y, r.W - 16f, "▾  Física");

            g.DrawText("Massa", lx + 4f, y + 4f, 10.5f, new UiColor(188, 192, 204, 255));
            UiRect massBox = new UiRect(vx, y, vw, 20f);
            g.FillRect(massBox, new UiColor(14, 15, 20, 255), 4.5f);
            g.StrokeRect(massBox, new UiColor(42, 45, 56, 220), 1f, 4.5f);
            g.DrawText(sel.Mass.ToString("F3", CultureInfo.InvariantCulture), massBox.X + 7f, massBox.Y + 4f, 10.5f, new UiColor(228, 232, 242, 255));
            y += 23f;

            g.DrawText("Gravidade", lx + 4f, y + 3f, 10.5f, new UiColor(188, 192, 204, 255));
            DrawToggleSwitch(g, "prp_grv", r.Right - 46f, y, sel.UseGravity, true, () =>
            {
                sel.UseGravity = !sel.UseGravity;
                sel.Anchored = !sel.UseGravity;
            });
            y += 23f;

            g.DrawText("Colisão", lx + 4f, y + 4f, 10.5f, new UiColor(188, 192, 204, 255));
            UiRect colCmb = new UiRect(vx, y, vw, 20f);
            g.FillRect(colCmb, new UiColor(14, 15, 20, 255), 4.5f);
            g.StrokeRect(colCmb, new UiColor(42, 45, 56, 220), 1f, 4.5f);
            g.DrawText(sel.CanCollide ? "Ativado" : "Desativado", colCmb.X + 7f, colCmb.Y + 4f, 10.5f, new UiColor(228, 232, 242, 255));
            g.DrawText("▾", colCmb.Right - 13f, colCmb.Y + 4f, 10f, new UiColor(160, 164, 176, 255));
            _hits.Register("prp_cld", colCmb, () => sel.CanCollide = !sel.CanCollide);
            y += 24f;

            DrawSectionRow(g, lx, ref y, r.W - 16f, "▸  Renderização");
            DrawSectionRow(g, lx, ref y, r.W - 16f, "▸  Outras Configurações");

            g.PopClip();
        }

        private static void DrawSectionRow(IMobileCanvas g, float x, ref float y, float w, string title)
        {
            g.FillRect(new UiRect(x, y, w, 19f), new UiColor(23, 25, 32, 255), 4f);
            g.DrawText(title, x + 6f, y + 3.5f, 10.5f, new UiColor(226, 230, 240, 255), true);
            y += 22f;
        }

        private void DrawVec3PropRow(IMobileCanvas g, string idPrefix, float lx, float vx, float vw, ref float y,
                                     string label, Vec3 val, Action<Vec3> onEdit)
        {
            g.DrawText(label, lx + 4f, y + 4f, 10.5f, new UiColor(188, 192, 204, 255));
            float bw = (vw - 6f) / 3f;
            string[] ax = { "X", "Y", "Z" };
            float[] comps = { val.X, val.Y, val.Z };
            for (int i = 0; i < 3; i++)
            {
                UiRect b = new UiRect(vx + i * (bw + 3f), y, bw, 19f);
                g.FillRect(b, new UiColor(14, 15, 20, 255), 4f);
                g.StrokeRect(b, new UiColor(42, 45, 56, 220), 1f, 4f);
                g.DrawText(ax[i], b.X + 4f, b.Y + 3.5f, 9.5f, new UiColor(138, 142, 154, 240));
                g.DrawText(comps[i].ToString("F3", CultureInfo.InvariantCulture), b.X + 14f, b.Y + 3.5f, 9.5f, new UiColor(228, 232, 242, 255));
            }
            _hits.Register(idPrefix, new UiRect(vx, y, vw, 19f), () =>
            {
                if (_host == null) return;
                string cur = string.Format(CultureInfo.InvariantCulture, "{0:F3},{1:F3},{2:F3}", val.X, val.Y, val.Z);
                _host.PromptText(label + " (X,Y,Z)", "Digite X,Y,Z separados por vírgula:", cur, false, s =>
                {
                    string[] parts = (s ?? "").Split(',');
                    if (parts.Length == 3 &&
                        float.TryParse(parts[0].Trim(), NumberStyles.Float, CultureInfo.InvariantCulture, out float nx) &&
                        float.TryParse(parts[1].Trim(), NumberStyles.Float, CultureInfo.InvariantCulture, out float ny) &&
                        float.TryParse(parts[2].Trim(), NumberStyles.Float, CultureInfo.InvariantCulture, out float nz))
                    {
                        onEdit(new Vec3(nx, ny, nz));
                    }
                });
            });
            y += 21.5f;
        }

        private void RenderToolboxPanel(IMobileCanvas g, UiRect r)
        {
            g.FillRect(r, new UiColor(17, 18, 23, 255));
            g.StrokeRect(r, new UiColor(36, 38, 48, 255), 1f);
            DrawDockHeader(g, r, "Caixa de Ferramentas", MobileIcon.Cube3D, new UiColor(180, 186, 200, 255), "cls_tbx", () => _showToolbox = false);

            string[] tabs = { "Modelos", "Recursos", "Materiais" };
            float tw = (r.W - 18f) / 3f;
            for (int i = 0; i < 3; i++)
            {
                UiRect tr = new UiRect(r.X + 6f + i * (tw + 3f), r.Y + 29f, tw, 22f);
                bool act = (_toolboxTab == i);
                g.FillRect(tr, act ? new UiColor(37, 114, 246, 255) : new UiColor(25, 27, 34, 255), 5f);
                g.DrawText(tabs[i], tr.CenterX, tr.Y + 5f, 10.5f, act ? new UiColor(255, 255, 255, 255) : new UiColor(168, 172, 184, 255), act, true);
                int idx = i;
                _hits.Register("tbx_t_" + idx, tr, () => _toolboxTab = idx);
            }

            if (_toolboxTab == 0)
            {
                string[] names = { "Part", "Sphere", "Cylinder", "Block" };
                AstronPrimitiveKind[] kinds =
                {
                    AstronPrimitiveKind.Cube, AstronPrimitiveKind.Sphere,
                    AstronPrimitiveKind.Cylinder, AstronPrimitiveKind.Wedge
                };

                float gap = 6f;
                float cw = (r.W - 16f - gap * 3f) / 4f;
                float ch = Math.Max(66f, Math.Min(88f, r.H - 62f));
                float cy = r.Y + 56f;

                for (int i = 0; i < 4; i++)
                {
                    UiRect card = new UiRect(r.X + 8f + i * (cw + gap), cy, cw, ch);
                    g.FillRect(card, new UiColor(22, 24, 31, 255), 8f);
                    g.StrokeRect(card, new UiColor(40, 43, 54, 230), 1f, 8f);
                    MobileVectorIcons.DrawToolboxShape(g, card.CenterX, card.Y + card.H * 0.42f, 36f, i);
                    g.DrawText(names[i], card.CenterX, card.Bottom - 17f, 10f, new UiColor(214, 218, 228, 255), false, true);

                    string nm = names[i];
                    AstronPrimitiveKind pk = kinds[i];
                    _hits.Register("tbx_sp_" + i, card, () => SpawnPrimitive(nm, pk));
                }
            }
            else if (_toolboxTab == 1)
            {
                DrawOutlineBtn(g, "tbx_pl", new UiRect(r.X + 10f, r.Y + 58f, r.W - 20f, 26f),
                    "res://scripts/Player.cs", 6f, MobileIcon.FileCode, MobileIcon.None, () => OpenCodeFile("res://scripts/Player.cs"));
                DrawOutlineBtn(g, "tbx_lu", new UiRect(r.X + 10f, r.Y + 90f, r.W - 20f, 26f),
                    "res://main.lua", 6f, MobileIcon.FileCode, MobileIcon.None, () => OpenCodeFile("res://main.lua"));
            }
            else
            {
                string[] mats = { "Plastic", "Metal", "Neon", "Glass" };
                AstronMaterialPreset[] mp = { AstronMaterialPreset.Plastic, AstronMaterialPreset.Metal, AstronMaterialPreset.Neon, AstronMaterialPreset.Glass };
                float mw = (r.W - 24f) * 0.5f;
                for (int i = 0; i < 4; i++)
                {
                    UiRect mb = new UiRect(r.X + 8f + (i % 2) * (mw + 8f), r.Y + 58f + (i / 2) * 30f, mw, 24f);
                    AstronMaterialPreset chosen = mp[i];
                    DrawOutlineBtn(g, "tbx_m_" + i, mb, mats[i], 6f, MobileIcon.SphereBall, MobileIcon.None, () =>
                    {
                        var s = GetSelectedNode();
                        if (s != null) s.Material = chosen;
                    });
                }
            }
        }

        private void SpawnPrimitive(string name, AstronPrimitiveKind kind)
        {
            float offset = (_world.Nodes.Count % 5) * 1.1f;
            Vec3 scale = kind == AstronPrimitiveKind.Plane ? new Vec3(12f, 0.2f, 12f) : Vec3.One;
            Vec3 pos = kind == AstronPrimitiveKind.Plane ? new Vec3(0f, -0.6f, 0f) : new Vec3(offset, 0.5f, 0f);
            var n = _world.SpawnPart(name, kind, pos, scale, AstronMaterialPreset.Plastic, true);
            n.ColorRgba = 0xFFFFFFFFu;
            _selectedNodeId = n.Id;
            PushLog("INFO", "Objeto 3D adicionado: " + name);
        }

        private void OpenCodeFile(string resPath)
        {
            _activeCodePath = resPath;
            ShowToast("Arquivo aberto: " + resPath);
        }

        // =========================================================================
        // MODALS & TOASTS
        // =========================================================================
        private void RenderSettingsModal(IMobileCanvas g, float w, float h)
        {
            g.FillRect(new UiRect(0, 0, w, h), new UiColor(0, 0, 0, 175));
            float mw = Math.Min(500f, w - 40f);
            float mh = Math.Min(360f, h - 40f);
            UiRect m = new UiRect((w - mw) * 0.5f, (h - mh) * 0.5f, mw, mh);
            g.FillRect(m, new UiColor(14, 15, 20, 252), 18f);
            g.StrokeRect(m, new UiColor(52, 56, 68, 220), 1.2f, 18f);

            g.DrawText("Configurações — Astron Engine & OpenRouter IA", m.X + 24f, m.Y + 20f, 16f, new UiColor(248, 249, 252, 255), true);
            g.DrawText("Provedor: " + MobileAIAgent.Presets[_agent.ProviderIndex].Name, m.X + 24f, m.Y + 54f, 12.5f, new UiColor(186, 190, 204, 255));
            DrawOutlineBtn(g, "md_prv", new UiRect(m.Right - 164f, m.Y + 48f, 140f, 28f), "Trocar Provedor", 8f, MobileIcon.None, MobileIcon.None, () =>
            {
                _agent.SelectProvider((_agent.ProviderIndex + 1) % MobileAIAgent.Presets.Length);
            });

            UiRect keyBox = new UiRect(m.X + 24f, m.Y + 96f, m.W - 48f, 40f);
            g.FillRect(keyBox, new UiColor(20, 22, 28, 255), 10f);
            g.StrokeRect(keyBox, new UiColor(46, 50, 62, 220), 1f, 10f);
            string kDisp = string.IsNullOrEmpty(_agent.ApiKey) ? "Toque para inserir API Key (sk-or-v1-...)" : "API Key configurada (••••••••)";
            g.DrawText(kDisp, keyBox.X + 14f, keyBox.CenterY - 6.5f, 12f, new UiColor(214, 218, 230, 255));
            _hits.Register("md_key", keyBox, () =>
            {
                if (_host != null)
                    _host.PromptText("Chave da API (OpenRouter / OpenAI)", "Cole sua API Key:", _agent.ApiKey, true, v => _agent.ApiKey = v ?? "");
            });

            UiRect modBox = new UiRect(m.X + 24f, m.Y + 150f, m.W - 48f, 40f);
            g.FillRect(modBox, new UiColor(20, 22, 28, 255), 10f);
            g.StrokeRect(modBox, new UiColor(46, 50, 62, 220), 1f, 10f);
            g.DrawText("Modelo: " + _agent.Model, modBox.X + 14f, modBox.CenterY - 6.5f, 12f, new UiColor(214, 218, 230, 255));
            _hits.Register("md_mod", modBox, () =>
            {
                if (_host != null)
                    _host.PromptText("ID do Modelo", "Ex.: openai/gpt-4o-mini, anthropic/claude-3.5-sonnet:", _agent.Model, false, v => { if (!string.IsNullOrWhiteSpace(v)) _agent.Model = v.Trim(); });
            });

            DrawOutlineBtn(g, "md_keys_url", new UiRect(m.X + 24f, m.Bottom - 62f, 190f, 40f),
                "Abrir OpenRouter Keys", 10f, MobileIcon.None, MobileIcon.ArrowRight,
                () => { if (_host != null) _host.OpenUrl("https://openrouter.ai/api/keys"); });

            DrawWhitePillBtn(g, "md_save", new UiRect(m.Right - 164f, m.Bottom - 62f, 140f, 40f),
                "Concluir", 10f, MobileIcon.None, () => _settingsModalOpen = false);
        }

        private void RenderBuildPipelineModal(IMobileCanvas g, float w, float h)
        {
            g.FillRect(new UiRect(0, 0, w, h), new UiColor(0, 0, 0, 180));
            float mw = Math.Min(540f, w - 40f);
            float mh = Math.Min(330f, h - 40f);
            UiRect m = new UiRect((w - mw) * 0.5f, (h - mh) * 0.5f, mw, mh);
            g.FillRect(m, new UiColor(13, 14, 19, 252), 18f);
            g.StrokeRect(m, new UiColor(52, 56, 68, 220), 1.2f, 18f);

            int step = Math.Min(5, (int)(_buildAnimTimer / 0.55f));
            g.DrawText(step >= 5 ? ("Build " + _buildTargetLabel + " Pronta!") : ("Compilando " + _buildTargetLabel + "..."),
                       m.X + 24f, m.Y + 20f, 17f, new UiColor(248, 249, 252, 255), true);

            string[] steps =
            {
                "Sincronizar scripts C# e cena 3D (" + _store.Current.Name + ")",
                "Validar shaders GLSL 3.30 / HLSL SM 5.0 e pacote .pak",
                "Executar workflow GitHub Actions (.NET 10 / CMake / Xcode / JDK 17)",
                "Assinar e empacotar binário nativo (" + _buildTargetLabel + ")",
                "Publicar artefato pronto para download em GitHub Actions"
            };

            float sy = m.Y + 62f;
            for (int i = 0; i < steps.Length; i++)
            {
                string prefix = i < step ? "[OK]  " : (i == step ? "[..]  " : "[  ]  ");
                UiColor sc = i < step ? new UiColor(74, 222, 128, 255) : (i == step ? new UiColor(96, 168, 255, 255) : new UiColor(124, 128, 140, 255));
                g.DrawText(prefix + steps[i], m.X + 24f, sy, 12f, sc);
                sy += 26f;
            }

            DrawWhitePillBtn(g, "bd_gh_act", new UiRect(m.X + 24f, m.Bottom - 58f, 206f, 38f),
                "Abrir GitHub Actions", 10f, MobileIcon.None,
                () => { if (_host != null) _host.OpenUrl("https://github.com/Veexdevs/Astron-Engine/actions"); });

            DrawOutlineBtn(g, "bd_cls_mod", new UiRect(m.Right - 134f, m.Bottom - 58f, 110f, 38f),
                "Fechar", 10f, MobileIcon.None, MobileIcon.None, () => _buildModalOpen = false);
        }

        private void RenderToast(IMobileCanvas g, float w, float h)
        {
            float tw = g.MeasureText(_toastText, 12f, false) + 32f;
            UiRect tr = new UiRect(w - tw - 20f, h - 56f, tw, 32f);
            g.FillRect(tr, new UiColor(20, 22, 28, 244), 10f);
            g.StrokeRect(tr, new UiColor(58, 122, 235, 220), 1.1f, 10f);
            g.DrawText(_toastText, tr.X + 16f, tr.CenterY - 6.5f, 12f, new UiColor(246, 248, 252, 255));
        }
    }
}
