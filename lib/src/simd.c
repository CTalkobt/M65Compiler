/* simd.c — SIMD Vector Operations Implementation */

#include "simd.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ===== Vector 2D Implementation ===== */

vec2_t vec2_create(float x, float y) {
    vec2_t v = {x, y};
    return v;
}

vec2_t vec2_add(vec2_t a, vec2_t b) {
    vec2_t result = {a.x + b.x, a.y + b.y};
    return result;
}

vec2_t vec2_sub(vec2_t a, vec2_t b) {
    vec2_t result = {a.x - b.x, a.y - b.y};
    return result;
}

vec2_t vec2_mul(vec2_t a, float scalar) {
    vec2_t result = {a.x * scalar, a.y * scalar};
    return result;
}

vec2_t vec2_div(vec2_t a, float scalar) {
    if (scalar == 0.0f) return a;
    vec2_t result = {a.x / scalar, a.y / scalar};
    return result;
}

float vec2_dot(vec2_t a, vec2_t b) {
    return a.x * b.x + a.y * b.y;
}

float vec2_length(vec2_t v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

float vec2_distance(vec2_t a, vec2_t b) {
    vec2_t diff = vec2_sub(a, b);
    return vec2_length(diff);
}

vec2_t vec2_normalize(vec2_t v) {
    float len = vec2_length(v);
    if (len == 0.0f) return v;
    return vec2_div(v, len);
}

vec2_t vec2_lerp(vec2_t a, vec2_t b, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    vec2_t result = {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t
    };
    return result;
}

vec2_t vec2_reflect(vec2_t v, vec2_t normal) {
    float d = vec2_dot(v, normal);
    vec2_t result = {
        v.x - 2.0f * d * normal.x,
        v.y - 2.0f * d * normal.y
    };
    return result;
}

vec2_t vec2_rotate(vec2_t v, float angle) {
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    vec2_t result = {
        v.x * cos_a - v.y * sin_a,
        v.x * sin_a + v.y * cos_a
    };
    return result;
}

vec2_t vec2_clamp(vec2_t v, vec2_t min_v, vec2_t max_v) {
    vec2_t result;
    result.x = v.x < min_v.x ? min_v.x : (v.x > max_v.x ? max_v.x : v.x);
    result.y = v.y < min_v.y ? min_v.y : (v.y > max_v.y ? max_v.y : v.y);
    return result;
}

/* ===== Vector 3D Implementation ===== */

vec3_t vec3_create(float x, float y, float z) {
    vec3_t v = {x, y, z};
    return v;
}

vec3_t vec3_add(vec3_t a, vec3_t b) {
    vec3_t result = {a.x + b.x, a.y + b.y, a.z + b.z};
    return result;
}

vec3_t vec3_sub(vec3_t a, vec3_t b) {
    vec3_t result = {a.x - b.x, a.y - b.y, a.z - b.z};
    return result;
}

vec3_t vec3_mul(vec3_t a, float scalar) {
    vec3_t result = {a.x * scalar, a.y * scalar, a.z * scalar};
    return result;
}

vec3_t vec3_div(vec3_t a, float scalar) {
    if (scalar == 0.0f) return a;
    vec3_t result = {a.x / scalar, a.y / scalar, a.z / scalar};
    return result;
}

float vec3_dot(vec3_t a, vec3_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3_t vec3_cross(vec3_t a, vec3_t b) {
    vec3_t result = {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
    return result;
}

float vec3_length(vec3_t v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

float vec3_distance(vec3_t a, vec3_t b) {
    vec3_t diff = vec3_sub(a, b);
    return vec3_length(diff);
}

vec3_t vec3_normalize(vec3_t v) {
    float len = vec3_length(v);
    if (len == 0.0f) return v;
    return vec3_div(v, len);
}

vec3_t vec3_lerp(vec3_t a, vec3_t b, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    vec3_t result = {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    };
    return result;
}

vec3_t vec3_reflect(vec3_t v, vec3_t normal) {
    float d = vec3_dot(v, normal);
    vec3_t result = {
        v.x - 2.0f * d * normal.x,
        v.y - 2.0f * d * normal.y,
        v.z - 2.0f * d * normal.z
    };
    return result;
}

vec3_t vec3_clamp(vec3_t v, vec3_t min_v, vec3_t max_v) {
    vec3_t result;
    result.x = v.x < min_v.x ? min_v.x : (v.x > max_v.x ? max_v.x : v.x);
    result.y = v.y < min_v.y ? min_v.y : (v.y > max_v.y ? max_v.y : v.y);
    result.z = v.z < min_v.z ? min_v.z : (v.z > max_v.z ? max_v.z : v.z);
    return result;
}

/* ===== Vector 4D Implementation ===== */

vec4_t vec4_create(float x, float y, float z, float w) {
    vec4_t v = {x, y, z, w};
    return v;
}

vec4_t vec4_add(vec4_t a, vec4_t b) {
    vec4_t result = {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
    return result;
}

vec4_t vec4_sub(vec4_t a, vec4_t b) {
    vec4_t result = {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
    return result;
}

vec4_t vec4_mul(vec4_t a, float scalar) {
    vec4_t result = {a.x * scalar, a.y * scalar, a.z * scalar, a.w * scalar};
    return result;
}

vec4_t vec4_div(vec4_t a, float scalar) {
    if (scalar == 0.0f) return a;
    vec4_t result = {a.x / scalar, a.y / scalar, a.z / scalar, a.w / scalar};
    return result;
}

float vec4_dot(vec4_t a, vec4_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float vec4_length(vec4_t v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

vec4_t vec4_normalize(vec4_t v) {
    float len = vec4_length(v);
    if (len == 0.0f) return v;
    return vec4_div(v, len);
}

vec4_t vec4_lerp(vec4_t a, vec4_t b, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    vec4_t result = {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    };
    return result;
}

/* ===== Integer Vector Implementation ===== */

ivec2_t ivec2_create(int x, int y) {
    ivec2_t v = {x, y};
    return v;
}

ivec2_t ivec2_add(ivec2_t a, ivec2_t b) {
    ivec2_t result = {a.x + b.x, a.y + b.y};
    return result;
}

ivec2_t ivec2_sub(ivec2_t a, ivec2_t b) {
    ivec2_t result = {a.x - b.x, a.y - b.y};
    return result;
}

ivec2_t ivec2_mul(ivec2_t a, int scalar) {
    ivec2_t result = {a.x * scalar, a.y * scalar};
    return result;
}

int ivec2_dot(ivec2_t a, ivec2_t b) {
    return a.x * b.x + a.y * b.y;
}

ivec3_t ivec3_create(int x, int y, int z) {
    ivec3_t v = {x, y, z};
    return v;
}

ivec3_t ivec3_add(ivec3_t a, ivec3_t b) {
    ivec3_t result = {a.x + b.x, a.y + b.y, a.z + b.z};
    return result;
}

ivec3_t ivec3_sub(ivec3_t a, ivec3_t b) {
    ivec3_t result = {a.x - b.x, a.y - b.y, a.z - b.z};
    return result;
}

ivec3_t ivec3_mul(ivec3_t a, int scalar) {
    ivec3_t result = {a.x * scalar, a.y * scalar, a.z * scalar};
    return result;
}

int ivec3_dot(ivec3_t a, ivec3_t b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/* ===== Batch Processing Implementation ===== */

void simd_vec2_add_batch(vec2_t *dest, const vec2_t *a, const vec2_t *b, int count) {
    for (int i = 0; i < count; i++) {
        dest[i] = vec2_add(a[i], b[i]);
    }
}

void simd_vec3_add_batch(vec3_t *dest, const vec3_t *a, const vec3_t *b, int count) {
    for (int i = 0; i < count; i++) {
        dest[i] = vec3_add(a[i], b[i]);
    }
}

void simd_vec2_mul_batch(vec2_t *dest, const vec2_t *src, float scalar, int count) {
    for (int i = 0; i < count; i++) {
        dest[i] = vec2_mul(src[i], scalar);
    }
}

void simd_vec3_mul_batch(vec3_t *dest, const vec3_t *src, float scalar, int count) {
    for (int i = 0; i < count; i++) {
        dest[i] = vec3_mul(src[i], scalar);
    }
}

int simd_vec2_normalize_batch(vec2_t *vectors, int count) {
    for (int i = 0; i < count; i++) {
        vectors[i] = vec2_normalize(vectors[i]);
    }
    return count;
}

int simd_vec3_normalize_batch(vec3_t *vectors, int count) {
    for (int i = 0; i < count; i++) {
        vectors[i] = vec3_normalize(vectors[i]);
    }
    return count;
}

vec2_t simd_vec2_min_batch(const vec2_t *vectors, int count) {
    vec2_t min_v = {1e9f, 1e9f};
    if (count == 0) return min_v;

    min_v = vectors[0];
    for (int i = 1; i < count; i++) {
        if (vectors[i].x < min_v.x) min_v.x = vectors[i].x;
        if (vectors[i].y < min_v.y) min_v.y = vectors[i].y;
    }

    return min_v;
}

vec2_t simd_vec2_max_batch(const vec2_t *vectors, int count) {
    vec2_t max_v = {-1e9f, -1e9f};
    if (count == 0) return max_v;

    max_v = vectors[0];
    for (int i = 1; i < count; i++) {
        if (vectors[i].x > max_v.x) max_v.x = vectors[i].x;
        if (vectors[i].y > max_v.y) max_v.y = vectors[i].y;
    }

    return max_v;
}

vec3_t simd_vec3_min_batch(const vec3_t *vectors, int count) {
    vec3_t min_v = {1e9f, 1e9f, 1e9f};
    if (count == 0) return min_v;

    min_v = vectors[0];
    for (int i = 1; i < count; i++) {
        if (vectors[i].x < min_v.x) min_v.x = vectors[i].x;
        if (vectors[i].y < min_v.y) min_v.y = vectors[i].y;
        if (vectors[i].z < min_v.z) min_v.z = vectors[i].z;
    }

    return min_v;
}

vec3_t simd_vec3_max_batch(const vec3_t *vectors, int count) {
    vec3_t max_v = {-1e9f, -1e9f, -1e9f};
    if (count == 0) return max_v;

    max_v = vectors[0];
    for (int i = 1; i < count; i++) {
        if (vectors[i].x > max_v.x) max_v.x = vectors[i].x;
        if (vectors[i].y > max_v.y) max_v.y = vectors[i].y;
        if (vectors[i].z > max_v.z) max_v.z = vectors[i].z;
    }

    return max_v;
}
