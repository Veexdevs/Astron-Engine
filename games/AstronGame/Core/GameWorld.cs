using System;
using System.Collections.Generic;

namespace Astron.Game
{
    /// <summary>Cor RGBA simples (0-255).</summary>
    public struct ColorRgba
    {
        public byte R, G, B, A;
        public ColorRgba(byte r, byte g, byte b, byte a = 255) { R = r; G = g; B = b; A = a; }
        public static ColorRgba Hex(uint rgb) =>
            new ColorRgba((byte)((rgb >> 16) & 255), (byte)((rgb >> 8) & 255), (byte)(rgb & 255));
    }

    /// <summary>Entrada do jogador — implementada por plataforma.</summary>
    public sealed class GameInput
    {
        public bool Left;
        public bool Right;
        public bool Jump;
    }

    /// <summary>API de desenho abstrata: cada plataforma implementa (OpenGL, Canvas, CoreGraphics...).</summary>
    public interface IRenderer
    {
        void Clear(ColorRgba color);
        void FillRect(float x, float y, float w, float h, ColorRgba color);
        void FillCircle(float cx, float cy, float r, ColorRgba color);
        void DrawText(string text, float x, float y, float size, ColorRgba color);
    }

    public struct RectF
    {
        public float X, Y, W, H;
        public RectF(float x, float y, float w, float h) { X = x; Y = y; W = w; H = h; }
        public float Right => X + W;
        public float Bottom => Y + H;
        public bool Intersects(RectF o) =>
            X < o.Right && Right > o.X && Y < o.Bottom && Bottom > o.Y;
    }

    public sealed class Pickup
    {
        public float X, Y;
        public bool Taken;
        public Pickup(float x, float y) { X = x; Y = y; }
    }

    /// <summary>
    /// Mundo do jogo (plataforma 2.5D) — lógica 100% C#, compartilhada por
    /// Windows (EXE), Linux, Android (APK) e iOS (IPA).
    /// </summary>
    public sealed class GameWorld
    {
        public const float Width = 1280f;
        public const float Height = 720f;

        const float PlayerSize = 36f;
        const float MoveSpeed = 340f;
        const float JumpPower = 840f;
        const float Gravity = 2400f;

        public float Px = 90f;
        public float Py = 520f;
        public float Vx;
        public float Vy;
        public bool Grounded;
        public int Score;
        public int Deaths;

        public readonly List<RectF> Platforms = new List<RectF>();
        public readonly List<Pickup> Pickups = new List<Pickup>();

        float _coyote;
        bool _prevJump;

        static readonly ColorRgba BgTop = ColorRgba.Hex(0x12151c);
        static readonly ColorRgba PlatformCol = ColorRgba.Hex(0x3d4654);
        static readonly ColorRgba PlatformTop = ColorRgba.Hex(0x59657a);
        static readonly ColorRgba PlayerCol = ColorRgba.Hex(0xf2f3f7);
        static readonly ColorRgba PlayerAccent = ColorRgba.Hex(0x3b82f6);
        static readonly ColorRgba CoinCol = ColorRgba.Hex(0xf5b544);
        static readonly ColorRgba HudCol = ColorRgba.Hex(0xffffff);

        public GameWorld()
        {
            // chão
            Platforms.Add(new RectF(0f, 660f, Width, 60f));
            // plataformas flutuantes
            Platforms.Add(new RectF(240f, 540f, 190f, 26f));
            Platforms.Add(new RectF(520f, 450f, 170f, 26f));
            Platforms.Add(new RectF(790f, 360f, 170f, 26f));
            Platforms.Add(new RectF(1020f, 260f, 180f, 26f));
            Platforms.Add(new RectF(640f, 200f, 160f, 26f));
            Platforms.Add(new RectF(120f, 300f, 150f, 26f));
            // paredes
            Platforms.Add(new RectF(-40f, 0f, 40f, Height));
            Platforms.Add(new RectF(Width, 0f, 40f, Height));

            Pickups.Add(new Pickup(320f, 500f));
            Pickups.Add(new Pickup(600f, 410f));
            Pickups.Add(new Pickup(870f, 320f));
            Pickups.Add(new Pickup(1100f, 220f));
            Pickups.Add(new Pickup(710f, 160f));
            Pickups.Add(new Pickup(190f, 260f));
        }

        public void Respawn()
        {
            Px = 90f;
            Py = 520f;
            Vx = 0f;
            Vy = 0f;
            Deaths++;
        }

        public void Update(GameInput input, float dt)
        {
            if (dt <= 0f) return;
            if (dt > 0.05f) dt = 0.05f;

            // movimento horizontal
            float axis = (input.Right ? 1f : 0f) - (input.Left ? 1f : 0f);
            Vx = axis * MoveSpeed;
            Px += Vx * dt;
            ResolveHorizontal();

            // gravidade
            Vy += Gravity * dt;
            Py += Vy * dt;
            Grounded = false;
            ResolveVertical();

            // coyote time + pulo (edge detection)
            _coyote = Grounded ? 0.12f : _coyote - dt;
            if (input.Jump && !_prevJump && (_coyote > 0f || Grounded))
            {
                Vy = -JumpPower;
                Grounded = false;
                _coyote = 0f;
            }
            _prevJump = input.Jump;

            // coletáveis
            var player = new RectF(Px, Py, PlayerSize, PlayerSize);
            for (int i = 0; i < Pickups.Count; i++)
            {
                var p = Pickups[i];
                if (p.Taken) continue;
                var coin = new RectF(p.X - 14f, p.Y - 14f, 28f, 28f);
                if (player.Intersects(coin))
                {
                    p.Taken = true;
                    Score += 10;
                }
            }

            // quedas
            if (Py > Height + 240f) Respawn();
        }

        void ResolveHorizontal()
        {
            var box = new RectF(Px, Py, PlayerSize, PlayerSize);
            for (int i = 0; i < Platforms.Count; i++)
            {
                var pl = Platforms[i];
                if (!box.Intersects(pl)) continue;
                if (Vx > 0f) Px = pl.X - PlayerSize;
                else if (Vx < 0f) Px = pl.Right;
                box = new RectF(Px, Py, PlayerSize, PlayerSize);
                Vx = 0f;
            }
        }

        void ResolveVertical()
        {
            var box = new RectF(Px, Py, PlayerSize, PlayerSize);
            for (int i = 0; i < Platforms.Count; i++)
            {
                var pl = Platforms[i];
                if (!box.Intersects(pl)) continue;
                if (Vy > 0f)
                {
                    Py = pl.Y - PlayerSize;
                    Grounded = true;
                }
                else if (Vy < 0f)
                {
                    Py = pl.Bottom;
                }
                box = new RectF(Px, Py, PlayerSize, PlayerSize);
                Vy = 0f;
            }
        }

        public void Render(IRenderer r)
        {
            r.Clear(BgTop);

            // plataformas
            for (int i = 0; i < Platforms.Count; i++)
            {
                var pl = Platforms[i];
                if (pl.X >= Width || pl.Right <= 0f) continue;
                r.FillRect(pl.X, pl.Y, pl.W, pl.H, PlatformCol);
                r.FillRect(pl.X, pl.Y, pl.W, 5f, PlatformTop);
            }

            // moedas
            for (int i = 0; i < Pickups.Count; i++)
            {
                var p = Pickups[i];
                if (p.Taken) continue;
                r.FillCircle(p.X, p.Y, 13f, CoinCol);
                r.FillCircle(p.X - 3f, p.Y - 4f, 4f, new ColorRgba(255, 244, 214));
            }

            // jogador
            r.FillRect(Px, Py, PlayerSize, PlayerSize, PlayerCol);
            r.FillRect(Px + 6f, Py + 8f, 8f, 8f, PlayerAccent);
            r.FillRect(Px + 22f, Py + 8f, 8f, 8f, PlayerAccent);
            r.FillRect(Px + 8f, Py + 24f, 20f, 4f, PlayerAccent);

            // HUD: pontuação em pips (funciona em todas as plataformas)
            int pips = Score / 10;
            for (int i = 0; i < pips && i < 20; i++)
                r.FillRect(24f + i * 22f, 24f, 16f, 16f, CoinCol);
            r.DrawText($"Score {Score}", 24f, 64f, 26f, HudCol);
            r.DrawText($"Deaths {Deaths}", 24f, 96f, 22f, new ColorRgba(180, 184, 196));
        }
    }
}
