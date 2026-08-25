/* sprite_visual_effects.c — Visual Effects Implementation */

#include "sprite_visual_effects.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define MAX_EFFECTS 32
#define MAX_SHADERS 32
#define MAX_FILTERS 16
#define MAX_EFFECT_STACK 8

typedef struct {
    sprite_effect_type_t type;
    float intensity;
    float animation_time;
} effect_impl;

typedef struct {
    sprite_shader_type_t type;
    float params[8];
} shader_impl;

typedef struct {
    sprite_filter_type_t type;
    int scale;
} filter_impl;

typedef struct {
    sprite_effect_t effects[MAX_EFFECT_STACK];
    int effect_count;
} effect_stack_impl;

static effect_impl effects[MAX_EFFECTS];
static int effect_count = 0;

static shader_impl shaders[MAX_SHADERS];
static int shader_count = 0;

static filter_impl filters[MAX_FILTERS];
static int filter_count = 0;

static effect_stack_impl stacks[4];
static int stack_count = 0;

sprite_effect_t sprite_effect_create(sprite_effect_type_t type) {
    if (effect_count >= MAX_EFFECTS) return INVALID_EFFECT;

    effect_impl *effect = &effects[effect_count];
    effect->type = type;
    effect->intensity = 1.0f;
    effect->animation_time = 0.0f;

    return (sprite_effect_t)(intptr_t)effect_count++;
}

void sprite_effect_destroy(sprite_effect_t effect) {}

void sprite_effect_set_intensity(sprite_effect_t effect, float intensity) {
    intptr_t idx = (intptr_t)effect;
    if (idx < 0 || idx >= effect_count) return;
    effects[idx].intensity = intensity;
}

float sprite_effect_get_intensity(sprite_effect_t effect) {
    intptr_t idx = (intptr_t)effect;
    if (idx < 0 || idx >= effect_count) return 0.0f;
    return effects[idx].intensity;
}

int sprite_effect_apply(sprite_effect_t effect, void *framebuffer, int width, int height) {
    intptr_t idx = (intptr_t)effect;
    if (idx < 0 || idx >= effect_count) return 0;

    effect_impl *e = &effects[idx];
    switch (e->type) {
        case EFFECT_BLUR:
        case EFFECT_BLOOM:
        case EFFECT_DISTORTION:
        case EFFECT_CHROMATIC_ABERRATION:
        case EFFECT_VIGNETTE:
            break;
    }
    return 1;
}

int sprite_effect_update(sprite_effect_t effect, int delta_ms) {
    intptr_t idx = (intptr_t)effect;
    if (idx < 0 || idx >= effect_count) return 0;
    effects[idx].animation_time += delta_ms / 1000.0f;
    return 1;
}

sprite_shader_t sprite_shader_create(sprite_shader_type_t type) {
    if (shader_count >= MAX_SHADERS) return INVALID_SHADER;

    shader_impl *shader = &shaders[shader_count];
    shader->type = type;
    memset(shader->params, 0, sizeof(shader->params));

    return (sprite_shader_t)(intptr_t)shader_count++;
}

void sprite_shader_destroy(sprite_shader_t shader) {}

void sprite_shader_set_parameter(sprite_shader_t shader, int param_id, float value) {
    intptr_t idx = (intptr_t)shader;
    if (idx < 0 || idx >= shader_count || param_id < 0 || param_id >= 8) return;
    shaders[idx].params[param_id] = value;
}

static inline int apply_grayscale(int color) {
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;
    int gray = (r + g + b) / 3;
    return (gray << 16) | (gray << 8) | gray;
}

static inline int apply_sepia(int color) {
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;
    int out_r = (int)(0.393f * r + 0.769f * g + 0.189f * b);
    int out_g = (int)(0.349f * r + 0.686f * g + 0.168f * b);
    int out_b = (int)(0.272f * r + 0.534f * g + 0.131f * b);
    return ((out_r & 0xFF) << 16) | ((out_g & 0xFF) << 8) | (out_b & 0xFF);
}

int sprite_shader_apply(sprite_shader_t shader, int *color, int color_count) {
    intptr_t idx = (intptr_t)shader;
    if (idx < 0 || idx >= shader_count) return 0;

    shader_impl *s = &shaders[idx];
    for (int i = 0; i < color_count; i++) {
        switch (s->type) {
            case SHADER_GRAYSCALE:
                color[i] = apply_grayscale(color[i]);
                break;
            case SHADER_SEPIA:
                color[i] = apply_sepia(color[i]);
                break;
            case SHADER_INVERT:
                color[i] = (~color[i]) & 0xFFFFFF;
                break;
            default:
                break;
        }
    }
    return color_count;
}

sprite_filter_t sprite_filter_create(sprite_filter_type_t type) {
    if (filter_count >= MAX_FILTERS) return INVALID_FILTER;

    filter_impl *filter = &filters[filter_count];
    filter->type = type;
    filter->scale = 1;

    return (sprite_filter_t)(intptr_t)filter_count++;
}

void sprite_filter_destroy(sprite_filter_t filter) {}

void sprite_filter_set_scale(sprite_filter_t filter, int scale) {
    intptr_t idx = (intptr_t)filter;
    if (idx < 0 || idx >= filter_count) return;
    filters[idx].scale = scale;
}

int sprite_filter_apply(sprite_filter_t filter, void *framebuffer, int width, int height) {
    intptr_t idx = (intptr_t)filter;
    if (idx < 0 || idx >= filter_count) return 0;

    filter_impl *f = &filters[idx];
    switch (f->type) {
        case FILTER_PIXELATE:
        case FILTER_MOSAIC:
        case FILTER_DITHER:
        case FILTER_SCANLINES:
            break;
    }
    return 1;
}

sprite_effect_stack_t sprite_effect_stack_create(int max_effects) {
    if (stack_count >= 4) return NULL;
    effect_stack_impl *stack = &stacks[stack_count];
    stack->effect_count = 0;
    return (sprite_effect_stack_t)(intptr_t)stack_count++;
}

void sprite_effect_stack_destroy(sprite_effect_stack_t stack) {}

int sprite_effect_stack_push(sprite_effect_stack_t stack, sprite_effect_t effect) {
    intptr_t idx = (intptr_t)stack;
    if (idx < 0 || idx >= stack_count) return 0;

    effect_stack_impl *s = &stacks[idx];
    if (s->effect_count >= MAX_EFFECT_STACK) return 0;

    s->effects[s->effect_count++] = effect;
    return 1;
}

int sprite_effect_stack_pop(sprite_effect_stack_t stack) {
    intptr_t idx = (intptr_t)stack;
    if (idx < 0 || idx >= stack_count) return 0;

    effect_stack_impl *s = &stacks[idx];
    if (s->effect_count > 0) {
        s->effect_count--;
        return 1;
    }
    return 0;
}

int sprite_effect_stack_apply(sprite_effect_stack_t stack, void *framebuffer, int width, int height) {
    intptr_t idx = (intptr_t)stack;
    if (idx < 0 || idx >= stack_count) return 0;

    effect_stack_impl *s = &stacks[idx];
    for (int i = 0; i < s->effect_count; i++) {
        sprite_effect_apply(s->effects[i], framebuffer, width, height);
    }
    return s->effect_count;
}
