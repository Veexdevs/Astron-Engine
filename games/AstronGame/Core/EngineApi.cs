using System;
using System.Collections.Generic;

namespace Astron.Engine
{
    public sealed class Entity
    {
        public string Name = "Entity";
        public float X;
        public float Y;
        public float Width = 36f;
        public float Height = 36f;
        public float VelocityX;
        public float VelocityY;
        public bool IsGrounded;

        readonly List<Component> _components = new List<Component>();

        public T AddComponent<T>() where T : Component, new()
        {
            var c = new T { Entity = this };
            _components.Add(c);
            c.OnStart();
            return c;
        }

        public void Update(float dt)
        {
            for (int i = 0; i < _components.Count; i++)
                _components[i].OnUpdate(dt);
        }
    }

    public abstract class Component
    {
        public Entity Entity { get; internal set; }
        public virtual void OnStart() { }
        public virtual void OnUpdate(float dt) { }
    }

    public static class Input
    {
        public static bool Left { get; set; }
        public static bool Right { get; set; }
        public static bool JumpPressed { get; set; }

        public static float GetHorizontalAxis() =>
            (Right ? 1f : 0f) - (Left ? 1f : 0f);
    }

    public static class EngineRuntime
    {
        public static string ActiveScenePath { get; private set; } = "res://default.scene";

        public static void RunScene(string scenePath)
        {
            ActiveScenePath = scenePath ?? "res://default.scene";
        }
    }
}
