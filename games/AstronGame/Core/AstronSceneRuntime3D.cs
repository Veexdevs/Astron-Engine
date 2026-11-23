using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;

namespace Astron.Engine
{
    /// <summary>
    /// Vetor 3D imutável de precisão simples do SDK C# da Astron Engine.
    /// Compatível com Android (.apk), iOS (.ipa), Windows (.exe) e Linux.
    /// </summary>
    public struct Vec3 : IEquatable<Vec3>
    {
        public float X;
        public float Y;
        public float Z;

        public Vec3(float x, float y, float z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        public static Vec3 Zero => new Vec3(0f, 0f, 0f);
        public static Vec3 One  => new Vec3(1f, 1f, 1f);
        public static Vec3 Up   => new Vec3(0f, 1f, 0f);

        public float Length => (float)Math.Sqrt(X * X + Y * Y + Z * Z);

        public Vec3 Normalized()
        {
            float len = Length;
            return len > 1e-6f ? new Vec3(X / len, Y / len, Z / len) : Zero;
        }

        public static float Dot(Vec3 a, Vec3 b) => a.X * b.X + a.Y * b.Y + a.Z * b.Z;

        public static Vec3 Cross(Vec3 a, Vec3 b) => new Vec3(
            a.Y * b.Z - a.Z * b.Y,
            a.Z * b.X - a.X * b.Z,
            a.X * b.Y - a.Y * b.X
        );

        public static Vec3 Lerp(Vec3 a, Vec3 b, float t)
        {
            if (t < 0f) t = 0f;
            if (t > 1f) t = 1f;
            return new Vec3(a.X + (b.X - a.X) * t, a.Y + (b.Y - a.Y) * t, a.Z + (b.Z - a.Z) * t);
        }

        public static Vec3 operator +(Vec3 a, Vec3 b) => new Vec3(a.X + b.X, a.Y + b.Y, a.Z + b.Z);
        public static Vec3 operator -(Vec3 a, Vec3 b) => new Vec3(a.X - b.X, a.Y - b.Y, a.Z - b.Z);
        public static Vec3 operator *(Vec3 a, float s) => new Vec3(a.X * s, a.Y * s, a.Z * s);

        public bool Equals(Vec3 other) =>
            Math.Abs(X - other.X) < 1e-5f &&
            Math.Abs(Y - other.Y) < 1e-5f &&
            Math.Abs(Z - other.Z) < 1e-5f;

        public override bool Equals(object obj) => obj is Vec3 other && Equals(other);

        public override int GetHashCode()
        {
            unchecked
            {
                int h = X.GetHashCode();
                h = (h * 397) ^ Y.GetHashCode();
                h = (h * 397) ^ Z.GetHashCode();
                return h;
            }
        }

        public override string ToString() =>
            string.Format(CultureInfo.InvariantCulture, "({0:F3}, {1:F3}, {2:F3})", X, Y, Z);
    }

    public enum AstronPrimitiveKind
    {
        Cube = 0,
        Sphere = 1,
        Cylinder = 2,
        Wedge = 3,
        Plane = 4,
        Empty = 5
    }

    public enum AstronMaterialPreset
    {
        Plastic = 0,
        SmoothPlastic = 1,
        Metal = 2,
        Neon = 3,
        Grass = 4,
        Concrete = 5,
        Glass = 6
    }

    public struct BoundingBox3D
    {
        public Vec3 Min;
        public Vec3 Max;

        public BoundingBox3D(Vec3 min, Vec3 max)
        {
            Min = min;
            Max = max;
        }

        public bool Intersects(BoundingBox3D other)
        {
            return (Min.X <= other.Max.X && Max.X >= other.Min.X) &&
                   (Min.Y <= other.Max.Y && Max.Y >= other.Min.Y) &&
                   (Min.Z <= other.Max.Z && Max.Z >= other.Min.Z);
        }
    }

    /// <summary>
    /// Entidade 3D com Transformação, Aparência PBR e Corpo Rígido (Física AABB).
    /// Espelha fielmente a estrutura <c>SceneEntity</c> do Editor C++20 da Astron Engine.
    /// </summary>
    public sealed class SceneNode3D
    {
        public int Id { get; set; }
        public string Name { get; set; } = "Part";
        public AstronPrimitiveKind Primitive { get; set; } = AstronPrimitiveKind.Cube;
        public AstronMaterialPreset Material { get; set; } = AstronMaterialPreset.Plastic;

        public Vec3 Position { get; set; } = Vec3.Zero;
        public Vec3 RotationEulerDeg { get; set; } = Vec3.Zero;
        public Vec3 Scale { get; set; } = Vec3.One;
        public Vec3 Velocity { get; set; } = Vec3.Zero;

        public uint ColorRgba { get; set; } = 0xFFFFFFFFu;
        public float Transparency { get; set; } = 0f;
        public bool Visible { get; set; } = true;

        public float Mass { get; set; } = 1.0f;
        public bool UseGravity { get; set; } = false;
        public bool Anchored { get; set; } = true;
        public bool CanCollide { get; set; } = true;
        public string ScriptName { get; set; } = string.Empty;

        public BoundingBox3D ComputeWorldBounds()
        {
            Vec3 half = new Vec3(
                Math.Abs(Scale.X) * 0.5f,
                Math.Abs(Scale.Y) * 0.5f,
                Math.Abs(Scale.Z) * 0.5f
            );
            return new BoundingBox3D(Position - half, Position + half);
        }
    }

    /// <summary>
    /// Mundo 3D e solucionador determinístico de física e colisão AABB em C#
    /// compartilhado entre o Editor Astron Engine e os builds Android/iOS/Desktop.
    /// </summary>
    public sealed class AstronWorld3D
    {
        private readonly List<SceneNode3D> _nodes = new List<SceneNode3D>();
        private int _nextId = 1;

        public string SceneName { get; set; } = "Cena Principal";
        public Vec3 Gravity { get; set; } = new Vec3(0f, -19.62f, 0f);
        public IReadOnlyList<SceneNode3D> Nodes => _nodes;

        public SceneNode3D SpawnPart(
            string name,
            AstronPrimitiveKind primitive,
            Vec3 position,
            Vec3 scale,
            AstronMaterialPreset material = AstronMaterialPreset.Plastic,
            bool anchored = true)
        {
            var node = new SceneNode3D
            {
                Id = _nextId++,
                Name = name ?? "Part",
                Primitive = primitive,
                Position = position,
                Scale = scale,
                Material = material,
                Anchored = anchored,
                UseGravity = !anchored
            };
            _nodes.Add(node);
            return node;
        }

        public SceneNode3D FindByName(string name)
        {
            for (int i = 0; i < _nodes.Count; i++)
            {
                if (string.Equals(_nodes[i].Name, name, StringComparison.OrdinalIgnoreCase))
                    return _nodes[i];
            }
            return null;
        }

        public void StepPhysics(float dt)
        {
            if (dt <= 0f) return;
            if (dt > 0.05f) dt = 0.05f;

            for (int i = 0; i < _nodes.Count; i++)
            {
                SceneNode3D body = _nodes[i];
                if (!body.Visible || body.Anchored || !body.UseGravity)
                    continue;

                body.Velocity = body.Velocity + Gravity * dt;
                body.Position = body.Position + body.Velocity * dt;

                BoundingBox3D myBox = body.ComputeWorldBounds();
                for (int j = 0; j < _nodes.Count; j++)
                {
                    if (i == j) continue;
                    SceneNode3D obstacle = _nodes[j];
                    if (!obstacle.Visible || !obstacle.CanCollide || !obstacle.Anchored)
                        continue;

                    BoundingBox3D otherBox = obstacle.ComputeWorldBounds();
                    if (myBox.Intersects(otherBox) && body.Velocity.Y <= 0f)
                    {
                        float resolvedY = otherBox.Max.Y + Math.Abs(body.Scale.Y) * 0.5f;
                        body.Position = new Vec3(body.Position.X, resolvedY, body.Position.Z);
                        body.Velocity = new Vec3(body.Velocity.X * 0.9f, -body.Velocity.Y * 0.2f, body.Velocity.Z * 0.9f);
                        myBox = body.ComputeWorldBounds();
                    }
                }
            }
        }

        public string ExportSummaryJson()
        {
            var sb = new StringBuilder();
            sb.Append("{\"engine\":\"Astron Engine\",\"scene\":\"");
            sb.Append(SceneName.Replace("\"", "\\\""));
            sb.Append("\",\"nodeCount\":");
            sb.Append(_nodes.Count.ToString(CultureInfo.InvariantCulture));
            sb.Append("}");
            return sb.ToString();
        }
    }
}
