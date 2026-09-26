/* graphics_particles.h — Particle Effect System (Phase 104b)
 *
 * Object-pooled particle emitter system with physics simulation.
 * Supports explosions, smoke, sparkles, rain, and custom effects.
 */

#pragma once

/* ============================================================================
 * PARTICLE STRUCTURES
 * ============================================================================ */

/**
 * Individual particle in the system
 */
typedef struct {
    int x, y;                       /* Position (fixed-point: pixels * 256) */
    int vx, vy;                     /* Velocity (pixels/frame * 256) */
    int ax, ay;                     /* Acceleration (gravity, wind) */
    unsigned char color;            /* Color index (0-15) */
    int lifetime;                   /* Frames remaining (0 = dead/available) */
    int max_lifetime;               /* Total lifetime (for fade calculations) */
    unsigned char size;             /* Optional size/sprite index */
    unsigned char type;             /* Custom type for effect variants */
} particle_t;

/**
 * Emitter configuration and state
 */
typedef struct {
    /* Core pool */
    particle_t *particles;          /* Pool of particles */
    int particle_count;             /* Current active particles */
    int max_particles;              /* Pool size */

    /* Position */
    int x, y;                       /* Emitter position (pixels) */

    /* Emission parameters */
    int emission_rate;              /* Particles spawned per frame (0 = burst only) */
    int emit_counter;               /* Internal: fractional emission tracking */
    int active;                     /* 1 = emitting, 0 = stopped */

    /* Particle configuration */
    int lifetime_min, lifetime_max;  /* Lifetime range (frames) */
    int velocity_x_min, velocity_x_max; /* X velocity range */
    int velocity_y_min, velocity_y_max; /* Y velocity range */
    int acceleration_x, acceleration_y; /* Gravity/wind */

    /* Visual parameters */
    unsigned char color_start;      /* Starting color */
    unsigned char color_end;        /* Ending color (fade) */
    int size_start, size_end;       /* Size progression */

    /* Behavior */
    int angle_spread;               /* Radial spread (0-360 degrees, 0=all directions) */
    int speed_variation;            /* Speed variation percentage */

    /* Callbacks */
    void (*on_particle_spawn)(particle_t *p);  /* Called when particle spawned */
    void (*on_particle_die)(particle_t *p);    /* Called when particle dies */

    void *user_data;                /* User-defined context */
} particle_emitter_t;

/* ============================================================================
 * EMITTER LIFECYCLE
 * ============================================================================ */

/**
 * particle_emitter_create - Create particle emitter
 *
 * Allocates emitter and particle pool.
 *
 * Parameters:
 *   emitter — Emitter structure (caller-allocated)
 *   max_particles — Maximum particles in pool (typical: 50-200)
 *
 * Returns:
 *   0 on success, -1 on error
 */
int particle_emitter_create(particle_emitter_t *emitter, int max_particles);

/**
 * particle_emitter_destroy - Cleanup emitter
 *
 * Frees particle pool and stops emission.
 *
 * Parameters:
 *   emitter — Emitter to cleanup
 */
void particle_emitter_destroy(particle_emitter_t *emitter);

/* ============================================================================
 * EMITTER CONTROL
 * ============================================================================ */

/**
 * particle_emitter_start - Begin continuous emission
 *
 * Starts emitting particles at configured rate.
 *
 * Parameters:
 *   emitter — Emitter to start
 *   x, y — Emitter position (pixels)
 */
void particle_emitter_start(particle_emitter_t *emitter, int x, int y);

/**
 * particle_emitter_stop - Stop continuous emission
 *
 * Stops spawning new particles but keeps existing ones alive.
 *
 * Parameters:
 *   emitter — Emitter to stop
 */
void particle_emitter_stop(particle_emitter_t *emitter);

/**
 * particle_emitter_burst - Spawn N particles immediately
 *
 * Creates N particles at emitter position with configured parameters.
 * Useful for explosions, magic effects, etc.
 *
 * Parameters:
 *   emitter — Emitter
 *   count — Number of particles to spawn
 */
void particle_emitter_burst(particle_emitter_t *emitter, int count);

/**
 * particle_emitter_set_position - Move emitter
 *
 * Parameters:
 *   emitter — Emitter
 *   x, y — New position (pixels)
 */
void particle_emitter_set_position(particle_emitter_t *emitter, int x, int y);

/* ============================================================================
 * EMITTER CONFIGURATION
 * ============================================================================ */

/**
 * particle_emitter_set_emission_rate - Set continuous emission rate
 *
 * Parameters:
 *   emitter — Emitter
 *   rate — Particles per frame (0 = burst only, 5 = 5 particles/frame)
 */
void particle_emitter_set_emission_rate(particle_emitter_t *emitter, int rate);

/**
 * particle_emitter_set_lifetime - Set particle lifetime range
 *
 * Each spawned particle gets random lifetime between min and max.
 *
 * Parameters:
 *   emitter — Emitter
 *   lifetime_min, lifetime_max — Lifetime in frames
 */
void particle_emitter_set_lifetime(particle_emitter_t *emitter,
                                   int lifetime_min, int lifetime_max);

/**
 * particle_emitter_set_velocity - Set velocity range
 *
 * Each particle spawns with random velocity in specified range.
 *
 * Parameters:
 *   emitter — Emitter
 *   vx_min, vx_max — X velocity (pixels/frame)
 *   vy_min, vy_max — Y velocity (pixels/frame)
 */
void particle_emitter_set_velocity(particle_emitter_t *emitter,
                                   int vx_min, int vx_max,
                                   int vy_min, int vy_max);

/**
 * particle_emitter_set_acceleration - Set gravity/wind
 *
 * Applied to all particles each frame (typical: 0, 10 for falling)
 *
 * Parameters:
 *   emitter — Emitter
 *   ax, ay — Acceleration (pixels/frame²)
 */
void particle_emitter_set_acceleration(particle_emitter_t *emitter, int ax, int ay);

/**
 * particle_emitter_set_color_gradient - Set color fade
 *
 * Particles fade from start_color to end_color over lifetime.
 *
 * Parameters:
 *   emitter — Emitter
 *   color_start, color_end — Color indices (0-15)
 */
void particle_emitter_set_color_gradient(particle_emitter_t *emitter,
                                         unsigned char color_start,
                                         unsigned char color_end);

/**
 * particle_emitter_set_size_progression - Set size scaling
 *
 * Particles scale from size_start to size_end over lifetime.
 *
 * Parameters:
 *   emitter — Emitter
 *   size_start, size_end — Sizes (pixels)
 */
void particle_emitter_set_size_progression(particle_emitter_t *emitter,
                                           int size_start, int size_end);

/**
 * particle_emitter_set_angle_spread - Set directional spread
 *
 * For radial emissions (explosions, fountains).
 *
 * Parameters:
 *   emitter — Emitter
 *   spread_degrees — Spread angle (0=all directions, 45=45° cone, 180=hemisphere)
 */
void particle_emitter_set_angle_spread(particle_emitter_t *emitter, int spread_degrees);

/**
 * particle_emitter_set_speed_variation - Set speed randomness
 *
 * Parameters:
 *   emitter — Emitter
 *   variation_percent — Speed variation (0-100%, 20=±20% variation)
 */
void particle_emitter_set_speed_variation(particle_emitter_t *emitter, int variation_percent);

/* ============================================================================
 * UPDATE & RENDER
 * ============================================================================ */

/**
 * particle_emitter_update - Update emitter and particles
 *
 * Advances all particles one frame:
 * - Applies velocity and acceleration
 * - Ages particles
 * - Removes dead particles
 * - Spawns new particles if emission_rate > 0
 *
 * Call this every game frame for all active emitters.
 *
 * Parameters:
 *   emitter — Emitter to update
 */
void particle_emitter_update(particle_emitter_t *emitter);

/**
 * particle_emitter_draw - Render all particles
 *
 * Draws particles as colored pixels at current positions.
 * Must be called after particle_emitter_update().
 *
 * Parameters:
 *   emitter — Emitter to draw
 */
void particle_emitter_draw(particle_emitter_t *emitter);

/**
 * particle_emitter_draw_custom - Render with custom draw function
 *
 * For custom rendering (sprites, specific patterns, etc.)
 *
 * Parameters:
 *   emitter — Emitter
 *   draw_fn — Callback: void(*)(particle_t *p, int x, int y)
 */
void particle_emitter_draw_custom(particle_emitter_t *emitter,
                                  void (*draw_fn)(particle_t *, int, int));

/* ============================================================================
 * QUERY FUNCTIONS
 * ============================================================================ */

/**
 * particle_emitter_is_active - Check if emitter is running
 *
 * Returns:
 *   1 if emitter is active, 0 if stopped
 */
int particle_emitter_is_active(particle_emitter_t *emitter);

/**
 * particle_emitter_count - Get number of active particles
 *
 * Returns:
 *   Number of living particles (0 = all dead)
 */
int particle_emitter_count(particle_emitter_t *emitter);

/**
 * particle_emitter_percent - Get pool utilization
 *
 * Returns:
 *   Percentage of pool used (0-100)
 */
int particle_emitter_percent(particle_emitter_t *emitter);

/**
 * particle_emitter_is_empty - Check if all particles dead
 *
 * Returns:
 *   1 if no particles alive, 0 otherwise
 */
int particle_emitter_is_empty(particle_emitter_t *emitter);

/* ============================================================================
 * PREDEFINED EFFECTS
 * ============================================================================ */

/**
 * particle_emitter_preset_explosion - Configure for explosion effect
 *
 * Radial burst, quick fade, orange to black gradient.
 *
 * Parameters:
 *   emitter — Emitter (must be created first)
 */
void particle_emitter_preset_explosion(particle_emitter_t *emitter);

/**
 * particle_emitter_preset_smoke - Configure for smoke/fog effect
 *
 * Upward drift, long fade, gray to transparent.
 *
 * Parameters:
 *   emitter — Emitter (must be created first)
 */
void particle_emitter_preset_smoke(particle_emitter_t *emitter);

/**
 * particle_emitter_preset_sparkle - Configure for sparkle/magic effect
 *
 * Radial burst, twinkling colors, very quick fade.
 *
 * Parameters:
 *   emitter — Emitter (must be created first)
 */
void particle_emitter_preset_sparkle(particle_emitter_t *emitter);

/**
 * particle_emitter_preset_rain - Configure for falling rain/snow
 *
 * Falling downward with slight drift, long lifetime.
 *
 * Parameters:
 *   emitter — Emitter (must be created first)
 */
void particle_emitter_preset_rain(particle_emitter_t *emitter);

/**
 * particle_emitter_preset_fire - Configure for fire/flame effect
 *
 * Upward movement, color gradient red->yellow->black, animated.
 *
 * Parameters:
 *   emitter — Emitter (must be created first)
 */
void particle_emitter_preset_fire(particle_emitter_t *emitter);

