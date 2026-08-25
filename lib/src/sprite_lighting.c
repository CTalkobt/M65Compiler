/* sprite_lighting.c — Lighting System Implementation */

#include "sprite_lighting.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define MAX_LIGHT_SYSTEMS 4
#define MAX_LIGHTS_PER_SYSTEM 32

typedef struct {
    sprite_light_type_t type;
    float x, y, z;
    float r, g, b;
    float intensity;
    float range;
    float dx, dy, dz;
    float angle;
} light_impl;

typedef struct {
    light_impl *lights;
    int light_count;
    int max_lights;
    float ambient_r, ambient_g, ambient_b;
    int shadows_enabled;
} light_system_impl;

static light_system_impl light_systems[MAX_LIGHT_SYSTEMS];
static int light_system_count = 0;

sprite_light_system_t sprite_light_system_create(int max_lights) {
    if (light_system_count >= MAX_LIGHT_SYSTEMS) return INVALID_LIGHT_SYSTEM;

    light_system_impl *sys = &light_systems[light_system_count];
    sys->lights = (light_impl *)malloc(max_lights * sizeof(light_impl));
    sys->light_count = 0;
    sys->max_lights = max_lights;
    sys->ambient_r = sys->ambient_g = sys->ambient_b = 0.2f;
    sys->shadows_enabled = 0;

    return (sprite_light_system_t)(intptr_t)light_system_count++;
}

void sprite_light_system_destroy(sprite_light_system_t system) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= light_system_count) return;
    free(light_systems[idx].lights);
}

void sprite_light_system_set_ambient(sprite_light_system_t system, float r, float g, float b) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= light_system_count) return;
    light_systems[idx].ambient_r = r;
    light_systems[idx].ambient_g = g;
    light_systems[idx].ambient_b = b;
}

int sprite_light_system_update(sprite_light_system_t system, int delta_ms) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= light_system_count) return 0;
    return light_systems[idx].light_count;
}

int sprite_light_system_render(sprite_light_system_t system, void *framebuffer, int width, int height) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= light_system_count) return 0;
    return light_systems[idx].light_count;
}

sprite_light_t sprite_light_create(sprite_light_system_t system, sprite_light_type_t type) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= light_system_count) return INVALID_LIGHT;

    light_system_impl *sys = &light_systems[idx];
    if (sys->light_count >= sys->max_lights) return INVALID_LIGHT;

    light_impl *light = &sys->lights[sys->light_count];
    light->type = type;
    light->x = light->y = light->z = 0.0f;
    light->r = light->g = light->b = 1.0f;
    light->intensity = 1.0f;
    light->range = 10.0f;
    light->dx = light->dy = 0.0f;
    light->dz = -1.0f;
    light->angle = 45.0f;

    sys->light_count++;
    return (sprite_light_t)(intptr_t)(sys->light_count - 1);
}

void sprite_light_destroy(sprite_light_t light) {}

void sprite_light_set_position(sprite_light_t light, float x, float y, float z) {
    intptr_t idx = (intptr_t)light;
    if (idx < 0) return;

    for (int s = 0; s < light_system_count; s++) {
        if (idx < light_systems[s].light_count) {
            light_systems[s].lights[idx].x = x;
            light_systems[s].lights[idx].y = y;
            light_systems[s].lights[idx].z = z;
            return;
        }
    }
}

void sprite_light_set_color(sprite_light_t light, float r, float g, float b) {
    intptr_t idx = (intptr_t)light;
    if (idx < 0) return;

    for (int s = 0; s < light_system_count; s++) {
        if (idx < light_systems[s].light_count) {
            light_systems[s].lights[idx].r = r;
            light_systems[s].lights[idx].g = g;
            light_systems[s].lights[idx].b = b;
            return;
        }
    }
}

void sprite_light_set_intensity(sprite_light_t light, float intensity) {
    intptr_t idx = (intptr_t)light;
    if (idx < 0) return;

    for (int s = 0; s < light_system_count; s++) {
        if (idx < light_systems[s].light_count) {
            light_systems[s].lights[idx].intensity = intensity;
            return;
        }
    }
}

void sprite_light_set_range(sprite_light_t light, float range) {
    intptr_t idx = (intptr_t)light;
    if (idx < 0) return;

    for (int s = 0; s < light_system_count; s++) {
        if (idx < light_systems[s].light_count) {
            light_systems[s].lights[idx].range = range;
            return;
        }
    }
}

void sprite_light_set_direction(sprite_light_t light, float dx, float dy, float dz) {
    intptr_t idx = (intptr_t)light;
    if (idx < 0) return;

    for (int s = 0; s < light_system_count; s++) {
        if (idx < light_systems[s].light_count) {
            light_systems[s].lights[idx].dx = dx;
            light_systems[s].lights[idx].dy = dy;
            light_systems[s].lights[idx].dz = dz;
            return;
        }
    }
}

void sprite_light_set_angle(sprite_light_t light, float angle) {
    intptr_t idx = (intptr_t)light;
    if (idx < 0) return;

    for (int s = 0; s < light_system_count; s++) {
        if (idx < light_systems[s].light_count) {
            light_systems[s].lights[idx].angle = angle;
            return;
        }
    }
}

int sprite_light_system_calculate_lighting(sprite_light_system_t system,
                                            float x, float y, float z,
                                            float *out_r, float *out_g, float *out_b) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= light_system_count || !out_r || !out_g || !out_b) return 0;

    light_system_impl *sys = &light_systems[idx];

    *out_r = sys->ambient_r;
    *out_g = sys->ambient_g;
    *out_b = sys->ambient_b;

    for (int i = 0; i < sys->light_count; i++) {
        light_impl *light = &sys->lights[i];

        float dx = light->x - x;
        float dy = light->y - y;
        float dz = light->z - z;
        float dist = sqrtf(dx*dx + dy*dy + dz*dz);

        if (dist < light->range) {
            float attenuation = 1.0f - (dist / light->range);
            *out_r += light->r * light->intensity * attenuation;
            *out_g += light->g * light->intensity * attenuation;
            *out_b += light->b * light->intensity * attenuation;
        }
    }

    if (*out_r > 1.0f) *out_r = 1.0f;
    if (*out_g > 1.0f) *out_g = 1.0f;
    if (*out_b > 1.0f) *out_b = 1.0f;

    return 1;
}

int sprite_light_system_enable_shadows(sprite_light_system_t system) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= light_system_count) return 0;
    light_systems[idx].shadows_enabled = 1;
    return 1;
}

int sprite_light_system_disable_shadows(sprite_light_system_t system) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= light_system_count) return 0;
    light_systems[idx].shadows_enabled = 0;
    return 1;
}

int sprite_light_system_get_lights_at(sprite_light_system_t system,
                                       float x, float y,
                                       sprite_light_t *lights, int max_lights) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= light_system_count) return 0;

    light_system_impl *sys = &light_systems[idx];
    int count = 0;

    for (int i = 0; i < sys->light_count && count < max_lights; i++) {
        light_impl *light = &sys->lights[i];
        float dx = light->x - x;
        float dy = light->y - y;
        float dist = sqrtf(dx*dx + dy*dy);

        if (dist < light->range) {
            lights[count++] = (sprite_light_t)(intptr_t)i;
        }
    }

    return count;
}
