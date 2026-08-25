/* sprite_particle_system.h — Phase 51: Particle Systems
 *
 * Particle effects with emission, physics, and rendering.
 */

#ifndef SPRITE_PARTICLE_SYSTEM_H
#define SPRITE_PARTICLE_SYSTEM_H

typedef void *sprite_particle_system_t;
typedef void *sprite_particle_emitter_t;

#define INVALID_PARTICLE_SYSTEM NULL
#define INVALID_EMITTER NULL

typedef enum {
    EMITTER_POINT = 0,
    EMITTER_CIRCLE = 1,
    EMITTER_BOX = 2,
    EMITTER_LINE = 3,
} sprite_emitter_shape_t;

typedef enum {
    PARTICLE_SPRITE = 0,
    PARTICLE_POINT = 1,
    PARTICLE_TRAIL = 2,
} sprite_particle_type_t;

typedef struct {
    float x, y;
    float vx, vy;
    float life, max_life;
    float scale;
    int color;
} sprite_particle_t;

/* Particle System */
sprite_particle_system_t sprite_particle_system_create(int max_particles);
void sprite_particle_system_destroy(sprite_particle_system_t system);
int sprite_particle_system_update(sprite_particle_system_t system, int delta_ms);
int sprite_particle_system_render(sprite_particle_system_t system);
int sprite_particle_system_get_particle_count(sprite_particle_system_t system);

/* Emitters */
sprite_particle_emitter_t sprite_particle_emitter_create(sprite_particle_system_t system,
                                                         sprite_emitter_shape_t shape);
void sprite_particle_emitter_destroy(sprite_particle_emitter_t emitter);
void sprite_particle_emitter_set_position(sprite_particle_emitter_t emitter, float x, float y);
void sprite_particle_emitter_set_emission_rate(sprite_particle_emitter_t emitter, float rate);
void sprite_particle_emitter_set_lifetime(sprite_particle_emitter_t emitter, float min_life, float max_life);
void sprite_particle_emitter_set_velocity(sprite_particle_emitter_t emitter, float vx, float vy);
void sprite_particle_emitter_set_velocity_variance(sprite_particle_emitter_t emitter, float variance);
int sprite_particle_emitter_emit(sprite_particle_emitter_t emitter, int count);

/* Particle Properties */
void sprite_particle_system_set_gravity(sprite_particle_system_t system, float gx, float gy);
void sprite_particle_system_set_damping(sprite_particle_system_t system, float damping);
void sprite_particle_system_set_particle_type(sprite_particle_system_t system, sprite_particle_type_t type);
void sprite_particle_system_set_particle_color(sprite_particle_system_t system, int color);
void sprite_particle_system_set_particle_scale(sprite_particle_system_t system, float scale_start, float scale_end);

/* Collision */
int sprite_particle_system_enable_collision(sprite_particle_system_t system);
int sprite_particle_system_disable_collision(sprite_particle_system_t system);

#endif
