using System;
using System.Collections.Generic;
using Astron.Game;
using OpenTK.Graphics.OpenGL4;
using OpenTK.Mathematics;
using OpenTK.Windowing.Common;
using OpenTK.Windowing.Desktop;
using OpenTK.Windowing.GraphicsLibraryFramework;

namespace Astron.Desktop
{
    /// <summary>Renderer 2D OpenGL 3.3 Core (compatível com Windows, Linux e macOS).</summary>
    internal sealed class DesktopRenderer : IRenderer, IDisposable
    {
        const string VertexShaderSrc = @"#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aCol;
out vec4 vCol;
uniform vec2 uScreen;
void main() {
    vec2 ndc = vec2((aPos.x / uScreen.x) * 2.0 - 1.0, 1.0 - (aPos.y / uScreen.y) * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
    vCol = aCol;
}";

        const string FragmentShaderSrc = @"#version 330 core
in vec4 vCol;
out vec4 FragColor;
void main() {
    FragColor = vCol;
}";

        readonly List<float> _verts = new List<float>(4096);
        int _program;
        int _vao;
        int _vbo;
        int _uScreenLoc;

        public void Init()
        {
            int vs = CompileShader(ShaderType.VertexShader, VertexShaderSrc);
            int fs = CompileShader(ShaderType.FragmentShader, FragmentShaderSrc);

            _program = GL.CreateProgram();
            GL.AttachShader(_program, vs);
            GL.AttachShader(_program, fs);
            GL.LinkProgram(_program);
            GL.DeleteShader(vs);
            GL.DeleteShader(fs);

            _uScreenLoc = GL.GetUniformLocation(_program, "uScreen");

            _vao = GL.GenVertexArray();
            _vbo = GL.GenBuffer();

            GL.BindVertexArray(_vao);
            GL.BindBuffer(BufferTarget.ArrayBuffer, _vbo);

            int stride = 6 * sizeof(float);
            GL.EnableVertexAttribArray(0);
            GL.VertexAttribPointer(0, 2, VertexAttribPointerType.Float, false, stride, 0);
            GL.EnableVertexAttribArray(1);
            GL.VertexAttribPointer(1, 4, VertexAttribPointerType.Float, false, stride, 2 * sizeof(float));

            GL.BindVertexArray(0);
        }

        static int CompileShader(ShaderType type, string src)
        {
            int id = GL.CreateShader(type);
            GL.ShaderSource(id, src);
            GL.CompileShader(id);
            GL.GetShader(id, ShaderParameter.CompileStatus, out int ok);
            if (ok == 0)
            {
                string log = GL.GetShaderInfoLog(id);
                throw new InvalidOperationException($"OpenGL shader error ({type}): {log}");
            }
            return id;
        }

        public void BeginFrame() => _verts.Clear();

        public void EndFrame()
        {
            if (_verts.Count == 0) return;

            float[] data = _verts.ToArray();
            GL.UseProgram(_program);
            GL.Uniform2(_uScreenLoc, GameWorld.Width, GameWorld.Height);
            GL.BindVertexArray(_vao);
            GL.BindBuffer(BufferTarget.ArrayBuffer, _vbo);
            GL.BufferData(BufferTarget.ArrayBuffer, data.Length * sizeof(float), data, BufferUsageHint.StreamDraw);
            GL.DrawArrays(PrimitiveType.Triangles, 0, data.Length / 6);
            GL.BindVertexArray(0);
        }

        void PushVertex(float x, float y, ColorRgba c)
        {
            _verts.Add(x);
            _verts.Add(y);
            _verts.Add(c.R / 255f);
            _verts.Add(c.G / 255f);
            _verts.Add(c.B / 255f);
            _verts.Add(c.A / 255f);
        }

        public void Clear(ColorRgba color)
        {
            GL.ClearColor(color.R / 255f, color.G / 255f, color.B / 255f, color.A / 255f);
            GL.Clear(ClearBufferMask.ColorBufferBit);
        }

        public void FillRect(float x, float y, float w, float h, ColorRgba color)
        {
            PushVertex(x, y, color);
            PushVertex(x + w, y, color);
            PushVertex(x + w, y + h, color);

            PushVertex(x, y, color);
            PushVertex(x + w, y + h, color);
            PushVertex(x, y + h, color);
        }

        public void FillCircle(float cx, float cy, float r, ColorRgba color)
        {
            const int segments = 32;
            double step = Math.PI * 2.0 / segments;
            for (int i = 0; i < segments; i++)
            {
                double a0 = i * step;
                double a1 = (i + 1) * step;
                PushVertex(cx, cy, color);
                PushVertex(cx + (float)(Math.Cos(a0) * r), cy + (float)(Math.Sin(a0) * r), color);
                PushVertex(cx + (float)(Math.Cos(a1) * r), cy + (float)(Math.Sin(a1) * r), color);
            }
        }

        public void DrawText(string text, float x, float y, float size, ColorRgba color)
        {
            // HUD principal desenha pips vetoriais via FillRect.
        }

        public void Dispose()
        {
            if (_vbo != 0) GL.DeleteBuffer(_vbo);
            if (_vao != 0) GL.DeleteVertexArray(_vao);
            if (_program != 0) GL.DeleteProgram(_program);
        }
    }

    internal sealed class AstronGameWindow : GameWindow
    {
        readonly GameWorld _world = new GameWorld();
        readonly DesktopRenderer _renderer = new DesktopRenderer();

        public AstronGameWindow()
            : base(
                new GameWindowSettings { UpdateFrequency = 60.0 },
                new NativeWindowSettings
                {
                    ClientSize = new Vector2i(1280, 720),
                    Title = "Astron Game",
                    API = ContextAPI.OpenGL,
                    APIVersion = new Version(3, 3),
                    Profile = ContextProfile.Core,
                    Flags = ContextFlags.ForwardCompatible,
                })
        {
        }

        protected override void OnLoad()
        {
            base.OnLoad();
            GL.Enable(EnableCap.Blend);
            GL.BlendFunc(BlendingFactor.SrcAlpha, BlendingFactor.OneMinusSrcAlpha);
            _renderer.Init();
        }

        protected override void OnUnload()
        {
            _renderer.Dispose();
            base.OnUnload();
        }

        protected override void OnUpdateFrame(FrameEventArgs args)
        {
            base.OnUpdateFrame(args);

            if (KeyboardState.IsKeyDown(Keys.Escape))
            {
                Close();
                return;
            }

            var input = new GameInput
            {
                Left = KeyboardState.IsKeyDown(Keys.Left) || KeyboardState.IsKeyDown(Keys.A),
                Right = KeyboardState.IsKeyDown(Keys.Right) || KeyboardState.IsKeyDown(Keys.D),
                Jump = KeyboardState.IsKeyDown(Keys.Space) || KeyboardState.IsKeyDown(Keys.W) || KeyboardState.IsKeyDown(Keys.Up),
            };
            _world.Update(input, (float)args.Time);
        }

        protected override void OnRenderFrame(FrameEventArgs args)
        {
            base.OnRenderFrame(args);

            GL.Viewport(0, 0, ClientSize.X, ClientSize.Y);
            _renderer.BeginFrame();
            _world.Render(_renderer);
            _renderer.EndFrame();
            SwapBuffers();
        }
    }

    internal static class Program
    {
        [STAThread]
        private static void Main()
        {
            using (var game = new AstronGameWindow())
            {
                game.Run();
            }
        }
    }
}
