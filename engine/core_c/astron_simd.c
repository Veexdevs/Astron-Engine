#include "astron_simd.h"

#if defined(__SSE2__) || (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2))
#define ASTRON_HAS_SSE2 1
#include <emmintrin.h>
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#define ASTRON_HAS_NEON 1
#include <arm_neon.h>
#endif

const char* astron_simd_backend_name(void) {
#if defined(ASTRON_HAS_SSE2)
    return "x86_64 SSE2";
#elif defined(ASTRON_HAS_NEON)
    return "ARM64 NEON";
#else
    return "C11 Scalar";
#endif
}

AstronMat4 astron_simd_mat4_mul(AstronMat4 a, AstronMat4 b) {
#if defined(ASTRON_HAS_SSE2)
    AstronMat4 out;
    __m128 col0 = _mm_loadu_ps(&a.m[0]);
    __m128 col1 = _mm_loadu_ps(&a.m[4]);
    __m128 col2 = _mm_loadu_ps(&a.m[8]);
    __m128 col3 = _mm_loadu_ps(&a.m[12]);

    for (int c = 0; c < 4; ++c) {
        __m128 bx = _mm_set1_ps(b.m[c * 4 + 0]);
        __m128 by = _mm_set1_ps(b.m[c * 4 + 1]);
        __m128 bz = _mm_set1_ps(b.m[c * 4 + 2]);
        __m128 bw = _mm_set1_ps(b.m[c * 4 + 3]);

        __m128 r = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(col0, bx), _mm_mul_ps(col1, by)),
            _mm_add_ps(_mm_mul_ps(col2, bz), _mm_mul_ps(col3, bw))
        );
        _mm_storeu_ps(&out.m[c * 4], r);
    }
    return out;
#elif defined(ASTRON_HAS_NEON)
    AstronMat4 out;
    float32x4_t col0 = vld1q_f32(&a.m[0]);
    float32x4_t col1 = vld1q_f32(&a.m[4]);
    float32x4_t col2 = vld1q_f32(&a.m[8]);
    float32x4_t col3 = vld1q_f32(&a.m[12]);

    for (int c = 0; c < 4; ++c) {
        float32x4_t r = vmulq_n_f32(col0, b.m[c * 4 + 0]);
        r = vmlaq_n_f32(r, col1, b.m[c * 4 + 1]);
        r = vmlaq_n_f32(r, col2, b.m[c * 4 + 2]);
        r = vmlaq_n_f32(r, col3, b.m[c * 4 + 3]);
        vst1q_f32(&out.m[c * 4], r);
    }
    return out;
#else
    return astron_mat4_mul(a, b);
#endif
}

void astron_simd_transform_points(
    const AstronMat4* mat,
    const float* inXYZ,
    float* outXYZ,
    size_t count
) {
    if (!mat || !inXYZ || !outXYZ) return;
#if defined(ASTRON_HAS_SSE2)
    __m128 c0 = _mm_loadu_ps(&mat->m[0]);
    __m128 c1 = _mm_loadu_ps(&mat->m[4]);
    __m128 c2 = _mm_loadu_ps(&mat->m[8]);
    __m128 c3 = _mm_loadu_ps(&mat->m[12]);

    for (size_t i = 0; i < count; ++i) {
        const float* p = inXYZ + i * 3;
        __m128 vx = _mm_set1_ps(p[0]);
        __m128 vy = _mm_set1_ps(p[1]);
        __m128 vz = _mm_set1_ps(p[2]);
        __m128 r = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(c0, vx), _mm_mul_ps(c1, vy)),
            _mm_add_ps(_mm_mul_ps(c2, vz), c3)
        );
        float tmp[4];
        _mm_storeu_ps(tmp, r);
        outXYZ[i * 3 + 0] = tmp[0];
        outXYZ[i * 3 + 1] = tmp[1];
        outXYZ[i * 3 + 2] = tmp[2];
    }
#else
    for (size_t i = 0; i < count; ++i) {
        const float x = inXYZ[i * 3 + 0];
        const float y = inXYZ[i * 3 + 1];
        const float z = inXYZ[i * 3 + 2];
        outXYZ[i * 3 + 0] = mat->m[0] * x + mat->m[4] * y + mat->m[8]  * z + mat->m[12];
        outXYZ[i * 3 + 1] = mat->m[1] * x + mat->m[5] * y + mat->m[9]  * z + mat->m[13];
        outXYZ[i * 3 + 2] = mat->m[2] * x + mat->m[6] * y + mat->m[10] * z + mat->m[14];
    }
#endif
}

AstronAABB astron_simd_compute_bounds(const float* inXYZ, size_t count) {
    AstronAABB box;
    if (!inXYZ || count == 0) {
        box.min = (AstronVec3){ 0.0f, 0.0f, 0.0f };
        box.max = (AstronVec3){ 0.0f, 0.0f, 0.0f };
        return box;
    }
    box.min = (AstronVec3){ inXYZ[0], inXYZ[1], inXYZ[2] };
    box.max = box.min;
    for (size_t i = 1; i < count; ++i) {
        float x = inXYZ[i * 3 + 0];
        float y = inXYZ[i * 3 + 1];
        float z = inXYZ[i * 3 + 2];
        if (x < box.min.x) box.min.x = x;
        if (y < box.min.y) box.min.y = y;
        if (z < box.min.z) box.min.z = z;
        if (x > box.max.x) box.max.x = x;
        if (y > box.max.y) box.max.y = y;
        if (z > box.max.z) box.max.z = z;
    }
    return box;
}
