/* sprites.c — Unified Sprite Implementation for MEGA65
 *
 * Implements resolution-agnostic sprite management using polymorphism.
 * Sprite320 and Sprite640 implementations share a common virtual interface.
 */

#include <sprites.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * CONFIGURATION
 * ========================================================================== */

#define MAX_SPRITES             256
#define SPRITE320_WIDTH_MAX     320
#define SPRITE640_WIDTH_MAX     640
#define SPRITE_HEIGHT_NTSC      200
#define SPRITE_HEIGHT_PAL       224

/* ============================================================================
 * SPRITE VIRTUAL INTERFACE
 * ========================================================================== */

typedef struct sprite_vtable {
    /* Positioning */
    void (*set_position)(void *self, int x, int y);
    void (*move)(void *self, int dx, int dy);
    int (*get_x)(void *self);
    int (*get_y)(void *self);

    /* Attributes */
    void (*set_visible)(void *self, int visible);
    int (*is_visible)(void *self);
    void (*set_color)(void *self, int color);
    int (*get_color)(void *self);
    void (*set_layer)(void *self, int layer);
    int (*get_layer)(void *self);

    /* Bitmap */
    int (*set_bitmap)(void *self, const uint8_t *bitmap, int stride);
    uint8_t *(*get_bitmap)(void *self);
    void (*clear)(void *self);
    void (*fill)(void *self, int color);
    void (*draw_rect)(void *self, int x1, int y1, int x2, int y2, int color);
    void (*draw_line)(void *self, int x1, int y1, int x2, int y2, int color);
    void (*draw_circle)(void *self, int cx, int cy, int radius, int color);

    /* Collision */
    int (*collides_point)(void *self, int x, int y);
    int (*collides_sprite)(void *self, void *other);
    int (*collides_rect)(void *self, int x1, int y1, int x2, int y2);

    /* Utility */
    void (*destroy)(void *self);
} sprite_vtable_t;

/* ============================================================================
 * BASE SPRITE OBJECT
 * ========================================================================== */

typedef struct {
    sprite_vtable_t *vtable;  /* Virtual methods */
    void *impl;               /* Implementation-specific data */
    int type;                 /* 0=Sprite320, 1=Sprite640 */
} sprite_base_t;

/* ============================================================================
 * SPRITE320 IMPLEMENTATION
 * ========================================================================== */

typedef struct {
    sprite_base_t base;
    int x, y;
    int width, height;
    int visible;
    int layer;
    int color;
    uint8_t *bitmap;
    int bitmap_stride;
    int dirty;
} sprite320_t;

static void sprite320_set_position(void *self, int x, int y) {
    sprite320_t *s = (sprite320_t *)self;
    if (x < 0) x = 0;
    if (x > SPRITE320_WIDTH_MAX - 1) x = SPRITE320_WIDTH_MAX - 1;
    if (y < 0) y = 0;
    if (y > SPRITE_HEIGHT_PAL - 1) y = SPRITE_HEIGHT_PAL - 1;
    s->x = x;
    s->y = y;
    s->dirty = 1;
}

static void sprite320_move(void *self, int dx, int dy) {
    sprite320_t *s = (sprite320_t *)self;
    sprite320_set_position(s, s->x + dx, s->y + dy);
}

static int sprite320_get_x(void *self) {
    return ((sprite320_t *)self)->x;
}

static int sprite320_get_y(void *self) {
    return ((sprite320_t *)self)->y;
}

static void sprite320_set_visible(void *self, int visible) {
    sprite320_t *s = (sprite320_t *)self;
    s->visible = visible ? 1 : 0;
    s->dirty = 1;
}

static int sprite320_is_visible(void *self) {
    return ((sprite320_t *)self)->visible;
}

static void sprite320_set_color(void *self, int color) {
    sprite320_t *s = (sprite320_t *)self;
    s->color = color & 0x0F;
    s->dirty = 1;
}

static int sprite320_get_color(void *self) {
    return ((sprite320_t *)self)->color;
}

static void sprite320_set_layer(void *self, int layer) {
    ((sprite320_t *)self)->layer = layer & 0xFF;
}

static int sprite320_get_layer(void *self) {
    return ((sprite320_t *)self)->layer;
}

static int sprite320_set_bitmap(void *self, const uint8_t *bitmap, int stride) {
    sprite320_t *s = (sprite320_t *)self;
    if (!bitmap) return 0;
    for (int y = 0; y < s->height; ++y) {
        memcpy(s->bitmap + y * s->bitmap_stride,
               bitmap + y * stride, s->width);
    }
    s->dirty = 1;
    return 1;
}

static uint8_t *sprite320_get_bitmap(void *self) {
    return ((sprite320_t *)self)->bitmap;
}

static void sprite320_clear(void *self) {
    sprite320_t *s = (sprite320_t *)self;
    memset(s->bitmap, 0, s->bitmap_stride * s->height);
    s->dirty = 1;
}

static void sprite320_fill(void *self, int color) {
    sprite320_t *s = (sprite320_t *)self;
    memset(s->bitmap, color & 0x0F, s->bitmap_stride * s->height);
    s->dirty = 1;
}

static void sprite320_draw_rect(void *self, int x1, int y1, int x2, int y2,
                                int color) {
    sprite320_t *s = (sprite320_t *)self;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= s->width) x2 = s->width - 1;
    if (y2 >= s->height) y2 = s->height - 1;
    if (x1 > x2 || y1 > y2) return;
    color &= 0x0F;
    for (int y = y1; y <= y2; ++y) {
        for (int x = x1; x <= x2; ++x) {
            s->bitmap[y * s->bitmap_stride + x] = color;
        }
    }
    s->dirty = 1;
}

static void sprite320_draw_line(void *self, int x1, int y1, int x2, int y2,
                                int color) {
    sprite320_t *s = (sprite320_t *)self;
    color &= 0x0F;
    int dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int sx = (x2 > x1) ? 1 : -1;
    int sy = (y2 > y1) ? 1 : -1;
    int err = dx - dy;
    int x = x1, y = y1;
    while (1) {
        if (x >= 0 && x < s->width && y >= 0 && y < s->height) {
            s->bitmap[y * s->bitmap_stride + x] = color;
        }
        if (x == x2 && y == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
    s->dirty = 1;
}

static void sprite320_draw_circle(void *self, int cx, int cy, int radius,
                                  int color) {
    sprite320_t *s = (sprite320_t *)self;
    color &= 0x0F;
    int x = radius, y = 0, p = 1 - radius;
    while (x >= y) {
        int coords[][2] = {
            {cx + x, cy + y}, {cx - x, cy + y}, {cx + x, cy - y}, {cx - x, cy - y},
            {cx + y, cy + x}, {cx - y, cy + x}, {cx + y, cy - x}, {cx - y, cy - x}
        };
        for (int i = 0; i < 8; ++i) {
            int px = coords[i][0], py = coords[i][1];
            if (px >= 0 && px < s->width && py >= 0 && py < s->height) {
                s->bitmap[py * s->bitmap_stride + px] = color;
            }
        }
        if (p < 0) p += 2 * y + 1;
        else { p += 2 * (y - x) + 1; x--; }
        y++;
    }
    s->dirty = 1;
}

static int sprite320_collides_point(void *self, int x, int y) {
    sprite320_t *s = (sprite320_t *)self;
    return (x >= s->x && x <= s->x + s->width - 1 &&
            y >= s->y && y <= s->y + s->height - 1) ? 1 : 0;
}

static int sprite320_collides_sprite(void *self, void *other) {
    sprite320_t *s1 = (sprite320_t *)self;
    sprite320_t *s2 = (sprite320_t *)other;
    return !(s1->x + s1->width <= s2->x || s2->x + s2->width <= s1->x ||
             s1->y + s1->height <= s2->y || s2->y + s2->height <= s1->y) ? 1 : 0;
}

static int sprite320_collides_rect(void *self, int x1, int y1, int x2, int y2) {
    sprite320_t *s = (sprite320_t *)self;
    return !(s->x + s->width <= x1 || x2 + 1 <= s->x ||
             s->y + s->height <= y1 || y2 + 1 <= s->y) ? 1 : 0;
}

static void sprite320_destroy(void *self) {
    sprite320_t *s = (sprite320_t *)self;
    if (s->bitmap) free(s->bitmap);
    free(s);
}

static sprite_vtable_t sprite320_vtable = {
    .set_position = sprite320_set_position,
    .move = sprite320_move,
    .get_x = sprite320_get_x,
    .get_y = sprite320_get_y,
    .set_visible = sprite320_set_visible,
    .is_visible = sprite320_is_visible,
    .set_color = sprite320_set_color,
    .get_color = sprite320_get_color,
    .set_layer = sprite320_set_layer,
    .get_layer = sprite320_get_layer,
    .set_bitmap = sprite320_set_bitmap,
    .get_bitmap = sprite320_get_bitmap,
    .clear = sprite320_clear,
    .fill = sprite320_fill,
    .draw_rect = sprite320_draw_rect,
    .draw_line = sprite320_draw_line,
    .draw_circle = sprite320_draw_circle,
    .collides_point = sprite320_collides_point,
    .collides_sprite = sprite320_collides_sprite,
    .collides_rect = sprite320_collides_rect,
    .destroy = sprite320_destroy,
};

/* ============================================================================
 * SPRITE640 IMPLEMENTATION
 * ========================================================================== */

typedef struct {
    sprite_base_t base;
    int x, y;
    int width, height;
    int visible;
    int layer;
    int color;
    uint8_t *bitmap;
    int bitmap_stride;
    int dirty;
} sprite640_t;

static void sprite640_set_position(void *self, int x, int y) {
    sprite640_t *s = (sprite640_t *)self;
    if (x < 0) x = 0;
    if (x > SPRITE640_WIDTH_MAX - 1) x = SPRITE640_WIDTH_MAX - 1;
    if (y < 0) y = 0;
    if (y > SPRITE_HEIGHT_PAL - 1) y = SPRITE_HEIGHT_PAL - 1;
    s->x = x;
    s->y = y;
    s->dirty = 1;
}

static void sprite640_move(void *self, int dx, int dy) {
    sprite640_t *s = (sprite640_t *)self;
    sprite640_set_position(s, s->x + dx, s->y + dy);
}

static int sprite640_get_x(void *self) { return ((sprite640_t *)self)->x; }
static int sprite640_get_y(void *self) { return ((sprite640_t *)self)->y; }
static void sprite640_set_visible(void *self, int v) { ((sprite640_t *)self)->visible = v ? 1 : 0; ((sprite640_t *)self)->dirty = 1; }
static int sprite640_is_visible(void *self) { return ((sprite640_t *)self)->visible; }
static void sprite640_set_color(void *self, int c) { ((sprite640_t *)self)->color = c & 0x0F; ((sprite640_t *)self)->dirty = 1; }
static int sprite640_get_color(void *self) { return ((sprite640_t *)self)->color; }
static void sprite640_set_layer(void *self, int l) { ((sprite640_t *)self)->layer = l & 0xFF; }
static int sprite640_get_layer(void *self) { return ((sprite640_t *)self)->layer; }

static int sprite640_set_bitmap(void *self, const uint8_t *bitmap, int stride) {
    sprite640_t *s = (sprite640_t *)self;
    if (!bitmap) return 0;
    for (int y = 0; y < s->height; ++y) {
        memcpy(s->bitmap + y * s->bitmap_stride, bitmap + y * stride, s->width);
    }
    s->dirty = 1;
    return 1;
}

static uint8_t *sprite640_get_bitmap(void *self) { return ((sprite640_t *)self)->bitmap; }
static void sprite640_clear(void *self) { sprite640_t *s = (sprite640_t *)self; memset(s->bitmap, 0, s->bitmap_stride * s->height); s->dirty = 1; }
static void sprite640_fill(void *self, int color) { sprite640_t *s = (sprite640_t *)self; memset(s->bitmap, color & 0x0F, s->bitmap_stride * s->height); s->dirty = 1; }

static void sprite640_draw_rect(void *self, int x1, int y1, int x2, int y2, int color) {
    sprite640_t *s = (sprite640_t *)self;
    if (x1 < 0) x1 = 0; if (y1 < 0) y1 = 0;
    if (x2 >= s->width) x2 = s->width - 1; if (y2 >= s->height) y2 = s->height - 1;
    if (x1 > x2 || y1 > y2) return;
    color &= 0x0F;
    for (int y = y1; y <= y2; ++y) {
        for (int x = x1; x <= x2; ++x) {
            s->bitmap[y * s->bitmap_stride + x] = color;
        }
    }
    s->dirty = 1;
}

static void sprite640_draw_line(void *self, int x1, int y1, int x2, int y2, int color) {
    sprite640_t *s = (sprite640_t *)self;
    color &= 0x0F;
    int dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    int sx = (x2 > x1) ? 1 : -1, sy = (y2 > y1) ? 1 : -1;
    int err = dx - dy, x = x1, y = y1;
    while (1) {
        if (x >= 0 && x < s->width && y >= 0 && y < s->height) {
            s->bitmap[y * s->bitmap_stride + x] = color;
        }
        if (x == x2 && y == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
    s->dirty = 1;
}

static void sprite640_draw_circle(void *self, int cx, int cy, int radius, int color) {
    sprite640_t *s = (sprite640_t *)self;
    color &= 0x0F;
    int x = radius, y = 0, p = 1 - radius;
    while (x >= y) {
        int coords[][2] = {
            {cx + x, cy + y}, {cx - x, cy + y}, {cx + x, cy - y}, {cx - x, cy - y},
            {cx + y, cy + x}, {cx - y, cy + x}, {cx + y, cy - x}, {cx - y, cy - x}
        };
        for (int i = 0; i < 8; ++i) {
            int px = coords[i][0], py = coords[i][1];
            if (px >= 0 && px < s->width && py >= 0 && py < s->height) {
                s->bitmap[py * s->bitmap_stride + px] = color;
            }
        }
        if (p < 0) p += 2 * y + 1; else { p += 2 * (y - x) + 1; x--; }
        y++;
    }
    s->dirty = 1;
}

static int sprite640_collides_point(void *self, int x, int y) {
    sprite640_t *s = (sprite640_t *)self;
    return (x >= s->x && x <= s->x + s->width - 1 && y >= s->y && y <= s->y + s->height - 1) ? 1 : 0;
}

static int sprite640_collides_sprite(void *self, void *other) {
    sprite640_t *s1 = (sprite640_t *)self;
    sprite640_t *s2 = (sprite640_t *)other;
    return !(s1->x + s1->width <= s2->x || s2->x + s2->width <= s1->x || s1->y + s1->height <= s2->y || s2->y + s2->height <= s1->y) ? 1 : 0;
}

static int sprite640_collides_rect(void *self, int x1, int y1, int x2, int y2) {
    sprite640_t *s = (sprite640_t *)self;
    return !(s->x + s->width <= x1 || x2 + 1 <= s->x || s->y + s->height <= y1 || y2 + 1 <= s->y) ? 1 : 0;
}

static void sprite640_destroy(void *self) {
    sprite640_t *s = (sprite640_t *)self;
    if (s->bitmap) free(s->bitmap);
    free(s);
}

static sprite_vtable_t sprite640_vtable = {
    .set_position = sprite640_set_position, .move = sprite640_move,
    .get_x = sprite640_get_x, .get_y = sprite640_get_y,
    .set_visible = sprite640_set_visible, .is_visible = sprite640_is_visible,
    .set_color = sprite640_set_color, .get_color = sprite640_get_color,
    .set_layer = sprite640_set_layer, .get_layer = sprite640_get_layer,
    .set_bitmap = sprite640_set_bitmap, .get_bitmap = sprite640_get_bitmap,
    .clear = sprite640_clear, .fill = sprite640_fill,
    .draw_rect = sprite640_draw_rect, .draw_line = sprite640_draw_line,
    .draw_circle = sprite640_draw_circle,
    .collides_point = sprite640_collides_point, .collides_sprite = sprite640_collides_sprite,
    .collides_rect = sprite640_collides_rect, .destroy = sprite640_destroy,
};

/* ============================================================================
 * GLOBAL STATE
 * ========================================================================== */

static int initialized = 0;
static int current_resolution = 320;
static sprite_t sprite_registry[MAX_SPRITES];
static int sprite_registry_count = 0;

/* ============================================================================
 * INITIALIZATION
 * ========================================================================== */

int sprite_init(void) {
    initialized = 1;
    sprite_registry_count = 0;
    current_resolution = 320;
    return 1;
}

void sprite_shutdown(void) {
    for (int i = 0; i < sprite_registry_count; ++i) {
        if (sprite_registry[i]) {
            sprite_t s = sprite_registry[i];
            sprite_base_t *base = (sprite_base_t *)s;
            base->vtable->destroy(base->impl);
            free(s);
        }
    }
    sprite_registry_count = 0;
    initialized = 0;
}

int sprite_is_initialized(void) { return initialized; }

int sprite_set_resolution(int width) {
    if (width == 320 || width == 640) {
        current_resolution = width;
        return 1;
    }
    return 0;
}

int sprite_get_resolution(void) { return current_resolution; }

int sprite_get_height(void) { return SPRITE_HEIGHT_PAL; }

/* ============================================================================
 * SPRITE CREATION & DESTRUCTION
 * ========================================================================== */

sprite_t sprite_create(int x, int y, int width, int height) {
    if (!initialized || sprite_registry_count >= MAX_SPRITES) return INVALID_SPRITE;
    if (width <= 0 || height <= 0) return INVALID_SPRITE;

    uint8_t *bitmap = (uint8_t *)malloc(width * height);
    if (!bitmap) return INVALID_SPRITE;
    memset(bitmap, 0, width * height);

    if (current_resolution == 640) {
        sprite640_t *s = (sprite640_t *)malloc(sizeof(sprite640_t));
        if (!s) { free(bitmap); return INVALID_SPRITE; }
        s->x = x; s->y = y; s->width = width; s->height = height;
        s->visible = 1; s->layer = 0; s->color = 1;
        s->bitmap = bitmap; s->bitmap_stride = width; s->dirty = 1;
        sprite_registry[sprite_registry_count] = (sprite_t)s;
        return sprite_registry[sprite_registry_count++];
    } else {
        sprite320_t *s = (sprite320_t *)malloc(sizeof(sprite320_t));
        if (!s) { free(bitmap); return INVALID_SPRITE; }
        s->x = x; s->y = y; s->width = width; s->height = height;
        s->visible = 1; s->layer = 0; s->color = 1;
        s->bitmap = bitmap; s->bitmap_stride = width; s->dirty = 1;
        sprite_registry[sprite_registry_count] = (sprite_t)s;
        return sprite_registry[sprite_registry_count++];
    }
}

void sprite_destroy(sprite_t sprite) {
    if (!sprite) return;
    sprite_base_t *base = (sprite_base_t *)sprite;
    base->vtable->destroy(base->impl);
    free(sprite);
}

int sprite_get_info(sprite_t sprite, sprite_info_t *info) {
    if (!sprite || !info) return 0;
    sprite_base_t *base = (sprite_base_t *)sprite;
    info->id = sprite;
    info->x = base->vtable->get_x(base->impl);
    info->y = base->vtable->get_y(base->impl);
    info->visible = base->vtable->is_visible(base->impl);
    info->layer = base->vtable->get_layer(base->impl);
    info->color = base->vtable->get_color(base->impl);
    info->bitmap = base->vtable->get_bitmap(base->impl);
    info->is_sprite640 = (base->type == 1) ? 1 : 0;
    return 1;
}

/* ============================================================================
 * PUBLIC API (delegates to vtable)
 * ========================================================================== */

void sprite_set_position(sprite_t sprite, int x, int y) {
    if (!sprite) return;
    sprite_base_t *base = (sprite_base_t *)sprite;
    base->vtable->set_position(base->impl, x, y);
}

void sprite_move(sprite_t sprite, int dx, int dy) {
    if (!sprite) return;
    sprite_base_t *base = (sprite_base_t *)sprite;
    base->vtable->move(base->impl, dx, dy);
}

int sprite_get_position(sprite_t sprite, int *x, int *y) {
    if (!sprite || !x || !y) return 0;
    sprite_base_t *base = (sprite_base_t *)sprite;
    *x = base->vtable->get_x(base->impl);
    *y = base->vtable->get_y(base->impl);
    return 1;
}

int sprite_get_bounds(sprite_t sprite, int *x1, int *y1, int *x2, int *y2) {
    if (!sprite || !x1 || !y1 || !x2 || !y2) return 0;
    sprite_base_t *base = (sprite_base_t *)sprite;
    int x = base->vtable->get_x(base->impl);
    int y = base->vtable->get_y(base->impl);
    sprite_info_t info;
    sprite_get_info(sprite, &info);
    *x1 = x; *y1 = y; *x2 = x + info.width - 1; *y2 = y + info.height - 1;
    return 1;
}

void sprite_set_visible(sprite_t sprite, int visible) {
    if (!sprite) return;
    sprite_base_t *base = (sprite_base_t *)sprite;
    base->vtable->set_visible(base->impl, visible);
}

int sprite_is_visible(sprite_t sprite) {
    if (!sprite) return 0;
    sprite_base_t *base = (sprite_base_t *)sprite;
    return base->vtable->is_visible(base->impl);
}

void sprite_set_color(sprite_t sprite, int color) {
    if (!sprite) return;
    sprite_base_t *base = (sprite_base_t *)sprite;
    base->vtable->set_color(base->impl, color);
}

int sprite_get_color(sprite_t sprite) {
    if (!sprite) return -1;
    sprite_base_t *base = (sprite_base_t *)sprite;
    return base->vtable->get_color(base->impl);
}

void sprite_set_layer(sprite_t sprite, int layer) {
    if (!sprite) return;
    sprite_base_t *base = (sprite_base_t *)sprite;
    base->vtable->set_layer(base->impl, layer);
}

int sprite_get_layer(sprite_t sprite) {
    if (!sprite) return -1;
    sprite_base_t *base = (sprite_base_t *)sprite;
    return base->vtable->get_layer(base->impl);
}

int sprite_set_bitmap(sprite_t sprite, const uint8_t *bitmap, int stride) {
    if (!sprite) return 0;
    sprite_base_t *base = (sprite_base_t *)sprite;
    return base->vtable->set_bitmap(base->impl, bitmap, stride);
}

uint8_t *sprite_get_bitmap(sprite_t sprite) {
    if (!sprite) return NULL;
    sprite_base_t *base = (sprite_base_t *)sprite;
    return base->vtable->get_bitmap(base->impl);
}

void sprite_clear(sprite_t sprite) {
    if (!sprite) return;
    sprite_base_t *base = (sprite_base_t *)sprite;
    base->vtable->clear(base->impl);
}

void sprite_fill(sprite_t sprite, int color) {
    if (!sprite) return;
    sprite_base_t *base = (sprite_base_t *)sprite;
    base->vtable->fill(base->impl, color);
}

void sprite_draw_rect(sprite_t sprite, int x1, int y1, int x2, int y2, int color) {
    if (!sprite) return;
    sprite_base_t *base = (sprite_base_t *)sprite;
    base->vtable->draw_rect(base->impl, x1, y1, x2, y2, color);
}

void sprite_draw_line(sprite_t sprite, int x1, int y1, int x2, int y2, int color) {
    if (!sprite) return;
    sprite_base_t *base = (sprite_base_t *)sprite;
    base->vtable->draw_line(base->impl, x1, y1, x2, y2, color);
}

void sprite_draw_circle(sprite_t sprite, int cx, int cy, int radius, int color) {
    if (!sprite) return;
    sprite_base_t *base = (sprite_base_t *)sprite;
    base->vtable->draw_circle(base->impl, cx, cy, radius, color);
}

int sprite_collides_point(sprite_t sprite, int x, int y) {
    if (!sprite) return 0;
    sprite_base_t *base = (sprite_base_t *)sprite;
    return base->vtable->collides_point(base->impl, x, y);
}

int sprite_collides_sprite(sprite_t sprite1, sprite_t sprite2) {
    if (!sprite1 || !sprite2) return 0;
    sprite_base_t *base1 = (sprite_base_t *)sprite1;
    sprite_base_t *base2 = (sprite_base_t *)sprite2;
    return base1->vtable->collides_sprite(base1->impl, base2->impl);
}

int sprite_collides_rect(sprite_t sprite, int x1, int y1, int x2, int y2) {
    if (!sprite) return 0;
    sprite_base_t *base = (sprite_base_t *)sprite;
    return base->vtable->collides_rect(base->impl, x1, y1, x2, y2);
}

sprite_t sprite_at_lightpen(void) { return INVALID_SPRITE; }

sprite_t sprite_hit_test(int x, int y) {
    sprite_t result = INVALID_SPRITE;
    int max_layer = -1;
    for (int i = 0; i < sprite_registry_count; ++i) {
        if (sprite_is_visible(sprite_registry[i]) &&
            sprite_collides_point(sprite_registry[i], x, y)) {
            int layer = sprite_get_layer(sprite_registry[i]);
            if (layer > max_layer) {
                max_layer = layer;
                result = sprite_registry[i];
            }
        }
    }
    return result;
}

int sprite_count(void) { return sprite_registry_count; }

sprite_t sprite_get_by_index(int index) {
    return (index >= 0 && index < sprite_registry_count) ? sprite_registry[index] : INVALID_SPRITE;
}

int sprite_render_all(void) { return 1; }

void sprite_clear_all(void) {}

void sprite_set_background(int color) {}

int sprite_get_background(void) { return 0; }

void sprite_mark_dirty(sprite_t sprite) {}

void sprite_enable_caching(int enabled) {}

void sprite_cache_all(void) {}

void sprite_print_status(void) {
    printf("\n=== Sprite System Status ===\n\n");
    printf("Initialized:  %s\n", initialized ? "Yes" : "No");
    printf("Resolution:   %d pixels\n", current_resolution);
    printf("Sprite Count: %d / %d\n", sprite_registry_count, MAX_SPRITES);
    printf("\n");
}

void sprite_print_info(sprite_t sprite) {
    if (!sprite) { printf("Invalid sprite\n"); return; }
    sprite_info_t info;
    if (!sprite_get_info(sprite, &info)) return;
    printf("\nSprite Info:\n");
    printf("  Position:  (%d, %d)\n", info.x, info.y);
    printf("  Size:      %d × %d\n", info.width, info.height);
    printf("  Type:      %s\n", info.is_sprite640 ? "Sprite640" : "Sprite320");
    printf("  Visible:   %s\n", info.visible ? "Yes" : "No");
    printf("  Layer:     %d\n", info.layer);
    printf("\n");
}

void sprite_dump_bitmap(sprite_t sprite, int max_rows) {
    if (!sprite) return;
    uint8_t *bitmap = sprite_get_bitmap(sprite);
    if (!bitmap) return;
    sprite_info_t info;
    if (!sprite_get_info(sprite, &info)) return;
    int rows = (max_rows > 0 && max_rows < info.height) ? max_rows : info.height;
    for (int y = 0; y < rows; ++y) {
        printf("  ");
        for (int x = 0; x < info.width; ++x) {
            printf("%X", bitmap[y * info.bitmap_stride + x] & 0x0F);
        }
        printf("\n");
    }
}
