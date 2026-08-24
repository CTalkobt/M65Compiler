/* graphics_rrb.c — Raster Re-Write Buffer (Phase 105)
 *
 * Struct method implementations for RRB system and layers.
 * Inline trivial methods (auto-inlined by compiler, zero overhead).
 */

#include <graphics_rrb.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

static int char_col_to_offset(int col, int is_seam) {
    if (is_seam) {
        return col * 2;
    }
    return col;
}

static int calc_char_offset(int col, int row, int width, int is_seam) {
    int row_offset = row * (char_col_to_offset(width, is_seam));
    int col_offset = char_col_to_offset(col, is_seam);
    return row_offset + col_offset;
}

/* ============================================================================
 * RRB_SYSTEM_T TRIVIAL METHODS (inlined by compiler, zero overhead)
 * ============================================================================ */

int rrb_system_t__is_enabled(rrb_system_t *this) {
    if (!this) return 0;
    return this->enabled;
}

int rrb_system_t__get_layer_count(rrb_system_t *this) {
    if (!this) return 0;
    return this->layer_count;
}

int rrb_system_t__get_screen_width(rrb_system_t *this) {
    if (!this) return 0;
    return this->screen_width;
}

int rrb_system_t__get_screen_height(rrb_system_t *this) {
    if (!this) return 0;
    return this->screen_height;
}

void rrb_system_t__enable(rrb_system_t *this) {
    if (!this) return;
    this->enabled = 1;
}

void rrb_system_t__disable(rrb_system_t *this) {
    if (!this) return;
    this->enabled = 0;
}

int rrb_system_t__is_double_time(rrb_system_t *this) {
    if (!this) return 0;
    return this->double_raster_time;
}

/* ============================================================================
 * RRB_SYSTEM_T LIFECYCLE & MANAGEMENT METHODS
 * ============================================================================ */

int rrb_system_t__init(rrb_system_t *this, int max_layers, int width, int height) {
    if (!this || max_layers <= 0 || max_layers > RRB_MAX_LAYERS) {
        return -1;
    }

    if (width != 40 && width != 80) {
        return -1;
    }

    if (height != 25 && height != 24 && height != 50) {
        return -1;
    }

    this->layers = (rrb_layer_t *)malloc(max_layers * sizeof(rrb_layer_t));
    if (!this->layers) {
        return -1;
    }

    memset(this->layers, 0, max_layers * sizeof(rrb_layer_t));
    this->max_layers = max_layers;
    this->layer_count = 0;

    this->screen_width = (width == 40) ? 320 : 640;
    this->screen_height = height;

    this->h640 = (width == 80) ? 1 : 0;
    this->chrcount = width;
    this->linestep = width * 2;

    int buffer_size = width * 2 * height;
    this->screen_buffer = (unsigned char *)malloc(buffer_size);
    this->color_buffer = (unsigned char *)malloc(buffer_size);

    if (!this->screen_buffer || !this->color_buffer) {
        free(this->layers);
        free(this->screen_buffer);
        free(this->color_buffer);
        return -1;
    }

    this->buffer_size = buffer_size;
    this->enabled = 0;
    this->double_raster_time = 0;

    /* Initialize method pointers (auto-inlined by compiler, zero overhead) */
    this->is_enabled = &rrb_system_t__is_enabled;
    this->get_layer_count = &rrb_system_t__get_layer_count;
    this->get_screen_width = &rrb_system_t__get_screen_width;
    this->get_screen_height = &rrb_system_t__get_screen_height;
    this->enable = &rrb_system_t__enable;
    this->disable = &rrb_system_t__disable;
    this->is_double_time = &rrb_system_t__is_double_time;

    this->init = &rrb_system_t__init;
    this->done = &rrb_system_t__done;

    this->create_layer = &rrb_system_t__create_layer;
    this->destroy_layer = &rrb_system_t__destroy_layer;
    this->get_layer = &rrb_system_t__get_layer;

    this->configure_vic = &rrb_system_t__configure_vic;
    this->enable_double_time = &rrb_system_t__enable_double_time;
    this->disable_double_time = &rrb_system_t__disable_double_time;

    this->render = &rrb_system_t__render;
    this->render_row = &rrb_system_t__render_row;
    this->update = &rrb_system_t__update;
    this->sync_display = &rrb_system_t__sync_display;

    return 0;
}

void rrb_system_t__done(rrb_system_t *this) {
    if (!this) return;

    rrb_system_t__disable(this);

    if (this->layers) {
        for (int i = 0; i < this->layer_count; i++) {
            rrb_layer_t *layer = &this->layers[i];
            if (layer->screen_addr) {
                free((void *)layer->screen_addr);
            }
            if (layer->color_addr) {
                free((void *)layer->color_addr);
            }
        }
        free(this->layers);
        this->layers = NULL;
    }

    if (this->screen_buffer) {
        free(this->screen_buffer);
        this->screen_buffer = NULL;
    }
    if (this->color_buffer) {
        free(this->color_buffer);
        this->color_buffer = NULL;
    }

    this->layer_count = 0;
    this->max_layers = 0;
}

int rrb_system_t__create_layer(rrb_system_t *this, rrb_layer_mode_t mode, int width, int height) {
    if (!this || this->layer_count >= this->max_layers) {
        return -1;
    }

    if (width <= 0 || height <= 0) {
        return -1;
    }

    rrb_layer_t *layer = &this->layers[this->layer_count];

    int bytes_per_char = 2;
    int total_size = width * height * bytes_per_char;

    layer->screen_addr = (unsigned int)malloc(total_size);
    if (!layer->screen_addr) {
        return -1;
    }

    layer->color_addr = (unsigned int)malloc(total_size);
    if (!layer->color_addr) {
        free((void *)layer->screen_addr);
        return -1;
    }

    memset((void *)layer->screen_addr, 32, total_size);
    memset((void *)layer->color_addr, 0, total_size);

    layer->width = width;
    layer->height = height;
    layer->mode = mode;
    layer->scroll_x = 0;
    layer->scroll_y = 0;
    layer->visible = 1;
    layer->priority = this->layer_count;
    layer->transparent_char = 32;

    /* Initialize method pointers (auto-inlined by compiler, zero overhead) */
    layer->show = &rrb_layer_t__show;
    layer->hide = &rrb_layer_t__hide;
    layer->set_scroll = &rrb_layer_t__set_scroll;
    layer->set_priority = &rrb_layer_t__set_priority;
    layer->set_position = &rrb_layer_t__set_position;
    layer->is_visible = &rrb_layer_t__is_visible;
    layer->get_priority = &rrb_layer_t__get_priority;
    layer->get_scroll_x = &rrb_layer_t__get_scroll_x;
    layer->get_scroll_y = &rrb_layer_t__get_scroll_y;
    layer->set_char = &rrb_layer_t__set_char;
    layer->get_char = &rrb_layer_t__get_char;
    layer->get_color = &rrb_layer_t__get_color;
    layer->clear = &rrb_layer_t__clear;
    layer->screen_ptr = &rrb_layer_t__screen_ptr;
    layer->color_ptr = &rrb_layer_t__color_ptr;

    int layer_idx = this->layer_count;
    this->layer_count++;

    return layer_idx;
}

void rrb_system_t__destroy_layer(rrb_system_t *this, int layer_idx) {
    if (!this || layer_idx < 0 || layer_idx >= this->layer_count) {
        return;
    }

    rrb_layer_t *layer = &this->layers[layer_idx];

    if (layer->screen_addr) {
        free((void *)layer->screen_addr);
        layer->screen_addr = 0;
    }
    if (layer->color_addr) {
        free((void *)layer->color_addr);
        layer->color_addr = 0;
    }

    for (int i = layer_idx; i < this->layer_count - 1; i++) {
        this->layers[i] = this->layers[i + 1];
    }
    this->layer_count--;
}

rrb_layer_t *rrb_system_t__get_layer(rrb_system_t *this, int layer_idx) {
    if (!this || layer_idx < 0 || layer_idx >= this->layer_count) {
        return NULL;
    }
    return &this->layers[layer_idx];
}

int rrb_system_t__configure_vic(rrb_system_t *this, int h640, int chrcount, int linestep) {
    if (!this) return -1;

    if (chrcount < 1 || chrcount > RRB_MAX_CHRCOUNT) {
        return -1;
    }

    if (linestep < chrcount) {
        return -1;
    }

    this->h640 = h640 ? 1 : 0;
    this->chrcount = chrcount;
    this->linestep = linestep;
    this->screen_width = h640 ? 640 : 320;

    return 0;
}

int rrb_system_t__enable_double_time(rrb_system_t *this) {
    if (!this) return -1;

    this->double_raster_time = 1;
    return 0;
}

void rrb_system_t__disable_double_time(rrb_system_t *this) {
    if (!this) return;
    this->double_raster_time = 0;
}

/* ============================================================================
 * RRB_LAYER_T TRIVIAL METHODS (inlined by compiler, zero overhead)
 * ============================================================================ */

void rrb_layer_t__show(rrb_layer_t *this) {
    if (!this) return;
    this->visible = 1;
}

void rrb_layer_t__hide(rrb_layer_t *this) {
    if (!this) return;
    this->visible = 0;
}

void rrb_layer_t__set_scroll(rrb_layer_t *this, int sx, int sy) {
    if (!this) return;
    this->scroll_x = sx;
    this->scroll_y = sy;
}

void rrb_layer_t__set_priority(rrb_layer_t *this, int p) {
    if (!this) return;
    this->priority = p;
}

void rrb_layer_t__set_position(rrb_layer_t *this, int x, int y) {
    if (!this) return;
    this->scroll_x = x;
    this->scroll_y = y;
}

int rrb_layer_t__is_visible(rrb_layer_t *this) {
    if (!this) return 0;
    return this->visible;
}

int rrb_layer_t__get_priority(rrb_layer_t *this) {
    if (!this) return 0;
    return this->priority;
}

int rrb_layer_t__get_scroll_x(rrb_layer_t *this) {
    if (!this) return 0;
    return this->scroll_x;
}

int rrb_layer_t__get_scroll_y(rrb_layer_t *this) {
    if (!this) return 0;
    return this->scroll_y;
}

/* ============================================================================
 * RRB_LAYER_T DATA ACCESS METHODS
 * ============================================================================ */

void rrb_layer_t__set_char(rrb_layer_t *this, int col, int row,
                           unsigned char ch, unsigned char color) {
    if (!this || col < 0 || col >= this->width || row < 0 || row >= this->height) {
        return;
    }

    int offset = calc_char_offset(col, row, this->width, 1);

    unsigned char *screen = (unsigned char *)this->screen_addr;
    unsigned char *color_mem = (unsigned char *)this->color_addr;

    screen[offset] = ch;
    screen[offset + 1] = 0;
    color_mem[offset] = color;
    color_mem[offset + 1] = 0;
}

unsigned char rrb_layer_t__get_char(rrb_layer_t *this, int col, int row) {
    if (!this || col < 0 || col >= this->width || row < 0 || row >= this->height) {
        return 0;
    }

    int offset = calc_char_offset(col, row, this->width, 1);
    unsigned char *screen = (unsigned char *)this->screen_addr;
    return screen[offset];
}

unsigned char rrb_layer_t__get_color(rrb_layer_t *this, int col, int row) {
    if (!this || col < 0 || col >= this->width || row < 0 || row >= this->height) {
        return 0;
    }

    int offset = calc_char_offset(col, row, this->width, 1);
    unsigned char *color_mem = (unsigned char *)this->color_addr;
    return color_mem[offset];
}

void rrb_layer_t__clear(rrb_layer_t *this, unsigned char fill_char, unsigned char fill_color) {
    if (!this) return;

    int size = this->width * this->height * 2;
    unsigned char *screen = (unsigned char *)this->screen_addr;
    unsigned char *color_mem = (unsigned char *)this->color_addr;

    for (int i = 0; i < size; i += 2) {
        screen[i] = fill_char;
        screen[i + 1] = 0;
        color_mem[i] = fill_color;
        color_mem[i + 1] = 0;
    }
}

unsigned char *rrb_layer_t__screen_ptr(rrb_layer_t *this) {
    if (!this) return NULL;
    return (unsigned char *)this->screen_addr;
}

unsigned char *rrb_layer_t__color_ptr(rrb_layer_t *this) {
    if (!this) return NULL;
    return (unsigned char *)this->color_addr;
}

/* ============================================================================
 * RENDERING & LAYER COMPOSITION (Phase 105.3: Layer Strategies)
 * ============================================================================ */

/* FULL Mode: Render entire layer sequentially */
static int rrb_compose_row_full(rrb_layer_t *layer, int row, int *write_pos,
                               unsigned char *out_screen, unsigned char *out_color,
                               int chrcount) {
    if (!layer || row >= layer->height) return 0;

    unsigned char *layer_screen = (unsigned char *)layer->screen_addr;
    unsigned char *layer_color = (unsigned char *)layer->color_addr;
    int row_offset = row * (layer->width * 2);

    for (int col = 0; col < layer->width && *write_pos < chrcount * 2 - 2; col++) {
        int src_offset = row_offset + col * 2;
        out_screen[*write_pos] = layer_screen[src_offset];
        out_screen[*write_pos + 1] = 0;
        out_color[*write_pos] = layer_color[src_offset];
        out_color[*write_pos + 1] = 0;
        *write_pos += 2;
    }
    return 0;
}

/* SPARSE Mode: Render only non-transparent characters with gap optimization */
static int rrb_compose_row_sparse(rrb_layer_t *layer, int row, int *write_pos,
                                 unsigned char *out_screen, unsigned char *out_color,
                                 int chrcount, int layer_x) {
    if (!layer || row >= layer->height) return 0;

    unsigned char *layer_screen = (unsigned char *)layer->screen_addr;
    unsigned char *layer_color = (unsigned char *)layer->color_addr;
    int row_offset = row * (layer->width * 2);
    int current_x = layer_x;

    for (int col = 0; col < layer->width && *write_pos < chrcount * 2 - 2; col++) {
        int src_offset = row_offset + col * 2;
        unsigned char ch = layer_screen[src_offset];

        if (ch == layer->transparent_char) {
            continue;  /* Skip transparent characters */
        }

        /* Emit character at calculated position */
        out_screen[*write_pos] = ch;
        out_screen[*write_pos + 1] = 0;
        out_color[*write_pos] = layer_color[src_offset];
        out_color[*write_pos + 1] = 0;
        *write_pos += 2;
        current_x += 8;
    }
    return 0;
}

/* STACK Mode: Render small positioned layer with precise GOTOX placement */
static int rrb_compose_row_stack(rrb_layer_t *layer, int row, int *write_pos,
                                unsigned char *out_screen, unsigned char *out_color,
                                int chrcount, int layer_x) {
    if (!layer || row >= layer->height) return 0;

    unsigned char *layer_screen = (unsigned char *)layer->screen_addr;
    unsigned char *layer_color = (unsigned char *)layer->color_addr;
    int row_offset = row * (layer->width * 2);

    /* STACK layers use scroll_x/scroll_y as absolute position on screen */
    int stack_x = layer_x;
    int stack_y = layer->scroll_y;

    /* Only render this row if it falls within the stack layer's vertical range */
    if (row < stack_y || row >= stack_y + layer->height) {
        return 0;
    }

    /* Emit GOTOX to position stack layer */
    rrb_write_gotox((unsigned int)&out_screen[*write_pos],
                    (unsigned int)&out_color[*write_pos],
                    stack_x, 0);
    *write_pos += 2;

    /* Calculate which row within the stack layer to render */
    int layer_row = row - stack_y;
    int layer_row_offset = layer_row * (layer->width * 2);

    for (int col = 0; col < layer->width && *write_pos < chrcount * 2 - 2; col++) {
        int src_offset = layer_row_offset + col * 2;
        unsigned char ch = layer_screen[src_offset];

        if (ch == layer->transparent_char) {
            continue;
        }

        out_screen[*write_pos] = ch;
        out_screen[*write_pos + 1] = 0;
        out_color[*write_pos] = layer_color[src_offset];
        out_color[*write_pos + 1] = 0;
        *write_pos += 2;
    }
    return 0;
}

static int rrb_compose_row(rrb_system_t *rrb, int row,
                           unsigned char *out_screen, unsigned char *out_color) {
    if (!rrb || !out_screen || !out_color) {
        return -1;
    }

    int write_pos = 0;
    int screen_width = rrb->screen_width;

    rrb_layer_t *visible[RRB_MAX_LAYERS];
    int visible_count = 0;

    for (int i = 0; i < rrb->layer_count; i++) {
        if (rrb->layers[i].visible) {
            visible[visible_count++] = &rrb->layers[i];
        }
    }

    for (int i = 0; i < visible_count - 1; i++) {
        for (int j = i + 1; j < visible_count; j++) {
            if (visible[i]->priority > visible[j]->priority) {
                rrb_layer_t *tmp = visible[i];
                visible[i] = visible[j];
                visible[j] = tmp;
            }
        }
    }

    for (int i = 0; i < rrb->chrcount * 2; i++) {
        out_screen[i] = 0;
        out_color[i] = 0;
    }

    for (int layer_idx = 0; layer_idx < visible_count; layer_idx++) {
        rrb_layer_t *layer = visible[layer_idx];

        if (write_pos >= rrb->chrcount * 2 - 4) {
            break;  /* Insufficient space in buffer */
        }

        int result = 0;

        switch (layer->mode) {
            case RRB_MODE_FULL:
                /* Full mode: position layer at scroll_x, render entire row */
                rrb_write_gotox((unsigned int)&out_screen[write_pos],
                               (unsigned int)&out_color[write_pos],
                               layer->scroll_x, 0);
                write_pos += 2;
                result = rrb_compose_row_full(layer, row, &write_pos,
                                             out_screen, out_color, rrb->chrcount);
                break;

            case RRB_MODE_SPARSE:
                /* Sparse mode: position layer, render only non-transparent chars */
                rrb_write_gotox((unsigned int)&out_screen[write_pos],
                               (unsigned int)&out_color[write_pos],
                               layer->scroll_x, 0);
                write_pos += 2;
                result = rrb_compose_row_sparse(layer, row, &write_pos,
                                               out_screen, out_color, rrb->chrcount,
                                               layer->scroll_x);
                break;

            case RRB_MODE_STACK:
                /* Stack mode: use scroll_x/scroll_y as absolute position, render if in range */
                result = rrb_compose_row_stack(layer, row, &write_pos,
                                              out_screen, out_color, rrb->chrcount,
                                              layer->scroll_x);
                break;

            default:
                /* Unknown mode, skip layer */
                break;
        }

        if (result != 0) {
            return -1;  /* Rendering error */
        }
    }

    rrb_write_gotox((unsigned int)&out_screen[write_pos],
                    (unsigned int)&out_color[write_pos],
                    screen_width, 0);
    write_pos += 2;

    if (write_pos > rrb->chrcount * 2) {
        return -1;
    }

    return 0;
}

int rrb_system_t__render(rrb_system_t *this) {
    if (!this || !this->enabled) {
        return 0;
    }

    for (int row = 0; row < this->screen_height; row++) {
        int row_offset = row * this->linestep;

        unsigned char *row_screen = &this->screen_buffer[row_offset];
        unsigned char *row_color = &this->color_buffer[row_offset];

        int result = rrb_compose_row(this, row, row_screen, row_color);
        if (result != 0) {
            return -1;
        }
    }

    return 0;
}

int rrb_system_t__render_row(rrb_system_t *this, int row) {
    if (!this || !this->enabled || row < 0 || row >= this->screen_height) {
        return 0;
    }

    int row_offset = row * this->linestep;

    unsigned char *row_screen = &this->screen_buffer[row_offset];
    unsigned char *row_color = &this->color_buffer[row_offset];

    return rrb_compose_row(this, row, row_screen, row_color);
}

void rrb_system_t__update(rrb_system_t *this) {
    if (!this) return;

    int result = rrb_system_t__render(this);

    if (result == 0) {
        rrb_system_t__sync_display(this);
    }
}

void rrb_system_t__sync_display(rrb_system_t *this) {
    if (!this || !this->enabled) return;

    /* TODO: Implement actual VIC-IV memory copy when hardware access available */
}

/* ============================================================================
 * GOTOX UTILITIES
 * ============================================================================ */

void rrb_write_gotox(unsigned int screen_addr, unsigned int color_addr,
                     int pixel_x, int transparent) {
    if (!screen_addr || !color_addr) return;

    unsigned char *screen = (unsigned char *)screen_addr;
    unsigned char *color = (unsigned char *)color_addr;

    int x_lo = pixel_x & 0xFF;
    int x_hi = (pixel_x >> 8) & 0x03;

    screen[0] = x_lo;
    screen[1] = x_hi;

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
    return chrcount * 2;
}

int rrb_test_raster_budget(rrb_system_t *rrb, int row) {
    if (!rrb || row < 0 || row >= rrb->screen_height) {
        return 0;
    }

    int cycle_budget = rrb->double_raster_time ? 2000 : 1000;
    int estimated_cycles = rrb->chrcount * 5 + rrb->chrcount * 2;

    return (estimated_cycles <= cycle_budget) ? 1 : 0;
}
