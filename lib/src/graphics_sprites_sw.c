/* graphics_sprites_sw.c — Software Sprite Rendering (Phase 103a)
 *
 * Renders sprites to framebuffer using current graphics mode's primitives.
 * Works on all graphics modes: text, block-char, bitmap.
 */

#include <graphics.h>
#include <string.h>

/* ============================================================================
 * SOFTWARE SPRITE RENDERING BY MODE
 * ============================================================================ */

/* Simple sprite blit for bitmap modes */
static void sprite_blit_bitmap(sprite_t *spr) {
    if (!spr->bitmap) return;

    int x = spr->x;
    int y = spr->y;
    int w = spr->width;
    int h = spr->height;

    /* Draw filled rectangle for sprite footprint (simplified for now) */
    graphics_setcolor(spr->color);
    graphics_bar(x, y, x + w - 1, y + h - 1);
}

/* Sprite rendering for text modes */
static void sprite_blit_text(sprite_t *spr) {
    if (!spr->bitmap) return;

    /* For text modes, render sprite as block of characters
     * This is a simplified implementation */
    int x = spr->x;
    int y = spr->y;

    graphics_setcolor(spr->color);
    graphics_plot(x, y);  /* Just mark position for now */
}

/* ============================================================================
 * UNIFIED SOFTWARE RENDERING
 * ============================================================================ */

void sprite_render_sw(sprite_t *spr) {
    if (!spr->bitmap) return;

    graphics_mode_t mode = graphics_getmode();

    /* Render based on current graphics mode */
    switch (mode) {
        case GRAPHICS_MODE_BITMAP_320x200:
        case GRAPHICS_MODE_BITMAP_160x200:
            sprite_blit_bitmap(spr);
            break;

        case GRAPHICS_MODE_TEXT_40x25:
        case GRAPHICS_MODE_TEXT_80x24:
        case GRAPHICS_MODE_GRAPHICS_80x50:
            sprite_blit_text(spr);
            break;

        default:
            /* Unknown mode, skip rendering */
            break;
    }
}

/* ============================================================================
 * SOFTWARE SPRITE ERASING
 * ============================================================================ */

void sprite_erase_sw(sprite_t *spr) {
    /* Erase sprite by redrawing background
     * For now, use black color to erase
     * Future: save background and restore
     */
    graphics_setcolor(0);  /* Black = erase */
    graphics_bar(spr->old_x, spr->old_y,
                 spr->old_x + spr->width - 1,
                 spr->old_y + spr->height - 1);
}

/* ============================================================================
 * STUB FUNCTIONS (Hardware interface, no-ops for software path)
 * ============================================================================ */

/* These are called from graphics_sprites.c but are no-ops for software mode */
void sprite_upload_bitmap_hw(sprite_t *spr) {
    /* No-op for software sprites */
    (void)spr;
}

void sprite_set_color_hw(sprite_t *spr) {
    /* No-op for software sprites */
    (void)spr;
}
