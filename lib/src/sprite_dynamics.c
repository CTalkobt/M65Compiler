/* sprite_dynamics.c — Sprite Movement & Animation Implementation
 *
 * Implements velocity-based movement and frame-based animation for sprites.
 * Updates are driven by sprite_update_interval() called at fixed time step.
 */

#include <sprite_dynamics.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * CONFIGURATION
 * ========================================================================== */

#define MAX_SPRITES             256
#define DEGREES_TO_RADIANS     (3.14159265f / 180.0f)
#define RADIANS_TO_DEGREES     (180.0f / 3.14159265f)

/* ============================================================================
 * SPRITE DYNAMICS OBJECT (INTERNAL)
 * ========================================================================== */

typedef struct {
    sprite_t sprite;        /* Associated sprite handle */

    /* Movement */
    sprite_movement_t movement;
    int accumulated_time_ms;

    /* Animation */
    sprite_animation_t animation;
    int animation_accumulated_time_ms;

    /* Callbacks */
    sprite_on_move_callback on_move;
    sprite_on_frame_callback on_frame;

    /* State */
    int active;
} sprite_dynamics_t;

/* ============================================================================
 * STATIC STATE
 * ========================================================================== */

static sprite_dynamics_t dynamics[MAX_SPRITES];
static int dynamics_count = 0;

/* ============================================================================
 * INTERNAL HELPERS
 * ========================================================================== */

/**
 * _find_dynamics - Find dynamics object for sprite
 */
static sprite_dynamics_t *_find_dynamics(sprite_t sprite) {
    for (int i = 0; i < MAX_SPRITES; ++i) {
        if (dynamics[i].active && dynamics[i].sprite == sprite) {
            return &dynamics[i];
        }
    }
    return NULL;
}

/**
 * _ensure_dynamics - Get or create dynamics for sprite
 */
static sprite_dynamics_t *_ensure_dynamics(sprite_t sprite) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (d) return d;

    if (dynamics_count >= MAX_SPRITES) return NULL;

    d = &dynamics[dynamics_count++];
    memset(d, 0, sizeof(sprite_dynamics_t));
    d->sprite = sprite;
    d->active = 1;
    d->movement.friction = 1.0f;  /* No friction by default */

    return d;
}

/**
 * _heading_to_velocity - Convert heading to Cartesian velocity
 */
static void _heading_to_velocity(float heading, float speed,
                                 float *vx, float *vy) {
    float rad = heading * DEGREES_TO_RADIANS;
    *vx = speed * cosf(rad);
    *vy = speed * sinf(rad);
}

/**
 * _velocity_to_heading - Convert Cartesian velocity to heading
 */
static void _velocity_to_heading(float vx, float vy,
                                 float *speed, float *heading) {
    *speed = sqrtf(vx * vx + vy * vy);
    *heading = atan2f(vy, vx) * RADIANS_TO_DEGREES;
    if (*heading < 0) *heading += 360.0f;
}

/**
 * _clamp_float - Clamp float to range
 */
static float _clamp_float(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

/* ============================================================================
 * MOVEMENT CONTROL
 * ========================================================================== */

void sprite_set_velocity(sprite_t sprite, float vx, float vy) {
    sprite_dynamics_t *d = _ensure_dynamics(sprite);
    if (!d) return;

    d->movement.vx = vx;
    d->movement.vy = vy;
    d->movement.active = 1;
}

void sprite_get_velocity(sprite_t sprite, float *vx, float *vy) {
    if (!vx || !vy) return;

    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d) { *vx = 0; *vy = 0; return; }

    *vx = d->movement.vx;
    *vy = d->movement.vy;
}

void sprite_set_speed_heading(sprite_t sprite, float speed, float heading) {
    sprite_dynamics_t *d = _ensure_dynamics(sprite);
    if (!d) return;

    d->movement.speed = speed;
    d->movement.heading = heading;

    _heading_to_velocity(heading, speed, &d->movement.vx, &d->movement.vy);
    d->movement.active = 1;
}

void sprite_get_speed_heading(sprite_t sprite, float *speed, float *heading) {
    if (!speed || !heading) return;

    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d) { *speed = 0; *heading = 0; return; }

    _velocity_to_heading(d->movement.vx, d->movement.vy, speed, heading);
}

void sprite_set_acceleration(sprite_t sprite, float ax, float ay) {
    sprite_dynamics_t *d = _ensure_dynamics(sprite);
    if (!d) return;

    d->movement.ax = ax;
    d->movement.ay = ay;
}

void sprite_get_acceleration(sprite_t sprite, float *ax, float *ay) {
    if (!ax || !ay) return;

    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d) { *ax = 0; *ay = 0; return; }

    *ax = d->movement.ax;
    *ay = d->movement.ay;
}

void sprite_set_friction(sprite_t sprite, float friction) {
    sprite_dynamics_t *d = _ensure_dynamics(sprite);
    if (!d) return;

    d->movement.friction = _clamp_float(friction, 0.0f, 1.0f);
}

float sprite_get_friction(sprite_t sprite) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    return d ? d->movement.friction : 1.0f;
}

void sprite_accelerate(sprite_t sprite, float ax, float ay) {
    sprite_dynamics_t *d = _ensure_dynamics(sprite);
    if (!d) return;

    d->movement.vx += ax;
    d->movement.vy += ay;
}

void sprite_set_movement_active(sprite_t sprite, int active) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d) return;

    d->movement.active = active ? 1 : 0;
}

int sprite_is_movement_active(sprite_t sprite) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    return d ? d->movement.active : 0;
}

int sprite_get_movement(sprite_t sprite, sprite_movement_t *movement) {
    if (!movement) return 0;

    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d) return 0;

    *movement = d->movement;
    return 1;
}

/* ============================================================================
 * ANIMATION CONTROL
 * ========================================================================== */

int sprite_set_animation_frames(sprite_t sprite, uint8_t **frames,
                                int frame_count, int frame_delay) {
    if (!frames || frame_count <= 0 || frame_delay <= 0) return 0;

    sprite_dynamics_t *d = _ensure_dynamics(sprite);
    if (!d) return 0;

    d->animation.frames = frames;
    d->animation.frame_count = frame_count;
    d->animation.frame_delay = frame_delay;
    d->animation.current_frame = 0;
    d->animation.frame_counter = 0;
    d->animation.playing = 0;
    d->animation.mode = ANIM_ONCE;
    d->animation.direction = 1;

    return 1;
}

int sprite_play_animation(sprite_t sprite, sprite_anim_mode_t mode) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d || !d->animation.frames) return 0;

    d->animation.playing = 1;
    d->animation.mode = mode;
    d->animation.current_frame = 0;
    d->animation.frame_counter = 0;
    d->animation.direction = 1;
    d->animation_accumulated_time_ms = 0;

    return 1;
}

void sprite_pause_animation(sprite_t sprite) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d) return;

    d->animation.playing = 0;
}

void sprite_stop_animation(sprite_t sprite) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d) return;

    d->animation.playing = 0;
    d->animation.current_frame = 0;
    d->animation.frame_counter = 0;
    d->animation_accumulated_time_ms = 0;
}

int sprite_is_animation_playing(sprite_t sprite) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    return d ? d->animation.playing : 0;
}

int sprite_get_animation_frame(sprite_t sprite) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    return d ? d->animation.current_frame : -1;
}

int sprite_set_animation_frame(sprite_t sprite, int frame) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d || frame < 0 || frame >= d->animation.frame_count) return 0;

    d->animation.current_frame = frame;
    if (d->animation.frames[frame]) {
        sprite_set_bitmap(sprite, d->animation.frames[frame], d->animation.frames[frame] ? 1 : 0);
    }

    return 1;
}

int sprite_get_animation(sprite_t sprite, sprite_animation_t *animation) {
    if (!animation) return 0;

    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d) return 0;

    *animation = d->animation;
    return 1;
}

int sprite_set_on_frame_callback(sprite_t sprite,
                                 sprite_on_frame_callback callback) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d) return 0;

    d->on_frame = callback;
    return 1;
}

int sprite_set_on_move_callback(sprite_t sprite,
                                sprite_on_move_callback callback) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d) return 0;

    d->on_move = callback;
    return 1;
}

/* ============================================================================
 * UPDATE SYSTEM
 * ========================================================================== */

int sprite_update_interval(int delta_ms) {
    int updated = 0;

    for (int i = 0; i < dynamics_count; ++i) {
        if (dynamics[i].active) {
            updated += sprite_update_single(dynamics[i].sprite, delta_ms);
        }
    }

    return updated;
}

int sprite_update_single(sprite_t sprite, int delta_ms) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d || !d->active) return 0;

    /* Update movement */
    if (d->movement.active) {
        d->accumulated_time_ms += delta_ms;

        /* Apply acceleration */
        d->movement.vx += d->movement.ax;
        d->movement.vy += d->movement.ay;

        /* Apply friction */
        d->movement.vx *= d->movement.friction;
        d->movement.vy *= d->movement.friction;

        /* Move sprite */
        int old_x, old_y;
        sprite_get_position(sprite, &old_x, &old_y);

        int new_x = old_x + (int)d->movement.vx;
        int new_y = old_y + (int)d->movement.vy;

        sprite_set_position(sprite, new_x, new_y);

        /* Invoke callback */
        if (d->on_move) {
            d->on_move(sprite, old_x, old_y, new_x, new_y);
        }
    }

    /* Update animation */
    if (d->animation.playing && d->animation.frames) {
        d->animation_accumulated_time_ms += delta_ms;

        int frame_ticks = d->animation_accumulated_time_ms / d->animation.frame_delay;

        if (frame_ticks > 0) {
            d->animation_accumulated_time_ms = 0;

            /* Advance frame */
            d->animation.current_frame += d->animation.direction;

            /* Handle loop modes */
            if (d->animation.mode == ANIM_LOOP) {
                if (d->animation.current_frame >= d->animation.frame_count) {
                    d->animation.current_frame = 0;
                }
            } else if (d->animation.mode == ANIM_PINGPONG) {
                if (d->animation.current_frame >= d->animation.frame_count) {
                    d->animation.current_frame = d->animation.frame_count - 2;
                    d->animation.direction = -1;
                } else if (d->animation.current_frame < 0) {
                    d->animation.current_frame = 1;
                    d->animation.direction = 1;
                }
            } else {  /* ANIM_ONCE */
                if (d->animation.current_frame >= d->animation.frame_count) {
                    d->animation.current_frame = d->animation.frame_count - 1;
                    d->animation.playing = 0;
                }
            }

            /* Update bitmap */
            if (d->animation.current_frame >= 0 &&
                d->animation.current_frame < d->animation.frame_count &&
                d->animation.frames[d->animation.current_frame]) {
                sprite_set_bitmap(sprite, d->animation.frames[d->animation.current_frame],
                                  32);  /* Assume 32-byte stride */
            }

            /* Invoke callback */
            if (d->on_frame) {
                d->on_frame(sprite, d->animation.current_frame);
            }
        }
    }

    return 1;
}

void sprite_reset_update_timers(void) {
    for (int i = 0; i < dynamics_count; ++i) {
        dynamics[i].accumulated_time_ms = 0;
        dynamics[i].animation_accumulated_time_ms = 0;
    }
}

/* ============================================================================
 * PHYSICS UTILITIES
 * ========================================================================== */

void sprite_apply_gravity(sprite_t sprite, float gravity) {
    sprite_dynamics_t *d = _ensure_dynamics(sprite);
    if (!d) return;

    d->movement.ay += gravity;
}

void sprite_clamp_velocity(sprite_t sprite, float max_speed) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d) return;

    float speed = sqrtf(d->movement.vx * d->movement.vx +
                        d->movement.vy * d->movement.vy);

    if (speed > max_speed && speed > 0) {
        float scale = max_speed / speed;
        d->movement.vx *= scale;
        d->movement.vy *= scale;
    }
}

void sprite_rotate_towards(sprite_t sprite, int target_x, int target_y,
                           float max_rotation) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d) return;

    int x, y;
    sprite_get_position(sprite, &x, &y);

    int dx = target_x - x;
    int dy = target_y - y;

    float target_heading = atan2f((float)dy, (float)dx) * RADIANS_TO_DEGREES;
    if (target_heading < 0) target_heading += 360.0f;

    float current_heading = d->movement.heading;
    float diff = target_heading - current_heading;

    if (diff > 180) diff -= 360;
    if (diff < -180) diff += 360;

    if (fabsf(diff) > max_rotation) {
        d->movement.heading += (diff > 0 ? max_rotation : -max_rotation);
    } else {
        d->movement.heading = target_heading;
    }

    if (d->movement.heading < 0) d->movement.heading += 360.0f;
    if (d->movement.heading >= 360) d->movement.heading -= 360.0f;
}

void sprite_move_towards(sprite_t sprite, int target_x, int target_y,
                         float acceleration) {
    sprite_dynamics_t *d = _ensure_dynamics(sprite);
    if (!d) return;

    int x, y;
    sprite_get_position(sprite, &x, &y);

    int dx = target_x - x;
    int dy = target_y - y;

    float dist = sqrtf((float)(dx * dx + dy * dy));
    if (dist > 0) {
        d->movement.vx += (acceleration * dx) / dist;
        d->movement.vy += (acceleration * dy) / dist;
    }
}

float sprite_distance_to(sprite_t sprite, int x, int y) {
    int sx, sy;
    if (!sprite_get_position(sprite, &sx, &sy)) return 0;

    int dx = x - sx;
    int dy = y - sy;

    return sqrtf((float)(dx * dx + dy * dy));
}

float sprite_distance_to_sprite(sprite_t sprite1, sprite_t sprite2) {
    int x1, y1, x2, y2;
    if (!sprite_get_position(sprite1, &x1, &y1)) return 0;
    if (!sprite_get_position(sprite2, &x2, &y2)) return 0;

    int dx = x2 - x1;
    int dy = y2 - y1;

    return sqrtf((float)(dx * dx + dy * dy));
}

/* ============================================================================
 * DEBUGGING & INSPECTION
 * ========================================================================== */

void sprite_print_dynamics(sprite_t sprite) {
    sprite_dynamics_t *d = _find_dynamics(sprite);
    if (!d) {
        printf("No dynamics for sprite\n");
        return;
    }

    printf("\n=== Sprite Dynamics ===\n");
    printf("Movement:\n");
    printf("  Velocity:     (%.2f, %.2f)\n", d->movement.vx, d->movement.vy);
    printf("  Acceleration: (%.2f, %.2f)\n", d->movement.ax, d->movement.ay);
    printf("  Heading:      %.1f°\n", d->movement.heading);
    printf("  Friction:     %.2f\n", d->movement.friction);
    printf("  Active:       %s\n", d->movement.active ? "Yes" : "No");

    printf("Animation:\n");
    printf("  Frames:       %d\n", d->animation.frame_count);
    printf("  Current:      %d\n", d->animation.current_frame);
    printf("  Playing:      %s\n", d->animation.playing ? "Yes" : "No");
    printf("  Mode:         %s\n",
           d->animation.mode == ANIM_ONCE ? "Once" :
           d->animation.mode == ANIM_LOOP ? "Loop" : "PingPong");
    printf("\n");
}
