#include "Viewport3D.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace astron {

namespace {

const char* kFallbackSkyVert = R"GLSL(
#version 330 core
layout(location = 0) in vec2 aQuadPos;
out vec2 vNdc;
void main() {
    vNdc = aQuadPos;
    gl_Position = vec4(aQuadPos, 0.9999, 1.0);
}
)GLSL";

const char* kFallbackSkyFrag = R"GLSL(
#version 330 core
in vec2 vNdc;
out vec4 FragColor;
uniform mat4 uInvViewProj;
uniform mat4 uViewProj;
uniform vec3 uCamPos;
uniform int uShowGrid;
vec3 unprojectPoint(float x, float y, float z) {
    vec4 p = uInvViewProj * vec4(x, y, z, 1.0);
    return p.xyz / max(abs(p.w), 1e-6);
}
void main() {
    vec3 nearPt = unprojectPoint(vNdc.x, vNdc.y, -1.0);
    vec3 farPt  = unprojectPoint(vNdc.x, vNdc.y,  1.0);
    vec3 rayDir = normalize(farPt - nearPt);
    float h = rayDir.y;
    vec3 zenCol     = vec3(0.115, 0.122, 0.140);
    vec3 horizonCol = vec3(0.420, 0.432, 0.455);
    vec3 groundBg   = vec3(0.055, 0.058, 0.066);
    vec3 sky = (h >= 0.0)
        ? mix(zenCol, horizonCol, pow(1.0 - clamp(h * 2.6, 0.0, 1.0), 3.0))
        : mix(groundBg, horizonCol * 0.72, pow(1.0 - clamp(-h * 5.0, 0.0, 1.0), 2.5));
    if (uShowGrid != 0 && abs(rayDir.y) > 1e-4) {
        float t = -nearPt.y / rayDir.y;
        if (t > 0.0) {
            vec3 hit = nearPt + t * rayDir;
            vec4 clip = uViewProj * vec4(hit, 1.0);
            gl_FragDepth = clamp((clip.z / clip.w) * 0.5 + 0.5, 0.0, 0.9998);
            float fade = exp(-length(hit.xz - uCamPos.xz) * 0.022);
            vec2 coord = hit.xz;
            vec2 deriv = max(fwidth(coord), vec2(1e-4));
            vec2 g1 = abs(fract(coord - 0.5) - 0.5) / deriv;
            float l1 = 1.0 - min(min(g1.x, g1.y), 1.0);
            vec2 g5 = abs(fract(coord * 0.2 - 0.5) - 0.5) / (deriv * 0.2);
            float l5 = 1.0 - min(min(g5.x, g5.y), 1.0);
            vec4 gc = vec4(0.26, 0.28, 0.31, max(l1 * 0.32, l5 * 0.58) * fade);
            float axX = 1.0 - min(abs(coord.y) / (deriv.y * 1.8), 1.0);
            if (axX > 0.01) gc = mix(gc, vec4(0.88, 0.20, 0.22, 0.92 * fade), axX);
            float axZ = 1.0 - min(abs(coord.x) / (deriv.x * 1.8), 1.0);
            if (axZ > 0.01) gc = mix(gc, vec4(0.22, 0.48, 0.95, 0.92 * fade), axZ);
            FragColor = vec4(mix(sky, gc.rgb, gc.a), 1.0);
            return;
        }
    }
    gl_FragDepth = 0.9999;
    FragColor = vec4(sky, 1.0);
}
)GLSL";

const char* kFallbackLitVert = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4 uMVP;
uniform mat4 uModel;
out vec3 vWorldPos;
out vec3 vNormal;
void main() {
    vec4 wp = uModel * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    vNormal = normalize(mat3(transpose(inverse(uModel))) * aNormal);
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)GLSL";

const char* kFallbackLitFrag = R"GLSL(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
out vec4 FragColor;
uniform vec4 uColor;
uniform vec3 uCamPos;
uniform int uMaterialType;
void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCamPos - vWorldPos);
    vec3 L = normalize(vec3(0.55, 0.85, 0.48));
    if (uMaterialType == 4) {
        FragColor = vec4(uColor.rgb * 1.35, uColor.a);
        return;
    }
    float diff = max(dot(N, L), 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), uMaterialType == 1 ? 96.0 : 36.0) * (uMaterialType == 1 ? 0.85 : 0.30);
    FragColor = vec4(uColor.rgb * (0.32 + diff * 0.72) + vec3(spec), uColor.a);
}
)GLSL";

const char* kFallbackGizmoVert = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aCol;
uniform mat4 uMVP;
out vec4 vCol;
void main() {
    vCol = aCol;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)GLSL";

const char* kFallbackGizmoFrag = R"GLSL(
#version 330 core
in vec4 vCol;
out vec4 FragColor;
void main() {
    FragColor = vCol;
}
)GLSL";

void destroyMesh(GpuMesh& m) {
    if (m.ebo && aglDeleteBuffers) aglDeleteBuffers(1, &m.ebo);
    if (m.vbo && aglDeleteBuffers) aglDeleteBuffers(1, &m.vbo);
    if (m.vao && aglDeleteVertexArrays) aglDeleteVertexArrays(1, &m.vao);
    m = GpuMesh{};
}

GpuMesh uploadPosNormMesh(const std::vector<float>& verts, const std::vector<uint32_t>& indices) {
    GpuMesh m;
    aglGenVertexArrays(1, &m.vao);
    aglGenBuffers(1, &m.vbo);
    aglGenBuffers(1, &m.ebo);

    aglBindVertexArray(m.vao);
    aglBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    aglBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(float)), verts.data(), GL_STATIC_DRAW);

    aglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    aglBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)), indices.data(), GL_STATIC_DRAW);

    GLsizei stride = static_cast<GLsizei>(6 * sizeof(float));
    aglEnableVertexAttribArray(0);
    aglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(0));
    aglEnableVertexAttribArray(1);
    aglVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(3 * sizeof(float)));

    aglBindVertexArray(0);
    m.indexCount = static_cast<GLsizei>(indices.size());
    return m;
}

GpuMesh uploadPosColLines(const std::vector<float>& verts) {
    GpuMesh m;
    aglGenVertexArrays(1, &m.vao);
    aglGenBuffers(1, &m.vbo);

    aglBindVertexArray(m.vao);
    aglBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    aglBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts.size() * sizeof(float)), verts.data(), GL_STATIC_DRAW);

    GLsizei stride = static_cast<GLsizei>(7 * sizeof(float));
    aglEnableVertexAttribArray(0);
    aglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(0));
    aglEnableVertexAttribArray(1);
    aglVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(3 * sizeof(float)));

    aglBindVertexArray(0);
    m.vertexCount = static_cast<GLsizei>(verts.size() / 7);
    return m;
}

} // namespace

Viewport3D::Viewport3D(std::filesystem::path workspaceRoot)
    : m_root(std::move(workspaceRoot)) {}

Viewport3D::~Viewport3D() {
    shutdown();
}

std::string Viewport3D::loadShaderFileOrFallback(const std::string& relPath, const char* fallback) const {
    std::ifstream f(m_root / relPath, std::ios::binary);
    if (!f) return fallback;
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string s = ss.str();
    return s.empty() ? std::string(fallback) : s;
}

GLuint Viewport3D::compileProgram(const std::string& vsSrc, const std::string& fsSrc, std::string* outErr) {
    auto compileOne = [&](GLenum type, const std::string& src, std::string& err) -> GLuint {
        GLuint sh = aglCreateShader(type);
        const char* ptr = src.c_str();
        aglShaderSource(sh, 1, &ptr, nullptr);
        aglCompileShader(sh);
        GLint ok = 0;
        aglGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char logBuf[2048] = {};
            GLsizei len = 0;
            aglGetShaderInfoLog(sh, sizeof(logBuf) - 1, &len, logBuf);
            err = logBuf;
            aglDeleteShader(sh);
            return 0;
        }
        return sh;
    };

    std::string err;
    GLuint vs = compileOne(GL_VERTEX_SHADER, vsSrc, err);
    if (!vs) {
        if (outErr) *outErr = "Vertex shader: " + err;
        return 0;
    }
    GLuint fs = compileOne(GL_FRAGMENT_SHADER, fsSrc, err);
    if (!fs) {
        aglDeleteShader(vs);
        if (outErr) *outErr = "Fragment shader: " + err;
        return 0;
    }

    GLuint prog = aglCreateProgram();
    aglAttachShader(prog, vs);
    aglAttachShader(prog, fs);
    aglLinkProgram(prog);
    aglDeleteShader(vs);
    aglDeleteShader(fs);

    GLint linked = 0;
    aglGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        char logBuf[2048] = {};
        GLsizei len = 0;
        aglGetProgramInfoLog(prog, sizeof(logBuf) - 1, &len, logBuf);
        if (outErr) *outErr = std::string("Link GLSL: ") + logBuf;
        aglDeleteProgram(prog);
        return 0;
    }
    return prog;
}

bool Viewport3D::init() {
    if (m_initialized || !aglCreateProgram) return m_initialized;

    m_skyGridProg = compileProgram(
        loadShaderFileOrFallback("shaders/sky_grid.vert", kFallbackSkyVert),
        loadShaderFileOrFallback("shaders/sky_grid.frag", kFallbackSkyFrag)
    );
    m_litProg = compileProgram(
        loadShaderFileOrFallback("shaders/pbr_lit.vert", kFallbackLitVert),
        loadShaderFileOrFallback("shaders/pbr_lit.frag", kFallbackLitFrag)
    );
    m_gizmoProg = compileProgram(
        loadShaderFileOrFallback("shaders/gizmo.vert", kFallbackGizmoVert),
        loadShaderFileOrFallback("shaders/gizmo.frag", kFallbackGizmoFrag)
    );

    buildPrimitiveMeshes();
    m_initialized = (m_skyGridProg != 0 && m_litProg != 0 && m_gizmoProg != 0);
    return m_initialized;
}

void Viewport3D::shutdown() {
    if (!m_initialized) return;
    destroyMesh(m_quadMesh);
    destroyMesh(m_cubeMesh);
    destroyMesh(m_sphereMesh);
    destroyMesh(m_cylinderMesh);
    destroyMesh(m_wireCubeMesh);
    destroyMesh(m_axesGizmoMesh);

    if (m_skyGridProg && aglDeleteProgram) aglDeleteProgram(m_skyGridProg);
    if (m_litProg && aglDeleteProgram) aglDeleteProgram(m_litProg);
    if (m_customLitProg && aglDeleteProgram) aglDeleteProgram(m_customLitProg);
    if (m_gizmoProg && aglDeleteProgram) aglDeleteProgram(m_gizmoProg);

    if (m_depthRbo && aglDeleteRenderbuffers) aglDeleteRenderbuffers(1, &m_depthRbo);
    if (m_colorTex && aglDeleteTextures) aglDeleteTextures(1, &m_colorTex);
    if (m_fbo && aglDeleteFramebuffers) aglDeleteFramebuffers(1, &m_fbo);

    m_skyGridProg = m_litProg = m_customLitProg = m_gizmoProg = 0;
    m_fbo = m_colorTex = m_depthRbo = 0;
    m_fbWidth = m_fbHeight = 0;
    m_initialized = false;
}

bool Viewport3D::compileCustomSurfaceShader(const std::string& fragGlsl, std::string& outLog) {
    if (!m_initialized) {
        outLog = "Contexto OpenGL ainda não inicializado.";
        return false;
    }
    std::string vs = loadShaderFileOrFallback("shaders/pbr_lit.vert", kFallbackLitVert);
    std::string err;
    GLuint p = compileProgram(vs, fragGlsl, &err);
    if (!p) {
        outLog = err;
        return false;
    }
    if (m_customLitProg) aglDeleteProgram(m_customLitProg);
    m_customLitProg = p;
    m_useCustomLit = true;
    outLog = "Shader GLSL 3.30 compilado e vinculado na GPU com sucesso!";
    return true;
}

void Viewport3D::buildPrimitiveMeshes() {
    // Fullscreen quad
    {
        const float quad[] = {
            -1.f, -1.f,  1.f, -1.f,  1.f,  1.f,
            -1.f, -1.f,  1.f,  1.f, -1.f,  1.f
        };
        aglGenVertexArrays(1, &m_quadMesh.vao);
        aglGenBuffers(1, &m_quadMesh.vbo);
        aglBindVertexArray(m_quadMesh.vao);
        aglBindBuffer(GL_ARRAY_BUFFER, m_quadMesh.vbo);
        aglBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        aglEnableVertexAttribArray(0);
        aglVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        aglBindVertexArray(0);
        m_quadMesh.vertexCount = 6;
    }

    // Unit Cube [-0.5, 0.5]
    {
        const std::vector<float> v = {
            // +Z
            -0.5f,-0.5f, 0.5f, 0,0,1,   0.5f,-0.5f, 0.5f, 0,0,1,   0.5f, 0.5f, 0.5f, 0,0,1,  -0.5f, 0.5f, 0.5f, 0,0,1,
            // -Z
             0.5f,-0.5f,-0.5f, 0,0,-1, -0.5f,-0.5f,-0.5f, 0,0,-1, -0.5f, 0.5f,-0.5f, 0,0,-1,  0.5f, 0.5f,-0.5f, 0,0,-1,
            // +X
             0.5f,-0.5f, 0.5f, 1,0,0,   0.5f,-0.5f,-0.5f, 1,0,0,   0.5f, 0.5f,-0.5f, 1,0,0,   0.5f, 0.5f, 0.5f, 1,0,0,
            // -X
            -0.5f,-0.5f,-0.5f,-1,0,0,  -0.5f,-0.5f, 0.5f,-1,0,0,  -0.5f, 0.5f, 0.5f,-1,0,0,  -0.5f, 0.5f,-0.5f,-1,0,0,
            // +Y
            -0.5f, 0.5f, 0.5f, 0,1,0,   0.5f, 0.5f, 0.5f, 0,1,0,   0.5f, 0.5f,-0.5f, 0,1,0,  -0.5f, 0.5f,-0.5f, 0,1,0,
            // -Y
            -0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f,-0.5f, 0,-1,0,  0.5f,-0.5f, 0.5f, 0,-1,0, -0.5f,-0.5f, 0.5f, 0,-1,0,
        };
        std::vector<uint32_t> idx;
        for (uint32_t f = 0; f < 6; ++f) {
            uint32_t b = f * 4;
            idx.insert(idx.end(), { b, b + 1, b + 2, b, b + 2, b + 3 });
        }
        m_cubeMesh = uploadPosNormMesh(v, idx);
    }

    // UV Sphere (radius 0.5)
    {
        const int stacks = 18, slices = 28;
        std::vector<float> v;
        std::vector<uint32_t> idx;
        for (int i = 0; i <= stacks; ++i) {
            float phi = static_cast<float>(M_PI) * static_cast<float>(i) / stacks;
            float y = std::cos(phi);
            float r = std::sin(phi);
            for (int j = 0; j <= slices; ++j) {
                float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(j) / slices;
                float x = r * std::cos(theta);
                float z = r * std::sin(theta);
                v.insert(v.end(), { x * 0.5f, y * 0.5f, z * 0.5f, x, y, z });
            }
        }
        for (int i = 0; i < stacks; ++i) {
            for (int j = 0; j < slices; ++j) {
                uint32_t a = static_cast<uint32_t>(i * (slices + 1) + j);
                uint32_t b = a + static_cast<uint32_t>(slices + 1);
                idx.insert(idx.end(), { a, b, a + 1, b, b + 1, a + 1 });
            }
        }
        m_sphereMesh = uploadPosNormMesh(v, idx);
    }

    // Unit Cylinder (radius 0.5, height 1.0)
    {
        const int segs = 28;
        std::vector<float> v;
        std::vector<uint32_t> idx;
        for (int i = 0; i <= segs; ++i) {
            float a = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / segs;
            float nx = std::cos(a), nz = std::sin(a);
            v.insert(v.end(), { nx * 0.5f, -0.5f, nz * 0.5f, nx, 0.f, nz });
            v.insert(v.end(), { nx * 0.5f,  0.5f, nz * 0.5f, nx, 0.f, nz });
        }
        for (int i = 0; i < segs; ++i) {
            uint32_t b = static_cast<uint32_t>(i * 2);
            idx.insert(idx.end(), { b, b + 2, b + 1, b + 1, b + 2, b + 3 });
        }
        // Top cap
        uint32_t topCenter = static_cast<uint32_t>(v.size() / 6);
        v.insert(v.end(), { 0.f, 0.5f, 0.f, 0.f, 1.f, 0.f });
        for (int i = 0; i <= segs; ++i) {
            float a = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / segs;
            v.insert(v.end(), { std::cos(a) * 0.5f, 0.5f, std::sin(a) * 0.5f, 0.f, 1.f, 0.f });
        }
        for (int i = 0; i < segs; ++i) {
            idx.insert(idx.end(), { topCenter, topCenter + 1 + static_cast<uint32_t>(i + 1), topCenter + 1 + static_cast<uint32_t>(i) });
        }
        m_cylinderMesh = uploadPosNormMesh(v, idx);
    }

    // Wireframe selection box
    {
        std::vector<float> lv;
        auto pushLine = [&](float x0, float y0, float z0, float x1, float y1, float z1) {
            lv.insert(lv.end(), { x0, y0, z0, 0.92f, 0.95f, 1.0f, 0.95f });
            lv.insert(lv.end(), { x1, y1, z1, 0.92f, 0.95f, 1.0f, 0.95f });
        };
        const float s = 0.505f;
        pushLine(-s,-s,-s,  s,-s,-s); pushLine( s,-s,-s,  s,-s, s);
        pushLine( s,-s, s, -s,-s, s); pushLine(-s,-s, s, -s,-s,-s);
        pushLine(-s, s,-s,  s, s,-s); pushLine( s, s,-s,  s, s, s);
        pushLine( s, s, s, -s, s, s); pushLine(-s, s, s, -s, s,-s);
        pushLine(-s,-s,-s, -s, s,-s); pushLine( s,-s,-s,  s, s,-s);
        pushLine( s,-s, s,  s, s, s); pushLine(-s,-s, s, -s, s, s);
        m_wireCubeMesh = uploadPosColLines(lv);
    }

    // RGB 3D transform arrows matching the Editor viewport gizmo
    {
        std::vector<float> gv;
        auto seg = [&](AstronVec3 a, AstronVec3 b, float r, float g, float bl) {
            gv.insert(gv.end(), { a.x, a.y, a.z, r, g, bl, 1.0f });
            gv.insert(gv.end(), { b.x, b.y, b.z, r, g, bl, 1.0f });
        };
        // X axis (Red) + arrow head
        seg({0,0,0}, {2.2f,0,0}, 0.95f, 0.20f, 0.20f);
        seg({2.2f,0,0}, {1.92f, 0.12f, 0}, 0.95f, 0.20f, 0.20f);
        seg({2.2f,0,0}, {1.92f,-0.12f, 0}, 0.95f, 0.20f, 0.20f);
        seg({2.2f,0,0}, {1.92f, 0,  0.12f}, 0.95f, 0.20f, 0.20f);
        seg({2.2f,0,0}, {1.92f, 0, -0.12f}, 0.95f, 0.20f, 0.20f);

        // Y axis (Green) + arrow head
        seg({0,0,0}, {0,2.2f,0}, 0.22f, 0.88f, 0.28f);
        seg({0,2.2f,0}, { 0.12f, 1.92f, 0}, 0.22f, 0.88f, 0.28f);
        seg({0,2.2f,0}, {-0.12f, 1.92f, 0}, 0.22f, 0.88f, 0.28f);
        seg({0,2.2f,0}, {0, 1.92f,  0.12f}, 0.22f, 0.88f, 0.28f);
        seg({0,2.2f,0}, {0, 1.92f, -0.12f}, 0.22f, 0.88f, 0.28f);

        // Z axis (Blue) + arrow head
        seg({0,0,0}, {0,0,2.2f}, 0.18f, 0.52f, 0.98f);
        seg({0,0,2.2f}, { 0.12f, 0, 1.92f}, 0.18f, 0.52f, 0.98f);
        seg({0,0,2.2f}, {-0.12f, 0, 1.92f}, 0.18f, 0.52f, 0.98f);
        seg({0,0,2.2f}, {0,  0.12f, 1.92f}, 0.18f, 0.52f, 0.98f);
        seg({0,0,2.2f}, {0, -0.12f, 1.92f}, 0.18f, 0.52f, 0.98f);

        m_axesGizmoMesh = uploadPosColLines(gv);
    }
}

void Viewport3D::ensureFramebuffer(int w, int h) {
    if (w < 16) w = 16;
    if (h < 16) h = 16;
    if (m_fbo != 0 && m_fbWidth == w && m_fbHeight == h) return;

    if (m_depthRbo) aglDeleteRenderbuffers(1, &m_depthRbo);
    if (m_colorTex) aglDeleteTextures(1, &m_colorTex);
    if (m_fbo) aglDeleteFramebuffers(1, &m_fbo);

    aglGenFramebuffers(1, &m_fbo);
    aglBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    aglGenTextures(1, &m_colorTex);
    aglBindTexture(GL_TEXTURE_2D, m_colorTex);
    aglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    aglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    aglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    aglFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTex, 0);

    aglGenRenderbuffers(1, &m_depthRbo);
    aglBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
    aglRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    aglFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_depthRbo);

    aglBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_fbWidth = w;
    m_fbHeight = h;
}

AstronVec3 Viewport3D::computeCameraEye() const {
    if (is2DMode) {
        return astron_vec3(target.x, target.y + distance * 1.4f, target.z + 0.001f);
    }
    float yr = yawDeg * static_cast<float>(M_PI / 180.0);
    float pr = pitchDeg * static_cast<float>(M_PI / 180.0);
    float cp = std::cos(pr);
    return astron_vec3(
        target.x + distance * cp * std::sin(yr),
        target.y + distance * std::sin(pr),
        target.z + distance * cp * std::cos(yr)
    );
}

AstronMat4 Viewport3D::computeViewMatrix() const {
    AstronVec3 eye = computeCameraEye();
    AstronVec3 up = is2DMode ? astron_vec3(0.f, 0.f, -1.f) : astron_vec3(0.f, 1.f, 0.f);
    return astron_mat4_look_at(eye, target, up);
}

AstronMat4 Viewport3D::computeProjMatrix(float aspect) const {
    if (aspect < 0.05f) aspect = 1.0f;
    if (is2DMode) {
        float halfH = distance * 0.55f;
        float halfW = halfH * aspect;
        return astron_mat4_ortho(-halfW, halfW, -halfH, halfH, 0.1f, 300.0f);
    }
    return astron_mat4_perspective(46.0f, aspect, 0.1f, 400.0f);
}

void Viewport3D::orbit(float deltaYawDeg, float deltaPitchDeg) {
    if (is2DMode) return;
    yawDeg += deltaYawDeg;
    pitchDeg += deltaPitchDeg;
    if (pitchDeg > 86.0f) pitchDeg = 86.0f;
    if (pitchDeg < -80.0f) pitchDeg = -80.0f;
}

void Viewport3D::pan(float dx, float dy) {
    AstronVec3 eye = computeCameraEye();
    AstronVec3 fwd = astron_vec3_normalize(astron_vec3_sub(target, eye));
    AstronVec3 right = astron_vec3_normalize(astron_vec3_cross(fwd, astron_vec3(0.f, 1.f, 0.f)));
    AstronVec3 up = astron_vec3_cross(right, fwd);
    float scale = distance * 0.0018f;
    target = astron_vec3_add(target, astron_vec3_scale(right, -dx * scale));
    target = astron_vec3_add(target, astron_vec3_scale(up, dy * scale));
}

void Viewport3D::zoom(float wheelDelta) {
    distance *= std::pow(0.88f, wheelDelta);
    if (distance < 1.5f) distance = 1.5f;
    if (distance > 120.0f) distance = 120.0f;
}

void Viewport3D::resetCamera() {
    yawDeg = -42.0f;
    pitchDeg = 22.0f;
    distance = 9.5f;
    target = { 0.0f, 0.4f, 0.0f };
}

int Viewport3D::pickObject(const Scene& scene, float mouseX, float mouseY, int vpW, int vpH) const {
    if (vpW <= 0 || vpH <= 0) return -1;
    AstronMat4 vp = astron_mat4_mul(computeProjMatrix(static_cast<float>(vpW) / vpH), computeViewMatrix());
    AstronMat4 invVp;
    if (!astron_mat4_invert(vp, &invVp)) return -1;

    AstronRay ray = astron_unproject_ray(mouseX, mouseY, static_cast<float>(vpW), static_cast<float>(vpH), invVp);

    int bestIdx = -1;
    float bestT = 1e30f;
    for (size_t i = 0; i < scene.objects.size(); ++i) {
        const auto& o = scene.objects[i];
        if (!o.visible) continue;
        AstronVec3 hs = astron_vec3_scale(o.scale, 0.5f);
        AstronAABB box = {
            astron_vec3_sub(o.position, hs),
            astron_vec3_add(o.position, hs)
        };
        float t = 0.0f;
        if (astron_ray_intersect_aabb(ray, box, &t) && t >= 0.0f && t < bestT) {
            bestT = t;
            bestIdx = static_cast<int>(i);
        }
    }
    return bestIdx;
}

void Viewport3D::render(const Scene& scene, int width, int height, float timeSec) {
    if (!m_initialized && !init()) return;
    ensureFramebuffer(width, height);

    aglBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    aglViewport(0, 0, m_fbWidth, m_fbHeight);
    aglEnable(GL_DEPTH_TEST);
    aglDepthFunc(GL_LEQUAL);
    aglEnable(GL_BLEND);
    aglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    aglClearColor(0.07f, 0.075f, 0.088f, 1.0f);
    aglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const float aspect = static_cast<float>(m_fbWidth) / static_cast<float>(m_fbHeight);
    const AstronMat4 view = computeViewMatrix();
    const AstronMat4 proj = computeProjMatrix(aspect);
    const AstronMat4 viewProj = astron_mat4_mul(proj, view);
    AstronMat4 invViewProj = astron_mat4_identity();
    astron_mat4_invert(viewProj, &invViewProj);
    const AstronVec3 eye = computeCameraEye();

    // 1. Atmospheric Sky + Infinite 3D Grid + X/Z Axes
    aglUseProgram(m_skyGridProg);
    aglUniformMatrix4fv(aglGetUniformLocation(m_skyGridProg, "uInvViewProj"), 1, GL_FALSE, invViewProj.m);
    aglUniformMatrix4fv(aglGetUniformLocation(m_skyGridProg, "uViewProj"), 1, GL_FALSE, viewProj.m);
    aglUniform3f(aglGetUniformLocation(m_skyGridProg, "uCamPos"), eye.x, eye.y, eye.z);
    aglUniform1i(aglGetUniformLocation(m_skyGridProg, "uShowGrid"), showGrid ? 1 : 0);
    aglBindVertexArray(m_quadMesh.vao);
    aglDrawArrays(GL_TRIANGLES, 0, m_quadMesh.vertexCount);

    // 2. 3D Scene Objects
    GLuint activeLit = (m_useCustomLit && m_customLitProg != 0) ? m_customLitProg : m_litProg;
    aglUseProgram(activeLit);
    aglUniform3f(aglGetUniformLocation(activeLit, "uCamPos"), eye.x, eye.y, eye.z);
    GLint timeLoc = aglGetUniformLocation(activeLit, "uTime");
    if (timeLoc >= 0) aglUniform1f(timeLoc, timeSec);

    if (wireframe) aglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    for (size_t i = 0; i < scene.objects.size(); ++i) {
        const auto& o = scene.objects[i];
        if (!o.visible || o.mesh == MeshKind::Camera || o.mesh == MeshKind::Light) continue;

        AstronMat4 model = astron_mat4_trs(o.position, o.rotation, o.scale);
        AstronMat4 mvp   = astron_mat4_mul(viewProj, model);

        AstronVec4 col = { 1.f, 1.f, 1.f, 1.f };
        astron_hex_to_rgba(o.colorHex.c_str(), &col);
        col.w = 1.0f - fminf(1.0f, fmaxf(0.0f, o.transparency));

        // Subtle translucent studio look for default center cube if untouched, matching Editor reference
        if (i == 0 && static_cast<int>(i) == scene.selectedIndex && o.transparency == 0.0f && scene.objects.size() == 1) {
            col.w = 0.28f;
        }

        aglUniformMatrix4fv(aglGetUniformLocation(activeLit, "uMVP"), 1, GL_FALSE, mvp.m);
        aglUniformMatrix4fv(aglGetUniformLocation(activeLit, "uModel"), 1, GL_FALSE, model.m);
        aglUniform4f(aglGetUniformLocation(activeLit, "uColor"), col.x, col.y, col.z, col.w);
        aglUniform1i(aglGetUniformLocation(activeLit, "uMaterialType"), static_cast<int>(o.material));

        const GpuMesh* mesh = &m_cubeMesh;
        if (o.mesh == MeshKind::Sphere) mesh = &m_sphereMesh;
        else if (o.mesh == MeshKind::Cylinder) mesh = &m_cylinderMesh;

        aglBindVertexArray(mesh->vao);
        aglDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, nullptr);
    }

    if (wireframe) aglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // 3. Selection Wireframe Box + 3D RGB Transform Gizmo Arrows
    const SceneObject* sel = scene.selected();
    if (sel && showGizmo) {
        aglUseProgram(m_gizmoProg);

        // Selection box
        AstronMat4 boxModel = astron_mat4_trs(sel->position, sel->rotation, sel->scale);
        AstronMat4 boxMvp   = astron_mat4_mul(viewProj, boxModel);
        aglUniformMatrix4fv(aglGetUniformLocation(m_gizmoProg, "uMVP"), 1, GL_FALSE, boxMvp.m);
        aglLineWidth(1.5f);
        aglBindVertexArray(m_wireCubeMesh.vao);
        aglDrawArrays(GL_LINES, 0, m_wireCubeMesh.vertexCount);

        // RGB Transform Arrows (drawn on top without depth occlusion)
        aglDisable(GL_DEPTH_TEST);
        AstronMat4 gizModel = astron_mat4_translate(sel->position);
        AstronMat4 gizMvp   = astron_mat4_mul(viewProj, gizModel);
        aglUniformMatrix4fv(aglGetUniformLocation(m_gizmoProg, "uMVP"), 1, GL_FALSE, gizMvp.m);
        aglLineWidth(2.6f);
        aglBindVertexArray(m_axesGizmoMesh.vao);
        aglDrawArrays(GL_LINES, 0, m_axesGizmoMesh.vertexCount);
        aglLineWidth(1.0f);
        aglEnable(GL_DEPTH_TEST);
    }

    aglBindVertexArray(0);
    aglUseProgram(0);
    aglBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace astron
