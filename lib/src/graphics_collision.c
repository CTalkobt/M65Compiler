/* graphics_collision.c — Optimized Collision Detection (Phase 103b)
 *
 * Pixel-perfect collision with collision masks and spatial acceleration.
 * Includes bounding-box fast-path and configurable collision detection.
 */

#include <graphics.h>
#include <string.h>

/* ============================================================================
 * COLLISION MASK SUPPORT
 * ============================================================================ */

/**
 * Build collision mask from sprite bitmap
 * Mask is 1 bit per pixel (set if pixel non-zero)
 */
static unsigned char *collision_mask_from_bitmap(unsigned char *bitmap,
                                                   int width, int height) {
    if (!bitmap) return NULL;

    /* Calculate mask size (1 bit per pixel, padded to bytes) */
    int mask_pitch = (width + 7) / 8;
    int mask_size = mask_pitch * height;
    unsigned char *mask = (unsigned char *)malloc(mask_size);

    if (!mask) return NULL;

    memset(mask, 0, mask_size);

    /* Generate mask from bitmap */
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (bitmap[y * width + x]) {
                int byte_idx = y * mask_pitch + (x / 8);
                int bit_idx = 7 - (x % 8);
                mask[byte_idx] |= (1 << bit_idx);
            }
        }
    }

    return mask;
}

/* ============================================================================
 * FAST BOUNDING-BOX COLLISION (AABB)
 * ============================================================================ */

int sprite_collides(sprite_t *a, sprite_t *b) {
    /* Axis-Aligned Bounding Box collision test
     * Extremely fast O(1) test, used as first-pass filter */
    return !(a->x + a->width <= b->x ||
             a->y + a->height <= b->y ||
             b->x + b->width <= a->x ||
             b->y + b->height <= a->y);
}

/* ============================================================================
 * OPTIMIZED PIXEL-PERFECT COLLISION
 * ============================================================================ */

int sprite_collides_precise(sprite_t *a, sprite_t *b) {
    /* Pixel-perfect collision with optimizations:
     * 1. Fast AABB test first (O(1))
     * 2. Compute overlap region (avoid out-of-bounds)
     * 3. Early exit on first pixel match
     */

    if (!a->bitmap || !b->bitmap) {
        return sprite_collides(a, b);  /* Fallback to AABB */
    }

    /* FAST PATH: Bounding-box test first */
    if (!sprite_collides(a, b)) {
        return 0;
    }

    /* Compute overlap region in world space */
    int overlap_x1 = (a->x > b->x) ? a->x : b->x;
    int overlap_y1 = (a->y > b->y) ? a->y : b->y;
    int overlap_x2 = (a->x + a->width < b->x + b->width) ?
                     (a->x + a->width) : (b->x + b->width);
    int overlap_y2 = (a->y + a->height < b->y + b->height) ?
                     (a->y + a->height) : (b->y + b->height);

    /* Check pixels in overlap region */
    for (int py = overlap_y1; py < overlap_y2; py++) {
        for (int px = overlap_x1; px < overlap_x2; px++) {
            /* Convert world coordinates to sprite-local coordinates */
            int ax = px - a->x;
            int ay = py - a->y;
            int bx = px - b->x;
            int by = py - b->y;

            /* Bounds check (safety) */
            if (ax < 0 || ax >= a->width || ay < 0 || ay >= a->height) continue;
            if (bx < 0 || bx >= b->width || by < 0 || by >= b->height) continue;

            /* Get pixels */
            unsigned char a_pixel = a->bitmap[ay * a->width + ax];
            unsigned char b_pixel = b->bitmap[by * b->width + bx];

            /* Early exit on first collision */
            if (a_pixel && b_pixel) {
                return 1;
            }
        }
    }

    return 0;
}

/* ============================================================================
 * COLLISION DETECTION WITH COLOR-BASED MASKING
 * ============================================================================ */

/**
 * Detect collision where only specific colors in bitmap trigger collision
 * Useful for: hit detection without transparency, selective collision
 */
int sprite_collides_color(sprite_t *a, sprite_t *b,
                           unsigned char mask_color_a,
                           unsigned char mask_color_b) {
    if (!a->bitmap || !b->bitmap) {
        return sprite_collides(a, b);
    }

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

    /* Check pixels matching mask colors */
    for (int py = overlap_y1; py < overlap_y2; py++) {
        for (int px = overlap_x1; px < overlap_x2; px++) {
            int ax = px - a->x;
            int ay = py - a->y;
            int bx = px - b->x;
            int by = py - b->y;

            if (ax < 0 || ax >= a->width || ay < 0 || ay >= a->height) continue;
            if (bx < 0 || bx >= b->width || by < 0 || by >= b->height) continue;

            unsigned char a_pixel = a->bitmap[ay * a->width + ax];
            unsigned char b_pixel = b->bitmap[by * b->width + bx];

            /* Only collide if both match their mask colors */
            if (a_pixel == mask_color_a && b_pixel == mask_color_b) {
                return 1;
            }
        }
    }

    return 0;
}

/* ============================================================================
 * CIRCLE-BASED COLLISION (For circular sprites)
 * ============================================================================ */

/**
 * Detect collision between two circular sprites
 * Faster than pixel-perfect for circular shapes
 */
int sprite_collides_circle(sprite_t *a, sprite_t *b) {
    /* Compute circle centers and radii
     * Assume sprite width/height determine circle size */
    int a_cx = a->x + a->width / 2;
    int a_cy = a->y + a->height / 2;
    int a_r = a->width / 2;

    int b_cx = b->x + b->width / 2;
    int b_cy = b->y + b->height / 2;
    int b_r = b->width / 2;

    /* Distance between centers */
    int dx = a_cx - b_cx;
    int dy = a_cy - b_cy;

    /* Rough check: distance squared vs sum of radii squared
     * Avoids expensive sqrt() */
    int dist_sq = dx * dx + dy * dy;
    int radii_sum = a_r + b_r;
    int radii_sum_sq = radii_sum * radii_sum;

    return dist_sq < radii_sum_sq;
}

/* ============================================================================
 * SPATIAL QUERY (Find all sprites in region)
 * ============================================================================ */

/**
 * Check if sprite overlaps with rectangular region
 * Useful for: query-based collision, spatial indexing
 */
int sprite_overlaps_region(sprite_t *spr, int x1, int y1, int x2, int y2) {
    return !(spr->x + spr->width <= x1 ||
             spr->y + spr->height <= y1 ||
             x2 <= spr->x ||
             y2 <= spr->y);
}
