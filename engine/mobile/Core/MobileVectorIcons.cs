using System;

namespace Astron.Engine.Mobile
{
    public enum MobileIcon
    {
        None = 0,
        User,
        Lock,
        Eye,
        EyeOff,
        GitHub,
        Home,
        Folder,
        Cube3D,
        Bag,
        Gear,
        InfoCircle,
        Bolt,
        Clock,
        Rocket,
        ArrowRight,
        Play,
        Pause,
        Stop,
        Plus,
        Close,
        Pin,
        Search,
        DotsH,
        DotsV,
        CursorSelect,
        Move4Way,
        RotateCw,
        ScaleDiag,
        TransformGizmo,
        Camera,
        Lightbulb,
        Mountain,
        Puzzle,
        Monitor,
        MusicNote,
        CodeTags,
        SphereBall,
        FileDoc,
        FileCode,
        Terminal,
        Speaker,
        Keyframe,
        ShaderEdit,
        SparklesAI,
        WarnTriangle,
        ErrorCircle,
        FilterFunnel,
        Trash,
        SendPlane,
        Eyedropper,
        ExpandCorners,
        SnapTarget,
        ShieldLight,
        FocusCube,
        GridIcon
    }

    public enum MobilePlatformKind
    {
        APK,
        EXE,
        IPA,
        Linux,
        ZIP,
        Outros
    }

    public static class MobileVectorIcons
    {
        private static readonly UiColor Transparent = new UiColor(0, 0, 0, 0);

        public static void DrawArcStroke(IMobileCanvas g, float cx, float cy, float rx, float ry,
                                         float startRad, float endRad, int segs, UiColor col, float width)
        {
            float prevX = cx + (float)Math.Cos(startRad) * rx;
            float prevY = cy + (float)Math.Sin(startRad) * ry;
            for (int i = 1; i <= segs; i++)
            {
                float t = startRad + (endRad - startRad) * (i / (float)segs);
                float nx = cx + (float)Math.Cos(t) * rx;
                float ny = cy + (float)Math.Sin(t) * ry;
                g.DrawLine(prevX, prevY, nx, ny, col, width);
                prevX = nx;
                prevY = ny;
            }
        }

        public static void DrawDarkOrbitalSphere(IMobileCanvas g, float cx, float cy, float radius,
                                                 float rimStartRad, float rimEndRad)
        {
            const int rings = 18;
            for (int i = rings; i >= 1; i--)
            {
                float t = i / (float)rings;
                float limb = (float)Math.Pow(t, 2.4);
                byte v = (byte)(5 + limb * 22f);
                byte b = (byte)(7 + limb * 27f);
                g.FillCircle(cx, cy, radius * t, new UiColor(v, (byte)(v + 1), b, 255));
            }
            DrawArcStroke(g, cx, cy, radius + 1.5f, radius + 1.5f, rimStartRad, rimEndRad, 44, new UiColor(190, 198, 218, 34), 4.5f);
            DrawArcStroke(g, cx, cy, radius, radius, rimStartRad, rimEndRad, 44, new UiColor(220, 226, 242, 78), 1.4f);
        }

        public static bool DrawImageCover(IMobileCanvas g, string assetName, float srcAspect,
                                          UiRect dst, byte alpha = 255, float cornerRadius = 0f)
        {
            if (dst.W <= 0f || dst.H <= 0f || srcAspect <= 0.05f) return false;
            float dstAspect = dst.W / dst.H;
            UiRect drawRect;
            if (srcAspect > dstAspect)
            {
                float w = dst.H * srcAspect;
                drawRect = new UiRect(dst.X - (w - dst.W) * 0.5f, dst.Y, w, dst.H);
            }
            else
            {
                float h = dst.W / srcAspect;
                drawRect = new UiRect(dst.X, dst.Y - (h - dst.H) * 0.5f, dst.W, h);
            }

            g.PushClip(dst);
            bool ok = g.DrawImage(assetName, drawRect, alpha, cornerRadius);
            g.PopClip();
            return ok;
        }

        public static void DrawDashedRoundedRect(IMobileCanvas g, UiRect r, float radius, UiColor col, float width = 1.25f)
        {
            float rad = Math.Min(radius, Math.Min(r.W, r.H) * 0.25f);
            DrawDashedSegment(g, r.X + rad, r.Y, r.Right - rad, r.Y, col, width);
            DrawDashedSegment(g, r.Right, r.Y + rad, r.Right, r.Bottom - rad, col, width);
            DrawDashedSegment(g, r.Right - rad, r.Bottom, r.X + rad, r.Bottom, col, width);
            DrawDashedSegment(g, r.X, r.Bottom - rad, r.X, r.Y + rad, col, width);

            DrawArcStroke(g, r.X + rad, r.Y + rad, rad, rad, (float)Math.PI, (float)(Math.PI * 1.5), 6, col, width);
            DrawArcStroke(g, r.Right - rad, r.Y + rad, rad, rad, (float)(Math.PI * 1.5), (float)(Math.PI * 2.0), 6, col, width);
            DrawArcStroke(g, r.Right - rad, r.Bottom - rad, rad, rad, 0f, (float)(Math.PI * 0.5), 6, col, width);
            DrawArcStroke(g, r.X + rad, r.Bottom - rad, rad, rad, (float)(Math.PI * 0.5), (float)Math.PI, 6, col, width);
        }

        private static void DrawDashedSegment(IMobileCanvas g, float x0, float y0, float x1, float y1, UiColor col, float width)
        {
            const float dash = 7f;
            const float gap = 5f;
            float dx = x1 - x0, dy = y1 - y0;
            float len = (float)Math.Sqrt(dx * dx + dy * dy);
            if (len < 1f) return;
            float ux = dx / len, uy = dy / len;
            for (float t = 0f; t < len; t += dash + gap)
            {
                float te = Math.Min(len, t + dash);
                g.DrawLine(x0 + ux * t, y0 + uy * t, x0 + ux * te, y0 + uy * te, col, width);
            }
        }

        private static void FillTri(IMobileCanvas g, float x0, float y0, float x1, float y1, float x2, float y2, UiColor col)
        {
            g.FillPolygon(new[] { x0, y0, x1, y1, x2, y2 }, 3, col, Transparent, 0f);
        }

        private static void FillQuad(IMobileCanvas g, float x0, float y0, float x1, float y1,
                                     float x2, float y2, float x3, float y3, UiColor col)
        {
            g.FillPolygon(new[] { x0, y0, x1, y1, x2, y2, x3, y3 }, 4, col, Transparent, 0f);
        }

        public static void DrawIcon(IMobileCanvas g, float cx, float cy, float size, MobileIcon icon, UiColor col)
        {
            if (icon == MobileIcon.None) return;
            float r = size * 0.5f;
            float sw = Math.Max(1.3f, size * 0.095f);
            UiColor cut = new UiColor(15, 16, 21, 255);

            switch (icon)
            {
                case MobileIcon.User:
                    g.StrokeCircle(cx, cy - r * 0.34f, r * 0.32f, col, sw);
                    DrawArcStroke(g, cx, cy + r * 0.78f, r * 0.68f, r * 0.68f,
                                  (float)(Math.PI * 1.16), (float)(Math.PI * 1.84), 12, col, sw);
                    break;

                case MobileIcon.Lock:
                    DrawArcStroke(g, cx, cy - r * 0.18f, r * 0.34f, r * 0.34f,
                                  (float)Math.PI, (float)(Math.PI * 2.0), 10, col, sw);
                    g.StrokeRect(new UiRect(cx - r * 0.52f, cy - r * 0.16f, r * 1.04f, r * 0.84f), col, sw, 2.5f);
                    g.FillCircle(cx, cy + r * 0.25f, sw * 0.9f, col);
                    break;

                case MobileIcon.Eye:
                case MobileIcon.EyeOff:
                    DrawArcStroke(g, cx, cy + r * 0.52f, r * 0.88f, r * 0.88f,
                                  (float)(Math.PI * 1.22), (float)(Math.PI * 1.78), 10, col, sw);
                    DrawArcStroke(g, cx, cy - r * 0.52f, r * 0.88f, r * 0.88f,
                                  (float)(Math.PI * 0.22), (float)(Math.PI * 0.78), 10, col, sw);
                    g.StrokeCircle(cx, cy, r * 0.24f, col, sw);
                    if (icon == MobileIcon.EyeOff)
                        g.DrawLine(cx - r * 0.68f, cy + r * 0.62f, cx + r * 0.68f, cy - r * 0.62f, col, sw);
                    break;

                case MobileIcon.GitHub:
                    g.FillCircle(cx, cy, r * 0.82f, col);
                    g.FillCircle(cx, cy - r * 0.05f, r * 0.44f, cut);
                    FillTri(g, cx - r * 0.42f, cy - r * 0.18f, cx - r * 0.28f, cy - r * 0.56f, cx - r * 0.08f, cy - r * 0.34f, cut);
                    FillTri(g, cx + r * 0.42f, cy - r * 0.18f, cx + r * 0.28f, cy - r * 0.56f, cx + r * 0.08f, cy - r * 0.34f, cut);
                    g.FillRect(new UiRect(cx - r * 0.18f, cy + r * 0.28f, r * 0.36f, r * 0.56f), cut, 2f);
                    break;

                case MobileIcon.Home:
                    FillTri(g, cx, cy - r * 0.72f, cx + r * 0.74f, cy - r * 0.04f, cx - r * 0.74f, cy - r * 0.04f, col);
                    g.FillRect(new UiRect(cx - r * 0.52f, cy - r * 0.06f, r * 1.04f, r * 0.74f), col, 1.5f);
                    g.FillRect(new UiRect(cx - r * 0.16f, cy + r * 0.20f, r * 0.32f, r * 0.50f), cut, 1f);
                    break;

                case MobileIcon.Folder:
                    g.FillRect(new UiRect(cx - r * 0.76f, cy - r * 0.56f, r * 0.70f, r * 0.32f), col.ScaleRgb(0.86f), 2f);
                    g.FillRect(new UiRect(cx - r * 0.76f, cy - r * 0.34f, r * 1.52f, r * 0.96f), col, 2.5f);
                    break;

                case MobileIcon.Cube3D:
                {
                    float sx = r * 0.72f, sy = r * 0.40f, h = r * 0.76f;
                    float t0x = cx, t0y = cy - h;
                    float tRx = cx + sx, tRy = cy - h + sy;
                    float tBx = cx, tBy = cy - h + sy * 2f;
                    float tLx = cx - sx, tLy = cy - h + sy;
                    float bh = h * 0.96f;
                    FillQuad(g, t0x, t0y, tRx, tRy, tBx, tBy, tLx, tLy, col.ScaleRgb(1.08f));
                    FillQuad(g, tLx, tLy, tBx, tBy, tBx, tBy + bh, tLx, tLy + bh, col.ScaleRgb(0.72f));
                    FillQuad(g, tBx, tBy, tRx, tRy, tRx, tRy + bh, tBx, tBy + bh, col.ScaleRgb(0.88f));
                    break;
                }

                case MobileIcon.Bag:
                    g.StrokeRect(new UiRect(cx - r * 0.58f, cy - r * 0.22f, r * 1.16f, r * 0.92f), col, sw, 2.5f);
                    DrawArcStroke(g, cx, cy - r * 0.22f, r * 0.30f, r * 0.30f, (float)Math.PI, (float)(Math.PI * 2.0), 10, col, sw);
                    break;

                case MobileIcon.Gear:
                    for (int i = 0; i < 8; i++)
                    {
                        float a = i * (float)(Math.PI * 0.25);
                        g.DrawLine(cx + (float)Math.Cos(a) * r * 0.46f, cy + (float)Math.Sin(a) * r * 0.46f,
                                   cx + (float)Math.Cos(a) * r * 0.80f, cy + (float)Math.Sin(a) * r * 0.80f, col, sw * 1.45f);
                    }
                    g.FillCircle(cx, cy, r * 0.58f, col);
                    g.FillCircle(cx, cy, r * 0.26f, cut);
                    break;

                case MobileIcon.InfoCircle:
                    g.FillCircle(cx, cy, r * 0.78f, col);
                    g.FillCircle(cx, cy - r * 0.34f, Math.Max(1.2f, r * 0.13f), cut);
                    g.DrawLine(cx, cy - r * 0.06f, cx, cy + r * 0.46f, cut, sw * 1.25f);
                    break;

                case MobileIcon.Bolt:
                    FillTri(g, cx + r * 0.14f, cy - r * 0.80f, cx - r * 0.54f, cy + r * 0.06f, cx - r * 0.04f, cy + r * 0.06f, col);
                    FillTri(g, cx - r * 0.18f, cy + r * 0.80f, cx + r * 0.54f, cy - r * 0.06f, cx + r * 0.04f, cy - r * 0.06f, col);
                    break;

                case MobileIcon.Clock:
                    g.StrokeCircle(cx, cy, r * 0.74f, col, sw);
                    g.DrawLine(cx, cy, cx, cy - r * 0.42f, col, sw);
                    g.DrawLine(cx, cy, cx + r * 0.34f, cy + r * 0.16f, col, sw);
                    break;

                case MobileIcon.Rocket:
                    FillTri(g, cx + r * 0.74f, cy - r * 0.74f, cx - r * 0.10f, cy - r * 0.42f, cx + r * 0.42f, cy + r * 0.10f, col);
                    FillQuad(g, cx - r * 0.10f, cy - r * 0.42f, cx + r * 0.42f, cy + r * 0.10f, cx - r * 0.14f, cy + r * 0.54f, cx - r * 0.54f, cy + r * 0.14f, col);
                    FillTri(g, cx - r * 0.42f, cy + r * 0.24f, cx - r * 0.74f, cy + r * 0.74f, cx - r * 0.24f, cy + r * 0.42f, col.WithAlpha(180));
                    break;

                case MobileIcon.ArrowRight:
                    g.DrawLine(cx - r * 0.64f, cy, cx + r * 0.58f, cy, col, sw);
                    g.DrawLine(cx + r * 0.14f, cy - r * 0.44f, cx + r * 0.62f, cy, col, sw);
                    g.DrawLine(cx + r * 0.14f, cy + r * 0.44f, cx + r * 0.62f, cy, col, sw);
                    break;

                case MobileIcon.Play:
                    FillTri(g, cx - r * 0.46f, cy - r * 0.66f, cx + r * 0.66f, cy, cx - r * 0.46f, cy + r * 0.66f, col);
                    break;

                case MobileIcon.Pause:
                    g.FillRect(new UiRect(cx - r * 0.50f, cy - r * 0.62f, r * 0.36f, r * 1.24f), col, 1.5f);
                    g.FillRect(new UiRect(cx + r * 0.14f, cy - r * 0.62f, r * 0.36f, r * 1.24f), col, 1.5f);
                    break;

                case MobileIcon.Stop:
                    g.FillRect(new UiRect(cx - r * 0.56f, cy - r * 0.56f, r * 1.12f, r * 1.12f), col, 2.5f);
                    break;

                case MobileIcon.Plus:
                    g.DrawLine(cx - r * 0.62f, cy, cx + r * 0.62f, cy, col, sw * 1.25f);
                    g.DrawLine(cx, cy - r * 0.62f, cx, cy + r * 0.62f, col, sw * 1.25f);
                    break;

                case MobileIcon.Close:
                    g.DrawLine(cx - r * 0.52f, cy - r * 0.52f, cx + r * 0.52f, cy + r * 0.52f, col, sw);
                    g.DrawLine(cx + r * 0.52f, cy - r * 0.52f, cx - r * 0.52f, cy + r * 0.52f, col, sw);
                    break;

                case MobileIcon.Pin:
                    g.DrawLine(cx + r * 0.36f, cy - r * 0.54f, cx - r * 0.16f, cy - r * 0.02f, col, sw * 1.5f);
                    g.DrawLine(cx - r * 0.42f, cy - r * 0.24f, cx + r * 0.24f, cy + r * 0.42f, col, sw);
                    g.DrawLine(cx - r * 0.08f, cy + r * 0.08f, cx - r * 0.54f, cy + r * 0.54f, col, sw);
                    break;

                case MobileIcon.Search:
                    g.StrokeCircle(cx - r * 0.12f, cy - r * 0.12f, r * 0.48f, col, sw);
                    g.DrawLine(cx + r * 0.22f, cy + r * 0.22f, cx + r * 0.66f, cy + r * 0.66f, col, sw * 1.2f);
                    break;

                case MobileIcon.DotsH:
                {
                    float d = Math.Max(1.4f, r * 0.20f);
                    g.FillCircle(cx - r * 0.54f, cy, d, col);
                    g.FillCircle(cx, cy, d, col);
                    g.FillCircle(cx + r * 0.54f, cy, d, col);
                    break;
                }

                case MobileIcon.DotsV:
                {
                    float d = Math.Max(1.3f, r * 0.18f);
                    g.FillCircle(cx, cy - r * 0.52f, d, col);
                    g.FillCircle(cx, cy, d, col);
                    g.FillCircle(cx, cy + r * 0.52f, d, col);
                    break;
                }

                case MobileIcon.CursorSelect:
                    FillTri(g, cx - r * 0.46f, cy - r * 0.74f, cx + r * 0.52f, cy + r * 0.18f, cx - r * 0.06f, cy + r * 0.24f, col);
                    FillTri(g, cx - r * 0.46f, cy - r * 0.74f, cx - r * 0.06f, cy + r * 0.24f, cx - r * 0.46f, cy + r * 0.56f, col);
                    g.DrawLine(cx - r * 0.06f, cy + r * 0.24f, cx + r * 0.20f, cy + r * 0.72f, col, sw * 1.5f);
                    break;

                case MobileIcon.Move4Way:
                case MobileIcon.TransformGizmo:
                {
                    float L = r * 0.76f, ah = r * 0.28f;
                    g.DrawLine(cx - L, cy, cx + L, cy, col, sw);
                    g.DrawLine(cx, cy - L, cx, cy + L, col, sw);
                    FillTri(g, cx, cy - L - 1.5f, cx - ah, cy - L + ah, cx + ah, cy - L + ah, col);
                    FillTri(g, cx, cy + L + 1.5f, cx - ah, cy + L - ah, cx + ah, cy + L - ah, col);
                    FillTri(g, cx - L - 1.5f, cy, cx - L + ah, cy - ah, cx - L + ah, cy + ah, col);
                    FillTri(g, cx + L + 1.5f, cy, cx + L - ah, cy - ah, cx + L - ah, cy + ah, col);
                    if (icon == MobileIcon.TransformGizmo)
                        g.StrokeRect(new UiRect(cx - r * 0.24f, cy - r * 0.24f, r * 0.48f, r * 0.48f), col, sw * 0.85f, 1f);
                    break;
                }

                case MobileIcon.RotateCw:
                    DrawArcStroke(g, cx, cy, r * 0.66f, r * 0.66f, (float)(-Math.PI * 0.25), (float)(Math.PI * 1.35), 16, col, sw * 1.15f);
                    FillTri(g, cx + r * 0.56f, cy - r * 0.42f, cx + r * 0.20f, cy - r * 0.48f, cx + r * 0.52f, cy - r * 0.80f, col);
                    break;

                case MobileIcon.ScaleDiag:
                    g.StrokeRect(new UiRect(cx - r * 0.66f, cy - r * 0.66f, r * 1.32f, r * 1.32f), col.WithAlpha(165), sw, 2.5f);
                    g.DrawLine(cx - r * 0.42f, cy + r * 0.42f, cx + r * 0.42f, cy - r * 0.42f, col, sw * 1.15f);
                    g.DrawLine(cx + r * 0.10f, cy - r * 0.42f, cx + r * 0.42f, cy - r * 0.42f, col, sw);
                    g.DrawLine(cx + r * 0.42f, cy - r * 0.10f, cx + r * 0.42f, cy - r * 0.42f, col, sw);
                    break;

                case MobileIcon.Camera:
                    g.FillRect(new UiRect(cx - r * 0.74f, cy - r * 0.38f, r * 1.48f, r * 0.98f), col, 3f);
                    g.FillRect(new UiRect(cx - r * 0.30f, cy - r * 0.62f, r * 0.60f, r * 0.28f), col, 2f);
                    g.FillCircle(cx, cy + r * 0.11f, r * 0.32f, cut);
                    g.StrokeCircle(cx, cy + r * 0.11f, r * 0.20f, col, 1.2f);
                    break;

                case MobileIcon.Lightbulb:
                    g.FillCircle(cx, cy - r * 0.16f, r * 0.48f, col);
                    g.FillRect(new UiRect(cx - r * 0.24f, cy + r * 0.24f, r * 0.48f, r * 0.38f), col.ScaleRgb(0.8f), 1.5f);
                    g.DrawLine(cx, cy - r * 0.84f, cx, cy - r * 0.70f, col, sw);
                    g.DrawLine(cx - r * 0.76f, cy - r * 0.18f, cx - r * 0.62f, cy - r * 0.18f, col, sw);
                    g.DrawLine(cx + r * 0.76f, cy - r * 0.18f, cx + r * 0.62f, cy - r * 0.18f, col, sw);
                    break;

                case MobileIcon.Mountain:
                    g.FillPolygon(new[] { cx - r * 0.16f, cy - r * 0.64f, cx + r * 0.56f, cy + r * 0.58f, cx - r * 0.78f, cy + r * 0.58f }, 3, Transparent, col, sw * 1.15f);
                    g.FillPolygon(new[] { cx + r * 0.34f, cy - r * 0.22f, cx + r * 0.80f, cy + r * 0.58f, cx - r * 0.08f, cy + r * 0.58f }, 3, Transparent, col.WithAlpha(210), sw);
                    break;

                case MobileIcon.Puzzle:
                    g.FillRect(new UiRect(cx - r * 0.54f, cy - r * 0.48f, r * 1.08f, r * 1.08f), col, 2.5f);
                    g.FillCircle(cx, cy - r * 0.58f, r * 0.22f, col);
                    g.FillCircle(cx + r * 0.62f, cy + r * 0.06f, r * 0.22f, col);
                    break;

                case MobileIcon.Monitor:
                    g.StrokeRect(new UiRect(cx - r * 0.74f, cy - r * 0.58f, r * 1.48f, r * 0.92f), col, sw, 2.5f);
                    g.DrawLine(cx, cy + r * 0.34f, cx, cy + r * 0.66f, col, sw);
                    g.DrawLine(cx - r * 0.40f, cy + r * 0.66f, cx + r * 0.40f, cy + r * 0.66f, col, sw);
                    break;

                case MobileIcon.MusicNote:
                    g.FillCircle(cx - r * 0.34f, cy + r * 0.44f, r * 0.24f, col);
                    g.FillCircle(cx + r * 0.32f, cy + r * 0.30f, r * 0.24f, col);
                    g.DrawLine(cx - r * 0.12f, cy + r * 0.42f, cx - r * 0.12f, cy - r * 0.48f, col, sw);
                    g.DrawLine(cx + r * 0.54f, cy + r * 0.28f, cx + r * 0.54f, cy - r * 0.62f, col, sw);
                    g.DrawLine(cx - r * 0.12f, cy - r * 0.48f, cx + r * 0.54f, cy - r * 0.62f, col, sw * 1.4f);
                    break;

                case MobileIcon.CodeTags:
                    g.DrawLine(cx - r * 0.34f, cy - r * 0.40f, cx - r * 0.76f, cy, col, sw);
                    g.DrawLine(cx - r * 0.76f, cy, cx - r * 0.34f, cy + r * 0.40f, col, sw);
                    g.DrawLine(cx + r * 0.34f, cy - r * 0.40f, cx + r * 0.76f, cy, col, sw);
                    g.DrawLine(cx + r * 0.76f, cy, cx + r * 0.34f, cy + r * 0.40f, col, sw);
                    g.DrawLine(cx + r * 0.14f, cy - r * 0.54f, cx - r * 0.14f, cy + r * 0.54f, col, sw);
                    break;

                case MobileIcon.SphereBall:
                    g.FillCircle(cx, cy, r * 0.70f, col.ScaleRgb(0.85f));
                    g.FillCircle(cx - r * 0.20f, cy - r * 0.22f, r * 0.34f, new UiColor(225, 240, 255, 190));
                    break;

                case MobileIcon.FileDoc:
                case MobileIcon.FileCode:
                    g.FillRect(new UiRect(cx - r * 0.56f, cy - r * 0.72f, r * 1.12f, r * 1.44f), col, 2f);
                    g.DrawLine(cx - r * 0.32f, cy - r * 0.22f, cx + r * 0.32f, cy - r * 0.22f, cut, 1.3f);
                    g.DrawLine(cx - r * 0.32f, cy + r * 0.08f, cx + r * 0.32f, cy + r * 0.08f, cut, 1.3f);
                    if (icon == MobileIcon.FileCode)
                        g.DrawLine(cx - r * 0.32f, cy + r * 0.36f, cx + r * 0.12f, cy + r * 0.36f, cut, 1.3f);
                    break;

                case MobileIcon.Terminal:
                    g.StrokeRect(new UiRect(cx - r * 0.74f, cy - r * 0.58f, r * 1.48f, r * 1.16f), col, sw, 2.5f);
                    g.DrawLine(cx - r * 0.42f, cy - r * 0.22f, cx - r * 0.10f, cy + r * 0.04f, col, sw);
                    g.DrawLine(cx - r * 0.42f, cy + r * 0.30f, cx - r * 0.10f, cy + r * 0.04f, col, sw);
                    g.DrawLine(cx + r * 0.04f, cy + r * 0.30f, cx + r * 0.42f, cy + r * 0.30f, col, sw);
                    break;

                case MobileIcon.Speaker:
                    g.FillRect(new UiRect(cx - r * 0.68f, cy - r * 0.26f, r * 0.36f, r * 0.52f), col, 1f);
                    FillQuad(g, cx - r * 0.36f, cy - r * 0.26f, cx + r * 0.12f, cy - r * 0.64f, cx + r * 0.12f, cy + r * 0.64f, cx - r * 0.36f, cy + r * 0.26f, col);
                    DrawArcStroke(g, cx + r * 0.14f, cy, r * 0.44f, r * 0.44f, -0.7f, 0.7f, 8, col, sw);
                    break;

                case MobileIcon.Keyframe:
                    g.FillPolygon(new[] { cx + r * 0.10f, cy - r * 0.56f, cx + r * 0.66f, cy, cx + r * 0.10f, cy + r * 0.56f, cx - r * 0.46f, cy }, 4, Transparent, col, sw);
                    g.DrawLine(cx - r * 0.76f, cy - r * 0.32f, cx - r * 0.42f, cy - r * 0.32f, col, sw);
                    g.DrawLine(cx - r * 0.76f, cy + r * 0.32f, cx - r * 0.42f, cy + r * 0.32f, col, sw);
                    break;

                case MobileIcon.ShaderEdit:
                    g.StrokeRect(new UiRect(cx - r * 0.68f, cy - r * 0.62f, r * 1.36f, r * 1.24f), col, sw, 2.5f);
                    g.DrawLine(cx - r * 0.30f, cy + r * 0.30f, cx + r * 0.34f, cy - r * 0.34f, col, sw * 1.2f);
                    break;

                case MobileIcon.SparklesAI:
                    FillQuad(g, cx, cy - r * 0.78f, cx + r * 0.22f, cy - r * 0.22f, cx, cy + r * 0.78f, cx - r * 0.22f, cy - r * 0.22f, col);
                    FillQuad(g, cx - r * 0.78f, cy, cx - r * 0.22f, cy - r * 0.22f, cx + r * 0.78f, cy, cx - r * 0.22f, cy + r * 0.22f, col);
                    break;

                case MobileIcon.WarnTriangle:
                    FillTri(g, cx, cy - r * 0.74f, cx + r * 0.78f, cy + r * 0.68f, cx - r * 0.78f, cy + r * 0.68f, col);
                    g.DrawLine(cx, cy - r * 0.22f, cx, cy + r * 0.22f, cut, sw * 1.25f);
                    g.FillCircle(cx, cy + r * 0.46f, Math.Max(1.2f, r * 0.12f), cut);
                    break;

                case MobileIcon.ErrorCircle:
                    g.FillCircle(cx, cy, r * 0.76f, col);
                    g.DrawLine(cx - r * 0.30f, cy - r * 0.30f, cx + r * 0.30f, cy + r * 0.30f, new UiColor(255, 255, 255, 255), sw);
                    g.DrawLine(cx + r * 0.30f, cy - r * 0.30f, cx - r * 0.30f, cy + r * 0.30f, new UiColor(255, 255, 255, 255), sw);
                    break;

                case MobileIcon.FilterFunnel:
                    g.FillPolygon(new[] { cx - r * 0.68f, cy - r * 0.56f, cx + r * 0.68f, cy - r * 0.56f, cx, cy + r * 0.16f }, 3, Transparent, col, sw);
                    g.DrawLine(cx, cy + r * 0.12f, cx, cy + r * 0.66f, col, sw * 1.3f);
                    break;

                case MobileIcon.Trash:
                    g.DrawLine(cx - r * 0.62f, cy - r * 0.46f, cx + r * 0.62f, cy - r * 0.46f, col, sw);
                    g.StrokeRect(new UiRect(cx - r * 0.46f, cy - r * 0.42f, r * 0.92f, r * 1.08f), col, sw, 2f);
                    break;

                case MobileIcon.SendPlane:
                    g.FillPolygon(new[] { cx - r * 0.66f, cy - r * 0.60f, cx + r * 0.72f, cy, cx - r * 0.26f, cy }, 3, Transparent, col, sw);
                    g.FillPolygon(new[] { cx - r * 0.26f, cy, cx + r * 0.72f, cy, cx - r * 0.66f, cy + r * 0.60f }, 3, Transparent, col, sw);
                    break;

                case MobileIcon.Eyedropper:
                    g.DrawLine(cx - r * 0.52f, cy + r * 0.52f, cx + r * 0.22f, cy - r * 0.22f, col, sw * 1.4f);
                    g.FillCircle(cx + r * 0.36f, cy - r * 0.36f, r * 0.28f, col);
                    break;

                case MobileIcon.ExpandCorners:
                {
                    float L = r * 0.62f, k = r * 0.34f;
                    g.DrawLine(cx - L, cy - L, cx - L + k, cy - L, col, sw);
                    g.DrawLine(cx - L, cy - L, cx - L, cy - L + k, col, sw);
                    g.DrawLine(cx + L, cy - L, cx + L - k, cy - L, col, sw);
                    g.DrawLine(cx + L, cy - L, cx + L, cy - L + k, col, sw);
                    g.DrawLine(cx - L, cy + L, cx - L + k, cy + L, col, sw);
                    g.DrawLine(cx - L, cy + L, cx - L, cy + L - k, col, sw);
                    g.DrawLine(cx + L, cy + L, cx + L - k, cy + L, col, sw);
                    g.DrawLine(cx + L, cy + L, cx + L, cy + L - k, col, sw);
                    break;
                }

                case MobileIcon.SnapTarget:
                    g.StrokeCircle(cx, cy, r * 0.56f, col, sw);
                    g.DrawLine(cx - r * 0.80f, cy, cx - r * 0.32f, cy, col, sw);
                    g.DrawLine(cx + r * 0.32f, cy, cx + r * 0.80f, cy, col, sw);
                    g.DrawLine(cx, cy - r * 0.80f, cx, cy - r * 0.32f, col, sw);
                    g.DrawLine(cx, cy + r * 0.32f, cx, cy + r * 0.80f, col, sw);
                    break;

                case MobileIcon.ShieldLight:
                    g.StrokeCircle(cx, cy, r * 0.62f, col, sw);
                    g.DrawLine(cx, cy - r * 0.62f, cx, cy + r * 0.62f, col, sw);
                    break;

                case MobileIcon.FocusCube:
                    DrawIcon(g, cx, cy, size, MobileIcon.ExpandCorners, col.WithAlpha(200));
                    DrawIcon(g, cx, cy, size * 0.58f, MobileIcon.Cube3D, col);
                    break;

                case MobileIcon.GridIcon:
                {
                    float L = r * 0.64f;
                    g.StrokeRect(new UiRect(cx - L, cy - L, L * 2f, L * 2f), col, sw, 2f);
                    g.DrawLine(cx, cy - L, cx, cy + L, col, sw * 0.9f);
                    g.DrawLine(cx - L, cy, cx + L, cy, col, sw * 0.9f);
                    break;
                }
            }
        }

        public static void DrawPlatformIcon(IMobileCanvas g, float cx, float cy, float size, MobilePlatformKind kind)
        {
            float s = size * 0.5f;
            UiColor w = new UiColor(255, 255, 255, 255);
            UiColor dark = new UiColor(14, 15, 19, 255);

            switch (kind)
            {
                case MobilePlatformKind.APK:
                    g.FillCircle(cx, cy - s * 0.16f, s * 0.44f, w);
                    g.FillRect(new UiRect(cx - s * 0.50f, cy - s * 0.16f, s, s * 0.18f), dark);
                    g.FillRect(new UiRect(cx - s * 0.44f, cy - s * 0.12f, s * 0.88f, s * 0.60f), w, 4f);
                    g.FillRect(new UiRect(cx - s * 0.64f, cy - s * 0.10f, s * 0.15f, s * 0.46f), w, 3.5f);
                    g.FillRect(new UiRect(cx + s * 0.49f, cy - s * 0.10f, s * 0.15f, s * 0.46f), w, 3.5f);
                    g.FillRect(new UiRect(cx - s * 0.27f, cy + s * 0.44f, s * 0.17f, s * 0.30f), w, 3.5f);
                    g.FillRect(new UiRect(cx + s * 0.10f, cy + s * 0.44f, s * 0.17f, s * 0.30f), w, 3.5f);
                    g.DrawLine(cx - s * 0.20f, cy - s * 0.46f, cx - s * 0.34f, cy - s * 0.68f, w, 2.0f);
                    g.DrawLine(cx + s * 0.20f, cy - s * 0.46f, cx + s * 0.34f, cy - s * 0.68f, w, 2.0f);
                    g.FillCircle(cx - s * 0.18f, cy - s * 0.30f, 2.1f, dark);
                    g.FillCircle(cx + s * 0.18f, cy - s * 0.30f, 2.1f, dark);
                    break;

                case MobilePlatformKind.EXE:
                {
                    float gp = 2.4f, r = s * 0.62f;
                    FillQuad(g, cx - r, cy - r * 0.82f, cx - gp, cy - r, cx - gp, cy - gp, cx - r, cy - gp, w);
                    FillQuad(g, cx + gp, cy - r, cx + r, cy - r * 1.12f, cx + r, cy - gp, cx + gp, cy - gp, w);
                    FillQuad(g, cx - r, cy + gp, cx - gp, cy + gp, cx - gp, cy + r, cx - r, cy + r * 0.82f, w);
                    FillQuad(g, cx + gp, cy + gp, cx + r, cy + gp, cx + r, cy + r * 1.12f, cx + gp, cy + r, w);
                    break;
                }

                case MobilePlatformKind.IPA:
                    g.FillCircle(cx - s * 0.17f, cy + s * 0.05f, s * 0.38f, w);
                    g.FillCircle(cx + s * 0.17f, cy + s * 0.05f, s * 0.38f, w);
                    g.FillCircle(cx - s * 0.14f, cy + s * 0.28f, s * 0.30f, w);
                    g.FillCircle(cx + s * 0.14f, cy + s * 0.28f, s * 0.30f, w);
                    g.FillCircle(cx + s * 0.50f, cy + s * 0.02f, s * 0.22f, dark);
                    FillQuad(g, cx + s * 0.02f, cy - s * 0.34f, cx + s * 0.24f, cy - s * 0.66f, cx + s * 0.30f, cy - s * 0.44f, cx + s * 0.08f, cy - s * 0.28f, w);
                    break;

                case MobilePlatformKind.Linux:
                {
                    float rad = s * 0.66f;
                    g.StrokeCircle(cx, cy, rad, w, 2.2f);
                    DrawArcStroke(g, cx, cy, rad * 0.48f, rad, 0f, (float)(Math.PI * 2.0), 22, w, 1.9f);
                    g.DrawLine(cx - rad, cy, cx + rad, cy, w, 1.9f);
                    g.DrawLine(cx - rad * 0.84f, cy - rad * 0.46f, cx + rad * 0.84f, cy - rad * 0.46f, w, 1.7f);
                    g.DrawLine(cx - rad * 0.84f, cy + rad * 0.46f, cx + rad * 0.84f, cy + rad * 0.46f, w, 1.7f);
                    break;
                }

                case MobilePlatformKind.ZIP:
                {
                    float rx = s * 0.54f, ry = s * 0.68f;
                    g.FillRect(new UiRect(cx - rx, cy - ry, rx * 2f, ry * 2f), w, 5f);
                    for (int i = 0; i < 4; i++)
                    {
                        float yy = cy - ry + 6f + i * 6.5f;
                        g.FillRect(new UiRect(cx - 4f, yy, 8f, 3.8f), dark);
                    }
                    g.FillRect(new UiRect(cx - 6f, cy + 4f, 12f, 10f), dark, 2.5f);
                    break;
                }

                case MobilePlatformKind.Outros:
                    g.FillCircle(cx, cy, s * 0.66f, w);
                    g.FillCircle(cx - s * 0.30f, cy, 3.1f, dark);
                    g.FillCircle(cx, cy, 3.1f, dark);
                    g.FillCircle(cx + s * 0.30f, cy, 3.1f, dark);
                    break;
            }
        }

        public static void DrawToolboxShape(IMobileCanvas g, float cx, float cy, float size, int shapeIndex)
        {
            float r = size * 0.44f;
            if (shapeIndex == 1) // Sphere
            {
                g.FillCircle(cx, cy, r, new UiColor(154, 156, 166, 255));
                g.FillCircle(cx - r * 0.16f, cy - r * 0.16f, r * 0.74f, new UiColor(204, 206, 214, 255));
                g.FillCircle(cx - r * 0.30f, cy - r * 0.30f, r * 0.36f, new UiColor(246, 247, 252, 255));
                return;
            }
            if (shapeIndex == 2) // Cylinder
            {
                float w = r * 0.78f, h = r * 0.74f;
                g.FillRect(new UiRect(cx - w, cy - h * 0.68f, w * 2f, h * 1.50f), new UiColor(184, 186, 196, 255), 3f);
                g.FillRect(new UiRect(cx - w * 0.46f, cy - h * 0.68f, w * 0.60f, h * 1.50f), new UiColor(222, 224, 232, 255));
                g.FillRect(new UiRect(cx - w, cy - h * 0.86f, w * 2f, h * 0.44f), new UiColor(244, 245, 250, 255), h * 0.22f);
                return;
            }
            if (shapeIndex == 3) // Block (wedge/stepped isometric block)
            {
                float sx = r * 0.96f, dy = r * 0.46f, h = r * 0.72f;
                float bLx = cx - sx, bLy = cy + dy * 0.55f;
                float bBx = cx,      bBy = cy + dy * 1.45f;
                float bRx = cx + sx, bRy = cy + dy * 0.55f;
                float tLx = bLx,     tLy = bLy - h * 0.55f;
                float mBx = bBx,     mBy = bBy - h * 0.55f;
                float tRx = bRx,     tRy = bRy - h * 1.05f;
                float apX = cx,      apY = cy - h * 1.05f - dy * 0.35f;
                FillQuad(g, apX, apY, tRx, tRy, mBx, mBy, tLx, tLy, new UiColor(238, 240, 246, 255));
                FillQuad(g, tLx, tLy, mBx, mBy, bBx, bBy, bLx, bLy, new UiColor(172, 174, 184, 255));
                FillQuad(g, mBx, mBy, tRx, tRy, bRx, bRy, bBx, bBy, new UiColor(204, 206, 216, 255));
                return;
            }

            // Part (Cube)
            float sy = r * 0.80f;
            float csx = r * 0.94f;
            float cdy = r * 0.46f;
            float t0x = cx, t0y = cy - sy - cdy * 0.45f;
            float trx = cx + csx, try0 = cy - sy + cdy * 0.55f;
            float tbx = cx, tby = cy - sy + cdy * 1.55f;
            float tlx = cx - csx, tly = cy - sy + cdy * 0.55f;
            float bh = sy * 1.16f;
            FillQuad(g, t0x, t0y, trx, try0, tbx, tby, tlx, tly, new UiColor(244, 245, 250, 255));
            FillQuad(g, tlx, tly, tbx, tby, tbx, tby + bh, tlx, tly + bh, new UiColor(174, 176, 186, 255));
            FillQuad(g, tbx, tby, trx, try0, trx, try0 + bh, tbx, tby + bh, new UiColor(206, 208, 218, 255));
        }
    }
}
