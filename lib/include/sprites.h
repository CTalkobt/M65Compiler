/* sprites.h — Unified Sprite System for MEGA65
 *
 * Provides resolution-agnostic sprite management for both 320-pixel (standard)
 * and 640-pixel (H640) display modes. Uses polymorphism to automatically
 * select the appropriate implementation (Sprite320 or Sprite640) based on
 * current display resolution.
 *
 * Design Pattern: Virtual methods via function pointers allow resolution-specific
 * optimizations while maintaining a unified public API. The sprite_create()
 * function returns the appropriate sprite type for the current mode.
 *
 * Usage:
 *   #include <sprites.h>
 *   sprite_init();
 *   sprite_t sprite = sprite_create(100, 100, 32, 32);
 *   sprite_set_position(sprite, 150, 150);
 *   sprite_render_all();
 */

#ifndef SPRITES_H
#define SPRITES_H

#include <stdint.h>

/* ============================================================================
 * SPRITE TYPE DEFINITION
 * ========================================================================== */

typedef void *sprite_t;  /* Opaque sprite handle (Sprite320 or Sprite640) */

#define INVALID_SPRITE  NULL

/* ============================================================================
 * SPRITE INFORMATION STRUCTURE
 * ========================================================================== */

typedef struct {
    sprite_t id;            /* Sprite handle */
    int x, y;               /* Position (pixels, resolution-aware) */
    int width, height;      /* Dimensions (pixels) */
    int visible;            /* Visibility flag (0-1) */
    int layer;              /* Z-order layer (0-255) */
    int color;              /* Primary color (0-15) */
    uint8_t *bitmap;        /* Bitmap data pointer */
    int bitmap_stride;      /* Bytes per row in bitmap */
    int is_sprite640;       /* 1 if Sprite640, 0 if Sprite320 */
} sprite_info_t;

/* ============================================================================
 * INITIALIZATION & MODE CONTROL
 * ========================================================================== */

/**
 * sprite_init - Initialize sprite system
 *
 * Must be called before any other sprite functions.
 * Auto-detects current display resolution and initializes accordingly.
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int sprite_init(void);

/**
 * sprite_shutdown - Disable sprite system and release resources
 */
void sprite_shutdown(void);

/**
 * sprite_is_initialized - Check if sprite system is active
 *
 * Returns:
 *   1 if initialized, 0 otherwise
 */
int sprite_is_initialized(void);

/**
 * sprite_set_resolution - Set display resolution
 *
 * Switches between 320-pixel (standard) and 640-pixel (H640) modes.
 * Requires Phase 30 feature detection for H640 support.
 *
 * Parameters:
 *   width — Display width (320 for standard, 640 for H640)
 *
 * Returns:
 *   1 if resolution set, 0 if unavailable
 */
int sprite_set_resolution(int width);

/**
 * sprite_get_resolution - Get current display resolution
 *
 * Returns:
 *   Display width in pixels (320 or 640)
 */
int sprite_get_resolution(void);

/**
 * sprite_get_height - Get current display height
 *
 * Returns:
 *   Display height in pixels (200 for NTSC, 224+ for PAL)
 */
int sprite_get_height(void);

/* ============================================================================
 * SPRITE CREATION & DESTRUCTION
 * ========================================================================== */

/**
 * sprite_create - Create a new sprite
 *
 * Allocates sprite with specified dimensions. The sprite type (Sprite320
 * or Sprite640) is automatically selected based on current resolution.
 * Sprite bitmap is allocated and cleared to transparent (0).
 *
 * Parameters:
 *   x, y — Initial position (pixels, resolution-aware)
 *   width, height — Sprite dimensions (pixels)
 *
 * Returns:
 *   Sprite handle on success, INVALID_SPRITE on error
 *
 * Note: Sprite coordinates are clamped to current resolution bounds.
 *       Creating 640-pixel sprite in 320-pixel mode will fail.
 */
sprite_t sprite_create(int x, int y, int width, int height);

/**
 * sprite_destroy - Delete a sprite
 *
 * Frees sprite bitmap and removes from display.
 *
 * Parameters:
 *   sprite — Sprite handle to destroy
 */
void sprite_destroy(sprite_t sprite);

/**
 * sprite_get_info - Get sprite information
 *
 * Reads current sprite properties including resolution type.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   info — Pointer to sprite_info_t to receive data
 *
 * Returns:
 *   1 if successful, 0 if sprite invalid
 */
int sprite_get_info(sprite_t sprite, sprite_info_t *info);

/* ============================================================================
 * SPRITE POSITIONING
 * ========================================================================== */

/**
 * sprite_set_position - Move sprite to new position
 *
 * Position is clamped to current resolution bounds.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   x, y — New position (pixels, resolution-aware)
 */
void sprite_set_position(sprite_t sprite, int x, int y);

/**
 * sprite_move - Move sprite by relative offset
 *
 * Parameters:
 *   sprite — Sprite handle
 *   dx, dy — Relative movement (pixels)
 */
void sprite_move(sprite_t sprite, int dx, int dy);

/**
 * sprite_get_position - Get sprite position
 *
 * Parameters:
 *   sprite — Sprite handle
 *   x, y — Pointers to receive position
 *
 * Returns:
 *   1 if successful, 0 if sprite invalid
 */
int sprite_get_position(sprite_t sprite, int *x, int *y);

/**
 * sprite_get_bounds - Get sprite bounding box
 *
 * Parameters:
 *   sprite — Sprite handle
 *   x1, y1 — Pointers for top-left corner
 *   x2, y2 — Pointers for bottom-right corner
 *
 * Returns:
 *   1 if successful, 0 if sprite invalid
 */
int sprite_get_bounds(sprite_t sprite, int *x1, int *y1, int *x2, int *y2);

/* ============================================================================
 * SPRITE VISIBILITY & ATTRIBUTES
 * ========================================================================== */

/**
 * sprite_set_visible - Show or hide sprite
 *
 * Parameters:
 *   sprite — Sprite handle
 *   visible — 1 to show, 0 to hide
 */
void sprite_set_visible(sprite_t sprite, int visible);

/**
 * sprite_is_visible - Check if sprite is visible
 *
 * Returns:
 *   1 if visible, 0 if hidden
 */
int sprite_is_visible(sprite_t sprite);

/**
 * sprite_set_color - Set sprite color
 *
 * Parameters:
 *   sprite — Sprite handle
 *   color — Color index (0-15)
 */
void sprite_set_color(sprite_t sprite, int color);

/**
 * sprite_get_color - Get sprite color
 *
 * Returns:
 *   Color index (0-15) or -1 if invalid
 */
int sprite_get_color(sprite_t sprite);

/**
 * sprite_set_layer - Set Z-order layer
 *
 * Higher layer values appear on top.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   layer — Layer index (0-255, lower = behind)
 */
void sprite_set_layer(sprite_t sprite, int layer);

/**
 * sprite_get_layer - Get sprite layer
 *
 * Returns:
 *   Layer index (0-255)
 */
int sprite_get_layer(sprite_t sprite);

/* ============================================================================
 * SPRITE BITMAP OPERATIONS
 * ========================================================================== */

/**
 * sprite_set_bitmap - Set sprite bitmap data
 *
 * Copies bitmap data into sprite's memory.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   bitmap — Source bitmap data
 *   stride — Bytes per row in source
 *
 * Returns:
 *   1 if successful, 0 if sprite invalid
 */
int sprite_set_bitmap(sprite_t sprite, const uint8_t *bitmap, int stride);

/**
 * sprite_get_bitmap - Get pointer to sprite bitmap
 *
 * Returns pointer to internal sprite bitmap for direct manipulation.
 *
 * Parameters:
 *   sprite — Sprite handle
 *
 * Returns:
 *   Pointer to bitmap data, NULL if sprite invalid
 */
uint8_t *sprite_get_bitmap(sprite_t sprite);

/**
 * sprite_clear - Clear sprite bitmap to transparent
 *
 * Parameters:
 *   sprite — Sprite handle
 */
void sprite_clear(sprite_t sprite);

/**
 * sprite_fill - Fill sprite with solid color
 *
 * Parameters:
 *   sprite — Sprite handle
 *   color — Color to fill with (0-15)
 */
void sprite_fill(sprite_t sprite, int color);

/**
 * sprite_draw_rect - Draw rectangle within sprite
 *
 * Parameters:
 *   sprite — Sprite handle
 *   x1, y1, x2, y2 — Rectangle bounds (relative to sprite origin)
 *   color — Color to draw with (0-15)
 */
void sprite_draw_rect(sprite_t sprite, int x1, int y1, int x2, int y2,
                      int color);

/**
 * sprite_draw_line - Draw line within sprite
 *
 * Parameters:
 *   sprite — Sprite handle
 *   x1, y1, x2, y2 — Line endpoints (relative to sprite origin)
 *   color — Color to draw with (0-15)
 */
void sprite_draw_line(sprite_t sprite, int x1, int y1, int x2, int y2,
                      int color);

/**
 * sprite_draw_circle - Draw circle within sprite
 *
 * Parameters:
 *   sprite — Sprite handle
 *   cx, cy — Center (relative to sprite origin)
 *   radius — Circle radius (pixels)
 *   color — Color to draw with (0-15)
 */
void sprite_draw_circle(sprite_t sprite, int cx, int cy, int radius,
                        int color);

/* ============================================================================
 * COLLISION DETECTION
 * ========================================================================== */

/**
 * sprite_collides_point - Check if sprite contains point
 *
 * Tests if a screen coordinate is within sprite bounds.
 *
 * Parameters:
 *   sprite — Sprite handle
 *   x, y — Screen coordinates (pixels)
 *
 * Returns:
 *   1 if point inside sprite, 0 otherwise
 */
int sprite_collides_point(sprite_t sprite, int x, int y);

/**
 * sprite_collides_sprite - Check collision between two sprites
 *
 * Tests if two sprites' bounding boxes overlap.
 *
 * Parameters:
 *   sprite1, sprite2 — Sprite handles
 *
 * Returns:
 *   1 if sprites overlap, 0 otherwise
 */
int sprite_collides_sprite(sprite_t sprite1, sprite_t sprite2);

/**
 * sprite_collides_rect - Check if sprite overlaps rectangle
 *
 * Parameters:
 *   sprite — Sprite handle
 *   x1, y1, x2, y2 — Rectangle bounds (screen coordinates)
 *
 * Returns:
 *   1 if sprite overlaps rectangle, 0 otherwise
 */
int sprite_collides_rect(sprite_t sprite, int x1, int y1, int x2, int y2);

/* ============================================================================
 * LIGHTPEN INTEGRATION (Phase 33)
 * ========================================================================== */

/**
 * sprite_at_lightpen - Get sprite at light pen position
 *
 * Finds the topmost sprite at the current light pen coordinates.
 * Requires Phase 33 (Light Pen) to be initialized.
 *
 * Returns:
 *   Sprite handle if found, INVALID_SPRITE if none at position
 */
sprite_t sprite_at_lightpen(void);

/**
 * sprite_hit_test - Find sprite containing point
 *
 * Finds topmost sprite containing the specified screen coordinates.
 * Considers Z-order (layer).
 *
 * Parameters:
 *   x, y — Screen coordinates (pixels)
 *
 * Returns:
 *   Sprite handle if found, INVALID_SPRITE otherwise
 */
sprite_t sprite_hit_test(int x, int y);

/* ============================================================================
 * SPRITE ENUMERATION
 * ========================================================================== */

/**
 * sprite_count - Get number of active sprites
 *
 * Returns:
 *   Number of allocated sprites
 */
int sprite_count(void);

/**
 * sprite_get_by_index - Get sprite by enumeration index
 *
 * Parameters:
 *   index — Sprite index (0 to sprite_count()-1)
 *
 * Returns:
 *   Sprite handle, or INVALID_SPRITE if index out of range
 */
sprite_t sprite_get_by_index(int index);

/* ============================================================================
 * RENDERING & DISPLAY
 * ========================================================================== */

/**
 * sprite_render_all - Render all sprites to screen
 *
 * Draws all visible sprites in Z-order to VIC-IV hardware.
 * Should be called once per frame.
 *
 * Returns:
 *   1 if successful, 0 on error
 */
int sprite_render_all(void);

/**
 * sprite_clear_all - Clear all sprites from screen
 *
 * Removes all rendered sprites but keeps sprite objects.
 */
void sprite_clear_all(void);

/**
 * sprite_set_background - Set background color
 *
 * Parameters:
 *   color — Color index (0-15)
 */
void sprite_set_background(int color);

/**
 * sprite_get_background - Get current background color
 *
 * Returns:
 *   Background color index
 */
int sprite_get_background(void);

/* ============================================================================
 * PERFORMANCE & OPTIMIZATION
 * ========================================================================== */

/**
 * sprite_mark_dirty - Mark sprite as modified
 *
 * Hints to renderer that sprite has changed and needs redraw.
 *
 * Parameters:
 *   sprite — Sprite handle
 */
void sprite_mark_dirty(sprite_t sprite);

/**
 * sprite_enable_caching - Enable sprite caching for performance
 *
 * Parameters:
 *   enabled — 1 to enable, 0 to disable
 */
void sprite_enable_caching(int enabled);

/**
 * sprite_cache_all - Cache all sprites for maximum performance
 */
void sprite_cache_all(void);

/* ============================================================================
 * DEBUGGING & INSPECTION
 * ========================================================================== */

/**
 * sprite_print_status - Print sprite system status (debug)
 */
void sprite_print_status(void);

/**
 * sprite_print_info - Print sprite information (debug)
 *
 * Parameters:
 *   sprite — Sprite handle
 */
void sprite_print_info(sprite_t sprite);

/**
 * sprite_dump_bitmap - Print sprite bitmap to console (debug)
 *
 * Parameters:
 *   sprite — Sprite handle
 *   max_rows — Maximum rows to print
 */
void sprite_dump_bitmap(sprite_t sprite, int max_rows);

#endif
