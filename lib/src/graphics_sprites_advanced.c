/* graphics_sprites_advanced.c — Advanced Sprite Features (Phase 8)
 *
 * Animation sequences, movement patterns, sprite groups, and callbacks.
 */

#include <graphics_sprites_advanced.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * ANIMATION SEQUENCE IMPLEMENTATION
 * ============================================================================ */

sprite_animation_sequence_t sprite_animation_sequence_create(
    sprite_animation_t *animations, int count, int auto_advance) {
    sprite_animation_sequence_t seq;

    seq.animations = animations;
    seq.count = count;
    seq.current_animation = 0;
    seq.auto_advance = auto_advance;

    return seq;
}

void sprite_animation_sequence_update(sprite_t *sprite, sprite_animation_sequence_t *seq) {
    if (!sprite || !seq || seq->count == 0) return;

    sprite_animation_t *current = &seq->animations[seq->current_animation];

    /* Update current animation */
    if (current->is_playing && current->frame_count > 0) {
        current->frame_timer--;

        if (current->frame_timer <= 0) {
            current->current_frame++;

            if (current->current_frame >= current->frame_count) {
                if (current->is_looping) {
                    current->current_frame = 0;
                } else {
                    current->current_frame = current->frame_count - 1;
                    current->is_playing = 0;

                    /* Auto-advance to next animation if enabled */
                    if (seq->auto_advance && seq->current_animation < seq->count - 1) {
                        seq->current_animation++;
                        sprite_animation_sequence_next(sprite, seq);
                    }
                }
            }

            /* Update sprite rendering */
            sprite_frame_t *frame = &current->frames[current->current_frame];
            sprite->ch = frame->ch;
            sprite->color = frame->color;
            current->frame_timer = frame->duration;
        }
    }
}

void sprite_animation_sequence_next(sprite_t *sprite, sprite_animation_sequence_t *seq) {
    if (!sprite || !seq || seq->current_animation >= seq->count - 1) return;

    seq->current_animation++;
    sprite_animation_t *next = &seq->animations[seq->current_animation];

    next->is_playing = 1;
    next->current_frame = 0;
    if (next->frame_count > 0) {
        sprite->ch = next->frames[0].ch;
        sprite->color = next->frames[0].color;
        next->frame_timer = next->frames[0].duration;
    }
}

int sprite_animation_sequence_current(sprite_animation_sequence_t *seq) {
    if (!seq) return -1;
    return seq->current_animation;
}

void sprite_animation_sequence_done(sprite_animation_sequence_t *seq) {
    if (!seq) return;
    seq->animations = NULL;
    seq->count = 0;
}

/* ============================================================================
 * MOVEMENT PATTERN IMPLEMENTATION
 * ============================================================================ */

sprite_pattern_t sprite_pattern_create(movement_pattern_type_t type, int amplitude,
                                       int frequency, int duration) {
    sprite_pattern_t pattern;

    pattern.type = type;
    pattern.amplitude = amplitude;
    pattern.frequency = frequency;
    pattern.duration = duration;
    pattern.elapsed = 0;
    pattern.base_x = 0;
    pattern.base_y = 0;

    return pattern;
}

void sprite_pattern_apply(sprite_t *sprite, sprite_pattern_t *pattern) {
    if (!sprite || !pattern) return;

    pattern->base_x = sprite->x;
    pattern->base_y = sprite->y;

    int cycle = pattern->elapsed % (pattern->frequency * 2);

    switch (pattern->type) {
        case PATTERN_LINEAR:
            sprite->x += pattern->amplitude;
            break;

        case PATTERN_SINE:
            /* Sinusoidal motion (approximated) */
            if (cycle < pattern->frequency) {
                sprite->y = pattern->base_y - (pattern->amplitude * cycle) / pattern->frequency;
            } else {
                sprite->y = pattern->base_y + (pattern->amplitude * (cycle - pattern->frequency)) / pattern->frequency;
            }
            break;

        case PATTERN_CIRCLE:
            /* Circular motion (approximated with angle) */
            sprite->x = pattern->base_x + (pattern->amplitude * cycle) / (pattern->frequency * 2);
            sprite->y = pattern->base_y + pattern->amplitude / 2;
            break;

        case PATTERN_ZIGZAG:
            /* Zigzag motion */
            if (cycle < pattern->frequency / 2) {
                sprite->y = pattern->base_y - pattern->amplitude;
            } else {
                sprite->y = pattern->base_y + pattern->amplitude;
            }
            sprite->x += 1;
            break;

        case PATTERN_BOUNCE:
            /* Bouncing motion */
            int bounce_cycle = pattern->elapsed % (pattern->frequency * 2);
            if (bounce_cycle < pattern->frequency) {
                sprite->y = pattern->base_y - (pattern->amplitude * bounce_cycle) / pattern->frequency;
            } else {
                sprite->y = pattern->base_y + (pattern->amplitude * (bounce_cycle - pattern->frequency)) / pattern->frequency;
            }
            break;
    }

    pattern->elapsed++;
}

int sprite_pattern_finished(sprite_pattern_t *pattern) {
    if (!pattern || pattern->duration == 0) return 0;
    return pattern->elapsed >= pattern->duration;
}

void sprite_pattern_reset(sprite_pattern_t *pattern) {
    if (!pattern) return;
    pattern->elapsed = 0;
}

/* ============================================================================
 * SPRITE GROUP IMPLEMENTATION
 * ============================================================================ */

sprite_group_t sprite_group_create(int capacity) {
    sprite_group_t group;

    if (capacity <= 0 || capacity > SPRITE_GROUP_MAX_SPRITES) {
        capacity = SPRITE_GROUP_MAX_SPRITES;
    }

    group.sprites = (sprite_t **)malloc(capacity * sizeof(sprite_t *));
    group.capacity = group.sprites ? capacity : 0;
    group.count = 0;

    return group;
}

int sprite_group_add(sprite_group_t *group, sprite_t *sprite) {
    if (!group || !sprite || group->count >= group->capacity) return 0;

    group->sprites[group->count++] = sprite;
    return 1;
}

int sprite_group_remove(sprite_group_t *group, sprite_t *sprite) {
    if (!group || !sprite) return 0;

    for (int i = 0; i < group->count; i++) {
        if (group->sprites[i] == sprite) {
            for (int j = i; j < group->count - 1; j++) {
                group->sprites[j] = group->sprites[j + 1];
            }
            group->count--;
            return 1;
        }
    }

    return 0;
}

int sprite_group_count(sprite_group_t *group) {
    if (!group) return 0;
    return group->count;
}

void sprite_group_clear(sprite_group_t *group) {
    if (!group) return;
    group->count = 0;
}

void sprite_group_update_all(sprite_group_t *group) {
    if (!group || !group->sprites) return;

    for (int i = 0; i < group->count; i++) {
        if (group->sprites[i] && group->sprites[i]->active) {
            group->sprites[i]->update(group->sprites[i]);
        }
    }
}

void sprite_group_render_all(sprite_group_t *group, rrb_layer_t *layer) {
    if (!group || !group->sprites || !layer) return;

    for (int i = 0; i < group->count; i++) {
        if (group->sprites[i] && group->sprites[i]->active && group->sprites[i]->visible) {
            group->sprites[i]->render(group->sprites[i], layer);
        }
    }
}

void sprite_group_check_collisions(sprite_group_t *group, sprite_event_callback_t evt_callback) {
    if (!group || !group->sprites || !callback) return;

    for (int i = 0; i < group->count; i++) {
        for (int j = i + 1; j < group->count; j++) {
            sprite_t *s1 = group->sprites[i];
            sprite_t *s2 = group->sprites[j];

            if (s1 && s2 && s1->active && s2->active) {
                if (sprite_collides(s1, s2)) {
                    evt_callback(s1, SPRITE_EVENT_COLLISION, (int)(long)s2);
                    evt_callback(s2, SPRITE_EVENT_COLLISION, (int)(long)s1);
                }
            }
        }
    }
}

void sprite_group_destroy(sprite_group_t *group) {
    if (!group || !group->sprites) return;

    free(group->sprites);
    group->sprites = NULL;
    group->capacity = 0;
    group->count = 0;
}

/* ============================================================================
 * EVENT CALLBACK SYSTEM
 * ============================================================================ */

void sprite_set_event_evt_callback(sprite_t *sprite, sprite_event_callback_t evt_callback) {
    if (!sprite) return;
    /* Store callback in user_data (simple implementation) */
    sprite->user_data = (int)(long)callback;
}

void sprite_dispatch_event(sprite_t *sprite, sprite_event_type_t event, int param) {
    if (!sprite || !sprite->user_data) return;

    sprite_event_callback_t evt_callback = (sprite_event_callback_t)(long)sprite->user_data;
    evt_callback(sprite, event, param);
}

int sprite_is_out_of_bounds(sprite_t *sprite, int max_x, int max_y) {
    if (!sprite) return 0;

    int char_x = sprite->x / 8;
    int char_y = sprite->y;

    return (char_x < 0 || char_x >= max_x || char_y < 0 || char_y >= max_y);
}

/* ============================================================================
 * ADVANCED SPRITE OPERATIONS
 * ============================================================================ */

void sprite_set_scale(sprite_t *sprite, int scale_percent) {
    if (!sprite || scale_percent < 50 || scale_percent > SPRITE_MAX_SCALE) return;
    /* Scale stored in collision_box.width for simplicity */
    sprite->collision_box.width = (sprite->collision_box.width * scale_percent) / 100;
}

int sprite_get_scale(sprite_t *sprite) {
    if (!sprite || !sprite->collision_box.width) return 100;
    return (sprite->collision_box.width * 100) / 16;  /* Assume base width 16 */
}

void sprite_apply_force(sprite_t *sprite, int fx, int fy) {
    if (!sprite) return;
    sprite->vx += fx;
    sprite->vy += fy;
}

int sprite_distance(sprite_t *sprite1, sprite_t *sprite2) {
    if (!sprite1 || !sprite2) return 0;

    int dx = sprite1->x - sprite2->x;
    int dy = sprite1->y - sprite2->y;

    /* Simple Manhattan distance approximation */
    return (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
}

void sprite_move_towards(sprite_t *sprite, int target_x, int target_y, int speed) {
    if (!sprite || speed == 0) return;

    int dx = target_x - sprite->x;
    int dy = target_y - sprite->y;

    if (dx > 0) sprite->vx = speed;
    else if (dx < 0) sprite->vx = -speed;
    else sprite->vx = 0;

    if (dy > 0) sprite->vy = speed;
    else if (dy < 0) sprite->vy = -speed;
    else sprite->vy = 0;
}
