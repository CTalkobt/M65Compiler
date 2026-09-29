/* sprite_postprocessing.c — Post-Processing Implementation */

#include "sprite_postprocessing.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define MAX_PP_CHAINS 4
#define MAX_TONE_MAPPERS 8
#define MAX_GRADERS 8
#define MAX_TEMPORAL_EFFECTS 8
#define MAX_LUTS 4

typedef struct {
    void *effects[16];
    int effect_count;
    int width, height;
} pp_chain_impl;

typedef struct {
    sprite_tone_map_type_t type;
    float exposure;
    float gamma;
} tone_mapper_impl;

typedef struct {
    float brightness;
    float contrast;
    float saturation;
    float hue;
} color_grader_impl;

typedef struct {
    sprite_temporal_type_t type;
    float intensity;
} temporal_effect_impl;

typedef struct {
    int *colors;
    int size;
} lut_impl;

static pp_chain_impl pp_chains[MAX_PP_CHAINS];
static int pp_chain_count = 0;

static tone_mapper_impl tone_mappers[MAX_TONE_MAPPERS];
static int tone_mapper_count = 0;

static color_grader_impl graders[MAX_GRADERS];
static int grader_count = 0;

static temporal_effect_impl temporal_effects[MAX_TEMPORAL_EFFECTS];
static int temporal_effect_count = 0;

static lut_impl luts[MAX_LUTS];
static int lut_count = 0;

sprite_postprocess_chain_t sprite_postprocess_chain_create(int width, int height) {
    if (pp_chain_count >= MAX_PP_CHAINS) return INVALID_PP_CHAIN;

    pp_chain_impl *chain = &pp_chains[pp_chain_count];
    chain->effect_count = 0;
    chain->width = width;
    chain->height = height;

    return (sprite_postprocess_chain_t)(intptr_t)pp_chain_count++;
}

void sprite_postprocess_chain_destroy(sprite_postprocess_chain_t chain) {}

int sprite_postprocess_chain_apply(sprite_postprocess_chain_t chain, void *framebuffer) {
    intptr_t idx = (intptr_t)chain;
    if (idx < 0 || idx >= pp_chain_count) return 0;

    pp_chain_impl *c = &pp_chains[idx];
    return c->effect_count;
}

int sprite_postprocess_chain_add_effect(sprite_postprocess_chain_t chain, void *effect) {
    intptr_t idx = (intptr_t)chain;
    if (idx < 0 || idx >= pp_chain_count) return 0;

    pp_chain_impl *c = &pp_chains[idx];
    if (c->effect_count >= 16) return 0;

    c->effects[c->effect_count++] = effect;
    return 1;
}

sprite_tone_mapper_t sprite_tone_mapper_create(sprite_tone_map_type_t type) {
    if (tone_mapper_count >= MAX_TONE_MAPPERS) return INVALID_TONE_MAPPER;

    tone_mapper_impl *mapper = &tone_mappers[tone_mapper_count];
    mapper->type = type;
    mapper->exposure = 1.0f;
    mapper->gamma = 2.2f;

    return (sprite_tone_mapper_t)(intptr_t)tone_mapper_count++;
}

void sprite_tone_mapper_destroy(sprite_tone_mapper_t mapper) {}

void sprite_tone_mapper_set_exposure(sprite_tone_mapper_t mapper, float exposure) {
    intptr_t idx = (intptr_t)mapper;
    if (idx < 0 || idx >= tone_mapper_count) return;
    tone_mappers[idx].exposure = exposure;
}

void sprite_tone_mapper_set_gamma(sprite_tone_mapper_t mapper, float gamma) {
    intptr_t idx = (intptr_t)mapper;
    if (idx < 0 || idx >= tone_mapper_count) return;
    tone_mappers[idx].gamma = gamma;
}

int sprite_tone_mapper_apply(sprite_tone_mapper_t mapper, int *colors, int count) {
    intptr_t idx = (intptr_t)mapper;
    if (idx < 0 || idx >= tone_mapper_count) return 0;

    tone_mapper_impl *m = &tone_mappers[idx];
    for (int i = 0; i < count; i++) {
        int r = (colors[i] >> 16) & 0xFF;
        int g = (colors[i] >> 8) & 0xFF;
        int b = colors[i] & 0xFF;

        float fr = (float)r / 255.0f * m->exposure;
        float fg = (float)g / 255.0f * m->exposure;
        float fb = (float)b / 255.0f * m->exposure;

        fr = powf(fr, 1.0f / m->gamma);
        fg = powf(fg, 1.0f / m->gamma);
        fb = powf(fb, 1.0f / m->gamma);

        int out_r = (int)(fr * 255.0f) & 0xFF;
        int out_g = (int)(fg * 255.0f) & 0xFF;
        int out_b = (int)(fb * 255.0f) & 0xFF;

        colors[i] = (out_r << 16) | (out_g << 8) | out_b;
    }

    return count;
}

sprite_color_grader_t sprite_color_grader_create(void) {
    if (grader_count >= MAX_GRADERS) return INVALID_GRADER;

    color_grader_impl *grader = &graders[grader_count];
    grader->brightness = 0.0f;
    grader->contrast = 1.0f;
    grader->saturation = 1.0f;
    grader->hue = 0.0f;

    return (sprite_color_grader_t)(intptr_t)grader_count++;
}

void sprite_color_grader_destroy(sprite_color_grader_t grader) {}

void sprite_color_grader_set_brightness(sprite_color_grader_t grader, float brightness) {
    intptr_t idx = (intptr_t)grader;
    if (idx < 0 || idx >= grader_count) return;
    graders[idx].brightness = brightness;
}

void sprite_color_grader_set_contrast(sprite_color_grader_t grader, float contrast) {
    intptr_t idx = (intptr_t)grader;
    if (idx < 0 || idx >= grader_count) return;
    graders[idx].contrast = contrast;
}

void sprite_color_grader_set_saturation(sprite_color_grader_t grader, float saturation) {
    intptr_t idx = (intptr_t)grader;
    if (idx < 0 || idx >= grader_count) return;
    graders[idx].saturation = saturation;
}

void sprite_color_grader_set_hue_shift(sprite_color_grader_t grader, float hue) {
    intptr_t idx = (intptr_t)grader;
    if (idx < 0 || idx >= grader_count) return;
    graders[idx].hue = hue;
}

int sprite_color_grader_apply(sprite_color_grader_t grader, int *colors, int count) {
    intptr_t idx = (intptr_t)grader;
    if (idx < 0 || idx >= grader_count) return 0;

    color_grader_impl *g = &graders[idx];
    for (int i = 0; i < count; i++) {
        int r = (colors[i] >> 16) & 0xFF;
        int gr = (colors[i] >> 8) & 0xFF;
        int b = colors[i] & 0xFF;

        r = (int)(r * g->contrast + g->brightness * 255.0f) & 0xFF;
        gr = (int)(gr * g->contrast + g->brightness * 255.0f) & 0xFF;
        b = (int)(b * g->contrast + g->brightness * 255.0f) & 0xFF;

        colors[i] = (r << 16) | (gr << 8) | b;
    }

    return count;
}

sprite_temporal_effect_t sprite_temporal_effect_create(sprite_temporal_type_t type) {
    if (temporal_effect_count >= MAX_TEMPORAL_EFFECTS) return INVALID_TEMPORAL;

    temporal_effect_impl *effect = &temporal_effects[temporal_effect_count];
    effect->type = type;
    effect->intensity = 1.0f;

    return (sprite_temporal_effect_t)(intptr_t)temporal_effect_count++;
}

void sprite_temporal_effect_destroy(sprite_temporal_effect_t effect) {}

void sprite_temporal_effect_set_intensity(sprite_temporal_effect_t effect, float intensity) {
    intptr_t idx = (intptr_t)effect;
    if (idx < 0 || idx >= temporal_effect_count) return;
    temporal_effects[idx].intensity = intensity;
}

int sprite_temporal_effect_apply(sprite_temporal_effect_t effect, void *framebuffer,
                                  void *prev_framebuffer, int width, int height) {
    intptr_t idx = (intptr_t)effect;
    if (idx < 0 || idx >= temporal_effect_count) return 0;

    temporal_effect_impl *e = &temporal_effects[idx];
    switch (e->type) {
        case TEMPORAL_MOTION_BLUR:
        case TEMPORAL_FRAME_BLEND:
        case TEMPORAL_ANTI_ALIAS:
            break;
    }
    return 1;
}

sprite_lut_t sprite_lut_create(int size) {
    if (lut_count >= MAX_LUTS) return INVALID_TEMPORAL;

    lut_impl *lut = &luts[lut_count];
    lut->colors = (int *)malloc(size * sizeof(int));
    lut->size = size;
    memset(lut->colors, 0xFF, size * sizeof(int));

    return (sprite_lut_t)(intptr_t)lut_count++;
}

void sprite_lut_destroy(sprite_lut_t lut) {
    intptr_t idx = (intptr_t)lut;
    if (idx < 0 || idx >= lut_count) return;
    free(luts[idx].colors);
}

void sprite_lut_set_color(sprite_lut_t lut, int index, int color) {
    intptr_t idx = (intptr_t)lut;
    if (idx < 0 || idx >= lut_count) return;

    lut_impl *l = &luts[idx];
    if (index >= 0 && index < l->size) {
        l->colors[index] = color;
    }
}

int sprite_lut_apply(sprite_lut_t lut, int *colors, int count) {
    intptr_t idx = (intptr_t)lut;
    if (idx < 0 || idx >= lut_count) return 0;

    lut_impl *l = &luts[idx];
    for (int i = 0; i < count; i++) {
        int color = colors[i];
        int index = color % l->size;
        colors[i] = l->colors[index];
    }

    return count;
}
