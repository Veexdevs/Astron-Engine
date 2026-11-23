using System;
using System.Collections.Generic;
using System.IO;

namespace Astron.Engine.Mobile
{
    public sealed class MobileProjectInfo
    {
        public string Id;
        public string Name;
        public string CreatedDate;
        public string LastModified;
        public string ThumbAsset;
        public string DirectoryPath;
    }

    /// <summary>
    /// Gerenciador de projetos e sistema de arquivos virtual res:// em armazenamento nativo
    /// persistente do Android (.apk) e iOS (.ipa).
    /// </summary>
    public sealed class MobileProjectStore
    {
        private readonly string _baseDir;
        private readonly List<MobileProjectInfo> _projects = new List<MobileProjectInfo>();
        private int _currentIndex;

        public IReadOnlyList<MobileProjectInfo> Projects => _projects;
        public MobileProjectInfo Current => _projects[_currentIndex];

        public MobileProjectStore(string persistentRoot)
        {
            _baseDir = Path.Combine(
                string.IsNullOrEmpty(persistentRoot) ? Path.GetTempPath() : persistentRoot,
                "AstronEngineProjects"
            );
            Directory.CreateDirectory(_baseDir);

            _projects.Add(new MobileProjectInfo
            {
                Id = "meu-jogo",
                Name = "Meu Jogo",
                CreatedDate = "22/11/2026",
                LastModified = "22/11/2026 • 14:32",
                ThumbAsset = "thumb-cube.jpg",
                DirectoryPath = Path.Combine(_baseDir, "MeuJogo")
            });
            _projects.Add(new MobileProjectInfo
            {
                Id = "projeto-rp",
                Name = "Projeto RP",
                CreatedDate = "21/11/2026",
                LastModified = "21/11/2026 • 19:10",
                ThumbAsset = "thumb-city.jpg",
                DirectoryPath = Path.Combine(_baseDir, "ProjetoRP")
            });
            _projects.Add(new MobileProjectInfo
            {
                Id = "obby-teste",
                Name = "Obby Teste",
                CreatedDate = "20/11/2026",
                LastModified = "20/11/2026 • 11:05",
                ThumbAsset = "thumb-obby.jpg",
                DirectoryPath = Path.Combine(_baseDir, "ObbyTeste")
            });

            SeedProject(_projects[0].DirectoryPath, "Meu Jogo", "platformer");
            SeedProject(_projects[1].DirectoryPath, "Projeto RP", "rp");
            SeedProject(_projects[2].DirectoryPath, "Obby Teste", "obby");
        }

        public void SelectProject(string id)
        {
            for (int i = 0; i < _projects.Count; i++)
            {
                if (_projects[i].Id == id)
                {
                    _currentIndex = i;
                    return;
                }
            }
        }

        public MobileProjectInfo CreateProject(string name, string templateId = "obby")
        {
            string clean = string.IsNullOrWhiteSpace(name) ? "Novo Projeto C#" : name.Trim();
            string slug = "proj-" + (_projects.Count + 1);
            string dir = Path.Combine(_baseDir, slug);
            SeedProject(dir, clean, templateId);

            var info = new MobileProjectInfo
            {
                Id = slug,
                Name = clean,
                CreatedDate = "23/11/2026",
                LastModified = "23/11/2026 • Agora",
                ThumbAsset = templateId == "obby" ? "thumb-obby.jpg" : "thumb-cube.jpg",
                DirectoryPath = dir
            };
            _projects.Insert(0, info);
            _currentIndex = 0;
            return info;
        }

        public List<string> ListResFiles()
        {
            var result = new List<string>();
            string root = Current.DirectoryPath;
            if (!Directory.Exists(root)) return result;

            foreach (string file in Directory.GetFiles(root, "*", SearchOption.AllDirectories))
            {
                string rel = file.Substring(root.Length).TrimStart(Path.DirectorySeparatorChar, '/');
                rel = rel.Replace('\\', '/');
                result.Add("res://" + rel);
            }
            result.Sort(StringComparer.Ordinal);
            return result;
        }

        public string ReadFile(string resPath)
        {
            string full = ResolvePath(resPath);
            return File.Exists(full) ? File.ReadAllText(full) : string.Empty;
        }

        public bool WriteFile(string resPath, string content)
        {
            try
            {
                string full = ResolvePath(resPath);
                string parent = Path.GetDirectoryName(full);
                if (!string.IsNullOrEmpty(parent)) Directory.CreateDirectory(parent);
                File.WriteAllText(full, content ?? string.Empty);
                Current.LastModified = "23/11/2026 • Agora";
                return true;
            }
            catch
            {
                return false;
            }
        }

        private string ResolvePath(string resPath)
        {
            string clean = (resPath ?? string.Empty).Trim();
            if (clean.StartsWith("res://", StringComparison.OrdinalIgnoreCase))
                clean = clean.Substring(6);
            clean = clean.Replace("..", "").TrimStart('/', '\\');
            return Path.Combine(Current.DirectoryPath, clean.Replace('/', Path.DirectorySeparatorChar));
        }

        private static void SeedProject(string dir, string title, string tpl)
        {
            Directory.CreateDirectory(Path.Combine(dir, "scripts"));
            Directory.CreateDirectory(Path.Combine(dir, "shaders"));
            Directory.CreateDirectory(Path.Combine(dir, "sounds"));

            string mainCs = Path.Combine(dir, "main.cs");
            if (!File.Exists(mainCs))
            {
                File.WriteAllText(mainCs,
                    "using Astron.Engine;\n\n" +
                    "// Projeto: " + title + " — Astron Engine (C# Nativo)\n" +
                    "public static class Program\n{\n" +
                    "    public static void Main()\n    {\n" +
                    "        var world = new AstronWorld3D { SceneName = \"Cena Principal\" };\n" +
                    "        world.SpawnPart(\"Part\", AstronPrimitiveKind.Cube, new Vec3(0, 0.5f, 0), Vec3.One);\n" +
                    "    }\n}\n");
            }

            string playerCs = Path.Combine(dir, "scripts", "Player.cs");
            if (!File.Exists(playerCs))
            {
                File.WriteAllText(playerCs,
                    "using Astron.Engine;\n\n" +
                    "namespace Astron.Scripts\n{\n" +
                    "    public sealed class PlayerController\n    {\n" +
                    "        public float Speed = 8.5f;\n" +
                    "        public float JumpImpulse = 11.0f;\n" +
                    "        public string Mode = \"" + tpl + "\";\n" +
                    "    }\n}\n");
            }

            string worldCs = Path.Combine(dir, "scripts", "GameWorld.cs");
            if (!File.Exists(worldCs))
            {
                File.WriteAllText(worldCs,
                    "using Astron.Engine;\n\n" +
                    "namespace Astron.Scripts\n{\n" +
                    "    public sealed class GameWorldScript\n    {\n" +
                    "        public void BuildLevel(AstronWorld3D world)\n        {\n" +
                    "            world.SpawnPart(\"Baseplate\", AstronPrimitiveKind.Plane, new Vec3(0, -0.05f, 0), new Vec3(24, 0.1f, 24));\n" +
                    "            world.SpawnPart(\"BlocoInicial\", AstronPrimitiveKind.Cube, new Vec3(0, 0.5f, 0), Vec3.One);\n" +
                    "        }\n    }\n}\n");
            }

            string mainLua = Path.Combine(dir, "main.lua");
            if (!File.Exists(mainLua))
            {
                File.WriteAllText(mainLua,
                    "-- Astron Engine — Script Lua 5.4 (" + title + ")\n" +
                    "local SceneConfig = { gravity = -19.62, sky = \"astron_dark\" }\n" +
                    "return SceneConfig\n");
            }

            string shaderGlsl = Path.Combine(dir, "shaders", "surface.glsl");
            if (!File.Exists(shaderGlsl))
            {
                File.WriteAllText(shaderGlsl,
                    "#version 330 core\n" +
                    "in vec3 vNormal;\n" +
                    "uniform vec4 uBaseColor;\n" +
                    "out vec4 FragColor;\n" +
                    "void main() {\n" +
                    "    float lit = max(dot(normalize(vNormal), normalize(vec3(0.5, 1.0, 0.4))), 0.25);\n" +
                    "    FragColor = vec4(uBaseColor.rgb * lit, uBaseColor.a);\n" +
                    "}\n");
            }
        }
    }
}
