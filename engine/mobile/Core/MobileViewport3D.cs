using System;
using System.Collections.Generic;
using Astron.Engine;

namespace Astron.Engine.Mobile
{
    /// <summary>
    /// Câmera orbital 3D (Perspectiva &amp; Ortográfica), projeção de malhas 3D iluminadas,
    /// grade infinita com eixos X/Z, raycast por toque e gizmo RGB para Android/iOS.
    /// </summary>
    public sealed class MobileViewport3D
    {
        public float YawDeg { get; set; } = -38f;
        public float PitchDeg { get; set; } = 24f;
        public float Distance { get; set; } = 10.5f;
        public Vec3 Target { get; set; } = new Vec3(0f, 0.5f, 0f);

        public bool Is2DMode { get; set; }
        public bool ShowGrid { get; set; } = true;
        public bool ShowGizmo { get; set; } = true;
        public bool Wireframe { get; set; }

        public void ResetCamera()
        {
            YawDeg = -38f;
            PitchDeg = 24f;
            Distance = 10.5f;
            Target = new Vec3(0f, 0.5f, 0f);
            Is2DMode = false;
        }

        public void Orbit(float deltaYawDeg, float deltaPitchDeg)
        {
            YawDeg += deltaYawDeg;
            PitchDeg += deltaPitchDeg;
            if (PitchDeg < -78f) PitchDeg = -78f;
            if (PitchDeg > 78f) PitchDeg = 78f;
        }

        public void Zoom(float factor)
        {
            Distance *= factor;
            if (Distance < 2.2f) Distance = 2.2f;
            if (Distance > 60f) Distance = 60f;
        }

        private void ComputeBasis(out Vec3 eye, out Vec3 right, out Vec3 up, out Vec3 forward)
        {
            float yaw = YawDeg * ((float)Math.PI / 180f);
            float pitch = PitchDeg * ((float)Math.PI / 180f);

            if (Is2DMode)
            {
                eye = new Vec3(Target.X, Target.Y, Target.Z + Distance);
            }
            else
            {
                float cp = (float)Math.Cos(pitch);
                eye = new Vec3(
                    Target.X + Distance * cp * (float)Math.Sin(yaw),
                    Target.Y + Distance * (float)Math.Sin(pitch),
                    Target.Z + Distance * cp * (float)Math.Cos(yaw)
                );
            }

            forward = (Target - eye).Normalized();
            if (forward.Length < 1e-4f) forward = new Vec3(0f, 0f, -1f);
            right = Vec3.Cross(forward, Vec3.Up).Normalized();
            if (right.Length < 1e-4f) right = new Vec3(1f, 0f, 0f);
            up = Vec3.Cross(right, forward).Normalized();
        }

        private bool ProjectPoint(
            Vec3 worldPt,
            UiRect vp,
            Vec3 eye,
            Vec3 right,
            Vec3 up,
            Vec3 forward,
            out float sx,
            out float sy,
            out float depth)
        {
            Vec3 rel = worldPt - eye;
            float cx = Vec3.Dot(rel, right);
            float cy = Vec3.Dot(rel, up);
            float cz = Vec3.Dot(rel, forward);
            depth = cz;

            if (cz <= 0.12f)
            {
                sx = 0f;
                sy = 0f;
                return false;
            }

            float aspect = Math.Max(0.2f, vp.W / Math.Max(1f, vp.H));
            float nx;
            float ny;
            if (Is2DMode)
            {
                float orthoScale = Distance * 0.45f;
                nx = cx / (orthoScale * aspect);
                ny = cy / orthoScale;
            }
            else
            {
                const float fovScale = 1.85f; // ~56 deg FOV
                nx = (cx * fovScale) / (cz * aspect);
                ny = (cy * fovScale) / cz;
            }

            sx = vp.X + (nx * 0.5f + 0.5f) * vp.W;
            sy = vp.Y + (0.5f - ny * 0.5f) * vp.H;
            return true;
        }

        private struct ProjectedFace
        {
            public float Depth;
            public float[] XY;
            public int Count;
            public UiColor Fill;
            public UiColor Stroke;
            public float StrokeW;
        }

        private static readonly int[][] CubeFaces = new int[][]
        {
            new[] { 0, 1, 2, 3 }, // -Z
            new[] { 4, 7, 6, 5 }, // +Z
            new[] { 0, 4, 5, 1 }, // -Y
            new[] { 3, 2, 6, 7 }, // +Y
            new[] { 0, 3, 7, 4 }, // -X
            new[] { 1, 5, 6, 2 }, // +X
        };

        private static readonly Vec3[] CubeFaceNormals = new Vec3[]
        {
            new Vec3(0f, 0f, -1f),
            new Vec3(0f, 0f,  1f),
            new Vec3(0f, -1f, 0f),
            new Vec3(0f,  1f, 0f),
            new Vec3(-1f, 0f, 0f),
            new Vec3( 1f, 0f, 0f),
        };

        public void Render(IMobileCanvas g, UiRect vp, AstronWorld3D world, int selectedId)
        {
            g.PushClip(vp);

            // 1. Horizonte atmosférico estilo Astron Engine Editor
            int bands = 10;
            float horizonH = vp.H * 0.34f;
            for (int b = 0; b < bands; b++)
            {
                float t = b / (float)bands;
                byte lum = (byte)(36 + t * 92);
                g.FillRect(
                    new UiRect(vp.X, vp.Y + t * horizonH, vp.W, horizonH / bands + 1.5f),
                    new UiColor(lum, (byte)(lum + 2), (byte)(lum + 6), 255)
                );
            }
            for (int b = 0; b < bands; b++)
            {
                float t = b / (float)bands;
                byte lum = (byte)(34 - t * 22);
                float y0 = vp.Y + horizonH + t * (vp.H - horizonH);
                g.FillRect(
                    new UiRect(vp.X, y0, vp.W, (vp.H - horizonH) / bands + 1.5f),
                    new UiColor(lum, (byte)(lum + 1), (byte)(lum + 3), 255)
                );
            }

            ComputeBasis(out Vec3 eye, out Vec3 right, out Vec3 up, out Vec3 forward);

            // 2. Grade 3D em perspectiva + Eixos X (vermelho) / Z (azul)
            if (ShowGrid)
            {
                const int halfExtent = 14;
                UiColor gridMinor = new UiColor(58, 62, 72, 130);
                UiColor axisXCol = new UiColor(218, 48, 56, 225);
                UiColor axisZCol = new UiColor(48, 122, 238, 225);

                for (int i = -halfExtent; i <= halfExtent; i++)
                {
                    // Linha paralela ao eixo X (z = i)
                    DrawClipped3DSegment(
                        g, vp, eye, right, up, forward,
                        new Vec3(-halfExtent, 0f, i),
                        new Vec3(halfExtent, 0f, i),
                        i == 0 ? axisXCol : gridMinor,
                        i == 0 ? 2.0f : 1.0f
                    );

                    // Linha paralela ao eixo Z (x = i)
                    DrawClipped3DSegment(
                        g, vp, eye, right, up, forward,
                        new Vec3(i, 0f, -halfExtent),
                        new Vec3(i, 0f, halfExtent),
                        i == 0 ? axisZCol : gridMinor,
                        i == 0 ? 2.0f : 1.0f
                    );
                }
            }

            // 3. Malhas 3D ordenadas por profundidade (Painter's Z-sort + iluminação direcional)
            Vec3 sunDir = new Vec3(0.48f, 0.78f, 0.40f).Normalized();
            var faceBuffer = new List<ProjectedFace>(world.Nodes.Count * 6);

            for (int n = 0; n < world.Nodes.Count; n++)
            {
                SceneNode3D node = world.Nodes[n];
                if (!node.Visible || node.Primitive == AstronPrimitiveKind.Empty)
                    continue;

                bool isSel = (node.Id == selectedId);
                UiColor baseCol = new UiColor(
                    (byte)((node.ColorRgba >> 24) & 0xFF),
                    (byte)((node.ColorRgba >> 16) & 0xFF),
                    (byte)((node.ColorRgba >> 8) & 0xFF),
                    (byte)Math.Max(30, 255 - (int)(node.Transparency * 220f))
                );

                if (node.Primitive == AstronPrimitiveKind.Sphere)
                {
                    if (ProjectPoint(node.Position, vp, eye, right, up, forward, out float cx, out float cy, out float d))
                    {
                        float radWorld = Math.Max(node.Scale.X, Math.Max(node.Scale.Y, node.Scale.Z)) * 0.5f;
                        float radPx = Is2DMode
                            ? (radWorld / (Distance * 0.45f)) * vp.H * 0.5f
                            : (radWorld * 1.85f / d) * vp.H * 0.5f;
                        if (radPx > 1.5f)
                        {
                            // Aproximação poligonal octogonal + brilho especular
                            float[] poly = new float[24];
                            for (int k = 0; k < 12; k++)
                            {
                                double ang = k * (Math.PI * 2.0 / 12.0);
                                poly[k * 2] = cx + (float)Math.Cos(ang) * radPx;
                                poly[k * 2 + 1] = cy + (float)Math.Sin(ang) * radPx;
                            }
                            faceBuffer.Add(new ProjectedFace
                            {
                                Depth = d,
                                XY = poly,
                                Count = 12,
                                Fill = Wireframe ? new UiColor(0, 0, 0, 0) : baseCol.ScaleRgb(0.92f),
                                Stroke = isSel ? new UiColor(95, 178, 255, 255) : baseCol.ScaleRgb(0.55f),
                                StrokeW = isSel ? 2.2f : 1.0f
                            });
                        }
                    }
                    continue;
                }

                // Cubo / Bloco / Cilindro / Plano (6 faces orientadas com rotação Y)
                float hx = Math.Abs(node.Scale.X) * 0.5f;
                float hy = Math.Abs(node.Scale.Y) * 0.5f;
                float hz = Math.Abs(node.Scale.Z) * 0.5f;
                float rotY = node.RotationEulerDeg.Y * ((float)Math.PI / 180f);
                float cosY = (float)Math.Cos(rotY);
                float sinY = (float)Math.Sin(rotY);

                Vec3[] localVerts = new Vec3[8]
                {
                    new Vec3(-hx, -hy, -hz),
                    new Vec3( hx, -hy, -hz),
                    new Vec3( hx,  hy, -hz),
                    new Vec3(-hx,  hy, -hz),
                    new Vec3(-hx, -hy,  hz),
                    new Vec3( hx, -hy,  hz),
                    new Vec3( hx,  hy,  hz),
                    new Vec3(-hx,  hy,  hz),
                };

                Vec3[] worldVerts = new Vec3[8];
                for (int v = 0; v < 8; v++)
                {
                    Vec3 lv = localVerts[v];
                    float rx = lv.X * cosY + lv.Z * sinY;
                    float rz = -lv.X * sinY + lv.Z * cosY;
                    worldVerts[v] = node.Position + new Vec3(rx, lv.Y, rz);
                }

                for (int f = 0; f < 6; f++)
                {
                    Vec3 ln = CubeFaceNormals[f];
                    Vec3 wn = new Vec3(ln.X * cosY + ln.Z * sinY, ln.Y, -ln.X * sinY + ln.Z * cosY);

                    int[] idx = CubeFaces[f];
                    Vec3 faceCenter = (worldVerts[idx[0]] + worldVerts[idx[2]]) * 0.5f;
                    if (!Wireframe && Vec3.Dot(wn, eye - faceCenter) <= 0f)
                        continue;

                    float[] xy = new float[8];
                    float avgDepth = 0f;
                    bool allOk = true;
                    for (int k = 0; k < 4; k++)
                    {
                        if (!ProjectPoint(worldVerts[idx[k]], vp, eye, right, up, forward,
                            out float px, out float py, out float dz))
                        {
                            allOk = false;
                            break;
                        }
                        xy[k * 2] = px;
                        xy[k * 2 + 1] = py;
                        avgDepth += dz * 0.25f;
                    }
                    if (!allOk) continue;

                    float ndotl = Math.Max(0f, Vec3.Dot(wn, sunDir));
                    float shade = node.Material == AstronMaterialPreset.Neon ? 1.15f : (0.42f + ndotl * 0.62f);

                    faceBuffer.Add(new ProjectedFace
                    {
                        Depth = avgDepth,
                        XY = xy,
                        Count = 4,
                        Fill = Wireframe ? new UiColor(0, 0, 0, 0) : baseCol.ScaleRgb(shade),
                        Stroke = isSel ? new UiColor(195, 224, 255, 255) : new UiColor(30, 34, 42, 120),
                        StrokeW = isSel ? 1.8f : 0.8f
                    });
                }
            }

            faceBuffer.Sort((a, b) => b.Depth.CompareTo(a.Depth));
            for (int i = 0; i < faceBuffer.Count; i++)
            {
                ProjectedFace pf = faceBuffer[i];
                g.FillPolygon(pf.XY, pf.Count, pf.Fill, pf.Stroke, pf.StrokeW);
            }

            // 4. Gizmo RGB 3D sobre o objeto selecionado (igual ao Editor Desktop!)
            if (ShowGizmo)
            {
                SceneNode3D selNode = null;
                for (int i = 0; i < world.Nodes.Count; i++)
                {
                    if (world.Nodes[i].Id == selectedId) { selNode = world.Nodes[i]; break; }
                }
                if (selNode != null &&
                    ProjectPoint(selNode.Position, vp, eye, right, up, forward, out float ox, out float oy, out _))
                {
                    float axisLen = Math.Max(1.4f, Distance * 0.16f);
                    DrawGizmoArrow(g, vp, eye, right, up, forward, selNode.Position,
                        selNode.Position + new Vec3(axisLen, 0f, 0f), ox, oy, new UiColor(242, 54, 62, 255));
                    DrawGizmoArrow(g, vp, eye, right, up, forward, selNode.Position,
                        selNode.Position + new Vec3(0f, axisLen, 0f), ox, oy, new UiColor(64, 232, 68, 255));
                    DrawGizmoArrow(g, vp, eye, right, up, forward, selNode.Position,
                        selNode.Position + new Vec3(0f, 0f, axisLen), ox, oy, new UiColor(46, 134, 255, 255));
                }
            }

            // 5. Gizmo de Orientação RGB X/Y/Z no canto superior direito do Viewport
            DrawOrientationWidget(g, vp.Right - 44f, vp.Y + 42f, 24f, right, up, forward);

            g.PopClip();
        }

        public int PickNode(AstronWorld3D world, UiRect vp, float screenX, float screenY)
        {
            if (!vp.Contains(screenX, screenY)) return -1;
            ComputeBasis(out Vec3 eye, out Vec3 right, out Vec3 up, out Vec3 forward);

            int bestId = -1;
            float bestDistSq = 44f * 44f; // Raio generoso de toque em pixels

            for (int i = 0; i < world.Nodes.Count; i++)
            {
                SceneNode3D n = world.Nodes[i];
                if (!n.Visible || string.Equals(n.Name, "Baseplate", StringComparison.OrdinalIgnoreCase))
                    continue;
                if (ProjectPoint(n.Position, vp, eye, right, up, forward, out float px, out float py, out _))
                {
                    float dx = screenX - px;
                    float dy = screenY - py;
                    float d2 = dx * dx + dy * dy;
                    if (d2 < bestDistSq)
                    {
                        bestDistSq = d2;
                        bestId = n.Id;
                    }
                }
            }
            return bestId;
        }

        private void DrawClipped3DSegment(
            IMobileCanvas g, UiRect vp,
            Vec3 eye, Vec3 right, Vec3 up, Vec3 forward,
            Vec3 a, Vec3 b, UiColor col, float width)
        {
            // Subdivide em segmentos curtos para evitar artefatos atrás do plano near
            const int segs = 6;
            for (int s = 0; s < segs; s++)
            {
                Vec3 p0 = Vec3.Lerp(a, b, s / (float)segs);
                Vec3 p1 = Vec3.Lerp(a, b, (s + 1) / (float)segs);
                if (ProjectPoint(p0, vp, eye, right, up, forward, out float x0, out float y0, out _) &&
                    ProjectPoint(p1, vp, eye, right, up, forward, out float x1, out float y1, out _))
                {
                    g.DrawLine(x0, y0, x1, y1, col, width);
                }
            }
        }

        private void DrawGizmoArrow(
            IMobileCanvas g, UiRect vp,
            Vec3 eye, Vec3 right, Vec3 up, Vec3 forward,
            Vec3 origin, Vec3 tip, float ox, float oy, UiColor col)
        {
            if (!ProjectPoint(tip, vp, eye, right, up, forward, out float tx, out float ty, out _))
                return;
            g.DrawLine(ox, oy, tx, ty, col, 3.0f);
            g.FillCircle(tx, ty, 5.2f, col);
        }

        private static void DrawOrientationWidget(
            IMobileCanvas g, float cx, float cy, float radius,
            Vec3 right, Vec3 up, Vec3 forward)
        {
            // Projeta os eixos X, Y, Z do mundo no espaço da câmera
            Vec3[] worldAxes = { new Vec3(1, 0, 0), new Vec3(0, 1, 0), new Vec3(0, 0, 1) };
            string[] labels = { "X", "Y", "Z" };
            UiColor[] cols =
            {
                new UiColor(238, 62, 68, 255),
                new UiColor(72, 218, 82, 255),
                new UiColor(58, 138, 252, 255)
            };

            for (int i = 0; i < 3; i++)
            {
                float sx = Vec3.Dot(worldAxes[i], right);
                float sy = -Vec3.Dot(worldAxes[i], up);
                float ex = cx + sx * radius;
                float ey = cy + sy * radius;
                g.DrawLine(cx, cy, ex, ey, cols[i], 2.4f);
                g.FillCircle(ex, ey, 7.5f, cols[i]);
                g.DrawText(labels[i], ex, ey - 5f, 9.5f, new UiColor(255, 255, 255, 255), true, true);
            }
        }
    }
}
