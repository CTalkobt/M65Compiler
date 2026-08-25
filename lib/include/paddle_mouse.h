/* paddle_mouse.h — Paddle and Mouse Input Library
 *
 * Support for analog paddle input and multiple mouse protocols:
 * - Commodore 1351 mouse (2 axes + 2 buttons)
 * - Amiga mouse (2 axes + 2 buttons, different protocol)
 * - Analog paddle (potentiometer-based XY input)
 * - PC mouse via adapter (basic 2-button support)
 */

#ifndef PADDLE_MOUSE_H
#define PADDLE_MOUSE_H

typedef void *paddle_mouse_manager_t;
typedef void *paddle_device_t;
typedef void *mouse_device_t;

#define INVALID_MANAGER NULL
#define INVALID_PADDLE NULL
#define INVALID_MOUSE NULL

/* Device types */
typedef enum {
    PADDLE_TYPE_STANDARD = 0,      /* Standard analog potentiometer paddle */
    PADDLE_TYPE_ANALOG_STICK = 1,  /* Analog stick (2 independent potentiometers) */
} paddle_type_t;

typedef enum {
    MOUSE_TYPE_1351 = 0,           /* Commodore 1351 (default) */
    MOUSE_TYPE_AMIGA = 1,          /* Amiga mouse (requires different protocol) */
    MOUSE_TYPE_PC = 2,             /* PC mouse via adapter */
} mouse_type_t;

/* Mouse button bits */
typedef enum {
    MOUSE_BUTTON_LEFT = 0x01,
    MOUSE_BUTTON_RIGHT = 0x02,
} mouse_button_t;

/* Paddle input state */
typedef struct {
    int x;                          /* X position (0-255) */
    int y;                          /* Y position (0-255) */
    int prev_x;
    int prev_y;
    int raw_x;                      /* Raw ADC value (0-1023) */
    int raw_y;
} paddle_state_t;

/* Mouse input state */
typedef struct {
    int x;                          /* X displacement (-127 to +127) */
    int y;                          /* Y displacement (-127 to +127) */
    int buttons;                    /* Button state (LEFT | RIGHT) */
    int prev_buttons;
    int abs_x;                      /* Absolute position tracking */
    int abs_y;
    int sensitivity;                /* Mouse sensitivity multiplier (1-4) */
} mouse_state_t;

/* Paddle movement callback */
typedef void (*paddle_callback_t)(int paddle_id, int x, int y, void *user_data);

/* Mouse movement/button callback */
typedef void (*mouse_callback_t)(int mouse_id, int dx, int dy, int buttons, void *user_data);

/* Manager */
paddle_mouse_manager_t paddle_mouse_manager_create(void);
void paddle_mouse_manager_destroy(paddle_mouse_manager_t manager);

/* Paddle API */
paddle_device_t paddle_init(paddle_mouse_manager_t manager,
                             int port,
                             paddle_type_t type);
int paddle_detect(paddle_mouse_manager_t manager);
int paddle_get_count(paddle_mouse_manager_t manager);
int paddle_read_x(paddle_device_t paddle);
int paddle_read_y(paddle_device_t paddle);
int paddle_read_raw_x(paddle_device_t paddle);
int paddle_read_raw_y(paddle_device_t paddle);
int paddle_update(paddle_device_t paddle, int delta_ms);
int paddle_manager_update(paddle_mouse_manager_t manager, int delta_ms);
int paddle_set_deadzone(paddle_device_t paddle, int deadzone);
int paddle_set_smoothing(paddle_device_t paddle, int enabled);
int paddle_is_connected(paddle_device_t paddle);
int paddle_set_callback(paddle_device_t paddle,
                        paddle_callback_t callback,
                        void *user_data);

/* Mouse API */
mouse_device_t mouse_init(paddle_mouse_manager_t manager,
                           int port,
                           mouse_type_t type);
int mouse_detect(paddle_mouse_manager_t manager);
int mouse_get_count(paddle_mouse_manager_t manager);
int mouse_read_x(mouse_device_t mouse);                /* Returns delta_x */
int mouse_read_y(mouse_device_t mouse);                /* Returns delta_y */
int mouse_read_buttons(mouse_device_t mouse);
int mouse_is_button_pressed(mouse_device_t mouse, int button);
int mouse_update(mouse_device_t mouse, int delta_ms);
int mouse_manager_update(paddle_mouse_manager_t manager, int delta_ms);
int mouse_get_abs_x(mouse_device_t mouse);
int mouse_get_abs_y(mouse_device_t mouse);
int mouse_set_sensitivity(mouse_device_t mouse, int level);
int mouse_is_connected(mouse_device_t mouse);
int mouse_set_callback(mouse_device_t mouse,
                       mouse_callback_t callback,
                       void *user_data);

/* Utility */
const char *mouse_type_name(mouse_type_t type);
const char *paddle_type_name(paddle_type_t type);

#endif
