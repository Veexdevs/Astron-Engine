using System.Diagnostics;
using Android.App;
using Android.Content;
using Android.Content.PM;
using Android.Graphics;
using Android.OS;
using Android.Views;
using Astron.Game;

namespace Astron.AndroidApp
{
    [Activity(
        Label = "Astron Game",
        MainLauncher = true,
        ScreenOrientation = ScreenOrientation.SensorLandscape,
        ConfigurationChanges = ConfigChanges.ScreenSize | ConfigChanges.Orientation)]
    public sealed class MainActivity : Activity
    {
        protected override void OnCreate(Bundle savedInstanceState)
        {
            base.OnCreate(savedInstanceState);
            Window?.SetFlags(
                WindowManagerFlags.Fullscreen,
                WindowManagerFlags.Fullscreen);
            SetContentView(new GameView(this));
        }
    }

    /// <summary>View do jogo: loop de desenho via Invalidate() + toques.</summary>
    public sealed class GameView : View
    {
        readonly GameWorld _world = new GameWorld();
        readonly GameInput _input = new GameInput();
        long _lastTicks;

        public GameView(Context context) : base(context) { }

        public override bool OnTouchEvent(MotionEvent e)
        {
            if (e == null) return false;
            float w = Width;
            float h = Height;
            switch (e.ActionMasked)
            {
                case MotionEventActions.Down:
                case MotionEventActions.Move:
                    float x = e.GetX();
                    float y = e.GetY();
                    _input.Left = x < w * 0.33f && y > h * 0.30f;
                    _input.Right = x > w * 0.67f && y > h * 0.30f;
                    _input.Jump = y < h * 0.45f;
                    break;
                case MotionEventActions.Up:
                case MotionEventActions.Cancel:
                    _input.Left = false;
                    _input.Right = false;
                    _input.Jump = false;
                    break;
            }
            return true;
        }

        protected override void OnDraw(Canvas canvas)
        {
            long now = Stopwatch.GetTimestamp();
            if (_lastTicks == 0) _lastTicks = now;
            float dt = (float)((now - _lastTicks) / (double)Stopwatch.Frequency);
            _lastTicks = now;

            _world.Update(_input, dt);
            _world.Render(new CanvasRenderer(canvas, Width, Height));
            Invalidate();
        }
    }

    sealed class CanvasRenderer : IRenderer
    {
        readonly Canvas _canvas;
        readonly float _sx;
        readonly float _sy;
        readonly Paint _paint = new Paint(PaintFlags.AntiAlias);

        public CanvasRenderer(Canvas canvas, int width, int height)
        {
            _canvas = canvas;
            _sx = width / GameWorld.Width;
            _sy = height / GameWorld.Height;
        }

        static Color Col(ColorRgba c) => Color.Argb(c.A, c.R, c.G, c.B);

        public void Clear(ColorRgba color) => _canvas.DrawColor(Col(color));

        public void FillRect(float x, float y, float w, float h, ColorRgba color)
        {
            _paint.Color = Col(color);
            _canvas.DrawRect(x * _sx, y * _sy, (x + w) * _sx, (y + h) * _sy, _paint);
        }

        public void FillCircle(float cx, float cy, float r, ColorRgba color)
        {
            _paint.Color = Col(color);
            _canvas.DrawCircle(cx * _sx, cy * _sy, r * _sx, _paint);
        }

        public void DrawText(string text, float x, float y, float size, ColorRgba color)
        {
            _paint.Color = Col(color);
            _paint.TextSize = size * _sy;
            _paint.SetTypeface(Typeface.Create(Typeface.Default, TypefaceStyle.Bold));
            _canvas.DrawText(text, x * _sx, y * _sy, _paint);
        }
    }
}
