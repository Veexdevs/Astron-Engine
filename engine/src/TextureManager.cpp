#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb/stb_image.h"
#include "TextureManager.hpp"

namespace astron {

TextureManager::TextureManager(std::filesystem::path workspaceRoot)
    : m_root(std::move(workspaceRoot)) {}

TextureManager::~TextureManager() {
    clear();
}

GpuTexture TextureManager::get(const std::string& relAssetPath) {
    auto it = m_cache.find(relAssetPath);
    if (it != m_cache.end()) return it->second;

    GpuTexture tex;
    if (!aglGenTextures) return tex;

    std::filesystem::path full = m_root / relAssetPath;
    int w = 0, h = 0, comp = 0;
    stbi_uc* pixels = stbi_load(full.string().c_str(), &w, &h, &comp, 4);
    if (!pixels) {
        m_cache[relAssetPath] = tex;
        return tex;
    }

    aglGenTextures(1, &tex.id);
    aglBindTexture(GL_TEXTURE_2D, tex.id);
    aglPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    aglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    aglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    aglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    aglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    aglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    aglBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);
    tex.width = w;
    tex.height = h;
    m_cache[relAssetPath] = tex;
    return tex;
}

void TextureManager::clear() {
    if (aglDeleteTextures) {
        for (auto& kv : m_cache) {
            if (kv.second.id != 0) {
                aglDeleteTextures(1, &kv.second.id);
            }
        }
    }
    m_cache.clear();
}

} // namespace astron
