/* joystick.c — Joystick Input Implementation (Testing Build)
 *
 * Supports Atari 2600, Sega Genesis (both 3-button and 6-button with R6),
 * and custom MEGA65 protocols.
 *
 * Note: Hardware register access disabled for testing builds.
 * Enable CIA1_HARDWARE_ACCESS in build to use real MEGA65 hardware.
 */

#include "joystick.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_MANAGERS 4
#define MAX_JOYSTICKS_PER_MANAGER 4

/* Hardware register access (disabled for testing) */
#ifdef CIA1_HARDWARE_ACCESS
#define CIA1_BASE 0xDC00
#define CIA1_PRA (*(volatile unsigned char *)(CIA1_BASE + 0))
#define CIA1_PRB (*(volatile unsigned char *)(CIA1_BASE + 1))
#define CIA1_DDRA (*(volatile unsigned char *)(CIA1_BASE + 2))
#define CIA1_DDRB (*(volatile unsigned char *)(CIA1_BASE + 3))
#else
static unsigned char cia1_pra = 0;
static unsigned char cia1_prb = 0;
static unsigned char cia1_ddra = 0;
static unsigned char cia1_ddrb = 0;
#define CIA1_PRA cia1_pra
#define CIA1_PRB cia1_prb
#define CIA1_DDRA cia1_ddra
#define CIA1_DDRB cia1_ddrb
#endif

typedef struct {
    int port;
    sprite_joystick_protocol_t protocol;
    sprite_joystick_state_t state;
    int connected;
    int button_count;
    char name[64];
    int button_remap[16];
    int axis_invert[2];
    sprite_joystick_callback_t callbacks[16];
    void *callback_data[16];
    int callback_mask[16];
    int sega_select_pin;
} joystick_impl;

typedef struct {
    joystick_impl *joysticks;
    int joystick_count;
    int max_joysticks;
} manager_impl;

static manager_impl managers[MAX_MANAGERS];
static int manager_count = 0;
static int is_r6_board = -1;

static int detect_r6_board(void) {
    if (is_r6_board != -1) return is_r6_board;
    /* Simplified: assume R6 in test builds */
    is_r6_board = 1;
    return is_r6_board;
}

sprite_joystick_manager_t sprite_joystick_manager_create(int max_joysticks) {
    if (manager_count >= MAX_MANAGERS) return INVALID_JOYSTICK_MANAGER;

    manager_impl *mgr = &managers[manager_count];
    mgr->joysticks = (joystick_impl *)calloc(max_joysticks, sizeof(joystick_impl));
    if (!mgr->joysticks) return INVALID_JOYSTICK_MANAGER;

    mgr->joystick_count = 0;
    mgr->max_joysticks = max_joysticks;

    /* Return manager index + 1 so we never return NULL (0) */
    return (sprite_joystick_manager_t)(intptr_t)(manager_count++ + 1);
}

void sprite_joystick_manager_destroy(sprite_joystick_manager_t manager) {
    intptr_t idx = (intptr_t)manager - 1;
    if (idx < 0 || idx >= manager_count) return;

    manager_impl *mgr = &managers[idx];
    if (mgr->joysticks) {
        free(mgr->joysticks);
        mgr->joysticks = NULL;
    }
    mgr->joystick_count = 0;
}

sprite_joystick_t sprite_joystick_init(sprite_joystick_manager_t manager,
                                        int port,
                                        sprite_joystick_protocol_t protocol) {
    intptr_t idx = (intptr_t)manager - 1;
    if (idx < 0 || idx >= manager_count) return INVALID_JOYSTICK;

    manager_impl *mgr = &managers[idx];
    if (!mgr->joysticks || mgr->joystick_count >= mgr->max_joysticks) {
        return INVALID_JOYSTICK;
    }

    joystick_impl *joy = &mgr->joysticks[mgr->joystick_count];
    memset(joy, 0, sizeof(joystick_impl));

    joy->port = port;
    joy->protocol = protocol;
    joy->connected = 1;
    joy->state.buttons = 0;
    joy->state.prev_buttons = 0;
    joy->state.buttons_select_0 = 0;
    joy->state.buttons_select_1 = 0;
    joy->state.debounce_time = 20;
    joy->state.debounce_counter = 0;
    joy->state.is_r6_board = 0;

    switch (protocol) {
        case JOYSTICK_PROTOCOL_ATARI:
            strcpy(joy->name, "Atari 2600");
            joy->button_count = 5;
            break;
        case JOYSTICK_PROTOCOL_SEGA:
            strcpy(joy->name, "Sega Genesis");
            joy->button_count = 6;
            break;
        case JOYSTICK_PROTOCOL_SEGA_FULL:
            strcpy(joy->name, "Sega Genesis (6-button)");
            joy->button_count = 10;
            joy->state.is_r6_board = detect_r6_board();
            break;
        default:
            strcpy(joy->name, "Custom");
            joy->button_count = 6;
    }

    for (int i = 0; i < 16; i++) {
        joy->button_remap[i] = i;
        joy->callbacks[i] = NULL;
        joy->callback_data[i] = NULL;
    }

    int joy_id = mgr->joystick_count++;
    /* Encode as: manager_idx (high byte) + joy_idx (low byte) + 1 to avoid NULL */
    return (sprite_joystick_t)(intptr_t)(((idx << 8) | joy_id) + 1);
}

int sprite_joystick_is_r6_board(void) {
    return detect_r6_board();
}

int sprite_joystick_enable_sega_full(sprite_joystick_t joystick, int port) {
    if (!detect_r6_board()) return 0;

    intptr_t handle = ((intptr_t)joystick) - 1;
    intptr_t mgr_idx = (handle >> 8) & 0xFF;
    intptr_t joy_idx = handle & 0xFF;

    if (mgr_idx < 0 || mgr_idx >= manager_count) return 0;

    manager_impl *mgr = &managers[mgr_idx];
    if (!mgr->joysticks || joy_idx < 0 || joy_idx >= mgr->joystick_count) return 0;

    joystick_impl *joy = &mgr->joysticks[joy_idx];
    joy->protocol = JOYSTICK_PROTOCOL_SEGA_FULL;
    joy->state.is_r6_board = 1;

    if (port == 1) {
        joy->sega_select_pin = 0x80;
    } else {
        joy->sega_select_pin = 0x10;
    }

    return 1;
}

int sprite_joystick_detect(sprite_joystick_manager_t manager) {
    intptr_t idx = (intptr_t)manager - 1;
    if (idx < 0 || idx >= manager_count) return 0;

    manager_impl *mgr = &managers[idx];
    if (!mgr->joysticks) return 0;

    int detected = 0;
    for (int i = 0; i < mgr->joystick_count; i++) {
        if (mgr->joysticks[i].connected) {
            detected++;
        }
    }

    return detected;
}

int sprite_joystick_get_count(sprite_joystick_manager_t manager) {
    intptr_t idx = (intptr_t)manager - 1;
    if (idx < 0 || idx >= manager_count) return 0;

    manager_impl *mgr = &managers[idx];
    if (!mgr->joysticks) return 0;
    return mgr->joystick_count;
}

static joystick_impl *get_joystick_impl(sprite_joystick_t joystick) {
    intptr_t handle = ((intptr_t)joystick) - 1;
    intptr_t mgr_idx = (handle >> 8) & 0xFF;
    intptr_t joy_idx = handle & 0xFF;

    if (mgr_idx < 0 || mgr_idx >= manager_count) return NULL;

    manager_impl *mgr = &managers[mgr_idx];
    if (!mgr->joysticks || joy_idx < 0 || joy_idx >= mgr->joystick_count) {
        return NULL;
    }

    return &mgr->joysticks[joy_idx];
}

int sprite_joystick_read_buttons(sprite_joystick_t joystick) {
    joystick_impl *joy = get_joystick_impl(joystick);
    if (!joy) return 0;
    return joy->state.buttons;
}

int sprite_joystick_is_pressed(sprite_joystick_t joystick, int button) {
    int buttons = sprite_joystick_read_buttons(joystick);
    return (buttons & button) != 0;
}

int sprite_joystick_was_pressed(sprite_joystick_t joystick, int button) {
    joystick_impl *joy = get_joystick_impl(joystick);
    if (!joy) return 0;

    int now_pressed = (joy->state.buttons & button) != 0;
    int was_pressed = (joy->state.prev_buttons & button) != 0;
    return now_pressed && !was_pressed;
}

int sprite_joystick_get_direction(sprite_joystick_t joystick, int *x, int *y) {
    if (!x || !y) return 0;

    int buttons = sprite_joystick_read_buttons(joystick);
    *x = 0;
    *y = 0;

    if (buttons & JOYSTICK_LEFT) (*x)--;
    if (buttons & JOYSTICK_RIGHT) (*x)++;
    if (buttons & JOYSTICK_UP) (*y)--;
    if (buttons & JOYSTICK_DOWN) (*y)++;

    return 1;
}

void sprite_joystick_set_debounce_time(sprite_joystick_t joystick, int ms) {
    joystick_impl *joy = get_joystick_impl(joystick);
    if (!joy) return;
    joy->state.debounce_time = ms;
}

int sprite_joystick_update(sprite_joystick_t joystick, int delta_ms) {
    joystick_impl *joy = get_joystick_impl(joystick);
    if (!joy || !joy->connected) return 0;

    joy->state.debounce_counter += delta_ms;
    if (joy->state.debounce_counter >= joy->state.debounce_time) {
        joy->state.prev_buttons = joy->state.buttons;
        joy->state.debounce_counter = 0;
    }

    return 1;
}

int sprite_joystick_set_callback(sprite_joystick_t joystick, int button,
                                  sprite_joystick_callback_t callback,
                                  void *data) {
    joystick_impl *joy = get_joystick_impl(joystick);
    if (!joy) return 0;

    for (int i = 0; i < 16; i++) {
        if (joy->callback_mask[i] == 0) {
            joy->callbacks[i] = callback;
            joy->callback_data[i] = data;
            joy->callback_mask[i] = button;
            return 1;
        }
    }

    return 0;
}

int sprite_joystick_clear_callback(sprite_joystick_t joystick, int button) {
    joystick_impl *joy = get_joystick_impl(joystick);
    if (!joy) return 0;

    for (int i = 0; i < 16; i++) {
        if (joy->callback_mask[i] == button) {
            joy->callbacks[i] = NULL;
            joy->callback_mask[i] = 0;
            return 1;
        }
    }

    return 0;
}

int sprite_joystick_remap_button(sprite_joystick_t joystick, int from_button,
                                  int to_button) {
    joystick_impl *joy = get_joystick_impl(joystick);
    if (!joy || from_button < 0 || from_button >= 16) return 0;

    joy->button_remap[from_button] = to_button;
    return 1;
}

int sprite_joystick_invert_axis(sprite_joystick_t joystick, int axis, int invert) {
    joystick_impl *joy = get_joystick_impl(joystick);
    if (!joy || axis < 0 || axis >= 2) return 0;

    joy->axis_invert[axis] = invert ? 1 : 0;
    return 1;
}

int sprite_joystick_is_connected(sprite_joystick_t joystick) {
    joystick_impl *joy = get_joystick_impl(joystick);
    if (!joy) return 0;
    return joy->connected;
}

int sprite_joystick_set_connected(sprite_joystick_t joystick, int connected) {
    joystick_impl *joy = get_joystick_impl(joystick);
    if (!joy) return 0;

    joy->connected = connected ? 1 : 0;
    return 1;
}

const char *sprite_joystick_get_name(sprite_joystick_t joystick) {
    joystick_impl *joy = get_joystick_impl(joystick);
    if (!joy) return "Unknown";
    return joy->name;
}

int sprite_joystick_get_button_count(sprite_joystick_t joystick) {
    joystick_impl *joy = get_joystick_impl(joystick);
    if (!joy) return 0;
    return joy->button_count;
}

sprite_joystick_protocol_t sprite_joystick_get_protocol(sprite_joystick_t joystick) {
    joystick_impl *joy = get_joystick_impl(joystick);
    if (!joy) return JOYSTICK_PROTOCOL_CUSTOM;
    return joy->protocol;
}
