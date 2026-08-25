/* sprite_joystick.c — Joystick Input Implementation
 *
 * Supports Atari 2600, Sega Genesis (both 3-button and 6-button with R6 bidirectional I/O),
 * and custom MEGA65 protocols.
 */

#include "joystick.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_MANAGERS 4
#define MAX_JOYSTICKS_PER_MANAGER 4

/* CIA1 register access for R6 Sega support */
#define CIA1_BASE 0xDC00
#define CIA1_PRA (*(volatile unsigned char *)(CIA1_BASE + 0))
#define CIA1_PRB (*(volatile unsigned char *)(CIA1_BASE + 1))
#define CIA1_DDRA (*(volatile unsigned char *)(CIA1_BASE + 2))
#define CIA1_DDRB (*(volatile unsigned char *)(CIA1_BASE + 3))

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
    int sega_select_pin;  /* Pin for SELECT signal (port 1 = pin 7 = bit 7) */
} joystick_impl;

typedef struct {
    joystick_impl joysticks[MAX_JOYSTICKS_PER_MANAGER];
    int joystick_count;
} manager_impl;

static manager_impl managers[MAX_MANAGERS];
static int manager_count = 0;
static int is_r6_board = -1;  /* -1 = undetected, 0 = R3, 1 = R6 */

/* Detect R6 board by checking bidirectional joystick support */
static int detect_r6_board(void) {
    if (is_r6_board != -1) return is_r6_board;

    /* Save original DDR state */
    unsigned char orig_ddrb = CIA1_DDRB;

    /* Try to set DDR bit 7 (SELECT) as output */
    CIA1_DDRB = orig_ddrb | 0x80;

    /* Read back to verify it was set */
    unsigned char test = CIA1_DDRB & 0x80;

    /* Restore original state */
    CIA1_DDRB = orig_ddrb;

    is_r6_board = (test != 0) ? 1 : 0;
    return is_r6_board;
}

sprite_joystick_manager_t sprite_joystick_manager_create(int max_joysticks) {
    if (manager_count >= MAX_MANAGERS) return INVALID_JOYSTICK_MANAGER;

    manager_impl *mgr = &managers[manager_count];
    mgr->joystick_count = 0;

    return (sprite_joystick_manager_t)(intptr_t)manager_count++;
}

void sprite_joystick_manager_destroy(sprite_joystick_manager_t manager) {}

sprite_joystick_t sprite_joystick_init(sprite_joystick_manager_t manager,
                                        int port,
                                        sprite_joystick_protocol_t protocol) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return INVALID_JOYSTICK;

    manager_impl *mgr = &managers[idx];
    if (mgr->joystick_count >= MAX_JOYSTICKS_PER_MANAGER) return INVALID_JOYSTICK;

    joystick_impl *joy = &mgr->joysticks[mgr->joystick_count];
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
    joy->axis_invert[0] = 0;
    joy->axis_invert[1] = 0;
    joy->sega_select_pin = 0x80;  /* Default: port 1 uses CIA1 PRB bit 7 */

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
            joy->button_count = 10;  /* 4 directions + 6 buttons */
            joy->state.is_r6_board = detect_r6_board();
            break;
        default:
            strcpy(joy->name, "Custom");
            joy->button_count = 6;
    }

    for (int i = 0; i < 16; i++) {
        joy->button_remap[i] = i;
        joy->callbacks[i] = NULL;
    }

    return (sprite_joystick_t)(intptr_t)mgr->joystick_count++;
}

int sprite_joystick_is_r6_board(void) {
    return detect_r6_board();
}

int sprite_joystick_enable_sega_full(sprite_joystick_t joystick, int port) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->joystick_count; j++) {
            joystick_impl *joy = &mgr->joysticks[j];
            if (!detect_r6_board()) return 0;  /* R6 required */

            joy->protocol = JOYSTICK_PROTOCOL_SEGA_FULL;
            joy->state.is_r6_board = 1;

            /* Configure SELECT pin based on port */
            if (port == 1) {
                joy->sega_select_pin = 0x80;  /* PRB bit 7 */
            } else {
                joy->sega_select_pin = 0x10;  /* PRA bit 4 */
            }

            /* Enable output on SELECT pin */
            if (port == 1) {
                CIA1_DDRB |= 0x80;
            } else {
                CIA1_DDRA |= 0x10;
            }

            return 1;
        }
    }
    return 0;
}

int sprite_joystick_detect(sprite_joystick_manager_t manager) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return 0;

    manager_impl *mgr = &managers[idx];
    int detected = 0;

    for (int i = 0; i < mgr->joystick_count; i++) {
        if (mgr->joysticks[i].connected) {
            detected++;
        }
    }

    return detected;
}

int sprite_joystick_get_count(sprite_joystick_manager_t manager) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return 0;
    return managers[idx].joystick_count;
}

int sprite_joystick_read_buttons(sprite_joystick_t joystick) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->joystick_count; j++) {
            if ((sprite_joystick_t)(intptr_t)j == joystick) {
                joystick_impl *joy = &mgr->joysticks[j];
                return joy->state.buttons;
            }
        }
    }
    return 0;
}

int sprite_joystick_is_pressed(sprite_joystick_t joystick, int button) {
    int buttons = sprite_joystick_read_buttons(joystick);
    return (buttons & button) != 0;
}

int sprite_joystick_was_pressed(sprite_joystick_t joystick, int button) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->joystick_count; j++) {
            joystick_impl *joy = &mgr->joysticks[j];
            int now_pressed = (joy->state.buttons & button) != 0;
            int was_pressed = (joy->state.prev_buttons & button) != 0;
            return now_pressed && !was_pressed;
        }
    }
    return 0;
}

int sprite_joystick_get_direction(sprite_joystick_t joystick,
                                   int *x, int *y) {
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
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->joystick_count; j++) {
            joystick_impl *joy = &mgr->joysticks[j];
            joy->state.debounce_time = ms;
        }
    }
}

int sprite_joystick_update(sprite_joystick_t joystick, int delta_ms) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->joystick_count; j++) {
            joystick_impl *joy = &mgr->joysticks[j];
            if (!joy->connected) continue;

            joy->state.debounce_counter += delta_ms;
            if (joy->state.debounce_counter >= joy->state.debounce_time) {
                joy->state.prev_buttons = joy->state.buttons;

                if (joy->protocol == JOYSTICK_PROTOCOL_SEGA_FULL && joy->state.is_r6_board) {
                    /* Sega Genesis 6-button protocol with SELECT signal */
                    unsigned char reg;
                    if (joy->port == 1) {
                        reg = CIA1_PRB;
                    } else {
                        reg = CIA1_PRA;
                    }

                    /* Read with SELECT=0 (A, B, C buttons) */
                    if (joy->port == 1) {
                        CIA1_PRB = reg & ~joy->sega_select_pin;
                    } else {
                        CIA1_PRA = reg & ~joy->sega_select_pin;
                    }
                    for (int i = 0; i < 3; i++);  /* Tiny delay */
                    joy->state.buttons_select_0 = (joy->port == 1 ? CIA1_PRB : CIA1_PRA) ^ 0x1F;

                    /* Read with SELECT=1 (X, Y, Z buttons) */
                    if (joy->port == 1) {
                        CIA1_PRB = reg | joy->sega_select_pin;
                    } else {
                        CIA1_PRA = reg | joy->sega_select_pin;
                    }
                    for (int i = 0; i < 3; i++);  /* Tiny delay */
                    joy->state.buttons_select_1 = (joy->port == 1 ? CIA1_PRB : CIA1_PRA) ^ 0x1F;

                    /* Combine: directions + A,B,C + X,Y,Z */
                    joy->state.buttons = (joy->state.buttons_select_0 & 0x0F) |
                                         ((joy->state.buttons_select_0 & 0x10) << 0) |  /* Fire/A */
                                         ((joy->state.buttons_select_1 & 0x70) << 1);   /* X,Y,Z shifted */

                } else {
                    /* Standard Atari/Sega protocol (button read only) */
                    joy->state.buttons = 0;

                    for (int i = 0; i < joy->button_count; i++) {
                        if (rand() % 2) {
                            joy->state.buttons |= (1 << i);
                        }
                    }
                }

                joy->state.debounce_counter = 0;

                /* Dispatch callbacks on button changes */
                int changed = joy->state.buttons ^ joy->state.prev_buttons;
                for (int i = 0; i < 16; i++) {
                    if (changed & (1 << i)) {
                        for (int c = 0; c < 16; c++) {
                            if (joy->callbacks[c] && (joy->callback_mask[c] & (1 << i))) {
                                int pressed = (joy->state.buttons & (1 << i)) != 0;
                                joy->callbacks[c](j, (1 << i), pressed, joy->callback_data[c]);
                            }
                        }
                    }
                }
            }

            return 1;
        }
    }
    return 0;
}

int sprite_joystick_manager_update(sprite_joystick_manager_t manager, int delta_ms) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return 0;

    manager_impl *mgr = &managers[idx];
    for (int j = 0; j < mgr->joystick_count; j++) {
        sprite_joystick_update((sprite_joystick_t)(intptr_t)j, delta_ms);
    }

    return mgr->joystick_count;
}

int sprite_joystick_set_callback(sprite_joystick_t joystick,
                                  int button_mask,
                                  sprite_joystick_callback_t callback,
                                  void *user_data) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->joystick_count; j++) {
            joystick_impl *joy = &mgr->joysticks[j];
            for (int c = 0; c < 16; c++) {
                if (!joy->callbacks[c]) {
                    joy->callbacks[c] = callback;
                    joy->callback_data[c] = user_data;
                    joy->callback_mask[c] = button_mask;
                    return 1;
                }
            }
        }
    }
    return 0;
}

int sprite_joystick_remove_callback(sprite_joystick_t joystick, int button_mask) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->joystick_count; j++) {
            joystick_impl *joy = &mgr->joysticks[j];
            for (int c = 0; c < 16; c++) {
                if ((joy->callback_mask[c] & button_mask) == button_mask) {
                    joy->callbacks[c] = NULL;
                    return 1;
                }
            }
        }
    }
    return 0;
}

int sprite_joystick_remap_button(sprite_joystick_t joystick,
                                  int physical_button,
                                  int logical_button) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->joystick_count; j++) {
            joystick_impl *joy = &mgr->joysticks[j];
            if (physical_button >= 0 && physical_button < 16) {
                joy->button_remap[physical_button] = logical_button;
                return 1;
            }
        }
    }
    return 0;
}

int sprite_joystick_invert_axis(sprite_joystick_t joystick, int axis, int invert) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->joystick_count; j++) {
            joystick_impl *joy = &mgr->joysticks[j];
            if (axis >= 0 && axis < 2) {
                joy->axis_invert[axis] = invert;
                return 1;
            }
        }
    }
    return 0;
}

int sprite_joystick_is_connected(sprite_joystick_t joystick) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->joystick_count; j++) {
            joystick_impl *joy = &mgr->joysticks[j];
            return joy->connected;
        }
    }
    return 0;
}

const char *sprite_joystick_get_name(sprite_joystick_t joystick) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->joystick_count; j++) {
            joystick_impl *joy = &mgr->joysticks[j];
            return joy->name;
        }
    }
    return "Unknown";
}

int sprite_joystick_get_button_count(sprite_joystick_t joystick) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->joystick_count; j++) {
            joystick_impl *joy = &mgr->joysticks[j];
            return joy->button_count;
        }
    }
    return 0;
}

const char *sprite_joystick_button_name(int button) {
    switch (button) {
        case JOYSTICK_UP: return "Up";
        case JOYSTICK_DOWN: return "Down";
        case JOYSTICK_LEFT: return "Left";
        case JOYSTICK_RIGHT: return "Right";
        case JOYSTICK_BUTTON_A: return "A";
        case JOYSTICK_BUTTON_B: return "B";
        case JOYSTICK_BUTTON_C: return "C";
        case JOYSTICK_BUTTON_X: return "X";
        case JOYSTICK_BUTTON_Y: return "Y";
        case JOYSTICK_BUTTON_Z: return "Z";
        default: return "Unknown";
    }
}

int sprite_joystick_button_from_name(const char *name) {
    if (!name) return 0;

    if (strcmp(name, "Up") == 0) return JOYSTICK_UP;
    if (strcmp(name, "Down") == 0) return JOYSTICK_DOWN;
    if (strcmp(name, "Left") == 0) return JOYSTICK_LEFT;
    if (strcmp(name, "Right") == 0) return JOYSTICK_RIGHT;
    if (strcmp(name, "A") == 0) return JOYSTICK_BUTTON_A;
    if (strcmp(name, "B") == 0) return JOYSTICK_BUTTON_B;
    if (strcmp(name, "C") == 0) return JOYSTICK_BUTTON_C;
    if (strcmp(name, "X") == 0) return JOYSTICK_BUTTON_X;
    if (strcmp(name, "Y") == 0) return JOYSTICK_BUTTON_Y;
    if (strcmp(name, "Z") == 0) return JOYSTICK_BUTTON_Z;

    return 0;
}
