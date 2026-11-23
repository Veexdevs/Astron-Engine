#include "astron_math.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float deg2rad(float d) { return d * (float)(M_PI / 180.0); }

AstronVec3 astron_vec3(float x, float y, float z) {
    AstronVec3 r = { x, y, z };
    return r;
}

AstronVec3 astron_vec3_add(AstronVec3 a, AstronVec3 b) {
    return astron_vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

AstronVec3 astron_vec3_sub(AstronVec3 a, AstronVec3 b) {
    return astron_vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

AstronVec3 astron_vec3_scale(AstronVec3 v, float s) {
    return astron_vec3(v.x * s, v.y * s, v.z * s);
}

float astron_vec3_dot(AstronVec3 a, AstronVec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

AstronVec3 astron_vec3_cross(AstronVec3 a, AstronVec3 b) {
    return astron_vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

float astron_vec3_length(AstronVec3 v) {
    return sqrtf(astron_vec3_dot(v, v));
}

AstronVec3 astron_vec3_normalize(AstronVec3 v) {
    float len = astron_vec3_length(v);
    if (len < 1e-7f) return astron_vec3(0.0f, 0.0f, 0.0f);
    return astron_vec3_scale(v, 1.0f / len);
}

AstronMat4 astron_mat4_identity(void) {
    AstronMat4 r;
    memset(&r, 0, sizeof(r));
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

AstronMat4 astron_mat4_mul(AstronMat4 a, AstronMat4 b) {
    AstronMat4 r;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            }
            r.m[col * 4 + row] = sum;
        }
    }
    return r;
}

AstronVec4 astron_mat4_mul_vec4(AstronMat4 m, AstronVec4 v) {
    AstronVec4 r;
    r.x = m.m[0] * v.x + m.m[4] * v.y + m.m[8]  * v.z + m.m[12] * v.w;
    r.y = m.m[1] * v.x + m.m[5] * v.y + m.m[9]  * v.z + m.m[13] * v.w;
    r.z = m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14] * v.w;
    r.w = m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z + m.m[15] * v.w;
    return r;
}

AstronMat4 astron_mat4_translate(AstronVec3 t) {
    AstronMat4 r = astron_mat4_identity();
    r.m[12] = t.x;
    r.m[13] = t.y;
    r.m[14] = t.z;
    return r;
}

AstronMat4 astron_mat4_scale(AstronVec3 s) {
    AstronMat4 r = astron_mat4_identity();
    r.m[0]  = s.x;
    r.m[5]  = s.y;
    r.m[10] = s.z;
    return r;
}

AstronMat4 astron_mat4_rotate_xyz_deg(AstronVec3 eulerDeg) {
    float rx = deg2rad(eulerDeg.x);
    float ry = deg2rad(eulerDeg.y);
    float rz = deg2rad(eulerDeg.z);

    float cx = cosf(rx), sx = sinf(rx);
    float cy = cosf(ry), sy = sinf(ry);
    float cz = cosf(rz), sz = sinf(rz);

    AstronMat4 mx = astron_mat4_identity();
    mx.m[5] = cx;  mx.m[9]  = -sx;
    mx.m[6] = sx;  mx.m[10] = cx;

    AstronMat4 my = astron_mat4_identity();
    my.m[0] = cy;  my.m[8]  = sy;
    my.m[2] = -sy; my.m[10] = cy;

    AstronMat4 mz = astron_mat4_identity();
    mz.m[0] = cz;  mz.m[4] = -sz;
    mz.m[1] = sz;  mz.m[5] = cz;

    return astron_mat4_mul(my, astron_mat4_mul(mx, mz));
}

AstronMat4 astron_mat4_trs(AstronVec3 pos, AstronVec3 eulerDeg, AstronVec3 scale) {
    AstronMat4 t = astron_mat4_translate(pos);
    AstronMat4 r = astron_mat4_rotate_xyz_deg(eulerDeg);
    AstronMat4 s = astron_mat4_scale(scale);
    return astron_mat4_mul(t, astron_mat4_mul(r, s));
}

AstronMat4 astron_mat4_perspective(float fovYDeg, float aspect, float zNear, float zFar) {
    AstronMat4 r;
    memset(&r, 0, sizeof(r));
    if (aspect <= 1e-5f) aspect = 1.0f;
    float f = 1.0f / tanf(deg2rad(fovYDeg) * 0.5f);
    r.m[0]  = f / aspect;
    r.m[5]  = f;
    r.m[10] = (zFar + zNear) / (zNear - zFar);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * zFar * zNear) / (zNear - zFar);
    return r;
}

AstronMat4 astron_mat4_ortho(float left, float right, float bottom, float top, float zNear, float zFar) {
    AstronMat4 r = astron_mat4_identity();
    r.m[0]  = 2.0f / (right - left);
    r.m[5]  = 2.0f / (top - bottom);
    r.m[10] = -2.0f / (zFar - zNear);
    r.m[12] = -(right + left) / (right - left);
    r.m[13] = -(top + bottom) / (top - bottom);
    r.m[14] = -(zFar + zNear) / (zFar - zNear);
    return r;
}

AstronMat4 astron_mat4_look_at(AstronVec3 eye, AstronVec3 center, AstronVec3 up) {
    AstronVec3 f = astron_vec3_normalize(astron_vec3_sub(center, eye));
    AstronVec3 s = astron_vec3_normalize(astron_vec3_cross(f, up));
    AstronVec3 u = astron_vec3_cross(s, f);

    AstronMat4 r = astron_mat4_identity();
    r.m[0] = s.x;  r.m[4] = s.y;  r.m[8]  = s.z;
    r.m[1] = u.x;  r.m[5] = u.y;  r.m[9]  = u.z;
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
    r.m[12] = -astron_vec3_dot(s, eye);
    r.m[13] = -astron_vec3_dot(u, eye);
    r.m[14] =  astron_vec3_dot(f, eye);
    return r;
}

bool astron_mat4_invert(AstronMat4 in, AstronMat4* out) {
    const float* m = in.m;
    float inv[16];

    inv[0] = m[5]  * m[10] * m[15] - m[5]  * m[11] * m[14] - m[9]  * m[6]  * m[15] +
             m[9]  * m[7]  * m[14] + m[13] * m[6]  * m[11] - m[13] * m[7]  * m[10];
    inv[4] = -m[4]  * m[10] * m[15] + m[4]  * m[11] * m[14] + m[8]  * m[6]  * m[15] -
              m[8]  * m[7]  * m[14] - m[12] * m[6]  * m[11] + m[12] * m[7]  * m[10];
    inv[8] = m[4]  * m[9] * m[15] - m[4]  * m[11] * m[13] - m[8]  * m[5] * m[15] +
             m[8]  * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4]  * m[9] * m[14] + m[4]  * m[10] * m[13] + m[8]  * m[5] * m[14] -
               m[8]  * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1] = -m[1]  * m[10] * m[15] + m[1]  * m[11] * m[14] + m[9]  * m[2] * m[15] -
              m[9]  * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] = m[0]  * m[10] * m[15] - m[0]  * m[11] * m[14] - m[8]  * m[2] * m[15] +
             m[8]  * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0]  * m[9] * m[15] + m[0]  * m[11] * m[13] + m[8]  * m[1] * m[15] -
              m[8]  * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0]  * m[9] * m[14] - m[0]  * m[10] * m[13] - m[8]  * m[1] * m[14] +
              m[8]  * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1]  * m[6] * m[15] - m[1]  * m[7] * m[14] - m[5]  * m[2] * m[15] +
             m[5]  * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0]  * m[6] * m[15] + m[0]  * m[7] * m[14] + m[4]  * m[2] * m[15] -
              m[4]  * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0]  * m[5] * m[15] - m[0]  * m[7] * m[13] - m[4]  * m[1] * m[15] +
              m[4]  * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0]  * m[5] * m[14] + m[0]  * m[6] * m[13] + m[4]  * m[1] * m[14] -
               m[4]  * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] -
              m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] +
             m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] -
               m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] +
              m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (fabsf(det) < 1e-8f) return false;

    float invDet = 1.0f / det;
    if (out) {
        for (int i = 0; i < 16; ++i) out->m[i] = inv[i] * invDet;
    }
    return true;
}

AstronVec3 astron_project_world_to_screen(AstronVec3 world, AstronMat4 viewProj, float vpW, float vpH, bool* inFront) {
    AstronVec4 p = { world.x, world.y, world.z, 1.0f };
    AstronVec4 c = astron_mat4_mul_vec4(viewProj, p);
    if (inFront) *inFront = (c.w > 0.001f);
    float iw = (fabsf(c.w) > 1e-6f) ? (1.0f / c.w) : 1.0f;
    float nx = c.x * iw;
    float ny = c.y * iw;
    float nz = c.z * iw;
    return astron_vec3((nx * 0.5f + 0.5f) * vpW, (1.0f - (ny * 0.5f + 0.5f)) * vpH, nz);
}

AstronRay astron_unproject_ray(float mouseX, float mouseY, float vpW, float vpH, AstronMat4 invViewProj) {
    float nx = (mouseX / vpW) * 2.0f - 1.0f;
    float ny = 1.0f - (mouseY / vpH) * 2.0f;

    AstronVec4 nearClip = { nx, ny, -1.0f, 1.0f };
    AstronVec4 farClip  = { nx, ny,  1.0f, 1.0f };

    AstronVec4 nw = astron_mat4_mul_vec4(invViewProj, nearClip);
    AstronVec4 fw = astron_mat4_mul_vec4(invViewProj, farClip);

    float inw = (fabsf(nw.w) > 1e-6f) ? (1.0f / nw.w) : 1.0f;
    float ifw = (fabsf(fw.w) > 1e-6f) ? (1.0f / fw.w) : 1.0f;

    AstronVec3 p0 = astron_vec3(nw.x * inw, nw.y * inw, nw.z * inw);
    AstronVec3 p1 = astron_vec3(fw.x * ifw, fw.y * ifw, fw.z * ifw);

    AstronRay ray;
    ray.origin = p0;
    ray.dir = astron_vec3_normalize(astron_vec3_sub(p1, p0));
    return ray;
}

bool astron_ray_intersect_aabb(AstronRay ray, AstronAABB box, float* outT) {
    float tmin = 0.0f;
    float tmax = 1e30f;

    const float orig[3] = { ray.origin.x, ray.origin.y, ray.origin.z };
    const float dir[3]  = { ray.dir.x, ray.dir.y, ray.dir.z };
    const float bmin[3] = { box.min.x, box.min.y, box.min.z };
    const float bmax[3] = { box.max.x, box.max.y, box.max.z };

    for (int i = 0; i < 3; ++i) {
        if (fabsf(dir[i]) < 1e-6f) {
            if (orig[i] < bmin[i] || orig[i] > bmax[i]) return false;
        } else {
            float invD = 1.0f / dir[i];
            float t1 = (bmin[i] - orig[i]) * invD;
            float t2 = (bmax[i] - orig[i]) * invD;
            if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return false;
        }
    }

    if (outT) *outT = tmin;
    return true;
}

static int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    c = (char)tolower((unsigned char)c);
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    return -1;
}

bool astron_hex_to_rgba(const char* hex, AstronVec4* outRgba) {
    if (!hex || !outRgba) return false;
    if (*hex == '#') ++hex;
    size_t len = strlen(hex);
    if (len != 6 && len != 8) return false;

    int vals[8];
    for (size_t i = 0; i < len; ++i) {
        vals[i] = hex_nibble(hex[i]);
        if (vals[i] < 0) return false;
    }

    outRgba->x = (float)(vals[0] * 16 + vals[1]) / 255.0f;
    outRgba->y = (float)(vals[2] * 16 + vals[3]) / 255.0f;
    outRgba->z = (float)(vals[4] * 16 + vals[5]) / 255.0f;
    outRgba->w = (len == 8) ? ((float)(vals[6] * 16 + vals[7]) / 255.0f) : 1.0f;
    return true;
}

void astron_rgba_to_hex(AstronVec4 rgba, char outHex[8]) {
    int r = (int)fminf(255.0f, fmaxf(0.0f, roundf(rgba.x * 255.0f)));
    int g = (int)fminf(255.0f, fmaxf(0.0f, roundf(rgba.y * 255.0f)));
    int b = (int)fminf(255.0f, fmaxf(0.0f, roundf(rgba.z * 255.0f)));
    snprintf(outHex, 8, "#%02X%02X%02X", r, g, b);
}
