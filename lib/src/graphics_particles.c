/* graphics_particles.c — Particle Effect System (Phase 104b)
 *
 * Object-pooled particle emitter with physics simulation.
 * Handles explosions, smoke, sparkles, rain, fire, and custom effects.
 */

#include <graphics_particles.h>
#include <graphics.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Random number helper */
static int random_range(int min, int max) {
    if (min >= max) return min;
    return min + (rand() % (max - min + 1));
}

/* ============================================================================
 * EMITTER LIFECYCLE
 * ============================================================================ */

int particle_emitter_create(particle_emitter_t *emitter, int max_particles) {
    if (!emitter || max_particles <= 0) {
        return -1;
    }

    /* Allocate particle pool */
    emitter->particles = (particle_t *)malloc(max_particles * sizeof(particle_t));
    if (!emitter->particles) {
        return -1;
    }

    /* Initialize pool */
    memset(emitter->particles, 0, max_particles * sizeof(particle_t));
    emitter->max_particles = max_particles;
    emitter->particle_count = 0;

    /* Initialize emitter state */
    emitter->x = 0;
    emitter->y = 0;
    emitter->active = 0;
    emitter->emit_counter = 0;

    /* Default emission parameters */
    emitter->emission_rate = 5;
    emitter->lifetime_min = 30;
    emitter->lifetime_max = 60;
    emitter->velocity_x_min = -100;
    emitter->velocity_x_max = 100;
    emitter->velocity_y_min = -100;
    emitter->velocity_y_max = 100;
    emitter->acceleration_x = 0;
    emitter->acceleration_y = 10;    /* Slight gravity by default */

    /* Visual parameters */
    emitter->color_start = 1;        /* White */
    emitter->color_end = 0;          /* Black (fade) */
    emitter->size_start = 1;
    emitter->size_end = 1;

    /* Behavior */
    emitter->angle_spread = 360;     /* All directions */
    emitter->speed_variation = 0;    /* No variation */

    /* Callbacks */
    emitter->on_particle_spawn = NULL;
    emitter->on_particle_die = NULL;
    emitter->user_data = NULL;

    return 0;
}

void particle_emitter_destroy(particle_emitter_t *emitter) {
    if (!emitter) return;

    emitter->active = 0;
    if (emitter->particles) {
        free(emitter->particles);
        emitter->particles = NULL;
    }
    emitter->particle_count = 0;
    emitter->max_particles = 0;
}

/* ============================================================================
 * EMITTER CONTROL
 * ============================================================================ */

void particle_emitter_start(particle_emitter_t *emitter, int x, int y) {
    if (!emitter) return;

    emitter->x = x;
    emitter->y = y;
    emitter->active = 1;
    emitter->emit_counter = 0;
}

void particle_emitter_stop(particle_emitter_t *emitter) {
    if (!emitter) return;
    emitter->active = 0;
}

void particle_emitter_set_position(particle_emitter_t *emitter, int x, int y) {
    if (!emitter) return;
    emitter->x = x;
    emitter->y = y;
}

void particle_emitter_burst(particle_emitter_t *emitter, int count) {
    if (!emitter || count <= 0) return;

    /* Spawn up to count particles */
    for (int i = 0; i < count && emitter->particle_count < emitter->max_particles; i++) {
        particle_t *p = &emitter->particles[emitter->particle_count];

        /* Initialize particle */
        p->x = emitter->x * 256;
        p->y = emitter->y * 256;

        /* Random velocity within range */
        int vx = random_range(emitter->velocity_x_min, emitter->velocity_x_max);
        int vy = random_range(emitter->velocity_y_min, emitter->velocity_y_max);

        /* Apply speed variation if enabled */
        if (emitter->speed_variation > 0) {
            int var = (emitter->speed_variation * abs(vx)) / 100;
            vx += random_range(-var, var);
            var = (emitter->speed_variation * abs(vy)) / 100;
            vy += random_range(-var, var);
        }

        p->vx = vx;
        p->vy = vy;
        p->ax = emitter->acceleration_x;
        p->ay = emitter->acceleration_y;

        /* Random lifetime */
        p->max_lifetime = random_range(emitter->lifetime_min, emitter->lifetime_max);
        p->lifetime = p->max_lifetime;

        /* Visual */
        p->color = emitter->color_start;
        p->size = emitter->size_start;
        p->type = 0;

        /* Fire spawn callback */
        if (emitter->on_particle_spawn) {
            emitter->on_particle_spawn(p);
        }

        emitter->particle_count++;
    }
}

/* ============================================================================
 * EMITTER CONFIGURATION
 * ============================================================================ */

void particle_emitter_set_emission_rate(particle_emitter_t *emitter, int rate) {
    if (!emitter) return;
    emitter->emission_rate = rate;
}

void particle_emitter_set_lifetime(particle_emitter_t *emitter,
                                   int lifetime_min, int lifetime_max) {
    if (!emitter) return;
    emitter->lifetime_min = lifetime_min;
    emitter->lifetime_max = lifetime_max;
}

void particle_emitter_set_velocity(particle_emitter_t *emitter,
                                   int vx_min, int vx_max,
                                   int vy_min, int vy_max) {
    if (!emitter) return;
    emitter->velocity_x_min = vx_min;
    emitter->velocity_x_max = vx_max;
    emitter->velocity_y_min = vy_min;
    emitter->velocity_y_max = vy_max;
}

void particle_emitter_set_acceleration(particle_emitter_t *emitter, int ax, int ay) {
    if (!emitter) return;
    emitter->acceleration_x = ax;
    emitter->acceleration_y = ay;
}

void particle_emitter_set_color_gradient(particle_emitter_t *emitter,
                                         unsigned char color_start,
                                         unsigned char color_end) {
    if (!emitter) return;
    emitter->color_start = color_start;
    emitter->color_end = color_end;
}

void particle_emitter_set_size_progression(particle_emitter_t *emitter,
                                           int size_start, int size_end) {
    if (!emitter) return;
    emitter->size_start = size_start;
    emitter->size_end = size_end;
}

void particle_emitter_set_angle_spread(particle_emitter_t *emitter, int spread_degrees) {
    if (!emitter) return;
    emitter->angle_spread = spread_degrees;
}

void particle_emitter_set_speed_variation(particle_emitter_t *emitter, int variation_percent) {
    if (!emitter) return;
    if (variation_percent < 0) variation_percent = 0;
    if (variation_percent > 100) variation_percent = 100;
    emitter->speed_variation = variation_percent;
}

/* ============================================================================
 * UPDATE & RENDER
 * ============================================================================ */

void particle_emitter_update(particle_emitter_t *emitter) {
    if (!emitter || !emitter->particles) return;

    /* Emit new particles if active */
    if (emitter->active && emitter->emission_rate > 0) {
        emitter->emit_counter += emitter->emission_rate;
        int spawn_count = emitter->emit_counter / 256;  /* Fixed-point scaling */
        if (spawn_count > 0) {
            emitter->emit_counter -= spawn_count * 256;
            particle_emitter_burst(emitter, spawn_count);
        }
    }

    /* Update all active particles */
    int write_idx = 0;
    for (int i = 0; i < emitter->particle_count; i++) {
        particle_t *p = &emitter->particles[i];

        /* Apply acceleration */
        p->vx += p->ax;
        p->vy += p->ay;

        /* Apply velocity */
        p->x += p->vx;
        p->y += p->vy;

        /* Age particle */
        p->lifetime--;

        if (p->lifetime > 0) {
            /* Calculate fade color */
            if (emitter->color_start != emitter->color_end) {
                int fade_progress = (p->max_lifetime - p->lifetime);
                int fade_max = p->max_lifetime;
                int color_range = emitter->color_end - emitter->color_start;
                p->color = emitter->color_start + (color_range * fade_progress) / fade_max;
            }

            /* Calculate size progression */
            if (emitter->size_start != emitter->size_end) {
                int size_range = emitter->size_end - emitter->size_start;
                int fade_progress = (p->max_lifetime - p->lifetime);
                int fade_max = p->max_lifetime;
                p->size = emitter->size_start + (size_range * fade_progress) / fade_max;
            }

            /* Keep particle */
            if (write_idx != i) {
                emitter->particles[write_idx] = *p;
            }
            write_idx++;
        } else {
            /* Fire death callback */
            if (emitter->on_particle_die) {
                emitter->on_particle_die(p);
            }
            /* Particle is removed (don't increment write_idx) */
        }
    }

    emitter->particle_count = write_idx;
}

void particle_emitter_draw(particle_emitter_t *emitter) {
    if (!emitter || !emitter->particles) return;

    for (int i = 0; i < emitter->particle_count; i++) {
        particle_t *p = &emitter->particles[i];

        /* Convert from fixed-point to pixels */
        int pixel_x = p->x / 256;
        int pixel_y = p->y / 256;

        /* Set color and draw pixel */
        graphics_setcolor(p->color);
        graphics_plot(pixel_x, pixel_y);
    }
}

void particle_emitter_draw_custom(particle_emitter_t *emitter,
                                  void (*draw_fn)(particle_t *, int, int)) {
    if (!emitter || !emitter->particles || !draw_fn) return;

    for (int i = 0; i < emitter->particle_count; i++) {
        particle_t *p = &emitter->particles[i];

        /* Convert from fixed-point to pixels */
        int pixel_x = p->x / 256;
        int pixel_y = p->y / 256;

        /* Call custom draw function */
        draw_fn(p, pixel_x, pixel_y);
    }
}

/* ============================================================================
 * QUERY FUNCTIONS
 * ============================================================================ */

int particle_emitter_is_active(particle_emitter_t *emitter) {
    if (!emitter) return 0;
    return emitter->active;
}

int particle_emitter_count(particle_emitter_t *emitter) {
    if (!emitter) return 0;
    return emitter->particle_count;
}

int particle_emitter_percent(particle_emitter_t *emitter) {
    if (!emitter || emitter->max_particles == 0) return 0;
    return (100 * emitter->particle_count) / emitter->max_particles;
}

int particle_emitter_is_empty(particle_emitter_t *emitter) {
    if (!emitter) return 1;
    return (emitter->particle_count == 0);
}

/* ============================================================================
 * PREDEFINED EFFECTS
 * ============================================================================ */

void particle_emitter_preset_explosion(particle_emitter_t *emitter) {
    if (!emitter) return;

    particle_emitter_set_emission_rate(emitter, 0);      /* Burst only */
    particle_emitter_set_lifetime(emitter, 20, 40);      /* Quick fade */
    particle_emitter_set_velocity(emitter, -200, 200, -200, 200);  /* Radial */
    particle_emitter_set_acceleration(emitter, 0, 10);   /* Slight gravity */
    particle_emitter_set_color_gradient(emitter, 8, 0);  /* Gray to black */
    particle_emitter_set_angle_spread(emitter, 360);     /* All directions */
}

void particle_emitter_preset_smoke(particle_emitter_t *emitter) {
    if (!emitter) return;

    particle_emitter_set_emission_rate(emitter, 2);      /* Continuous */
    particle_emitter_set_lifetime(emitter, 60, 120);     /* Long fade */
    particle_emitter_set_velocity(emitter, -10, 10, -30, -10);  /* Upward drift */
    particle_emitter_set_acceleration(emitter, 0, -5);   /* Rise */
    particle_emitter_set_color_gradient(emitter, 12, 0); /* Dark gray to black */
    particle_emitter_set_size_progression(emitter, 1, 2); /* Expand slightly */
}

void particle_emitter_preset_sparkle(particle_emitter_t *emitter) {
    if (!emitter) return;

    particle_emitter_set_emission_rate(emitter, 0);      /* Burst only */
    particle_emitter_set_lifetime(emitter, 15, 30);      /* Very quick fade */
    particle_emitter_set_velocity(emitter, -150, 150, -150, 150);  /* Radial burst */
    particle_emitter_set_acceleration(emitter, 0, 0);    /* No gravity */
    particle_emitter_set_color_gradient(emitter, 1, 0);  /* White to black */
    particle_emitter_set_angle_spread(emitter, 360);     /* All directions */
    particle_emitter_set_speed_variation(emitter, 50);   /* High variation */
}

void particle_emitter_preset_rain(particle_emitter_t *emitter) {
    if (!emitter) return;

    particle_emitter_set_emission_rate(emitter, 5);      /* Continuous */
    particle_emitter_set_lifetime(emitter, 200, 300);    /* Long lifetime */
    particle_emitter_set_velocity(emitter, -5, 5, 50, 70);  /* Falling */
    particle_emitter_set_acceleration(emitter, 0, 20);   /* Gravity pulls down */
    particle_emitter_set_color_gradient(emitter, 1, 1);  /* No color change */
    particle_emitter_set_angle_spread(emitter, 360);
}

void particle_emitter_preset_fire(particle_emitter_t *emitter) {
    if (!emitter) return;

    particle_emitter_set_emission_rate(emitter, 3);      /* Continuous */
    particle_emitter_set_lifetime(emitter, 40, 80);      /* Medium fade */
    particle_emitter_set_velocity(emitter, -20, 20, -60, -20);  /* Upward */
    particle_emitter_set_acceleration(emitter, 0, -10);  /* Rise */
    particle_emitter_set_color_gradient(emitter, 2, 0);  /* Red to black */
    particle_emitter_set_size_progression(emitter, 2, 0); /* Shrink */
}
