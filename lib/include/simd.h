/* simd.h — Phase 78: SIMD Vector Operations
 *
 * Vector math library with SIMD-friendly data layouts and batch processing
 * utilities for performance-critical graphics and physics operations.
 */

#ifndef SIMD_H
#define SIMD_H

/* Vector types (aligned for SIMD) */
typedef struct {
    float x, y;
} __attribute__((aligned(8))) vec2_t;

typedef struct {
    float x, y, z;
} __attribute__((aligned(16))) vec3_t;

typedef struct {
    float x, y, z, w;
} __attribute__((aligned(16))) vec4_t;

typedef struct {
    int x, y;
} __attribute__((aligned(8))) ivec2_t;

typedef struct {
    int x, y, z;
} __attribute__((aligned(16))) ivec3_t;

typedef struct {
    int x, y, z, w;
} __attribute__((aligned(16))) ivec4_t;

/* ===== Vector 2D Operations ===== */

vec2_t vec2_create(float x, float y);
vec2_t vec2_add(vec2_t a, vec2_t b);
vec2_t vec2_sub(vec2_t a, vec2_t b);
vec2_t vec2_mul(vec2_t a, float scalar);
vec2_t vec2_div(vec2_t a, float scalar);
float vec2_dot(vec2_t a, vec2_t b);
float vec2_length(vec2_t v);
float vec2_distance(vec2_t a, vec2_t b);
vec2_t vec2_normalize(vec2_t v);
vec2_t vec2_lerp(vec2_t a, vec2_t b, float t);
vec2_t vec2_reflect(vec2_t v, vec2_t normal);
vec2_t vec2_rotate(vec2_t v, float angle);
vec2_t vec2_clamp(vec2_t v, vec2_t min_v, vec2_t max_v);

/* ===== Vector 3D Operations ===== */

vec3_t vec3_create(float x, float y, float z);
vec3_t vec3_add(vec3_t a, vec3_t b);
vec3_t vec3_sub(vec3_t a, vec3_t b);
vec3_t vec3_mul(vec3_t a, float scalar);
vec3_t vec3_div(vec3_t a, float scalar);
float vec3_dot(vec3_t a, vec3_t b);
vec3_t vec3_cross(vec3_t a, vec3_t b);
float vec3_length(vec3_t v);
float vec3_distance(vec3_t a, vec3_t b);
vec3_t vec3_normalize(vec3_t v);
vec3_t vec3_lerp(vec3_t a, vec3_t b, float t);
vec3_t vec3_reflect(vec3_t v, vec3_t normal);
vec3_t vec3_clamp(vec3_t v, vec3_t min_v, vec3_t max_v);

/* ===== Vector 4D Operations ===== */

vec4_t vec4_create(float x, float y, float z, float w);
vec4_t vec4_add(vec4_t a, vec4_t b);
vec4_t vec4_sub(vec4_t a, vec4_t b);
vec4_t vec4_mul(vec4_t a, float scalar);
vec4_t vec4_div(vec4_t a, float scalar);
float vec4_dot(vec4_t a, vec4_t b);
float vec4_length(vec4_t v);
vec4_t vec4_normalize(vec4_t v);
vec4_t vec4_lerp(vec4_t a, vec4_t b, float t);

/* ===== Integer Vector Operations ===== */

ivec2_t ivec2_create(int x, int y);
ivec2_t ivec2_add(ivec2_t a, ivec2_t b);
ivec2_t ivec2_sub(ivec2_t a, ivec2_t b);
ivec2_t ivec2_mul(ivec2_t a, int scalar);
int ivec2_dot(ivec2_t a, ivec2_t b);

ivec3_t ivec3_create(int x, int y, int z);
ivec3_t ivec3_add(ivec3_t a, ivec3_t b);
ivec3_t ivec3_sub(ivec3_t a, ivec3_t b);
ivec3_t ivec3_mul(ivec3_t a, int scalar);
int ivec3_dot(ivec3_t a, ivec3_t b);

/* ===== Batch Processing ===== */

/* Apply operation to vector array (SIMD-friendly) */
void simd_vec2_add_batch(vec2_t *dest, const vec2_t *a, const vec2_t *b, int count);
void simd_vec3_add_batch(vec3_t *dest, const vec3_t *a, const vec3_t *b, int count);
void simd_vec2_mul_batch(vec2_t *dest, const vec2_t *src, float scalar, int count);
void simd_vec3_mul_batch(vec3_t *dest, const vec3_t *src, float scalar, int count);

/* Batch normalize vectors */
int simd_vec2_normalize_batch(vec2_t *vectors, int count);
int simd_vec3_normalize_batch(vec3_t *vectors, int count);

/* Batch min/max */
vec2_t simd_vec2_min_batch(const vec2_t *vectors, int count);
vec2_t simd_vec2_max_batch(const vec2_t *vectors, int count);
vec3_t simd_vec3_min_batch(const vec3_t *vectors, int count);
vec3_t simd_vec3_max_batch(const vec3_t *vectors, int count);

#endif
