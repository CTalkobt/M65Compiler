/* sprite_joystick.h — Joystick Input Library
 *
 * Support for 4-5 button joystick protocols with debouncing and event callbacks.
 *
 * Supported Protocols:
 * - Atari 2600 (4-way + fire)
 * - Sega Genesis (6-button)
 * - Custom MEGA65 protocol
 */

#ifndef SPRITE_JOYSTICK_H
#define SPRITE_JOYSTICK_H

typedef void *sprite_joystick_manager_t;
typedef void *sprite_joystick_t;

#define INVALID_JOYSTICK_MANAGER NULL
#define INVALID_JOYSTICK NULL

typedef enum {
    JOYSTICK_PROTOCOL_ATARI = 0,
    JOYSTICK_PROTOCOL_SEGA = 1,
    JOYSTICK_PROTOCOL_CUSTOM = 2,
    JOYSTICK_PROTOCOL_SEGA_FULL = 3,  /* R6 boards: 6-button with bidirectional SELECT */
} sprite_joystick_protocol_t;

typedef enum {
    JOYSTICK_UP = 0x01,
    JOYSTICK_DOWN = 0x02,
    JOYSTICK_LEFT = 0x04,
    JOYSTICK_RIGHT = 0x08,
    JOYSTICK_BUTTON_A = 0x10,
    JOYSTICK_BUTTON_B = 0x20,
    JOYSTICK_BUTTON_C = 0x40,
    JOYSTICK_BUTTON_X = 0x80,
    JOYSTICK_BUTTON_Y = 0x100,
    JOYSTICK_BUTTON_Z = 0x200,
} sprite_joystick_button_t;

typedef struct {
    int buttons;
    int prev_buttons;
    int debounce_time;
    int debounce_counter;
    int buttons_select_0;  /* Sega: buttons read with SELECT=0 (A,B,C) */
    int buttons_select_1;  /* Sega: buttons read with SELECT=1 (X,Y,Z) */
    int is_r6_board;       /* 1 if R6 with bidirectional joystick support */
} sprite_joystick_state_t;

typedef void (*sprite_joystick_callback_t)(int joystick_id,
                                            int button,
                                            int pressed,
                                            void *user_data);

/* Manager */
sprite_joystick_manager_t sprite_joystick_manager_create(int max_joysticks);
void sprite_joystick_manager_destroy(sprite_joystick_manager_t manager);

/* Joystick Detection & Initialization */
sprite_joystick_t sprite_joystick_init(sprite_joystick_manager_t manager,
                                        int port,
                                        sprite_joystick_protocol_t protocol);
int sprite_joystick_detect(sprite_joystick_manager_t manager);
int sprite_joystick_get_count(sprite_joystick_manager_t manager);

/* R6 Board Detection (for bidirectional joystick support) */
int sprite_joystick_is_r6_board(void);
int sprite_joystick_enable_sega_full(sprite_joystick_t joystick, int port);

/* Button Reading */
int sprite_joystick_read_buttons(sprite_joystick_t joystick);
int sprite_joystick_is_pressed(sprite_joystick_t joystick, int button);
int sprite_joystick_was_pressed(sprite_joystick_t joystick, int button);
int sprite_joystick_get_direction(sprite_joystick_t joystick,
                                   int *x, int *y);

/* Debouncing */
void sprite_joystick_set_debounce_time(sprite_joystick_t joystick, int ms);
int sprite_joystick_update(sprite_joystick_t joystick, int delta_ms);
int sprite_joystick_manager_update(sprite_joystick_manager_t manager, int delta_ms);

/* Callbacks */
int sprite_joystick_set_callback(sprite_joystick_t joystick,
                                  int button_mask,
                                  sprite_joystick_callback_t callback,
                                  void *user_data);
int sprite_joystick_remove_callback(sprite_joystick_t joystick, int button_mask);

/* Mapping & Configuration */
int sprite_joystick_remap_button(sprite_joystick_t joystick,
                                  int physical_button,
                                  int logical_button);
int sprite_joystick_invert_axis(sprite_joystick_t joystick, int axis, int invert);

/* Status & Info */
int sprite_joystick_is_connected(sprite_joystick_t joystick);
const char *sprite_joystick_get_name(sprite_joystick_t joystick);
int sprite_joystick_get_button_count(sprite_joystick_t joystick);

/* Utility */
const char *sprite_joystick_button_name(int button);
int sprite_joystick_button_from_name(const char *name);

#endif
