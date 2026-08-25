/* sprite_ui_framework.h — UI Framework (Phases 66-70)
 *
 * Phases 66-70: Windows, menus, input, layout, styling
 */

#ifndef SPRITE_UI_FRAMEWORK_H
#define SPRITE_UI_FRAMEWORK_H

typedef void *sprite_ui_window_t;
typedef void *sprite_ui_widget_t;
typedef void *sprite_ui_layout_t;
typedef void *sprite_ui_theme_t;
typedef void *sprite_ui_input_handler_t;

#define INVALID_WINDOW NULL
#define INVALID_WIDGET NULL
#define INVALID_LAYOUT NULL
#define INVALID_THEME NULL
#define INVALID_INPUT NULL

typedef enum {
    WIDGET_BUTTON = 0,
    WIDGET_LABEL = 1,
    WIDGET_TEXTBOX = 2,
    WIDGET_SLIDER = 3,
    WIDGET_CHECKBOX = 4,
    WIDGET_LIST = 5,
    WIDGET_IMAGE = 6,
    WIDGET_PANEL = 7,
} sprite_widget_type_t;

typedef enum {
    LAYOUT_VERTICAL = 0,
    LAYOUT_HORIZONTAL = 1,
    LAYOUT_GRID = 2,
    LAYOUT_ABSOLUTE = 3,
} sprite_layout_type_t;

typedef void (*sprite_ui_callback_t)(sprite_ui_widget_t widget, void *user_data);

/* Phase 66: Window Management */
sprite_ui_window_t sprite_ui_window_create(int x, int y, int width, int height);
void sprite_ui_window_destroy(sprite_ui_window_t window);
void sprite_ui_window_set_title(sprite_ui_window_t window, const char *title);
void sprite_ui_window_set_visible(sprite_ui_window_t window, int visible);
int sprite_ui_window_is_visible(sprite_ui_window_t window);
int sprite_ui_window_update(sprite_ui_window_t window, int delta_ms);

/* Phase 67: Menu System */
typedef void *sprite_ui_menu_t;
typedef void *sprite_ui_menu_item_t;

sprite_ui_menu_t sprite_ui_menu_create(void);
void sprite_ui_menu_destroy(sprite_ui_menu_t menu);
sprite_ui_menu_item_t sprite_ui_menu_add_item(sprite_ui_menu_t menu, const char *label, sprite_ui_callback_t callback, void *user_data);
int sprite_ui_menu_remove_item(sprite_ui_menu_t menu, sprite_ui_menu_item_t item);
int sprite_ui_menu_popup(sprite_ui_menu_t menu, int x, int y);
void sprite_ui_menu_close(sprite_ui_menu_t menu);

/* Phase 68: Input Handling */
sprite_ui_input_handler_t sprite_ui_input_handler_create(void);
void sprite_ui_input_handler_destroy(sprite_ui_input_handler_t handler);
void sprite_ui_input_handler_on_key(sprite_ui_input_handler_t handler, int key_code, sprite_ui_callback_t callback, void *user_data);
void sprite_ui_input_handler_on_mouse(sprite_ui_input_handler_t handler, int button, sprite_ui_callback_t callback, void *user_data);
int sprite_ui_input_handler_process(sprite_ui_input_handler_t handler, int event_type, int data);

/* Phase 69: Layout Management */
sprite_ui_layout_t sprite_ui_layout_create(sprite_layout_type_t type);
void sprite_ui_layout_destroy(sprite_ui_layout_t layout);
void sprite_ui_layout_add_widget(sprite_ui_layout_t layout, sprite_ui_widget_t widget, int flex_grow);
void sprite_ui_layout_set_spacing(sprite_ui_layout_t layout, int spacing);
int sprite_ui_layout_compute(sprite_ui_layout_t layout, int width, int height);

/* Phase 70: Widgets & Styling */
sprite_ui_widget_t sprite_ui_widget_create(sprite_widget_type_t type);
void sprite_ui_widget_destroy(sprite_ui_widget_t widget);
void sprite_ui_widget_set_bounds(sprite_ui_widget_t widget, int x, int y, int width, int height);
void sprite_ui_widget_set_text(sprite_ui_widget_t widget, const char *text);
const char *sprite_ui_widget_get_text(sprite_ui_widget_t widget);
void sprite_ui_widget_set_callback(sprite_ui_widget_t widget, sprite_ui_callback_t callback, void *user_data);
int sprite_ui_widget_on_event(sprite_ui_widget_t widget, int event_type, int data);

sprite_ui_theme_t sprite_ui_theme_create(void);
void sprite_ui_theme_destroy(sprite_ui_theme_t theme);
void sprite_ui_theme_set_color(sprite_ui_theme_t theme, const char *element, int color);
void sprite_ui_theme_set_font(sprite_ui_theme_t theme, const char *font_name, int size);
int sprite_ui_theme_apply(sprite_ui_theme_t theme, sprite_ui_widget_t widget);

#endif
