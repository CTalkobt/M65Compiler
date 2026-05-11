/* util.h — Shared declarations for multi_module example */

#pragma once

/* vec2d — simple 2D vector type */
typedef struct {
    int x;
    int y;
} vec2d;

/* Vector operations (vec.c) */
vec2d vec_add(vec2d a, vec2d b);
vec2d vec_scale(vec2d v, int factor);
int   vec_dot(vec2d a, vec2d b);
int   vec_magnitude_sq(vec2d v);

/* Formatting (format.c) */
void print_vec(char *label, vec2d v);
void print_int(char *label, int value);
