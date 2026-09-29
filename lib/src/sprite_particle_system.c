/* sprite_particle_system.c — Particle System Implementation */

#include "sprite_particle_system.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define MAX_SYSTEMS 8
#define MAX_EMITTERS_PER_SYSTEM 16

typedef struct {
    sprite_particle_t *particles;
    int particle_count;
    int max_particles;
    float gravity_x, gravity_y;
    float damping;
    sprite_particle_type_t particle_type;
    int particle_color;
    float scale_start, scale_end;
    int collision_enabled;
    sprite_particle_emitter_t *emitters;
    int emitter_count;
} particle_system_impl;

typedef struct {
    particle_system_impl *system;
    sprite_emitter_shape_t shape;
    float x, y;
    float emission_rate;
    float min_life, max_life;
    float vx, vy;
    float velocity_variance;
    float emission_accumulator;
} emitter_impl;

static particle_system_impl systems[MAX_SYSTEMS];
static int system_count = 0;

sprite_particle_system_t sprite_particle_system_create(int max_particles) {
    if (system_count >= MAX_SYSTEMS) return INVALID_PARTICLE_SYSTEM;

    particle_system_impl *sys = &systems[system_count];
    sys->particles = (sprite_particle_t *)malloc(max_particles * sizeof(sprite_particle_t));
    sys->particle_count = 0;
    sys->max_particles = max_particles;
    sys->gravity_x = 0.0f;
    sys->gravity_y = 9.8f;
    sys->damping = 0.99f;
    sys->particle_type = PARTICLE_SPRITE;
    sys->particle_color = 0xFFFFFF;
    sys->scale_start = 1.0f;
    sys->scale_end = 0.0f;
    sys->collision_enabled = 0;
    sys->emitters = (sprite_particle_emitter_t *)malloc(MAX_EMITTERS_PER_SYSTEM * sizeof(sprite_particle_emitter_t));
    sys->emitter_count = 0;

    return (sprite_particle_system_t)(intptr_t)system_count++;
}

void sprite_particle_system_destroy(sprite_particle_system_t system) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= system_count) return;
    free(systems[idx].particles);
    free(systems[idx].emitters);
}

int sprite_particle_system_update(sprite_particle_system_t system, int delta_ms) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= system_count) return 0;

    particle_system_impl *sys = &systems[idx];
    float dt = delta_ms / 1000.0f;

    for (int i = 0; i < sys->particle_count; i++) {
        sprite_particle_t *p = &sys->particles[i];

        p->vx += sys->gravity_x * dt;
        p->vy += sys->gravity_y * dt;
        p->vx *= sys->damping;
        p->vy *= sys->damping;

        p->x += p->vx * dt;
        p->y += p->vy * dt;

        p->life -= delta_ms;
        p->scale = sys->scale_start + (sys->scale_end - sys->scale_start) * (1.0f - p->life / p->max_life);
    }

    int alive = 0;
    for (int i = 0; i < sys->particle_count; i++) {
        if (sys->particles[i].life > 0) {
            if (alive != i) {
                sys->particles[alive] = sys->particles[i];
            }
            alive++;
        }
    }
    sys->particle_count = alive;

    return sys->particle_count;
}

int sprite_particle_system_render(sprite_particle_system_t system) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= system_count) return 0;
    return systems[idx].particle_count;
}

int sprite_particle_system_get_particle_count(sprite_particle_system_t system) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= system_count) return 0;
    return systems[idx].particle_count;
}

sprite_particle_emitter_t sprite_particle_emitter_create(sprite_particle_system_t system,
                                                         sprite_emitter_shape_t shape) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= system_count) return INVALID_EMITTER;

    particle_system_impl *sys = &systems[idx];
    if (sys->emitter_count >= MAX_EMITTERS_PER_SYSTEM) return INVALID_EMITTER;

    emitter_impl *emitter = (emitter_impl *)malloc(sizeof(emitter_impl));
    emitter->system = sys;
    emitter->shape = shape;
    emitter->x = emitter->y = 0.0f;
    emitter->emission_rate = 10.0f;
    emitter->min_life = 1.0f;
    emitter->max_life = 2.0f;
    emitter->vx = emitter->vy = 0.0f;
    emitter->velocity_variance = 0.0f;
    emitter->emission_accumulator = 0.0f;

    sys->emitters[sys->emitter_count++] = (sprite_particle_emitter_t)emitter;
    return (sprite_particle_emitter_t)emitter;
}

void sprite_particle_emitter_destroy(sprite_particle_emitter_t emitter) {
    free(emitter);
}

void sprite_particle_emitter_set_position(sprite_particle_emitter_t emitter, float x, float y) {
    emitter_impl *e = (emitter_impl *)emitter;
    if (!e) return;
    e->x = x;
    e->y = y;
}

void sprite_particle_emitter_set_emission_rate(sprite_particle_emitter_t emitter, float rate) {
    emitter_impl *e = (emitter_impl *)emitter;
    if (!e) return;
    e->emission_rate = rate;
}

void sprite_particle_emitter_set_lifetime(sprite_particle_emitter_t emitter, float min_life, float max_life) {
    emitter_impl *e = (emitter_impl *)emitter;
    if (!e) return;
    e->min_life = min_life;
    e->max_life = max_life;
}

void sprite_particle_emitter_set_velocity(sprite_particle_emitter_t emitter, float vx, float vy) {
    emitter_impl *e = (emitter_impl *)emitter;
    if (!e) return;
    e->vx = vx;
    e->vy = vy;
}

void sprite_particle_emitter_set_velocity_variance(sprite_particle_emitter_t emitter, float variance) {
    emitter_impl *e = (emitter_impl *)emitter;
    if (!e) return;
    e->velocity_variance = variance;
}

int sprite_particle_emitter_emit(sprite_particle_emitter_t emitter, int count) {
    emitter_impl *e = (emitter_impl *)emitter;
    if (!e || !e->system) return 0;

    particle_system_impl *sys = e->system;
    int emitted = 0;

    for (int i = 0; i < count && sys->particle_count < sys->max_particles; i++) {
        sprite_particle_t *p = &sys->particles[sys->particle_count++];
        p->x = e->x;
        p->y = e->y;
        p->vx = e->vx;
        p->vy = e->vy;
        p->life = e->min_life + (float)(rand() % 1000) / 1000.0f * (e->max_life - e->min_life);
        p->max_life = p->life;
        p->scale = sys->scale_start;
        emitted++;
    }

    return emitted;
}

void sprite_particle_system_set_gravity(sprite_particle_system_t system, float gx, float gy) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= system_count) return;
    systems[idx].gravity_x = gx;
    systems[idx].gravity_y = gy;
}

void sprite_particle_system_set_damping(sprite_particle_system_t system, float damping) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= system_count) return;
    systems[idx].damping = damping;
}

void sprite_particle_system_set_particle_type(sprite_particle_system_t system, sprite_particle_type_t type) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= system_count) return;
    systems[idx].particle_type = type;
}

void sprite_particle_system_set_particle_color(sprite_particle_system_t system, int color) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= system_count) return;
    systems[idx].particle_color = color;
}

void sprite_particle_system_set_particle_scale(sprite_particle_system_t system, float scale_start, float scale_end) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= system_count) return;
    systems[idx].scale_start = scale_start;
    systems[idx].scale_end = scale_end;
}

int sprite_particle_system_enable_collision(sprite_particle_system_t system) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= system_count) return 0;
    systems[idx].collision_enabled = 1;
    return 1;
}

int sprite_particle_system_disable_collision(sprite_particle_system_t system) {
    intptr_t idx = (intptr_t)system;
    if (idx < 0 || idx >= system_count) return 0;
    systems[idx].collision_enabled = 0;
    return 1;
}
