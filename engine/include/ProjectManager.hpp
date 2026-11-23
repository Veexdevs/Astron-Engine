#pragma once
#include "Scene.hpp"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace astron {

struct ProjectInfo {
    std::string id;
    std::string name;
    std::string createdDate;
    std::string lastModified;
    std::string thumbAsset;
    std::filesystem::path rootDir;
};

struct ResTreeEntry {
    std::string name;
    std::string resPath;
    bool isDir = false;
    std::vector<ResTreeEntry> children;
};

class ProjectManager {
public:
    explicit ProjectManager(std::filesystem::path workspaceRoot = ".");

    const std::vector<ProjectInfo>& projects() const { return m_projects; }
    const ProjectInfo& currentProject() const;

    bool openProject(const std::string& id);
    ProjectInfo createProject(const std::string& name, const std::string& templateId);
    bool renameProject(const std::string& id, const std::string& newName);
    bool deleteProject(const std::string& id);

    // res:// filesystem operations (thread-safe for AI Agent worker thread)
    std::string normalizeResPath(const std::string& path) const;
    std::filesystem::path resolveDiskPath(const std::string& resPath) const;

    std::vector<std::string> listFiles() const;
    bool fileExists(const std::string& resPath) const;
    std::string readFile(const std::string& resPath, bool* ok = nullptr) const;
    bool writeFile(const std::string& resPath, const std::string& content);

    ResTreeEntry buildResTree() const;

    // Exports a portable archive/bundle of all C# & scene files in the project
    std::filesystem::path exportProjectBundle(const std::string& targetTag) const;

    const std::filesystem::path& workspaceRoot() const { return m_workspaceRoot; }

private:
    void seedProjectFiles(const std::filesystem::path& dir, const std::string& projectName, const std::string& templateId);
    void syncCSharpToRuntime(const std::string& normResPath, const std::string& content);

    std::filesystem::path m_workspaceRoot;
    std::vector<ProjectInfo> m_projects;
    size_t m_currentIdx = 0;
    mutable std::mutex m_ioMutex;
};

} // namespace astron
