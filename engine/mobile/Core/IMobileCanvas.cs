using System;
using System.Collections.Generic;

namespace Astron.Engine.Mobile
{
    public struct UiColor
    {
        public byte R;
        public byte G;
        public byte B;
        public byte A;

        public UiColor(byte r, byte g, byte b, byte a = 255)
        {
            R = r;
            G = g;
            B = b;
            A = a;
        }

        public static UiColor FromHex(uint rgba)
        {
            return new UiColor(
                (byte)((rgba >> 24) & 0xFF),
                (byte)((rgba >> 16) & 0xFF),
                (byte)((rgba >> 8) & 0xFF),
                (byte)(rgba & 0xFF)
            );
        }

        public static UiColor ParseHtml(string hex, UiColor fallback)
        {
            if (string.IsNullOrEmpty(hex)) return fallback;
            string s = hex.Trim();
            if (s.StartsWith("#")) s = s.Substring(1);
            if (s.Length == 6 && uint.TryParse(s, System.Globalization.NumberStyles.HexNumber, null, out uint rgb))
            {
                return new UiColor(
                    (byte)((rgb >> 16) & 0xFF),
                    (byte)((rgb >> 8) & 0xFF),
                    (byte)(rgb & 0xFF),
                    255
                );
            }
            return fallback;
        }

        public UiColor WithAlpha(byte alpha) => new UiColor(R, G, B, alpha);

        public UiColor ScaleRgb(float factor)
        {
            int nr = (int)(R * factor);
            int ng = (int)(G * factor);
            int nb = (int)(B * factor);
            if (nr < 0) nr = 0; if (nr > 255) nr = 255;
            if (ng < 0) ng = 0; if (ng > 255) ng = 255;
            if (nb < 0) nb = 0; if (nb > 255) nb = 255;
            return new UiColor((byte)nr, (byte)ng, (byte)nb, A);
        }
    }

    public struct UiRect
    {
        public float X;
        public float Y;
        public float W;
        public float H;

        public UiRect(float x, float y, float w, float h)
        {
            X = x;
            Y = y;
            W = w;
            H = h;
        }

        public float Right => X + W;
        public float Bottom => Y + H;
        public float CenterX => X + W * 0.5f;
        public float CenterY => Y + H * 0.5f;

        public bool Contains(float px, float py) =>
            px >= X && px <= X + W && py >= Y && py <= Y + H;
    }

    public enum TouchPhase
    {
        Began,
        Moved,
        Ended,
        Cancelled
    }

    /// <summary>
    /// Abstração nativa de desenho 2D/3D sem WebView:
    /// implementada em Android.Graphics.Canvas (APK) e CoreGraphics CGContext (IPA).
    /// </summary>
    public interface IMobileCanvas
    {
        float Width { get; }
        float Height { get; }

        void Clear(UiColor color);
        void PushClip(UiRect rect);
        void PopClip();

        void FillRect(UiRect rect, UiColor color, float cornerRadius = 0f);
        void StrokeRect(UiRect rect, UiColor color, float strokeWidth = 1f, float cornerRadius = 0f);
        void DrawLine(float x1, float y1, float x2, float y2, UiColor color, float strokeWidth = 1.5f);
        void FillCircle(float cx, float cy, float radius, UiColor color);
        void StrokeCircle(float cx, float cy, float radius, UiColor color, float strokeWidth = 1.5f);
        void FillPolygon(float[] xyPairs, int vertexCount, UiColor fill, UiColor stroke, float strokeWidth = 1f);
        bool DrawImage(string assetName, UiRect destRect, byte alpha = 255, float cornerRadius = 0f);
        void DrawText(string text, float x, float y, float fontSize, UiColor color, bool bold = false, bool centered = false);
        float MeasureText(string text, float fontSize, bool bold = false);
    }

    /// <summary>
    /// Ponte nativa com o sistema operacional Android / iOS (teclado nativo, URLs e armazenamento local).
    /// </summary>
    public interface IMobileHostBridge
    {
        string PersistentDataPath { get; }
        string PlatformName { get; }
        void PromptText(string title, string subtitle, string currentValue, bool password, Action<string> onAccepted);
        void OpenUrl(string httpsUrl);
        void RequestRedraw();
    }

    public sealed class UiHitZone
    {
        public string Id;
        public UiRect Rect;
        public Action OnTap;
    }

    public sealed class UiHitMap
    {
        private readonly List<UiHitZone> _zones = new List<UiHitZone>(128);

        public void Clear() => _zones.Clear();

        public void Register(string id, UiRect rect, Action onTap)
        {
            if (onTap == null) return;
            _zones.Add(new UiHitZone { Id = id, Rect = rect, OnTap = onTap });
        }

        public bool DispatchTap(float x, float y)
        {
            // Percorre do topo (último desenhado) para o fundo
            for (int i = _zones.Count - 1; i >= 0; i--)
            {
                if (_zones[i].Rect.Contains(x, y))
                {
                    _zones[i].OnTap?.Invoke();
                    return true;
                }
            }
            return false;
        }
    }
}
