#ifndef ASTRON_MATH_H
#define ASTRON_MATH_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AstronVec2 { float x, y; } AstronVec2;
typedef struct AstronVec3 { float x, y, z; } AstronVec3;
typedef struct AstronVec4 { float x, y, z, w; } AstronVec4;

/* Column-major 4x4 matrix (OpenGL convention: m[col * 4 + row]) */
typedef struct AstronMat4 { float m[16]; } AstronMat4;

typedef struct AstronRay {
    AstronVec3 origin;
    AstronVec3 dir;
} AstronRay;

typedef struct AstronAABB {
    AstronVec3 min;
    AstronVec3 max;
} AstronAABB;

AstronVec3 astron_vec3(float x, float y, float z);
AstronVec3 astron_vec3_add(AstronVec3 a, AstronVec3 b);
AstronVec3 astron_vec3_sub(AstronVec3 a, AstronVec3 b);
AstronVec3 astron_vec3_scale(AstronVec3 v, float s);
float      astron_vec3_dot(AstronVec3 a, AstronVec3 b);
AstronVec3 astron_vec3_cross(AstronVec3 a, AstronVec3 b);
float      astron_vec3_length(AstronVec3 v);
AstronVec3 astron_vec3_normalize(AstronVec3 v);

AstronMat4 astron_mat4_identity(void);
AstronMat4 astron_mat4_mul(AstronMat4 a, AstronMat4 b);
AstronVec4 astron_mat4_mul_vec4(AstronMat4 m, AstronVec4 v);
AstronMat4 astron_mat4_translate(AstronVec3 t);
AstronMat4 astron_mat4_scale(AstronVec3 s);
AstronMat4 astron_mat4_rotate_xyz_deg(AstronVec3 eulerDeg);
AstronMat4 astron_mat4_trs(AstronVec3 pos, AstronVec3 eulerDeg, AstronVec3 scale);
AstronMat4 astron_mat4_perspective(float fovYDeg, float aspect, float zNear, float zFar);
AstronMat4 astron_mat4_ortho(float left, float right, float bottom, float top, float zNear, float zFar);
AstronMat4 astron_mat4_look_at(AstronVec3 eye, AstronVec3 center, AstronVec3 up);
bool       astron_mat4_invert(AstronMat4 in, AstronMat4* out);

AstronVec3 astron_project_world_to_screen(AstronVec3 world, AstronMat4 viewProj, float vpW, float vpH, bool* inFront);
AstronRay  astron_unproject_ray(float mouseX, float mouseY, float vpW, float vpH, AstronMat4 invViewProj);
bool       astron_ray_intersect_aabb(AstronRay ray, AstronAABB box, float* outT);

bool       astron_hex_to_rgba(const char* hex, AstronVec4* outRgba);
void       astron_rgba_to_hex(AstronVec4 rgba, char outHex[8]);

#ifdef __cplusplus
}
#endif

#endif /* ASTRON_MATH_H */
