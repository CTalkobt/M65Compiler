/* graphics_sprites.c — Soft Sprite System Implementation (Phase 7)
 *
 * Object-oriented sprite engine using RRB SPARSE layers.
 * Efficient pooling, animation, physics, and collision detection.
 */

#include <graphics_sprites.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * SPRITE TRIVIAL METHODS (auto-inlined by compiler)
 * ============================================================================ */

void sprite_t__set_position(sprite_t *this, int x, int y) {
    if (!this) return;
    this->x = x;
    this->y = y;
}

void sprite_t__set_velocity(sprite_t *this, int vx, int vy) {
    if (!this) return;
    this->vx = vx;
    this->vy = vy;
}

void sprite_t__set_acceleration(sprite_t *this, int ax, int ay) {
    if (!this) return;
    this->ax = ax;
    this->ay = ay;
}

void sprite_t__show(sprite_t *this) {
    if (!this) return;
    this->visible = 1;
}

void sprite_t__hide(sprite_t *this) {
    if (!this) return;
    this->visible = 0;
}

/* ============================================================================
 * SPRITE ANIMATION METHODS
 * ============================================================================ */

void sprite_t__set_animation(sprite_t *this, sprite_animation_t *anim) {
    if (!this || !anim) return;
    this->animation = *anim;
    this->animation.current_frame = 0;
    this->animation.frame_timer = 0;
    if (anim->frame_count > 0) {
        this->ch = anim->frames[0].ch;
        this->color = anim->frames[0].color;
        this->animation.frame_timer = anim->frames[0].duration;
    }
}

void sprite_t__play_animation(sprite_t *this) {
    if (!this) return;
    this->animation.is_playing = 1;
}

void sprite_t__stop_animation(sprite_t *this) {
    if (!this) return;
    this->animation.is_playing = 0;
}

/* ============================================================================
 * SPRITE UPDATE & RENDERING
 * ============================================================================ */

void sprite_t__update(sprite_t *this) {
    if (!this || !this->active) return;

    /* Update velocity based on acceleration */
    this->vx += this->ax;
    this->vy += this->ay;

    /* Update position based on velocity */
    this->x += this->vx;
    this->y += this->vy;

    /* Update animation */
    if (this->animation.is_playing && this->animation.frame_count > 0) {
        this->animation.frame_timer--;

        if (this->animation.frame_timer <= 0) {
            /* Advance to next frame */
            this->animation.current_frame++;

            if (this->animation.current_frame >= this->animation.frame_count) {
                if (this->animation.is_looping) {
                    this->animation.current_frame = 0;
                } else {
                    this->animation.current_frame = this->animation.frame_count - 1;
                    this->animation.is_playing = 0;
                }
            }

            /* Update character and color from current frame */
            sprite_frame_t *frame = &this->animation.frames[this->animation.current_frame];
            this->ch = frame->ch;
            this->color = frame->color;
            this->animation.frame_timer = frame->duration;
        }
    }
}

void sprite_t__render(sprite_t *this, rrb_layer_t *layer) {
    if (!this || !layer || !this->active || !this->visible) return;

    /* Convert pixel position to character grid */
    int char_x = this->x / 8;      /* 8 pixels per character */
    int char_y = this->y;           /* Already in row units */

    /* Bounds check */
    if (char_x < 0 || char_x >= layer->width || char_y < 0 || char_y >= layer->height) {
        return;  /* Out of bounds */
    }

    /* Render sprite character */
    layer->set_char(layer, char_x, char_y, this->ch, this->color);
}

/* ============================================================================
 * SPRITE MANAGER TRIVIAL METHODS
 * ============================================================================ */

int sprite_manager_t__get_active_count(sprite_manager_t *this) {
    if (!this) return 0;
    return this->active_count;
}

/* ============================================================================
 * SPRITE MANAGER IMPLEMENTATION
 * ============================================================================ */

sprite_manager_t sprite_manager_create(int max_sprites, rrb_system_t *rrb, int layer_index) {
    sprite_manager_t manager;

    if (max_sprites <= 0 || max_sprites > SPRITE_MAX_SPRITES) {
        max_sprites = SPRITE_MAX_SPRITES;
    }

    manager.sprites = (sprite_t *)malloc(max_sprites * sizeof(sprite_t));
    if (!manager.sprites) {
        manager.max_sprites = 0;
        manager.active_count = 0;
        manager.rrb = NULL;
        manager.sprite_layer_index = -1;
        return manager;
    }

    /* Initialize all sprites as inactive */
    for (int i = 0; i < max_sprites; i++) {
        sprite_t *sprite = &manager.sprites[i];
        memset(sprite, 0, sizeof(sprite_t));
        sprite->active = 0;
        sprite->visible = 1;
        sprite->ch = 'S';
        sprite->color = 0x0F;
        sprite->layer_index = layer_index;
        sprite->collision_box.enabled = 0;

        /* Initialize method pointers */
        sprite->set_position = &sprite_t__set_position;
        sprite->set_velocity = &sprite_t__set_velocity;
        sprite->set_acceleration = &sprite_t__set_acceleration;
        sprite->set_animation = &sprite_t__set_animation;
        sprite->play_animation = &sprite_t__play_animation;
        sprite->stop_animation = &sprite_t__stop_animation;
        sprite->show = &sprite_t__show;
        sprite->hide = &sprite_t__hide;
        sprite->update = &sprite_t__update;
        sprite->render = &sprite_t__render;
    }

    manager.max_sprites = max_sprites;
    manager.active_count = 0;
    manager.rrb = rrb;
    manager.sprite_layer_index = layer_index;

    /* Initialize manager method pointers */
    manager.allocate = &sprite_allocate;
    manager.free = &sprite_free;
    manager.update_all = &sprite_update_all;
    manager.render_all = &sprite_render_all;
    manager.get_active_count = &sprite_manager_t__get_active_count;
    manager.clear = &sprite_manager_clear;

    return manager;
}

void sprite_manager_destroy(sprite_manager_t *manager) {
    if (!manager || !manager->sprites) return;

    free(manager->sprites);
    manager->sprites = NULL;
    manager->max_sprites = 0;
    manager->active_count = 0;
}

sprite_t *sprite_allocate(sprite_manager_t *manager) {
    if (!manager || !manager->sprites) return NULL;

    /* Find first inactive sprite */
    for (int i = 0; i < manager->max_sprites; i++) {
        if (!manager->sprites[i].active) {
            sprite_t *sprite = &manager->sprites[i];
            sprite->active = 1;
            sprite->visible = 1;
            manager->active_count++;
            return sprite;
        }
    }

    return NULL;  /* Pool exhausted */
}

void sprite_free(sprite_manager_t *manager, sprite_t *sprite) {
    if (!manager || !sprite || !sprite->active) return;

    sprite->active = 0;
    sprite->animation.is_playing = 0;
    manager->active_count--;
}

void sprite_update_all(sprite_manager_t *manager) {
    if (!manager || !manager->sprites) return;

    for (int i = 0; i < manager->max_sprites; i++) {
        sprite_t *sprite = &manager->sprites[i];
        if (sprite->active) {
            sprite->update(sprite);
        }
    }
}

void sprite_render_all(sprite_manager_t *manager) {
    if (!manager || !manager->rrb || !manager->sprites) return;

    rrb_layer_t *layer = manager->rrb->get_layer(manager->rrb, manager->sprite_layer_index);
    if (!layer) return;

    for (int i = 0; i < manager->max_sprites; i++) {
        sprite_t *sprite = &manager->sprites[i];
        if (sprite->active && sprite->visible) {
            sprite->render(sprite, layer);
        }
    }
}

void sprite_manager_clear(sprite_manager_t *manager) {
    if (!manager || !manager->sprites) return;

    for (int i = 0; i < manager->max_sprites; i++) {
        manager->sprites[i].active = 0;
        manager->sprites[i].animation.is_playing = 0;
    }

    manager->active_count = 0;
}

/* ============================================================================
 * COLLISION DETECTION
 * ============================================================================ */

int sprite_collides(sprite_t *sprite1, sprite_t *sprite2) {
    if (!sprite1 || !sprite2 || !sprite1->active || !sprite2->active) return 0;
    if (!sprite1->collision_box.enabled || !sprite2->collision_box.enabled) return 0;

    /* Simple AABB collision detection */
    int s1_left = sprite1->x + sprite1->collision_box.x_offset;
    int s1_right = s1_left + sprite1->collision_box.width;
    int s1_top = sprite1->y + sprite1->collision_box.y_offset;
    int s1_bottom = s1_top + sprite1->collision_box.height;

    int s2_left = sprite2->x + sprite2->collision_box.x_offset;
    int s2_right = s2_left + sprite2->collision_box.width;
    int s2_top = sprite2->y + sprite2->collision_box.y_offset;
    int s2_bottom = s2_top + sprite2->collision_box.height;

    return !(s1_right < s2_left || s1_left > s2_right ||
             s1_bottom < s2_top || s1_top > s2_bottom);
}

/* ============================================================================
 * ANIMATION UTILITIES
 * ============================================================================ */

sprite_animation_t sprite_animation_create(sprite_frame_t *frames, int count, int looping) {
    sprite_animation_t anim;

    anim.frames = frames;
    anim.frame_count = count;
    anim.current_frame = 0;
    anim.frame_timer = 0;
    anim.is_looping = looping;
    anim.is_playing = 0;

    return anim;
}
