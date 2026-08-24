/* graphics_sprites.c — Unified Sprite Framework (Phase 103)
 *
 * Software-rendered sprites with optional hardware acceleration.
 * Single API works across all graphics modes and platforms.
 */

#include <graphics.h>
#include <stdlib.h>
#include <string.h>

/* Hardware sprite tracking (MEGA65 only) */
#define MAX_HW_SPRITES 8
static sprite_t *hw_sprite_table[MAX_HW_SPRITES];
static unsigned char hw_sprite_count = 0;

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

/* Software rendering (graphics_sprites_sw.c) */
void sprite_render_sw(sprite_t *spr);
void sprite_erase_sw(sprite_t *spr);

/* Hardware rendering (graphics_sprites_hw.c) */
int sprite_alloc_hw(sprite_t *spr);
void sprite_free_hw(sprite_t *spr);
void sprite_render_hw(sprite_t *spr);
void sprite_erase_hw(sprite_t *spr);
void sprite_upload_bitmap_hw(sprite_t *spr);
void sprite_set_color_hw(sprite_t *spr);

/* ============================================================================
 * SPRITE INITIALIZATION & CLEANUP
 * ============================================================================ */

int sprite_init(sprite_t *spr, int width, int height) {
    spr->width = width;
    spr->height = height;
    spr->x = 0;
    spr->y = 0;
    spr->color = 1;                 /* Default color: white */
    spr->visible = 1;
    spr->frame = 0;
    spr->bitmap = NULL;
    spr->old_x = 0;
    spr->old_y = 0;

    /* Auto-detect render mode based on hardware availability */
#ifdef __MEGA65__
    /* Try hardware sprite if size fits and slots available */
    if (width <= 64 && height <= 64 && hw_sprite_count < MAX_HW_SPRITES) {
        if (sprite_alloc_hw(spr) == 0) {
            spr->render_mode = SPRITE_MODE_HARDWARE;
            return 0;
        }
    }
#endif

    /* Default to software rendering */
    spr->render_mode = SPRITE_MODE_SOFTWARE;
    spr->sprite_num = 0xFF;         /* Invalid sprite number */
    return 0;
}

void sprite_done(sprite_t *spr) {
    if (spr->render_mode == SPRITE_MODE_HARDWARE) {
        sprite_free_hw(spr);
    }
    spr->visible = 0;
    spr->bitmap = NULL;
}

/* ============================================================================
 * SPRITE POSITIONING & GRAPHICS
 * ============================================================================ */

void sprite_set_position(sprite_t *spr, int x, int y) {
    spr->old_x = spr->x;
    spr->old_y = spr->y;
    spr->x = x;
    spr->y = y;
}

void sprite_set_bitmap(sprite_t *spr, unsigned char *bitmap) {
    spr->bitmap = bitmap;

    /* For hardware sprites, upload bitmap data to sprite area */
    if (spr->render_mode == SPRITE_MODE_HARDWARE) {
        sprite_upload_bitmap_hw(spr);
    }
}

void sprite_set_color(sprite_t *spr, unsigned char color) {
    spr->color = color;

    /* For hardware sprites, update color register immediately */
    if (spr->render_mode == SPRITE_MODE_HARDWARE) {
        sprite_set_color_hw(spr);
    }
}

/* ============================================================================
 * SPRITE RENDERING
 * ============================================================================ */

void sprite_draw(sprite_t *spr) {
    if (!spr->visible) return;

    if (spr->render_mode == SPRITE_MODE_HARDWARE) {
        sprite_render_hw(spr);
    } else {
        sprite_render_sw(spr);
    }
}

void sprite_clear(sprite_t *spr) {
    if (spr->render_mode == SPRITE_MODE_HARDWARE) {
        sprite_erase_hw(spr);
    } else {
        sprite_erase_sw(spr);
    }
}

/* ============================================================================
 * SPRITE VISIBILITY
 * ============================================================================ */

void sprite_show(sprite_t *spr) {
    spr->visible = 1;
}

void sprite_hide(sprite_t *spr) {
    spr->visible = 0;

    if (spr->render_mode == SPRITE_MODE_HARDWARE) {
        sprite_erase_hw(spr);
    }
}

/* ============================================================================
 * COLLISION DETECTION
 * ============================================================================ */

int sprite_collides(sprite_t *a, sprite_t *b) {
    /* Bounding-box collision test */
    return !(a->x + a->width <= b->x ||
             a->y + a->height <= b->y ||
             b->x + b->width <= a->x ||
             b->y + b->height <= a->y);
}

int sprite_collides_precise(sprite_t *a, sprite_t *b) {
    /* Pixel-perfect collision test
     * Requires both sprites have bitmap data */
    if (!a->bitmap || !b->bitmap) {
        return sprite_collides(a, b);  /* Fallback to bounding-box */
    }

    /* Check if bounding boxes overlap first (fast path) */
    if (!sprite_collides(a, b)) {
        return 0;
    }

    /* Compute overlap region */
    int overlap_x1 = (a->x > b->x) ? a->x : b->x;
    int overlap_y1 = (a->y > b->y) ? a->y : b->y;
    int overlap_x2 = (a->x + a->width < b->x + b->width) ?
                     (a->x + a->width) : (b->x + b->width);
    int overlap_y2 = (a->y + a->height < b->y + b->height) ?
                     (a->y + a->height) : (b->y + b->height);

    /* Check pixels in overlap region */
    for (int py = overlap_y1; py < overlap_y2; py++) {
        for (int px = overlap_x1; px < overlap_x2; px++) {
            int ax = px - a->x;
            int ay = py - a->y;
            int bx = px - b->x;
            int by = py - b->y;

            /* Get pixel from each sprite's bitmap */
            unsigned char a_pixel = a->bitmap[ay * a->width + ax];
            unsigned char b_pixel = b->bitmap[by * b->width + bx];

            /* If both pixels are non-zero, collision detected */
            if (a_pixel && b_pixel) {
                return 1;
            }
        }
    }

    return 0;  /* No pixel collision */
}
