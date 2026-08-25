/* sprite_ui_framework.c — UI Framework Implementation */

#include "ui.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_WINDOWS 16
#define MAX_WIDGETS 256
#define MAX_MENUS 8
#define MAX_LAYOUTS 32
#define MAX_THEMES 8

typedef struct {
    int x, y, width, height;
    char title[64];
    int visible;
    sprite_ui_widget_t root_widget;
} window_impl;

typedef struct {
    sprite_widget_type_t type;
    int x, y, width, height;
    char text[128];
    sprite_ui_callback_t callback;
    void *user_data;
    int focused;
} widget_impl;

typedef struct {
    char label[64];
    sprite_ui_callback_t callback;
    void *user_data;
} menu_item_impl;

typedef struct {
    menu_item_impl items[32];
    int item_count;
    int visible;
    int x, y;
} menu_impl;

typedef struct {
    sprite_layout_type_t type;
    sprite_ui_widget_t widgets[64];
    int flex_grows[64];
    int widget_count;
    int spacing;
} layout_impl;

typedef struct {
    char colors[32][16];
    char fonts[16][32];
    int font_sizes[16];
} theme_impl;

typedef struct {
    int key_code;
    sprite_ui_callback_t callback;
    void *user_data;
} input_key_binding;

typedef struct {
    input_key_binding bindings[64];
    int binding_count;
} input_handler_impl;

static window_impl windows[MAX_WINDOWS];
static int window_count = 0;

static widget_impl widgets[MAX_WIDGETS];
static int widget_count = 0;

static menu_impl menus[MAX_MENUS];
static int menu_count = 0;

static layout_impl layouts[MAX_LAYOUTS];
static int layout_count = 0;

static theme_impl themes[MAX_THEMES];
static int theme_count = 0;

static input_handler_impl input_handlers[4];
static int input_handler_count = 0;

/* Phase 66: Windows */
sprite_ui_window_t sprite_ui_window_create(int x, int y, int width, int height) {
    if (window_count >= MAX_WINDOWS) return INVALID_WINDOW;
    window_impl *win = &windows[window_count];
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    strcpy(win->title, "Window");
    win->visible = 1;
    win->root_widget = NULL;
    return (sprite_ui_window_t)(intptr_t)window_count++;
}

void sprite_ui_window_destroy(sprite_ui_window_t window) {}

void sprite_ui_window_set_title(sprite_ui_window_t window, const char *title) {
    intptr_t idx = (intptr_t)window;
    if (idx < 0 || idx >= window_count) return;
    strncpy(windows[idx].title, title, sizeof(windows[idx].title) - 1);
}

void sprite_ui_window_set_visible(sprite_ui_window_t window, int visible) {
    intptr_t idx = (intptr_t)window;
    if (idx < 0 || idx >= window_count) return;
    windows[idx].visible = visible;
}

int sprite_ui_window_is_visible(sprite_ui_window_t window) {
    intptr_t idx = (intptr_t)window;
    if (idx < 0 || idx >= window_count) return 0;
    return windows[idx].visible;
}

int sprite_ui_window_update(sprite_ui_window_t window, int delta_ms) {
    intptr_t idx = (intptr_t)window;
    if (idx < 0 || idx >= window_count) return 0;
    return windows[idx].visible;
}

/* Phase 67: Menus */
sprite_ui_menu_t sprite_ui_menu_create(void) {
    if (menu_count >= MAX_MENUS) return NULL;
    menu_impl *menu = &menus[menu_count];
    menu->item_count = 0;
    menu->visible = 0;
    menu->x = menu->y = 0;
    return (sprite_ui_menu_t)(intptr_t)menu_count++;
}

void sprite_ui_menu_destroy(sprite_ui_menu_t menu) {}

sprite_ui_menu_item_t sprite_ui_menu_add_item(sprite_ui_menu_t menu, const char *label, sprite_ui_callback_t callback, void *user_data) {
    intptr_t idx = (intptr_t)menu;
    if (idx < 0 || idx >= menu_count) return NULL;
    menu_impl *m = &menus[idx];
    if (m->item_count >= 32) return NULL;
    menu_item_impl *item = &m->items[m->item_count];
    strncpy(item->label, label, sizeof(item->label) - 1);
    item->callback = callback;
    item->user_data = user_data;
    return (sprite_ui_menu_item_t)(intptr_t)m->item_count++;
}

int sprite_ui_menu_remove_item(sprite_ui_menu_t menu, sprite_ui_menu_item_t item) {
    return 1;
}

int sprite_ui_menu_popup(sprite_ui_menu_t menu, int x, int y) {
    intptr_t idx = (intptr_t)menu;
    if (idx < 0 || idx >= menu_count) return 0;
    menus[idx].visible = 1;
    menus[idx].x = x;
    menus[idx].y = y;
    return 1;
}

void sprite_ui_menu_close(sprite_ui_menu_t menu) {
    intptr_t idx = (intptr_t)menu;
    if (idx < 0 || idx >= menu_count) return;
    menus[idx].visible = 0;
}

/* Phase 68: Input */
sprite_ui_input_handler_t sprite_ui_input_handler_create(void) {
    if (input_handler_count >= 4) return INVALID_INPUT;
    input_handler_impl *handler = &input_handlers[input_handler_count];
    handler->binding_count = 0;
    return (sprite_ui_input_handler_t)(intptr_t)input_handler_count++;
}

void sprite_ui_input_handler_destroy(sprite_ui_input_handler_t handler) {}

void sprite_ui_input_handler_on_key(sprite_ui_input_handler_t handler, int key_code, sprite_ui_callback_t callback, void *user_data) {
    intptr_t idx = (intptr_t)handler;
    if (idx < 0 || idx >= input_handler_count) return;
    input_handler_impl *h = &input_handlers[idx];
    if (h->binding_count < 64) {
        h->bindings[h->binding_count].key_code = key_code;
        h->bindings[h->binding_count].callback = callback;
        h->bindings[h->binding_count].user_data = user_data;
        h->binding_count++;
    }
}

void sprite_ui_input_handler_on_mouse(sprite_ui_input_handler_t handler, int button, sprite_ui_callback_t callback, void *user_data) {
}

int sprite_ui_input_handler_process(sprite_ui_input_handler_t handler, int event_type, int data) {
    intptr_t idx = (intptr_t)handler;
    if (idx < 0 || idx >= input_handler_count) return 0;
    return 1;
}

/* Phase 69: Layouts */
sprite_ui_layout_t sprite_ui_layout_create(sprite_layout_type_t type) {
    if (layout_count >= MAX_LAYOUTS) return INVALID_LAYOUT;
    layout_impl *layout = &layouts[layout_count];
    layout->type = type;
    layout->widget_count = 0;
    layout->spacing = 4;
    return (sprite_ui_layout_t)(intptr_t)layout_count++;
}

void sprite_ui_layout_destroy(sprite_ui_layout_t layout) {}

void sprite_ui_layout_add_widget(sprite_ui_layout_t layout, sprite_ui_widget_t widget, int flex_grow) {
    intptr_t idx = (intptr_t)layout;
    if (idx < 0 || idx >= layout_count) return;
    layout_impl *l = &layouts[idx];
    if (l->widget_count < 64) {
        l->widgets[l->widget_count] = widget;
        l->flex_grows[l->widget_count] = flex_grow;
        l->widget_count++;
    }
}

void sprite_ui_layout_set_spacing(sprite_ui_layout_t layout, int spacing) {
    intptr_t idx = (intptr_t)layout;
    if (idx < 0 || idx >= layout_count) return;
    layouts[idx].spacing = spacing;
}

int sprite_ui_layout_compute(sprite_ui_layout_t layout, int width, int height) {
    intptr_t idx = (intptr_t)layout;
    if (idx < 0 || idx >= layout_count) return 0;
    return 1;
}

/* Phase 70: Widgets */
sprite_ui_widget_t sprite_ui_widget_create(sprite_widget_type_t type) {
    if (widget_count >= MAX_WIDGETS) return INVALID_WIDGET;
    widget_impl *widget = &widgets[widget_count];
    widget->type = type;
    widget->x = widget->y = 0;
    widget->width = widget->height = 32;
    strcpy(widget->text, "");
    widget->callback = NULL;
    widget->user_data = NULL;
    widget->focused = 0;
    return (sprite_ui_widget_t)(intptr_t)widget_count++;
}

void sprite_ui_widget_destroy(sprite_ui_widget_t widget) {}

void sprite_ui_widget_set_bounds(sprite_ui_widget_t widget, int x, int y, int width, int height) {
    intptr_t idx = (intptr_t)widget;
    if (idx < 0 || idx >= widget_count) return;
    widgets[idx].x = x;
    widgets[idx].y = y;
    widgets[idx].width = width;
    widgets[idx].height = height;
}

void sprite_ui_widget_set_text(sprite_ui_widget_t widget, const char *text) {
    intptr_t idx = (intptr_t)widget;
    if (idx < 0 || idx >= widget_count) return;
    strncpy(widgets[idx].text, text, sizeof(widgets[idx].text) - 1);
}

const char *sprite_ui_widget_get_text(sprite_ui_widget_t widget) {
    intptr_t idx = (intptr_t)widget;
    if (idx < 0 || idx >= widget_count) return "";
    return widgets[idx].text;
}

void sprite_ui_widget_set_callback(sprite_ui_widget_t widget, sprite_ui_callback_t callback, void *user_data) {
    intptr_t idx = (intptr_t)widget;
    if (idx < 0 || idx >= widget_count) return;
    widgets[idx].callback = callback;
    widgets[idx].user_data = user_data;
}

int sprite_ui_widget_on_event(sprite_ui_widget_t widget, int event_type, int data) {
    intptr_t idx = (intptr_t)widget;
    if (idx < 0 || idx >= widget_count) return 0;
    widget_impl *w = &widgets[idx];
    if (w->callback) {
        w->callback(widget, w->user_data);
        return 1;
    }
    return 0;
}

sprite_ui_theme_t sprite_ui_theme_create(void) {
    if (theme_count >= MAX_THEMES) return INVALID_THEME;
    return (sprite_ui_theme_t)(intptr_t)theme_count++;
}

void sprite_ui_theme_destroy(sprite_ui_theme_t theme) {}

void sprite_ui_theme_set_color(sprite_ui_theme_t theme, const char *element, int color) {
}

void sprite_ui_theme_set_font(sprite_ui_theme_t theme, const char *font_name, int size) {
}

int sprite_ui_theme_apply(sprite_ui_theme_t theme, sprite_ui_widget_t widget) {
    return 1;
}
