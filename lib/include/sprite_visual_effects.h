/* sprite_visual_effects.h — Phase 52: Visual Effects & Shaders
 *
 * Screen effects, sprite shaders, and visual filters.
 */

#ifndef SPRITE_VISUAL_EFFECTS_H
#define SPRITE_VISUAL_EFFECTS_H

typedef void *sprite_effect_t;
typedef void *sprite_shader_t;
typedef void *sprite_filter_t;

#define INVALID_EFFECT NULL
#define INVALID_SHADER NULL
#define INVALID_FILTER NULL

typedef enum {
    EFFECT_BLUR = 0,
    EFFECT_BLOOM = 1,
    EFFECT_DISTORTION = 2,
    EFFECT_CHROMATIC_ABERRATION = 3,
    EFFECT_VIGNETTE = 4,
} sprite_effect_type_t;

typedef enum {
    SHADER_GRAYSCALE = 0,
    SHADER_SEPIA = 1,
    SHADER_INVERT = 2,
    SHADER_HUE_SHIFT = 3,
    SHADER_COLORIZE = 4,
} sprite_shader_type_t;

typedef enum {
    FILTER_PIXELATE = 0,
    FILTER_MOSAIC = 1,
    FILTER_DITHER = 2,
    FILTER_SCANLINES = 3,
} sprite_filter_type_t;

/* Screen Effects */
sprite_effect_t sprite_effect_create(sprite_effect_type_t type);
void sprite_effect_destroy(sprite_effect_t effect);
void sprite_effect_set_intensity(sprite_effect_t effect, float intensity);
float sprite_effect_get_intensity(sprite_effect_t effect);
int sprite_effect_apply(sprite_effect_t effect, void *framebuffer, int width, int height);
int sprite_effect_update(sprite_effect_t effect, int delta_ms);

/* Sprite Shaders */
sprite_shader_t sprite_shader_create(sprite_shader_type_t type);
void sprite_shader_destroy(sprite_shader_t shader);
void sprite_shader_set_parameter(sprite_shader_t shader, int param_id, float value);
int sprite_shader_apply(sprite_shader_t shader, int *color, int color_count);

/* Post-Processing Filters */
sprite_filter_t sprite_filter_create(sprite_filter_type_t type);
void sprite_filter_destroy(sprite_filter_t filter);
void sprite_filter_set_scale(sprite_filter_t filter, int scale);
int sprite_filter_apply(sprite_filter_t filter, void *framebuffer, int width, int height);

/* Effect Stack */
typedef void *sprite_effect_stack_t;

sprite_effect_stack_t sprite_effect_stack_create(int max_effects);
void sprite_effect_stack_destroy(sprite_effect_stack_t stack);
int sprite_effect_stack_push(sprite_effect_stack_t stack, sprite_effect_t effect);
int sprite_effect_stack_pop(sprite_effect_stack_t stack);
int sprite_effect_stack_apply(sprite_effect_stack_t stack, void *framebuffer, int width, int height);

#endif
