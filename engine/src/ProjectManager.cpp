#include "ProjectManager.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace astron {

namespace fs = std::filesystem;

namespace {

std::string slugify(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if (std::isalnum(c)) out.push_back(static_cast<char>(std::tolower(c)));
        else if (c == ' ' || c == '-' || c == '_') {
            if (!out.empty() && out.back() != '-') out.push_back('-');
        }
    }
    while (!out.empty() && out.back() == '-') out.pop_back();
    return out.empty() ? "novo-jogo" : out;
}

std::string readTextFile(const fs::path& p, bool* ok = nullptr) {
    std::ifstream f(p, std::ios::binary);
    if (!f) {
        if (ok) *ok = false;
        return {};
    }
    if (ok) *ok = true;
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

bool writeTextFile(const fs::path& p, const std::string& content) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    std::ofstream f(p, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(content.data(), static_cast<std::streamsize>(content.size()));
    return f.good();
}

} // namespace

ProjectManager::ProjectManager(fs::path workspaceRoot)
    : m_workspaceRoot(std::move(workspaceRoot)) {
    fs::path projectsBase = m_workspaceRoot / "projects";
    std::error_code ec;
    fs::create_directories(projectsBase, ec);

    m_projects = {
        { "meu-jogo",   "Meu Jogo",   "22/11/2026", "22/11/2026 • 14:32", "assets/textures/thumb-cube.jpg", projectsBase / "MeuJogo" },
        { "projeto-rp", "Projeto RP", "21/11/2026", "21/11/2026 • 19:10", "assets/textures/thumb-city.jpg", projectsBase / "ProjetoRP" },
        { "obby-teste", "Obby Teste", "20/11/2026", "20/11/2026 • 11:05", "assets/textures/thumb-obby.jpg", projectsBase / "ObbyTeste" }
    };

    seedProjectFiles(m_projects[0].rootDir, "Meu Jogo", "platformer");
    seedProjectFiles(m_projects[1].rootDir, "Projeto RP", "topdown");
    seedProjectFiles(m_projects[2].rootDir, "Obby Teste", "obby");
}

const ProjectInfo& ProjectManager::currentProject() const {
    return m_projects[m_currentIdx];
}

bool ProjectManager::openProject(const std::string& id) {
    for (size_t i = 0; i < m_projects.size(); ++i) {
        if (m_projects[i].id == id) {
            m_currentIdx = i;
            return true;
        }
    }
    return false;
}

ProjectInfo ProjectManager::createProject(const std::string& name, const std::string& templateId) {
    std::string cleanName = name.empty() ? "Novo Projeto" : name;
    std::string slug = slugify(cleanName);
    std::string uniqueId = slug;
    int suffix = 1;
    while (std::any_of(m_projects.begin(), m_projects.end(), [&](const ProjectInfo& p) { return p.id == uniqueId; })) {
        uniqueId = slug + "-" + std::to_string(++suffix);
    }

    fs::path dir = m_workspaceRoot / "projects" / uniqueId;
    seedProjectFiles(dir, cleanName, templateId);

    std::string thumb = "assets/textures/thumb-cube.jpg";
    if (templateId == "obby") thumb = "assets/textures/thumb-obby.jpg";
    else if (templateId == "topdown") thumb = "assets/textures/thumb-city.jpg";

    ProjectInfo info{
        uniqueId,
        cleanName,
        "23/11/2026",
        "23/11/2026 • Agora",
        thumb,
        dir
    };
    m_projects.insert(m_projects.begin(), info);
    m_currentIdx = 0;
    return info;
}

bool ProjectManager::renameProject(const std::string& id, const std::string& newName) {
    if (newName.empty()) return false;
    for (auto& p : m_projects) {
        if (p.id == id) {
            p.name = newName;
            return true;
        }
    }
    return false;
}

bool ProjectManager::deleteProject(const std::string& id) {
    if (m_projects.size() <= 1) return false;
    for (size_t i = 0; i < m_projects.size(); ++i) {
        if (m_projects[i].id == id) {
            m_projects.erase(m_projects.begin() + static_cast<ptrdiff_t>(i));
            if (m_currentIdx >= m_projects.size()) m_currentIdx = 0;
            return true;
        }
    }
    return false;
}

std::string ProjectManager::normalizeResPath(const std::string& path) const {
    std::string s = path;
    for (char& c : s) {
        if (c == '\\') c = '/';
    }
    if (s.rfind("res://", 0) == 0) {
        s = s.substr(6);
    }
    while (!s.empty() && s.front() == '/') s.erase(s.begin());
    // Prevent directory traversal outside project root
    std::string clean;
    std::istringstream iss(s);
    std::string seg;
    while (std::getline(iss, seg, '/')) {
        if (seg.empty() || seg == "." || seg == "..") continue;
        if (!clean.empty()) clean.push_back('/');
        clean += seg;
    }
    return "res://" + clean;
}

fs::path ProjectManager::resolveDiskPath(const std::string& resPath) const {
    std::string norm = normalizeResPath(resPath);
    std::string rel = norm.substr(6);
    return currentProject().rootDir / fs::path(rel);
}

std::vector<std::string> ProjectManager::listFiles() const {
    std::lock_guard<std::mutex> lock(m_ioMutex);
    std::vector<std::string> out;
    const fs::path& root = currentProject().rootDir;
    std::error_code ec;
    if (!fs::exists(root, ec)) return out;

    for (auto it = fs::recursive_directory_iterator(root, ec); it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (it->is_regular_file(ec)) {
            fs::path rel = fs::relative(it->path(), root, ec);
            std::string s = rel.generic_string();
            out.push_back("res://" + s);
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

bool ProjectManager::fileExists(const std::string& resPath) const {
    std::lock_guard<std::mutex> lock(m_ioMutex);
    std::error_code ec;
    return fs::is_regular_file(resolveDiskPath(resPath), ec);
}

std::string ProjectManager::readFile(const std::string& resPath, bool* ok) const {
    std::lock_guard<std::mutex> lock(m_ioMutex);
    return readTextFile(resolveDiskPath(resPath), ok);
}

bool ProjectManager::writeFile(const std::string& resPath, const std::string& content) {
    std::lock_guard<std::mutex> lock(m_ioMutex);
    std::string norm = normalizeResPath(resPath);
    if (norm == "res://") return false;
    fs::path disk = resolveDiskPath(norm);
    if (!writeTextFile(disk, content)) return false;
    syncCSharpToRuntime(norm, content);
    return true;
}

void ProjectManager::syncCSharpToRuntime(const std::string& normResPath, const std::string& content) {
    if (normResPath.size() < 3 || normResPath.substr(normResPath.size() - 3) != ".cs") return;

    fs::path coreDir = m_workspaceRoot / "games" / "AstronGame" / "Core";
    std::error_code ec;
    if (!fs::exists(coreDir, ec)) return;

    // If the file defines Astron.Game.GameWorld, update Core/GameWorld.cs directly so APK/IPA/EXE/Linux build it!
    bool isGameWorld = (normResPath == "res://scripts/GameWorld.cs" || normResPath == "res://GameWorld.cs") &&
                       content.find("namespace Astron.Game") != std::string::npos &&
                       content.find("class GameWorld") != std::string::npos;

    if (isGameWorld) {
        writeTextFile(coreDir / "GameWorld.cs", content);
    } else if (content.find("namespace Astron.") != std::string::npos) {
        // Sync auxiliary Astron.Game / Astron.Engine classes into Core/Scripts/
        fs::path fname = fs::path(normResPath.substr(6)).filename();
        if (fname != "main.cs") {
            writeTextFile(coreDir / "Scripts" / fname, content);
        }
    }
}

ResTreeEntry ProjectManager::buildResTree() const {
    std::lock_guard<std::mutex> lock(m_ioMutex);
    ResTreeEntry root;
    root.name = "res://";
    root.resPath = "res://";
    root.isDir = true;

    const fs::path& baseDir = currentProject().rootDir;
    std::error_code ec;
    if (!fs::exists(baseDir, ec)) return root;

    auto populate = [&](auto& self, const fs::path& dir, const std::string& resPrefix, ResTreeEntry& node) -> void {
        std::vector<fs::directory_entry> dirs;
        std::vector<fs::directory_entry> files;
        for (const auto& entry : fs::directory_iterator(dir, ec)) {
            if (entry.is_directory(ec)) dirs.push_back(entry);
            else if (entry.is_regular_file(ec)) files.push_back(entry);
        }

        const std::vector<std::string> preferredOrder = { "models", "textures", "scripts", "sounds", "ui", "shaders" };
        std::sort(dirs.begin(), dirs.end(), [&](const fs::directory_entry& a, const fs::directory_entry& b) {
            std::string na = a.path().filename().string();
            std::string nb = b.path().filename().string();
            auto ia = std::find(preferredOrder.begin(), preferredOrder.end(), na);
            auto ib = std::find(preferredOrder.begin(), preferredOrder.end(), nb);
            if (ia != ib) return ia < ib;
            return na < nb;
        });
        std::sort(files.begin(), files.end(), [](const fs::directory_entry& a, const fs::directory_entry& b) {
            std::string na = a.path().filename().string();
            std::string nb = b.path().filename().string();
            if (na == "default.scene" && nb != "default.scene") return true;
            if (nb == "default.scene" && na != "default.scene") return false;
            return na < nb;
        });

        for (const auto& d : dirs) {
            ResTreeEntry child;
            child.name = d.path().filename().string();
            child.resPath = resPrefix + child.name;
            child.isDir = true;
            self(self, d.path(), child.resPath + "/", child);
            node.children.push_back(std::move(child));
        }
        for (const auto& f : files) {
            ResTreeEntry child;
            child.name = f.path().filename().string();
            child.resPath = resPrefix + child.name;
            child.isDir = false;
            node.children.push_back(std::move(child));
        }
    };

    populate(populate, baseDir, "res://", root);
    return root;
}

fs::path ProjectManager::exportProjectBundle(const std::string& targetTag) const {
    auto paths = listFiles();
    Json bundle = Json::object();
    bundle["engine"] = "Astron Engine 1.0.0 (Native C++/C/C#/GLSL)";
    bundle["project"] = currentProject().name;
    bundle["target"] = targetTag;

    Json filesObj = Json::object();
    for (const auto& p : paths) {
        filesObj[p] = readFile(p);
    }
    bundle["files"] = std::move(filesObj);

    fs::path outDir = m_workspaceRoot / "out";
    std::error_code ec;
    fs::create_directories(outDir, ec);
    fs::path outPath = outDir / (currentProject().id + "-" + targetTag + "-bundle.json");
    writeTextFile(outPath, bundle.dump(2));
    return outPath;
}

void ProjectManager::seedProjectFiles(const fs::path& dir, const std::string& projectName, const std::string& templateId) {
    std::error_code ec;
    for (const char* sub : { "models", "textures", "scripts", "sounds", "ui", "shaders" }) {
        fs::create_directories(dir / sub, ec);
    }

    if (!fs::exists(dir / "default.scene", ec)) {
        Scene sc = Scene::createDefault();
        sc.name = "Cena Principal";
        if (templateId == "obby") {
            sc.addObject("Plataforma", MeshKind::Block, { 3.0f, 1.0f, 0.0f });
            sc.addObject("Plataforma", MeshKind::Block, { 6.0f, 2.0f, 0.0f });
            sc.selectedIndex = 0;
        }
        writeTextFile(dir / "default.scene", sc.toJson().dump(2));
    }

    if (!fs::exists(dir / "main.cs", ec)) {
        std::string mainCs =
            "using Astron.Engine;\n\n"
            "// Ponto de entrada C# — Astron Engine (" + projectName + ")\n"
            "public static class Program\n"
            "{\n"
            "    public static void Main(string[] args)\n"
            "    {\n"
            "        EngineRuntime.RunScene(\"res://default.scene\");\n"
            "    }\n"
            "}\n";
        writeTextFile(dir / "main.cs", mainCs);
    }

    if (!fs::exists(dir / "scripts" / "Player.cs", ec)) {
        std::string playerCs =
            "using Astron.Engine;\n\n"
            "namespace Astron.Scripts\n"
            "{\n"
            "    public sealed class Player : Component\n"
            "    {\n"
            "        public float Speed = 340f;\n"
            "        public float JumpForce = 840f;\n\n"
            "        public override void OnUpdate(float dt)\n"
            "        {\n"
            "            float move = Input.GetHorizontalAxis();\n"
            "            Entity.VelocityX = move * Speed;\n"
            "            if (Input.JumpPressed && Entity.IsGrounded)\n"
            "            {\n"
            "                Entity.VelocityY = -JumpForce;\n"
            "            }\n"
            "        }\n"
            "    }\n"
            "}\n";
        writeTextFile(dir / "scripts" / "Player.cs", playerCs);
    }

    if (!fs::exists(dir / "scripts" / "GameWorld.cs", ec)) {
        fs::path coreWorld = m_workspaceRoot / "games" / "AstronGame" / "Core" / "GameWorld.cs";
        bool ok = false;
        std::string existing = readTextFile(coreWorld, &ok);
        if (ok && !existing.empty()) {
            writeTextFile(dir / "scripts" / "GameWorld.cs", existing);
        }
    }

    if (!fs::exists(dir / "shaders" / "surface.glsl", ec)) {
        bool ok = false;
        std::string sh = readTextFile(m_workspaceRoot / "shaders" / "custom_surface.glsl", &ok);
        if (ok && !sh.empty()) {
            writeTextFile(dir / "shaders" / "surface.glsl", sh);
        }
    }
}

} // namespace astron
