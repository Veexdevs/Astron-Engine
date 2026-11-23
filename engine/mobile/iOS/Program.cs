using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using Astron.Engine.Mobile;
using CoreGraphics;
using Foundation;
using UIKit;

namespace AstronEngine.iOSApp
{
    [Register("AppDelegate")]
    public sealed class AppDelegate : UIApplicationDelegate
    {
        public override UIWindow Window { get; set; }

        public override bool FinishedLaunching(UIApplication application, NSDictionary launchOptions)
        {
            application.IdleTimerDisabled = true;
            Window = new UIWindow(UIScreen.MainScreen.Bounds)
            {
                RootViewController = new AstronEngineViewController(),
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

    public sealed class AstronEngineViewController : UIViewController, IMobileHostBridge
    {
        private AstronEngineIosView _engineView;

        public string PersistentDataPath =>
            Environment.GetFolderPath(Environment.SpecialFolder.Personal);

        public string PlatformName => "iOS (.ipa)";

        public override void ViewDidLoad()
        {
            base.ViewDidLoad();
            _engineView = new AstronEngineIosView(UIScreen.MainScreen.Bounds, this)
            {
                BackgroundColor = UIColor.FromRGB(9, 10, 14),
                AutoresizingMask = UIViewAutoresizing.FlexibleWidth | UIViewAutoresizing.FlexibleHeight,
            };
            View = _engineView;
        }

        public override bool PrefersStatusBarHidden() => true;

        public override UIInterfaceOrientationMask GetSupportedInterfaceOrientations() =>
            UIInterfaceOrientationMask.Landscape;

        public void RequestRedraw()
        {
            InvokeOnMainThread(() => _engineView?.SetNeedsDisplay());
        }

        public void PromptText(string title, string subtitle, string currentValue, bool password, Action<string> onAccepted)
        {
            InvokeOnMainThread(() =>
            {
                bool multiline = !password && currentValue != null && currentValue.IndexOf('\n') >= 0;
                if (multiline)
                {
                    PresentMultilineCodeSheet(title ?? "Editor Astron", currentValue ?? string.Empty, onAccepted);
                    return;
                }

                var alert = UIAlertController.Create(
                    title ?? "Astron Engine",
                    string.IsNullOrEmpty(subtitle) ? null : subtitle,
                    UIAlertControllerStyle.Alert);

                alert.AddTextField(tf =>
                {
                    tf.Text = currentValue ?? string.Empty;
                    tf.SecureTextEntry = password;
                    tf.AutocorrectionType = UITextAutocorrectionType.No;
                    tf.AutocapitalizationType = UITextAutocapitalizationType.None;
                    tf.ClearButtonMode = UITextFieldViewMode.WhileEditing;
                });

                alert.AddAction(UIAlertAction.Create("Cancelar", UIAlertActionStyle.Cancel, null));
                alert.AddAction(UIAlertAction.Create("Confirmar", UIAlertActionStyle.Default, _ =>
                {
                    string txt = (alert.TextFields != null && alert.TextFields.Length > 0)
                        ? (alert.TextFields[0].Text ?? string.Empty)
                        : string.Empty;
                    onAccepted?.Invoke(txt);
                    _engineView?.SetNeedsDisplay();
                }));

                PresentViewController(alert, true, null);
            });
        }

        private void PresentMultilineCodeSheet(string title, string initialText, Action<string> onAccepted)
        {
            var sheet = new UIViewController
            {
                ModalPresentationStyle = UIModalPresentationStyle.FullScreen,
            };
            CGRect bounds = UIScreen.MainScreen.Bounds;
            var root = new UIView(bounds)
            {
                BackgroundColor = UIColor.FromRGB(13, 14, 19),
                AutoresizingMask = UIViewAutoresizing.FlexibleWidth | UIViewAutoresizing.FlexibleHeight,
            };

            var header = new UIView(new CGRect(0, 0, bounds.Width, 46))
            {
                BackgroundColor = UIColor.FromRGB(20, 22, 30),
                AutoresizingMask = UIViewAutoresizing.FlexibleWidth,
            };

            var titleLabel = new UILabel(new CGRect(110, 8, bounds.Width - 220, 30))
            {
                Text = title,
                TextColor = UIColor.FromRGB(235, 238, 248),
                TextAlignment = UITextAlignment.Center,
                Font = UIFont.BoldSystemFontOfSize(14f),
                AutoresizingMask = UIViewAutoresizing.FlexibleWidth,
            };
            header.AddSubview(titleLabel);

            var cancelBtn = new UIButton(UIButtonType.System)
            {
                Frame = new CGRect(14, 6, 90, 34),
            };
            cancelBtn.SetTitle("Cancelar", UIControlState.Normal);
            cancelBtn.SetTitleColor(UIColor.FromRGB(175, 182, 198), UIControlState.Normal);

            var saveBtn = new UIButton(UIButtonType.System)
            {
                Frame = new CGRect(bounds.Width - 104, 6, 90, 34),
                AutoresizingMask = UIViewAutoresizing.FlexibleLeftMargin,
            };
            saveBtn.SetTitle("Salvar", UIControlState.Normal);
            saveBtn.SetTitleColor(UIColor.FromRGB(115, 134, 255), UIControlState.Normal);

            var textView = new UITextView(new CGRect(12, 54, bounds.Width - 24, bounds.Height - 66))
            {
                BackgroundColor = UIColor.FromRGB(18, 20, 27),
                TextColor = UIColor.FromRGB(230, 235, 245),
                Font = UIFont.FromName("Menlo", 13f) ?? UIFont.SystemFontOfSize(13f),
                Text = initialText,
                AutocorrectionType = UITextAutocorrectionType.No;
                AutocapitalizationType = UITextAutocapitalizationType.None,
                AutoresizingMask = UIViewAutoresizing.FlexibleWidth | UIViewAutoresizing.FlexibleHeight,
            };

            cancelBtn.TouchUpInside += (_, _) =>
            {
                sheet.DismissViewController(true, null);
            };
            saveBtn.TouchUpInside += (_, _) =>
            {
                string updated = textView.Text ?? string.Empty;
                sheet.DismissViewController(true, () =>
                {
                    onAccepted?.Invoke(updated);
                    _engineView?.SetNeedsDisplay();
                });
            };

            header.AddSubview(cancelBtn);
            header.AddSubview(saveBtn);
            root.AddSubview(header);
            root.AddSubview(textView);
            sheet.View = root;

            PresentViewController(sheet, true, () => textView.BecomeFirstResponder());
        }

        public void OpenUrl(string httpsUrl)
        {
            InvokeOnMainThread(() =>
            {
                try
                {
                    var nsUrl = NSUrl.FromString(httpsUrl);
                    if (nsUrl != null)
                        UIApplication.SharedApplication.OpenUrl(nsUrl, new UIApplicationOpenUrlOptions(), null);
                }
                catch
                {
                    // Ignore invalid URL scheme
                }
            });
        }
    }

    public sealed class AstronEngineIosView : UIView
    {
        private readonly AstronMobileEngineApp _app;
        private readonly AstronCoreGraphicsCanvas _canvasAdapter = new AstronCoreGraphicsCanvas();
        private NSTimer _timer;
        private long _lastTicks;
        private float _vScale = 1.0f;

        public AstronEngineIosView(CGRect frame, IMobileHostBridge host) : base(frame)
        {
            UserInteractionEnabled = true;
            MultipleTouchEnabled = true;
            _app = new AstronMobileEngineApp(host);
            _lastTicks = Stopwatch.GetTimestamp();
            _timer = NSTimer.CreateRepeatingScheduledTimer(1.0 / 60.0, OnTick);

            var pinch = new UIPinchGestureRecognizer(OnPinch);
            AddGestureRecognizer(pinch);
        }

        private float ComputeVirtualScale()
        {
            float h = (float)Bounds.Height;
            if (h <= 1f) return 1.0f;
            float targetH = 600f;
            return h < targetH ? (h / targetH) : 1.0f;
        }

        private void OnTick(NSTimer timer)
        {
            long now = Stopwatch.GetTimestamp();
            float dt = (float)((now - _lastTicks) / (double)Stopwatch.Frequency);
            _lastTicks = now;
            _app.Update(dt);
            if (_app.NeedsContinuousAnimation)
                SetNeedsDisplay();
        }

        private void OnPinch(UIPinchGestureRecognizer gr)
        {
            if (gr.State == UIGestureRecognizerState.Changed)
            {
                float ratio = (float)gr.Scale;
                if (Math.Abs(ratio - 1f) > 0.015f)
                {
                    _app.HandlePinchZoom(ratio);
                    gr.Scale = 1.0f;
                    SetNeedsDisplay();
                }
            }
        }

        public override void Draw(CGRect rect)
        {
            CGContext ctx = UIGraphics.GetCurrentContext();
            if (ctx == null) return;

            _vScale = ComputeVirtualScale();
            float logicalW = (float)Bounds.Width / _vScale;
            float logicalH = (float)Bounds.Height / _vScale;

            ctx.SaveState();
            ctx.ScaleCTM((nfloat)_vScale, (nfloat)_vScale);
            _canvasAdapter.Bind(ctx, logicalW, logicalH);
            _app.Render(_canvasAdapter);
            ctx.RestoreState();
        }

        public override void TouchesBegan(NSSet touches, UIEvent evt)
        {
            var touch = touches?.AnyObject as UITouch;
            if (touch == null) return;
            CGPoint p = touch.LocationInView(this);
            _app.HandleTouch(TouchPhase.Began, (float)p.X / _vScale, (float)p.Y / _vScale);
            SetNeedsDisplay();
        }

        public override void TouchesMoved(NSSet touches, UIEvent evt)
        {
            var touch = touches?.AnyObject as UITouch;
            if (touch == null) return;
            CGPoint p = touch.LocationInView(this);
            _app.HandleTouch(TouchPhase.Moved, (float)p.X / _vScale, (float)p.Y / _vScale);
            SetNeedsDisplay();
        }

        public override void TouchesEnded(NSSet touches, UIEvent evt)
        {
            var touch = touches?.AnyObject as UITouch;
            if (touch == null) return;
            CGPoint p = touch.LocationInView(this);
            _app.HandleTouch(TouchPhase.Ended, (float)p.X / _vScale, (float)p.Y / _vScale);
            SetNeedsDisplay();
        }

        public override void TouchesCancelled(NSSet touches, UIEvent evt)
        {
            var touch = touches?.AnyObject as UITouch;
            CGPoint p = touch != null ? touch.LocationInView(this) : new CGPoint(0, 0);
            _app.HandleTouch(TouchPhase.Cancelled, (float)p.X / _vScale, (float)p.Y / _vScale);
            SetNeedsDisplay();
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

    internal sealed class AstronCoreGraphicsCanvas : IMobileCanvas
    {
        private readonly Dictionary<string, UIImage> _imageCache = new Dictionary<string, UIImage>();
        private CGContext _g;

        public float Width { get; private set; }
        public float Height { get; private set; }

        public void Bind(CGContext ctx, float width, float height)
        {
            _g = ctx;
            Width = width;
            Height = height;
        }

        private void SetFill(UiColor c) =>
            _g.SetFillColor((nfloat)(c.R / 255f), (nfloat)(c.G / 255f), (nfloat)(c.B / 255f), (nfloat)(c.A / 255f));

        private void SetStroke(UiColor c) =>
            _g.SetStrokeColor((nfloat)(c.R / 255f), (nfloat)(c.G / 255f), (nfloat)(c.B / 255f), (nfloat)(c.A / 255f));

        public void Clear(UiColor color)
        {
            if (_g == null) return;
            SetFill(color);
            _g.FillRect(new CGRect(0, 0, Width, Height));
        }

        public void PushClip(UiRect rect)
        {
            if (_g == null) return;
            _g.SaveState();
            _g.ClipToRect(new CGRect(rect.X, rect.Y, Math.Max(0f, rect.W), Math.Max(0f, rect.H)));
        }

        public void PopClip()
        {
            _g?.RestoreState();
        }

        public void FillRect(UiRect rect, UiColor color, float cornerRadius = 0f)
        {
            if (_g == null || rect.W <= 0f || rect.H <= 0f) return;
            SetFill(color);
            var cgRect = new CGRect(rect.X, rect.Y, rect.W, rect.H);
            if (cornerRadius > 0.5f)
            {
                using (var bp = UIBezierPath.FromRoundedRect(cgRect, (nfloat)cornerRadius))
                {
                    _g.AddPath(bp.CGPath);
                    _g.FillPath();
                }
            }
            else
            {
                _g.FillRect(cgRect);
            }
        }

        public void StrokeRect(UiRect rect, UiColor color, float strokeWidth = 1f, float cornerRadius = 0f)
        {
            if (_g == null || rect.W <= 0f || rect.H <= 0f) return;
            SetStroke(color);
            _g.SetLineWidth((nfloat)strokeWidth);
            var cgRect = new CGRect(rect.X, rect.Y, rect.W, rect.H);
            if (cornerRadius > 0.5f)
            {
                using (var bp = UIBezierPath.FromRoundedRect(cgRect, (nfloat)cornerRadius))
                {
                    _g.AddPath(bp.CGPath);
                    _g.StrokePath();
                }
            }
            else
            {
                _g.StrokeRect(cgRect);
            }
        }

        public void DrawLine(float x1, float y1, float x2, float y2, UiColor color, float strokeWidth = 1.5f)
        {
            if (_g == null) return;
            SetStroke(color);
            _g.SetLineWidth((nfloat)strokeWidth);
            _g.MoveTo((nfloat)x1, (nfloat)y1);
            _g.AddLineToPoint((nfloat)x2, (nfloat)y2);
            _g.StrokePath();
        }

        public void FillCircle(float cx, float cy, float radius, UiColor color)
        {
            if (_g == null || radius <= 0f) return;
            SetFill(color);
            _g.FillEllipseInRect(new CGRect(cx - radius, cy - radius, radius * 2f, radius * 2f));
        }

        public void StrokeCircle(float cx, float cy, float radius, UiColor color, float strokeWidth = 1.5f)
        {
            if (_g == null || radius <= 0f) return;
            SetStroke(color);
            _g.SetLineWidth((nfloat)strokeWidth);
            _g.StrokeEllipseInRect(new CGRect(cx - radius, cy - radius, radius * 2f, radius * 2f));
        }

        public void FillPolygon(float[] xyPairs, int vertexCount, UiColor fill, UiColor stroke, float strokeWidth = 1f)
        {
            if (_g == null || vertexCount < 3 || xyPairs == null || xyPairs.Length < vertexCount * 2) return;

            if (fill.A > 0)
            {
                SetFill(fill);
                _g.MoveTo((nfloat)xyPairs[0], (nfloat)xyPairs[1]);
                for (int i = 1; i < vertexCount; i++)
                    _g.AddLineToPoint((nfloat)xyPairs[i * 2], (nfloat)xyPairs[i * 2 + 1]);
                _g.ClosePath();
                _g.FillPath();
            }

            if (stroke.A > 0 && strokeWidth > 0f)
            {
                SetStroke(stroke);
                _g.SetLineWidth((nfloat)strokeWidth);
                _g.MoveTo((nfloat)xyPairs[0], (nfloat)xyPairs[1]);
                for (int i = 1; i < vertexCount; i++)
                    _g.AddLineToPoint((nfloat)xyPairs[i * 2], (nfloat)xyPairs[i * 2 + 1]);
                _g.ClosePath();
                _g.StrokePath();
            }
        }

        private UIImage GetImage(string assetName)
        {
            if (string.IsNullOrEmpty(assetName)) return null;
            if (_imageCache.TryGetValue(assetName, out var cached))
                return cached;

            UIImage img = null;
            try
            {
                string fullPath = Path.Combine(NSBundle.MainBundle.BundlePath, assetName);
                if (File.Exists(fullPath))
                    img = UIImage.FromFile(fullPath);
                if (img == null)
                    img = UIImage.FromBundle(assetName);
            }
            catch
            {
                img = null;
            }

            _imageCache[assetName] = img;
            return img;
        }

        public bool DrawImage(string assetName, UiRect destRect, byte alpha = 255, float cornerRadius = 0f)
        {
            if (_g == null || destRect.W <= 0f || destRect.H <= 0f) return false;
            var img = GetImage(assetName);
            if (img == null) return false;

            var rect = new CGRect(destRect.X, destRect.Y, destRect.W, destRect.H);
            nfloat cgAlpha = (nfloat)(alpha / 255f);
            if (cornerRadius > 0.5f)
            {
                _g.SaveState();
                using (var bp = UIBezierPath.FromRoundedRect(rect, (nfloat)cornerRadius))
                {
                    _g.AddPath(bp.CGPath);
                    _g.Clip();
                }
                img.Draw(rect, CGBlendMode.Normal, cgAlpha);
                _g.RestoreState();
            }
            else
            {
                img.Draw(rect, CGBlendMode.Normal, cgAlpha);
            }
            return true;
        }

        public void DrawText(string text, float x, float y, float fontSize, UiColor color, bool bold = false, bool centered = false)
        {
            if (_g == null || string.IsNullOrEmpty(text)) return;
            using (var str = new NSString(text))
            {
                var attrs = new UIStringAttributes
                {
                    ForegroundColor = UIColor.FromRGBA(color.R, color.G, color.B, color.A),
                    Font = bold
                        ? UIFont.BoldSystemFontOfSize((nfloat)fontSize)
                        : UIFont.SystemFontOfSize((nfloat)fontSize),
                };
                nfloat drawX = (nfloat)x;
                if (centered)
                {
                    CGSize sz = str.GetSizeUsingAttributes(attrs);
                    drawX = (nfloat)x - sz.Width * 0.5f;
                }
                str.DrawString(new CGPoint(drawX, (nfloat)y), attrs);
            }
        }

        public float MeasureText(string text, float fontSize, bool bold = false)
        {
            if (string.IsNullOrEmpty(text)) return 0f;
            using (var str = new NSString(text))
            {
                var attrs = new UIStringAttributes
                {
                    Font = bold
                        ? UIFont.BoldSystemFontOfSize((nfloat)fontSize)
                        : UIFont.SystemFontOfSize((nfloat)fontSize),
                };
                CGSize sz = str.GetSizeUsingAttributes(attrs);
                return (float)sz.Width;
            }
        }
    }
}
