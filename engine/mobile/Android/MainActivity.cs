using System;
using System.Collections.Generic;
using System.Diagnostics;
using Android.App;
using Android.Content;
using Android.Content.PM;
using Android.Graphics;
using Android.OS;
using Android.Text;
using Android.Views;
using Android.Widget;
using Astron.Engine.Mobile;

namespace AstronEngine.AndroidApp
{
    [Android.App.Activity(
        Label = "Astron Engine",
        Icon = "@mipmap/ic_launcher",
        MainLauncher = true,
        ConfigurationChanges = ConfigChanges.Orientation | ConfigChanges.ScreenSize | ConfigChanges.KeyboardHidden,
        ScreenOrientation = ScreenOrientation.SensorLandscape)]
    public sealed class MainActivity : Android.App.Activity, IMobileHostBridge
    {
        private AstronEngineSurfaceView _view;

        public string PersistentDataPath =>
            System.Environment.GetFolderPath(System.Environment.SpecialFolder.Personal);

        public string PlatformName => "Android (.apk)";

        protected override void OnCreate(Bundle savedInstanceState)
        {
            base.OnCreate(savedInstanceState);
            RequestWindowFeature(WindowFeatures.NoTitle);
            Window?.AddFlags(WindowManagerFlags.Fullscreen | WindowManagerFlags.KeepScreenOn);

            _view = new AstronEngineSurfaceView(this, this);
            SetContentView(_view);
        }

        public void RequestRedraw()
        {
            RunOnUiThread(() => _view?.PostInvalidateOnAnimation());
        }

        public void PromptText(string title, string subtitle, string currentValue, bool password, Action<string> onAccepted)
        {
            RunOnUiThread(() =>
            {
                if (IsFinishing || IsDestroyed) return;

                bool multiline = !password && currentValue != null && currentValue.IndexOf('\n') >= 0;

                var container = new LinearLayout(this)
                {
                    Orientation = Orientation.Vertical,
                };
                int pad = (int)(16f * (Resources?.DisplayMetrics?.Density ?? 2f));
                container.SetPadding(pad, pad / 2, pad, pad / 2);

                if (!string.IsNullOrEmpty(subtitle))
                {
                    var subView = new TextView(this)
                    {
                        Text = subtitle,
                        TextSize = 12.5f,
                    };
                    subView.SetPadding(0, 0, 0, pad / 2);
                    container.AddView(subView);
                }

                InputTypes inputType = InputTypes.ClassText | InputTypes.TextFlagNoSuggestions;
                if (password)
                    inputType |= InputTypes.TextVariationPassword;
                else if (multiline)
                    inputType |= InputTypes.TextFlagMultiLine;

                var edit = new EditText(this)
                {
                    Text = currentValue ?? string.Empty,
                    InputType = inputType,
                };
                edit.SetSingleLine(!multiline);
                if (multiline)
                {
                    edit.SetMinLines(6);
                    edit.SetMaxLines(14);
                    edit.Gravity = GravityFlags.Top | GravityFlags.Left;
                    edit.Typeface = Typeface.Monospace;
                    edit.TextSize = 12.5f;
                }
                edit.SetSelection(edit.Text != null ? edit.Text.Length : 0);
                container.AddView(edit);

                var builder = new AlertDialog.Builder(this);
                builder.SetTitle(title ?? "Astron Engine");
                builder.SetView(container);
                builder.SetPositiveButton("Confirmar", (_, _) =>
                {
                    onAccepted?.Invoke(edit.Text ?? string.Empty);
                    _view?.PostInvalidateOnAnimation();
                });
                builder.SetNegativeButton("Cancelar", (_, _) => { });
                builder.Show();
            });
        }

        public void OpenUrl(string httpsUrl)
        {
            try
            {
                var uri = Android.Net.Uri.Parse(httpsUrl);
                if (uri != null)
                {
                    var intent = new Intent(Intent.ActionView, uri);
                    intent.AddFlags(ActivityFlags.NewTask);
                    StartActivity(intent);
                }
            }
            catch
            {
                // Ignore missing browser activity on minimal images
            }
        }
    }

    public sealed class AstronEngineSurfaceView : View
    {
        private readonly AstronMobileEngineApp _app;
        private readonly AstronAndroidCanvas _canvasAdapter;
        private long _lastTicks;
        private float _uiScale = 1.75f;
        private bool _pinching;
        private float _lastPinchDist;

        public AstronEngineSurfaceView(Context context, IMobileHostBridge host) : base(context)
        {
            Focusable = true;
            FocusableInTouchMode = true;
            _app = new AstronMobileEngineApp(host);
            _canvasAdapter = new AstronAndroidCanvas(context);
        }

        private float ComputeScale(int pixelWidth, int pixelHeight)
        {
            float density = Resources?.DisplayMetrics?.Density ?? 2.0f;
            float targetHeight = 620f;
            float fitScale = pixelHeight > 0 ? pixelHeight / targetHeight : 1.5f;
            float chosen = Math.Min(density, fitScale);
            return Math.Max(1.0f, chosen);
        }

        public override bool OnTouchEvent(MotionEvent e)
        {
            if (e == null) return false;

            if (e.PointerCount >= 2)
            {
                float dx = e.GetX(0) - e.GetX(1);
                float dy = e.GetY(0) - e.GetY(1);
                float dist = (float)Math.Sqrt(dx * dx + dy * dy);

                if (!_pinching || e.ActionMasked == MotionEventActions.PointerDown)
                {
                    _pinching = true;
                    _lastPinchDist = dist;
                }
                else if (e.ActionMasked == MotionEventActions.Move && _lastPinchDist > 8f && dist > 8f)
                {
                    float ratio = dist / _lastPinchDist;
                    if (Math.Abs(ratio - 1f) > 0.015f)
                    {
                        _app.HandlePinchZoom(ratio);
                        _lastPinchDist = dist;
                    }
                }
                return true;
            }

            if (_pinching)
            {
                if (e.ActionMasked == MotionEventActions.Up || e.ActionMasked == MotionEventActions.Cancel)
                    _pinching = false;
                return true;
            }

            float lx = e.GetX(0) / _uiScale;
            float ly = e.GetY(0) / _uiScale;

            switch (e.ActionMasked)
            {
                case MotionEventActions.Down:
                    _app.HandleTouch(TouchPhase.Began, lx, ly);
                    break;
                case MotionEventActions.Move:
                    _app.HandleTouch(TouchPhase.Moved, lx, ly);
                    break;
                case MotionEventActions.Up:
                    _app.HandleTouch(TouchPhase.Ended, lx, ly);
                    break;
                case MotionEventActions.Cancel:
                    _app.HandleTouch(TouchPhase.Cancelled, lx, ly);
                    break;
            }
            PostInvalidateOnAnimation();
            return true;
        }

        protected override void OnDraw(Canvas canvas)
        {
            long now = Stopwatch.GetTimestamp();
            if (_lastTicks == 0) _lastTicks = now;
            float dt = (float)((now - _lastTicks) / (double)Stopwatch.Frequency);
            _lastTicks = now;

            _uiScale = ComputeScale(Width, Height);
            float logicalW = Width > 0 ? Width / _uiScale : 1100f;
            float logicalH = Height > 0 ? Height / _uiScale : 620f;

            _app.Update(dt);

            canvas.Save();
            canvas.Scale(_uiScale, _uiScale);
            _canvasAdapter.Bind(canvas, logicalW, logicalH);
            _app.Render(_canvasAdapter);
            canvas.Restore();

            if (_app.NeedsContinuousAnimation)
                PostInvalidateOnAnimation();
        }
    }

    internal sealed class AstronAndroidCanvas : IMobileCanvas
    {
        private readonly Context _context;
        private readonly Dictionary<string, Bitmap> _bitmapCache = new Dictionary<string, Bitmap>();
        private readonly Paint _fillPaint = new Paint(PaintFlags.AntiAlias) { Style = Paint.Style.Fill };
        private readonly Paint _strokePaint = new Paint(PaintFlags.AntiAlias) { Style = Paint.Style.Stroke };
        private readonly Paint _textPaint = new Paint(PaintFlags.AntiAlias) { Style = Paint.Style.Fill };
        private readonly Paint _bitmapPaint = new Paint(PaintFlags.AntiAlias | PaintFlags.FilterBitmap);

        private Canvas _canvas;
        public float Width { get; private set; }
        public float Height { get; private set; }

        public AstronAndroidCanvas(Context context)
        {
            _context = context;
        }

        public void Bind(Canvas canvas, float width, float height)
        {
            _canvas = canvas;
            Width = width;
            Height = height;
        }

        private static Color ToColor(UiColor c) => Color.Argb(c.A, c.R, c.G, c.B);

        private static void BuildRoundedRectPath(Path p, float x, float y, float w, float h, float r)
        {
            r = Math.Min(r, Math.Min(w, h) * 0.5f);
            p.Reset();
            p.MoveTo(x + r, y);
            p.LineTo(x + w - r, y);
            p.QuadTo(x + w, y, x + w, y + r);
            p.LineTo(x + w, y + h - r);
            p.QuadTo(x + w, y + h, x + w - r, y + h);
            p.LineTo(x + r, y + h);
            p.QuadTo(x, y + h, x, y + h - r);
            p.LineTo(x, y + r);
            p.QuadTo(x, y, x + r, y);
            p.Close();
        }

        public void Clear(UiColor color)
        {
            _canvas?.DrawColor(ToColor(color));
        }

        public void PushClip(UiRect rect)
        {
            if (_canvas == null) return;
            _canvas.Save();
            _canvas.ClipRect(new RectF(rect.X, rect.Y, rect.Right, rect.Bottom));
        }

        public void PopClip()
        {
            _canvas?.Restore();
        }

        public void FillRect(UiRect rect, UiColor color, float cornerRadius = 0f)
        {
            if (_canvas == null || rect.W <= 0f || rect.H <= 0f) return;
            _fillPaint.Color = ToColor(color);
            var rf = new RectF(rect.X, rect.Y, rect.Right, rect.Bottom);
            if (cornerRadius > 0.5f)
                _canvas.DrawRoundRect(rf, cornerRadius, cornerRadius, _fillPaint);
            else
                _canvas.DrawRect(rf, _fillPaint);
        }

        public void StrokeRect(UiRect rect, UiColor color, float strokeWidth = 1f, float cornerRadius = 0f)
        {
            if (_canvas == null || rect.W <= 0f || rect.H <= 0f) return;
            _strokePaint.Color = ToColor(color);
            _strokePaint.StrokeWidth = strokeWidth;
            var rf = new RectF(rect.X, rect.Y, rect.Right, rect.Bottom);
            if (cornerRadius > 0.5f)
                _canvas.DrawRoundRect(rf, cornerRadius, cornerRadius, _strokePaint);
            else
                _canvas.DrawRect(rf, _strokePaint);
        }

        public void DrawLine(float x1, float y1, float x2, float y2, UiColor color, float strokeWidth = 1.5f)
        {
            if (_canvas == null) return;
            _strokePaint.Color = ToColor(color);
            _strokePaint.StrokeWidth = strokeWidth;
            _canvas.DrawLine(x1, y1, x2, y2, _strokePaint);
        }

        public void FillCircle(float cx, float cy, float radius, UiColor color)
        {
            if (_canvas == null || radius <= 0f) return;
            _fillPaint.Color = ToColor(color);
            _canvas.DrawCircle(cx, cy, radius, _fillPaint);
        }

        public void StrokeCircle(float cx, float cy, float radius, UiColor color, float strokeWidth = 1.5f)
        {
            if (_canvas == null || radius <= 0f) return;
            _strokePaint.Color = ToColor(color);
            _strokePaint.StrokeWidth = strokeWidth;
            _canvas.DrawCircle(cx, cy, radius, _strokePaint);
        }

        public void FillPolygon(float[] xyPairs, int vertexCount, UiColor fill, UiColor stroke, float strokeWidth = 1f)
        {
            if (_canvas == null || vertexCount < 3 || xyPairs == null || xyPairs.Length < vertexCount * 2) return;
            using (var path = new Path())
            {
                path.MoveTo(xyPairs[0], xyPairs[1]);
                for (int i = 1; i < vertexCount; i++)
                    path.LineTo(xyPairs[i * 2], xyPairs[i * 2 + 1]);
                path.Close();

                if (fill.A > 0)
                {
                    _fillPaint.Color = ToColor(fill);
                    _canvas.DrawPath(path, _fillPaint);
                }

                if (stroke.A > 0 && strokeWidth > 0f)
                {
                    _strokePaint.Color = ToColor(stroke);
                    _strokePaint.StrokeWidth = strokeWidth;
                    _canvas.DrawPath(path, _strokePaint);
                }
            }
        }

        private Bitmap GetBitmap(string assetName)
        {
            if (string.IsNullOrEmpty(assetName)) return null;
            if (_bitmapCache.TryGetValue(assetName, out var cached))
                return cached;

            Bitmap bmp = null;
            try
            {
                using (var stream = _context.Assets?.Open(assetName))
                {
                    if (stream != null)
                        bmp = BitmapFactory.DecodeStream(stream);
                }
            }
            catch
            {
                bmp = null;
            }

            _bitmapCache[assetName] = bmp;
            return bmp;
        }

        public bool DrawImage(string assetName, UiRect destRect, byte alpha = 255, float cornerRadius = 0f)
        {
            if (_canvas == null || destRect.W <= 0f || destRect.H <= 0f) return false;
            var bmp = GetBitmap(assetName);
            if (bmp == null) return false;

            _bitmapPaint.Alpha = alpha;
            var dst = new RectF(destRect.X, destRect.Y, destRect.Right, destRect.Bottom);

            if (cornerRadius > 0.5f)
            {
                _canvas.Save();
                using (var clipPath = new Path())
                {
                    BuildRoundedRectPath(clipPath, destRect.X, destRect.Y, destRect.W, destRect.H, cornerRadius);
                    _canvas.ClipPath(clipPath);
                    _canvas.DrawBitmap(bmp, null, dst, _bitmapPaint);
                }
                _canvas.Restore();
            }
            else
            {
                _canvas.DrawBitmap(bmp, null, dst, _bitmapPaint);
            }
            return true;
        }

        public void DrawText(string text, float x, float y, float fontSize, UiColor color, bool bold = false, bool centered = false)
        {
            if (_canvas == null || string.IsNullOrEmpty(text)) return;
            _textPaint.Color = ToColor(color);
            _textPaint.TextSize = fontSize;
            _textPaint.FakeBoldText = bold;
            float drawX = centered ? (x - _textPaint.MeasureText(text) * 0.5f) : x;
            float baselineY = y + fontSize * 0.86f;
            _canvas.DrawText(text, drawX, baselineY, _textPaint);
        }

        public float MeasureText(string text, float fontSize, bool bold = false)
        {
            if (string.IsNullOrEmpty(text)) return 0f;
            _textPaint.TextSize = fontSize;
            _textPaint.FakeBoldText = bold;
            return _textPaint.MeasureText(text);
        }
    }
}
