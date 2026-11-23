#pragma once
#include "../core_c/astron_gl.h"
#include "../core_c/astron_math.h"
#include "Scene.hpp"
#include <filesystem>
#include <string>

namespace astron {

struct GpuMesh {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei indexCount = 0;
    GLsizei vertexCount = 0;
};

class Viewport3D {
public:
    explicit Viewport3D(std::filesystem::path workspaceRoot = ".");
    ~Viewport3D();

    bool init();
    void shutdown();

    void render(const Scene& scene, int width, int height, float timeSec);

    GLuint colorTexture() const { return m_colorTex; }

    // Camera controls
    void orbit(float deltaYawDeg, float deltaPitchDeg);
    void pan(float dx, float dy);
    void zoom(float wheelDelta);
    void resetCamera();

    // Picking in viewport-local pixel coordinates [0..width, 0..height]
    int pickObject(const Scene& scene, float mouseX, float mouseY, int vpW, int vpH) const;

    // Compile live GLSL fragment shader from the "Editor de Shader" panel
    bool compileCustomSurfaceShader(const std::string& fragGlsl, std::string& outLog);
    void useDefaultSurfaceShader() { m_useCustomLit = false; }

    AstronMat4 computeViewMatrix() const;
    AstronMat4 computeProjMatrix(float aspect) const;
    AstronVec3 computeCameraEye() const;

    bool is2DMode = false;
    bool showGrid = true;
    bool showGizmo = true;
    bool wireframe = false;

    float yawDeg = -42.0f;
    float pitchDeg = 22.0f;
    float distance = 9.5f;
    AstronVec3 target = { 0.0f, 0.4f, 0.0f };

private:
    void ensureFramebuffer(int w, int h);
    GLuint compileProgram(const std::string& vsSrc, const std::string& fsSrc, std::string* outErr = nullptr);
    std::string loadShaderFileOrFallback(const std::string& relPath, const char* fallback) const;

    void buildPrimitiveMeshes();

    std::filesystem::path m_root;
    bool m_initialized = false;

    GLuint m_fbo = 0;
    GLuint m_colorTex = 0;
    GLuint m_depthRbo = 0;
    int m_fbWidth = 0;
    int m_fbHeight = 0;

    GLuint m_skyGridProg = 0;
    GLuint m_litProg = 0;
    GLuint m_customLitProg = 0;
    bool   m_useCustomLit = false;
    GLuint m_gizmoProg = 0;

    GpuMesh m_quadMesh;
    GpuMesh m_cubeMesh;
    GpuMesh m_sphereMesh;
    GpuMesh m_cylinderMesh;
    GpuMesh m_wireCubeMesh;
    GpuMesh m_axesGizmoMesh;
};

} // namespace astron
