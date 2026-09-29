/* graphics_animation.c — Frame-Based Animation System (Phase 104a)
 *
 * Sprite animation with frame sequences, state machine, and callbacks.
 * Supports looping, ping-pong, and event-driven animation control.
 */

#include <graphics.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * ANIMATION STATE MACHINE
 * ============================================================================ */

/**
 * Initialize sprite animation
 *
 * Creates animation player for sprite with frame sequence.
 * Animation starts in STOPPED state.
 */
int sprite_animation_init(sprite_animation_t *anim, sprite_t *sprite,
                          unsigned char **frames, int frame_count,
                          int ticks_per_frame) {
    if (!anim || !sprite || !frames || frame_count <= 0) {
        return -1;
    }

    anim->sprite = sprite;
    anim->frames = frames;
    anim->frame_count = frame_count;
    anim->ticks_per_frame = ticks_per_frame;
    anim->current_frame = 0;
    anim->tick_counter = 0;
    anim->state = ANIM_STATE_STOPPED;
    anim->loop_mode = ANIM_LOOP_ONCE;
    anim->playback_speed = 1.0;  /* 1.0 = normal speed */
    anim->on_frame_change = NULL;
    anim->on_loop_complete = NULL;
    anim->on_animation_done = NULL;
    anim->user_data = NULL;

    /* Set initial frame */
    if (sprite) {
        sprite_set_bitmap(sprite, frames[0]);
    }

    return 0;
}

/**
 * Cleanup animation
 */
void sprite_animation_done(sprite_animation_t *anim) {
    if (anim) {
        anim->state = ANIM_STATE_STOPPED;
        anim->frames = NULL;
    }
}

/* ============================================================================
 * ANIMATION PLAYBACK CONTROL
 * ============================================================================ */

void sprite_animation_play(sprite_animation_t *anim) {
    if (!anim) return;
    if (anim->state != ANIM_STATE_PLAYING) {
        anim->state = ANIM_STATE_PLAYING;
        anim->current_frame = 0;
        anim->tick_counter = 0;
    }
}

void sprite_animation_stop(sprite_animation_t *anim) {
    if (!anim) return;
    anim->state = ANIM_STATE_STOPPED;
    anim->current_frame = 0;
    anim->tick_counter = 0;
}

void sprite_animation_pause(sprite_animation_t *anim) {
    if (!anim) return;
    if (anim->state == ANIM_STATE_PLAYING) {
        anim->state = ANIM_STATE_PAUSED;
    }
}

void sprite_animation_resume(sprite_animation_t *anim) {
    if (!anim) return;
    if (anim->state == ANIM_STATE_PAUSED) {
        anim->state = ANIM_STATE_PLAYING;
    }
}

/* ============================================================================
 * ANIMATION UPDATE & FRAME MANAGEMENT
 * ============================================================================ */

int sprite_animation_get_frame(sprite_animation_t *anim) {
    if (!anim) return -1;
    return anim->current_frame;
}

void sprite_animation_set_frame(sprite_animation_t *anim, int frame) {
    if (!anim || frame < 0 || frame >= anim->frame_count) return;

    if (anim->current_frame != frame) {
        anim->current_frame = frame;
        anim->tick_counter = 0;

        /* Update sprite bitmap */
        if (anim->sprite && anim->frames) {
            sprite_set_bitmap(anim->sprite, anim->frames[frame]);
        }

        /* Fire frame-change callback */
        if (anim->on_frame_change) {
            anim->on_frame_change(anim, frame);
        }
    }
}

void sprite_animation_set_speed(sprite_animation_t *anim, double speed) {
    if (!anim || speed <= 0) return;
    anim->playback_speed = speed;
}

/* ============================================================================
 * ANIMATION UPDATE (MAIN LOOP)
 * ============================================================================ */

void sprite_animation_update(sprite_animation_t *anim) {
    if (!anim || anim->state != ANIM_STATE_PLAYING) {
        return;
    }

    /* Advance tick counter with playback speed */
    anim->tick_counter += (int)anim->playback_speed;

    /* Check if time to advance to next frame */
    if (anim->tick_counter >= anim->ticks_per_frame) {
        anim->tick_counter -= anim->ticks_per_frame;

        int next_frame = anim->current_frame + 1;
        int loop_happened = 0;

        /* Handle end of animation based on loop mode */
        if (next_frame >= anim->frame_count) {
            loop_happened = 1;

            switch (anim->loop_mode) {
                case ANIM_LOOP_ONCE:
                    /* Stop at last frame */
                    anim->state = ANIM_STATE_STOPPED;
                    if (anim->on_animation_done) {
                        anim->on_animation_done(anim);
                    }
                    return;

                case ANIM_LOOP_REPEAT:
                    /* Restart from frame 0 */
                    next_frame = 0;
                    break;

                case ANIM_LOOP_PINGPONG:
                    /* Reverse direction */
                    if (anim->direction == ANIM_DIR_FORWARD) {
                        anim->direction = ANIM_DIR_BACKWARD;
                        next_frame = anim->frame_count - 2;
                    } else {
                        anim->direction = ANIM_DIR_FORWARD;
                        next_frame = 1;
                    }
                    break;

                default:
                    next_frame = 0;
                    break;
            }
        }

        /* Update frame */
        anim->current_frame = next_frame;
        anim->tick_counter = 0;

        /* Update sprite bitmap */
        if (anim->sprite && anim->frames) {
            sprite_set_bitmap(anim->sprite, anim->frames[anim->current_frame]);
        }

        /* Fire callbacks */
        if (loop_happened && anim->on_loop_complete) {
            anim->on_loop_complete(anim);
        }

        if (anim->on_frame_change) {
            anim->on_frame_change(anim, anim->current_frame);
        }
    }
}

/* ============================================================================
 * ANIMATION SEQUENCING (Play multiple animations in sequence)
 * ============================================================================ */

int sprite_animation_sequence_init(sprite_animation_sequence_t *seq) {
    if (!seq) return -1;
    seq->animations = NULL;
    seq->animation_count = 0;
    seq->current_animation = 0;
    seq->on_sequence_complete = NULL;
    return 0;
}

void sprite_animation_sequence_add(sprite_animation_sequence_t *seq,
                                    sprite_animation_t *anim) {
    if (!seq || !anim) return;

    /* Resize animations array */
    if (seq->animation_count == 0) {
        seq->animations = (sprite_animation_t **)malloc(sizeof(sprite_animation_t *));
    } else {
        sprite_animation_t **new_anims = (sprite_animation_t **)malloc(
            (seq->animation_count + 1) * sizeof(sprite_animation_t *));
        if (!new_anims) return;

        for (int i = 0; i < seq->animation_count; i++) {
            new_anims[i] = seq->animations[i];
        }
        free(seq->animations);
        seq->animations = new_anims;
    }

    seq->animations[seq->animation_count] = anim;
    seq->animation_count++;
}

void sprite_animation_sequence_play(sprite_animation_sequence_t *seq) {
    if (!seq || seq->animation_count == 0) return;

    seq->current_animation = 0;
    sprite_animation_play(seq->animations[0]);
}

void sprite_animation_sequence_update(sprite_animation_sequence_t *seq) {
    if (!seq || seq->animation_count == 0) return;

    sprite_animation_t *current = seq->animations[seq->current_animation];

    /* Update current animation */
    sprite_animation_update(current);

    /* Check if animation is done */
    if (current->state == ANIM_STATE_STOPPED &&
        current->loop_mode == ANIM_LOOP_ONCE) {

        /* Move to next animation */
        seq->current_animation++;

        if (seq->current_animation >= seq->animation_count) {
            /* Sequence complete */
            if (seq->on_sequence_complete) {
                seq->on_sequence_complete(seq);
            }
        } else {
            /* Play next animation */
            sprite_animation_play(seq->animations[seq->current_animation]);
        }
    }
}

void sprite_animation_sequence_done(sprite_animation_sequence_t *seq) {
    if (!seq) return;

    for (int i = 0; i < seq->animation_count; i++) {
        sprite_animation_done(seq->animations[i]);
    }

    free(seq->animations);
    seq->animations = NULL;
    seq->animation_count = 0;
}
