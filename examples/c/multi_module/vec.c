/* vec.c — Vector math operations
 *
 * Demonstrates: struct pass-by-value, struct return, pure functions.
 * This file has no #include <stdio.h> — it only depends on util.h.
 */

#include "util.h"

vec2d vec_add(vec2d a, vec2d b) {
    vec2d result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

vec2d vec_scale(vec2d v, int factor) {
    vec2d result;
    result.x = v.x * factor;
    result.y = v.y * factor;
    return result;
}

int vec_dot(vec2d a, vec2d b) {
    return a.x * b.x + a.y * b.y;
}

int vec_magnitude_sq(vec2d v) {
    return v.x * v.x + v.y * v.y;
}
