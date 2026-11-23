using System;
using System.Diagnostics;
using Astron.Game;
using CoreGraphics;
using Foundation;
using UIKit;

namespace Astron.iOSApp
{
    [Register("AppDelegate")]
    public sealed class AppDelegate : UIApplicationDelegate
    {
        public override UIWindow Window { get; set; }

        public override bool FinishedLaunching(UIApplication application, NSDictionary launchOptions)
        {
            Window = new UIWindow(UIScreen.MainScreen.Bounds)
            {
                RootViewController = new GameViewController(),
            };
            Window.MakeKeyAndVisible();
            return true;
        }
    }

    public static class EntryPoint
    {
        public static void Main(string[] args)
        {
            UIApplication.Main(args, null, typeof(AppDelegate));
        }
    }

    public sealed class GameViewController : UIViewController
    {
        public override void ViewDidLoad()
        {
            base.ViewDidLoad();
            View = new GameView(UIScreen.MainScreen.Bounds)
            {
                BackgroundColor = UIColor.Black,
            };
        }

        public override bool PrefersStatusBarHidden() => true;

        public override UIInterfaceOrientationMask GetSupportedInterfaceOrientations() =>
            UIInterfaceOrientationMask.Landscape;
    }

    /// <summary>View do jogo: timer de 60 FPS + desenho em CoreGraphics.</summary>
    public sealed class GameView : UIView
    {
        readonly GameWorld _world = new GameWorld();
        readonly GameInput _input = new GameInput();
        NSTimer _timer;
        long _lastTicks;

        public GameView(CGRect frame) : base(frame)
        {
            UserInteractionEnabled = true;
            _lastTicks = Stopwatch.GetTimestamp();
            _timer = NSTimer.CreateRepeatingScheduledTimer(1.0 / 60.0, OnTick);
        }

        void OnTick(NSTimer timer)
        {
            long now = Stopwatch.GetTimestamp();
            float dt = (float)((now - _lastTicks) / (double)Stopwatch.Frequency);
            _lastTicks = now;
            _world.Update(_input, dt);
            SetNeedsDisplay();
        }

        public override void Draw(CGRect rect)
        {
            CGContext ctx = UIGraphics.GetCurrentContext();
            if (ctx == null) return;
            _world.Render(new CgRenderer(ctx, Bounds.Size));
        }

        public override void TouchesBegan(NSSet touches, UIEvent evt) => ApplyTouches(touches);

        public override void TouchesMoved(NSSet touches, UIEvent evt) => ApplyTouches(touches);

        public override void TouchesEnded(NSSet touches, UIEvent evt) => ReleaseTouches();

        public override void TouchesCancelled(NSSet touches, UIEvent evt) => ReleaseTouches();

        void ApplyTouches(NSSet touches)
        {
            var touch = touches?.AnyObject as UITouch;
            if (touch == null) return;
            CGPoint p = touch.LocationInView(this);
            _input.Left = p.X < Bounds.Width * 0.33 && p.Y > Bounds.Height * 0.30;
            _input.Right = p.X > Bounds.Width * 0.67 && p.Y > Bounds.Height * 0.30;
            _input.Jump = p.Y < Bounds.Height * 0.45;
        }

        void ReleaseTouches()
        {
            _input.Left = false;
            _input.Right = false;
            _input.Jump = false;
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                _timer?.Invalidate();
                _timer?.Dispose();
                _timer = null;
            }
            base.Dispose(disposing);
        }
    }

    sealed class CgRenderer : IRenderer
    {
        readonly CGContext _g;
        readonly CGSize _size;
        readonly float _sx;
        readonly float _sy;

        public CgRenderer(CGContext g, CGSize size)
        {
            _g = g;
            _size = size;
            _sx = (float)(size.Width / GameWorld.Width);
            _sy = (float)(size.Height / GameWorld.Height);
        }

        void SetFill(ColorRgba c) =>
            _g.SetFillColor(c.R / 255f, c.G / 255f, c.B / 255f, c.A / 255f);

        public void Clear(ColorRgba color)
        {
            SetFill(color);
            _g.FillRect(new CGRect(0, 0, _size.Width, _size.Height));
        }

        public void FillRect(float x, float y, float w, float h, ColorRgba color)
        {
            SetFill(color);
            _g.FillRect(new CGRect(x * _sx, y * _sy, w * _sx, h * _sy));
        }

        public void FillCircle(float cx, float cy, float r, ColorRgba color)
        {
            SetFill(color);
            _g.FillEllipseInRect(new CGRect((cx - r) * _sx, (cy - r) * _sy, 2 * r * _sx, 2 * r * _sy));
        }

        public void DrawText(string text, float x, float y, float size, ColorRgba color)
        {
            using (var str = new NSString(text ?? string.Empty))
            {
                var attrs = new UIStringAttributes
                {
                    ForegroundColor = UIColor.FromRGBA(color.R, color.G, color.B, color.A),
                    Font = UIFont.BoldSystemFontOfSize((nfloat)(size * _sy)),
                };
                str.DrawString(new CGPoint(x * _sx, y * _sy), attrs);
            }
        }
    }
}
