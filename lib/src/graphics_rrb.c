/* graphics_rrb.c — Raster Re-Write Buffer (Phase 105.1)
 *
 * Core VIC-IV integration and layer management for RRB system.
 * Handles per-raster-line character repositioning via GOTOX instructions.
 */

#include <graphics_rrb.h>
#include <stdlib.h>
#include <string.h>
#include <mega65.h>

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

/* Convert character column to byte offset (accounting for SEAM) */
static int char_col_to_offset(int col, int is_seam) {
    if (is_seam) {
        return col * 2;  /* SEAM uses 2 bytes per character */
    }
    return col;
}

/* Calculate memory address for character at (col, row) in layer */
static int calc_char_offset(int col, int row, int width, int is_seam) {
    int row_offset = row * (char_col_to_offset(width, is_seam));
    int col_offset = char_col_to_offset(col, is_seam);
    return row_offset + col_offset;
}

/* ============================================================================
 * INITIALIZATION & LIFECYCLE
 * ============================================================================ */

int rrb_init(rrb_system_t *rrb, int max_layers, int width, int height) {
    if (!rrb || max_layers <= 0 || max_layers > RRB_MAX_LAYERS) {
        return -1;
    }

    if (width != 40 && width != 80) {
        return -1;
    }

    if (height != 25 && height != 24 && height != 50) {
        return -1;
    }

    /* Allocate layer array */
    rrb->layers = (rrb_layer_t *)malloc(max_layers * sizeof(rrb_layer_t));
    if (!rrb->layers) {
        return -1;
    }

    /* Initialize layer array */
    memset(rrb->layers, 0, max_layers * sizeof(rrb_layer_t));
    rrb->max_layers = max_layers;
    rrb->layer_count = 0;

    /* Set screen dimensions */
    rrb->screen_width = (width == 40) ? 320 : 640;
    rrb->screen_height = height;

    /* Default VIC-IV configuration */
    rrb->h640 = (width == 80) ? 1 : 0;
    rrb->chrcount = width;
    rrb->linestep = width * 2;  /* 2 bytes per character for SEAM */

    /* Allocate working buffers */
    int buffer_size = width * 2 * height;  /* Max size for SEAM mode */
    rrb->screen_buffer = (unsigned char *)malloc(buffer_size);
    rrb->color_buffer = (unsigned char *)malloc(buffer_size);

    if (!rrb->screen_buffer || !rrb->color_buffer) {
        free(rrb->layers);
        free(rrb->screen_buffer);
        free(rrb->color_buffer);
        return -1;
    }

    rrb->buffer_size = buffer_size;
    rrb->enabled = 0;
    rrb->double_raster_time = 0;

    return 0;
}

void rrb_done(rrb_system_t *rrb) {
    if (!rrb) return;

    rrb_disable(rrb);

    /* Free layers */
    if (rrb->layers) {
        for (int i = 0; i < rrb->layer_count; i++) {
            rrb_layer_t *layer = &rrb->layers[i];
            if (layer->screen_addr) {
                free((void *)layer->screen_addr);
            }
            if (layer->color_addr) {
                free((void *)layer->color_addr);
            }
        }
        free(rrb->layers);
        rrb->layers = NULL;
    }

    /* Free buffers */
    if (rrb->screen_buffer) {
        free(rrb->screen_buffer);
        rrb->screen_buffer = NULL;
    }
    if (rrb->color_buffer) {
        free(rrb->color_buffer);
        rrb->color_buffer = NULL;
    }

    rrb->layer_count = 0;
    rrb->max_layers = 0;
}

/* ============================================================================
 * ENABLE / DISABLE
 * ============================================================================ */

void rrb_enable(rrb_system_t *rrb) {
    if (!rrb) return;
    rrb->enabled = 1;
}

void rrb_disable(rrb_system_t *rrb) {
    if (!rrb) return;
    rrb->enabled = 0;
}

int rrb_is_enabled(rrb_system_t *rrb) {
    if (!rrb) return 0;
    return rrb->enabled;
}

/* ============================================================================
 * LAYER MANAGEMENT
 * ============================================================================ */

int rrb_layer_create(rrb_system_t *rrb, rrb_layer_mode_t mode, int width, int height) {
    if (!rrb || rrb->layer_count >= rrb->max_layers) {
        return -1;
    }

    if (width <= 0 || height <= 0) {
        return -1;
    }

    rrb_layer_t *layer = &rrb->layers[rrb->layer_count];

    /* Calculate memory size (SEAM = 2 bytes per character) */
    int bytes_per_char = 2;
    int total_size = width * height * bytes_per_char;

    /* Allocate screen memory */
    layer->screen_addr = (unsigned int)malloc(total_size);
    if (!layer->screen_addr) {
        return -1;
    }

    /* Allocate color memory */
    layer->color_addr = (unsigned int)malloc(total_size);
    if (!layer->color_addr) {
        free((void *)layer->screen_addr);
        return -1;
    }

    /* Initialize layer */
    memset((void *)layer->screen_addr, 32, total_size);  /* Space char */
    memset((void *)layer->color_addr, 0, total_size);     /* Black */

    layer->width = width;
    layer->height = height;
    layer->mode = mode;
    layer->scroll_x = 0;
    layer->scroll_y = 0;
    layer->visible = 1;
    layer->priority = rrb->layer_count;  /* Default priority = creation order */
    layer->transparent_char = 32;         /* Space is transparent */

    int layer_idx = rrb->layer_count;
    rrb->layer_count++;

    return layer_idx;
}

void rrb_layer_destroy(rrb_system_t *rrb, int layer_idx) {
    if (!rrb || layer_idx < 0 || layer_idx >= rrb->layer_count) {
        return;
    }

    rrb_layer_t *layer = &rrb->layers[layer_idx];

    if (layer->screen_addr) {
        free((void *)layer->screen_addr);
        layer->screen_addr = 0;
    }
    if (layer->color_addr) {
        free((void *)layer->color_addr);
        layer->color_addr = 0;
    }

    /* Remove from array by shifting */
    for (int i = layer_idx; i < rrb->layer_count - 1; i++) {
        rrb->layers[i] = rrb->layers[i + 1];
    }
    rrb->layer_count--;
}

rrb_layer_t *rrb_get_layer(rrb_system_t *rrb, int layer_idx) {
    if (!rrb || layer_idx < 0 || layer_idx >= rrb->layer_count) {
        return NULL;
    }
    return &rrb->layers[layer_idx];
}

/* ============================================================================
 * LAYER CONFIGURATION
 * ============================================================================ */

void rrb_layer_set_scroll(rrb_layer_t *layer, int scroll_x, int scroll_y) {
    if (!layer) return;
    layer->scroll_x = scroll_x;
    layer->scroll_y = scroll_y;
}

void rrb_layer_set_position(rrb_layer_t *layer, int x, int y) {
    if (!layer) return;
    /* Position is handled during rendering for sparse/stack modes */
    layer->scroll_x = x;
    layer->scroll_y = y;
}

void rrb_layer_show(rrb_layer_t *layer) {
    if (!layer) return;
    layer->visible = 1;
}

void rrb_layer_hide(rrb_layer_t *layer) {
    if (!layer) return;
    layer->visible = 0;
}

void rrb_layer_set_priority(rrb_layer_t *layer, int priority) {
    if (!layer) return;
    layer->priority = priority;
}

/* ============================================================================
 * LAYER DATA ACCESS
 * ============================================================================ */

void rrb_layer_set_char(rrb_layer_t *layer, int col, int row,
                        unsigned char ch, unsigned char color) {
    if (!layer || col < 0 || col >= layer->width || row < 0 || row >= layer->height) {
        return;
    }

    int offset = calc_char_offset(col, row, layer->width, 1);  /* SEAM mode */

    unsigned char *screen = (unsigned char *)layer->screen_addr;
    unsigned char *color_mem = (unsigned char *)layer->color_addr;

    screen[offset] = ch;
    screen[offset + 1] = 0;        /* SEAM high byte */
    color_mem[offset] = color;
    color_mem[offset + 1] = 0;     /* SEAM high byte */
}

unsigned char rrb_layer_get_char(rrb_layer_t *layer, int col, int row) {
    if (!layer || col < 0 || col >= layer->width || row < 0 || row >= layer->height) {
        return 0;
    }

    int offset = calc_char_offset(col, row, layer->width, 1);
    unsigned char *screen = (unsigned char *)layer->screen_addr;
    return screen[offset];
}

unsigned char rrb_layer_get_color(rrb_layer_t *layer, int col, int row) {
    if (!layer || col < 0 || col >= layer->width || row < 0 || row >= layer->height) {
        return 0;
    }

    int offset = calc_char_offset(col, row, layer->width, 1);
    unsigned char *color_mem = (unsigned char *)layer->color_addr;
    return color_mem[offset];
}

void rrb_layer_clear(rrb_layer_t *layer, unsigned char fill_char, unsigned char fill_color) {
    if (!layer) return;

    int size = layer->width * layer->height * 2;
    unsigned char *screen = (unsigned char *)layer->screen_addr;
    unsigned char *color_mem = (unsigned char *)layer->color_addr;

    for (int i = 0; i < size; i += 2) {
        screen[i] = fill_char;
        screen[i + 1] = 0;
        color_mem[i] = fill_color;
        color_mem[i + 1] = 0;
    }
}

unsigned char *rrb_layer_screen_ptr(rrb_layer_t *layer) {
    if (!layer) return NULL;
    return (unsigned char *)layer->screen_addr;
}

unsigned char *rrb_layer_color_ptr(rrb_layer_t *layer) {
    if (!layer) return NULL;
    return (unsigned char *)layer->color_addr;
}

/* ============================================================================
 * RENDERING
 * ============================================================================ */

int rrb_render(rrb_system_t *rrb) {
    if (!rrb || !rrb->enabled) {
        return 0;
    }

    /* For now, just mark success - full GOTOX injection in Phase 105.2 */
    return 0;
}

int rrb_render_row(rrb_system_t *rrb, int row) {
    if (!rrb || !rrb->enabled || row < 0 || row >= rrb->screen_height) {
        return 0;
    }

    /* For now, just mark success - full GOTOX injection in Phase 105.2 */
    return 0;
}

void rrb_update(rrb_system_t *rrb) {
    if (!rrb) return;

    rrb_render(rrb);
    /* Scroll updates would happen here */
}

void rrb_sync_display(rrb_system_t *rrb) {
    if (!rrb || !rrb->enabled) return;

    /* Copy composite buffers to VIC-IV screen memory */
    /* Implementation in Phase 105.2 */
}

/* ============================================================================
 * VIC-IV CONFIGURATION
 * ============================================================================ */

int rrb_configure_vic(rrb_system_t *rrb, int h640, int chrcount, int linestep) {
    if (!rrb) return -1;

    if (chrcount < 1 || chrcount > RRB_MAX_CHRCOUNT) {
        return -1;
    }

    if (linestep < chrcount) {
        return -1;
    }

    rrb->h640 = h640 ? 1 : 0;
    rrb->chrcount = chrcount;
    rrb->linestep = linestep;
    rrb->screen_width = h640 ? 640 : 320;

    /* VIC-IV register writes would happen here (Phase 105.2) */

    return 0;
}

int rrb_enable_double_time(rrb_system_t *rrb) {
    if (!rrb) return -1;

    rrb->double_raster_time = 1;

    /* VIC-IV register writes: V400, CHRYSCL, DBLRR, TEXTYPOS */
    /* Implementation in Phase 105.4 */

    return 0;
}

void rrb_disable_double_time(rrb_system_t *rrb) {
    if (!rrb) return;

    rrb->double_raster_time = 0;

    /* VIC-IV register writes to reset */
    /* Implementation in Phase 105.4 */
}

/* ============================================================================
 * GOTOX UTILITIES
 * ============================================================================ */

void rrb_write_gotox(unsigned int screen_addr, unsigned int color_addr,
                     int pixel_x, int transparent) {
    if (!screen_addr || !color_addr) return;

    unsigned char *screen = (unsigned char *)screen_addr;
    unsigned char *color = (unsigned char *)color_addr;

    /* Encode 10-bit pixel position */
    int x_lo = pixel_x & 0xFF;
    int x_hi = (pixel_x >> 8) & 0x03;

    /* Write to screen memory (10 bits of position) */
    screen[0] = x_lo;
    screen[1] = x_hi;

    /* Write to color memory (GOTOX flag + transparency flag) */
    int color_byte = RRB_GOTOX_FLAG;
    if (transparent) {
        color_byte |= RRB_TRANSPARENCY_FLAG;
    }

    color[0] = color_byte;
    color[1] = 0;
}

int rrb_calc_chrcount(int num_chars, int num_gotox) {
    int needed = num_chars + num_gotox;
    if (needed > RRB_MAX_CHRCOUNT) {
        needed = RRB_MAX_CHRCOUNT;
    }
    return needed;
}

int rrb_calc_linestep(int chrcount) {
    return chrcount * 2;  /* SEAM = 2 bytes per character */
}

int rrb_test_raster_budget(rrb_system_t *rrb, int row) {
    if (!rrb || row < 0 || row >= rrb->screen_height) {
        return 0;
    }

    /* Estimate raster time:
     * - Normal mode: ~1000 cycles/raster available
     * - Each character: ~5 cycles
     * - Each GOTOX: ~10 cycles
     * - Rough check: if chrcount > 150 in normal mode, likely exceeds
     */

    int cycle_budget = rrb->double_raster_time ? 2000 : 1000;
    int estimated_cycles = rrb->chrcount * 5 + rrb->chrcount * 2;  /* rough estimate */

    return (estimated_cycles <= cycle_budget) ? 1 : 0;
}
