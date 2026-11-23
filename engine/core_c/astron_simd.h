#ifndef ASTRON_SIMD_H
#define ASTRON_SIMD_H

#include "astron_math.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns active hardware SIMD instruction set ("x86_64 SSE2", "ARM64 NEON", or "C11 Scalar"). */
const char* astron_simd_backend_name(void);

/* Hardware-accelerated 4x4 column-major matrix multiply. */
AstronMat4 astron_simd_mat4_mul(AstronMat4 a, AstronMat4 b);

/* Batch transforms packed XYZ vertices (stride = 3 floats) by a 4x4 matrix. */
void astron_simd_transform_points(
    const AstronMat4* mat,
    const float* inXYZ,
    float* outXYZ,
    size_t count
);

/* Computes tight world-space AABB over a packed XYZ vertex buffer. */
AstronAABB astron_simd_compute_bounds(const float* inXYZ, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* ASTRON_SIMD_H */
