/* sprite_lighting.h — Phase 53: Lighting System
 *
 * Dynamic lighting with multiple light sources and shadows.
 */

#ifndef SPRITE_LIGHTING_H
#define SPRITE_LIGHTING_H

typedef void *sprite_light_system_t;
typedef void *sprite_light_t;

#define INVALID_LIGHT_SYSTEM NULL
#define INVALID_LIGHT NULL

typedef enum {
    LIGHT_POINT = 0,
    LIGHT_DIRECTIONAL = 1,
    LIGHT_SPOT = 2,
    LIGHT_AREA = 3,
} sprite_light_type_t;

typedef struct {
    float x, y, z;
    float r, g, b;
    float intensity;
    float range;
} sprite_light_info_t;

/* Lighting System */
sprite_light_system_t sprite_light_system_create(int max_lights);
void sprite_light_system_destroy(sprite_light_system_t system);
void sprite_light_system_set_ambient(sprite_light_system_t system, float r, float g, float b);
int sprite_light_system_update(sprite_light_system_t system, int delta_ms);
int sprite_light_system_render(sprite_light_system_t system, void *framebuffer, int width, int height);

/* Light Sources */
sprite_light_t sprite_light_create(sprite_light_system_t system, sprite_light_type_t type);
void sprite_light_destroy(sprite_light_t light);
void sprite_light_set_position(sprite_light_t light, float x, float y, float z);
void sprite_light_set_color(sprite_light_t light, float r, float g, float b);
void sprite_light_set_intensity(sprite_light_t light, float intensity);
void sprite_light_set_range(sprite_light_t light, float range);
void sprite_light_set_direction(sprite_light_t light, float dx, float dy, float dz);
void sprite_light_set_angle(sprite_light_t light, float angle);

/* Light Calculations */
int sprite_light_system_calculate_lighting(sprite_light_system_t system,
                                            float x, float y, float z,
                                            float *out_r, float *out_g, float *out_b);

/* Shadows */
int sprite_light_system_enable_shadows(sprite_light_system_t system);
int sprite_light_system_disable_shadows(sprite_light_system_t system);

/* Light Culling */
int sprite_light_system_get_lights_at(sprite_light_system_t system,
                                       float x, float y,
                                       sprite_light_t *lights, int max_lights);

#endif
