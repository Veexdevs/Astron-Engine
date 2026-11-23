#pragma once
#include "../core_c/astron_gl.h"
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace astron {

struct GpuTexture {
    GLuint id = 0;
    int width = 0;
    int height = 0;
};

class TextureManager {
public:
    explicit TextureManager(std::filesystem::path workspaceRoot = ".");
    ~TextureManager();

    GpuTexture get(const std::string& relAssetPath);
    void clear();

    static bool loadRgbaPixels(
        const std::filesystem::path& filePath,
        std::vector<unsigned char>& outPixels,
        int& outW,
        int& outH
    );

private:
    std::filesystem::path m_root;
    std::map<std::string, GpuTexture> m_cache;
};

} // namespace astron
