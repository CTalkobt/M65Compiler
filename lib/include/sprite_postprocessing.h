/* sprite_postprocessing.h — Phase 55: Post-Processing Effects
 *
 * Advanced post-processing: tone mapping, color grading, temporal effects.
 */

#ifndef SPRITE_POSTPROCESSING_H
#define SPRITE_POSTPROCESSING_H

typedef void *sprite_postprocess_chain_t;
typedef void *sprite_tone_mapper_t;
typedef void *sprite_color_grader_t;
typedef void *sprite_temporal_effect_t;

#define INVALID_PP_CHAIN NULL
#define INVALID_TONE_MAPPER NULL
#define INVALID_GRADER NULL
#define INVALID_TEMPORAL NULL

typedef enum {
    TONE_MAP_LINEAR = 0,
    TONE_MAP_REINHARD = 1,
    TONE_MAP_FILMIC = 2,
    TONE_MAP_ACES = 3,
} sprite_tone_map_type_t;

typedef enum {
    TEMPORAL_MOTION_BLUR = 0,
    TEMPORAL_FRAME_BLEND = 1,
    TEMPORAL_ANTI_ALIAS = 2,
} sprite_temporal_type_t;

/* Post-Processing Chain */
sprite_postprocess_chain_t sprite_postprocess_chain_create(int width, int height);
void sprite_postprocess_chain_destroy(sprite_postprocess_chain_t chain);
int sprite_postprocess_chain_apply(sprite_postprocess_chain_t chain, void *framebuffer);
int sprite_postprocess_chain_add_effect(sprite_postprocess_chain_t chain, void *effect);

/* Tone Mapping */
sprite_tone_mapper_t sprite_tone_mapper_create(sprite_tone_map_type_t type);
void sprite_tone_mapper_destroy(sprite_tone_mapper_t mapper);
void sprite_tone_mapper_set_exposure(sprite_tone_mapper_t mapper, float exposure);
void sprite_tone_mapper_set_gamma(sprite_tone_mapper_t mapper, float gamma);
int sprite_tone_mapper_apply(sprite_tone_mapper_t mapper, int *colors, int count);

/* Color Grading */
sprite_color_grader_t sprite_color_grader_create(void);
void sprite_color_grader_destroy(sprite_color_grader_t grader);
void sprite_color_grader_set_brightness(sprite_color_grader_t grader, float brightness);
void sprite_color_grader_set_contrast(sprite_color_grader_t grader, float contrast);
void sprite_color_grader_set_saturation(sprite_color_grader_t grader, float saturation);
void sprite_color_grader_set_hue_shift(sprite_color_grader_t grader, float hue);
int sprite_color_grader_apply(sprite_color_grader_t grader, int *colors, int count);

/* Temporal Effects */
sprite_temporal_effect_t sprite_temporal_effect_create(sprite_temporal_type_t type);
void sprite_temporal_effect_destroy(sprite_temporal_effect_t effect);
void sprite_temporal_effect_set_intensity(sprite_temporal_effect_t effect, float intensity);
int sprite_temporal_effect_apply(sprite_temporal_effect_t effect, void *framebuffer,
                                  void *prev_framebuffer, int width, int height);

/* LUT-Based Color Correction */
typedef void *sprite_lut_t;

sprite_lut_t sprite_lut_create(int size);
void sprite_lut_destroy(sprite_lut_t lut);
void sprite_lut_set_color(sprite_lut_t lut, int index, int color);
int sprite_lut_apply(sprite_lut_t lut, int *colors, int count);

#endif
