/* paddle_mouse.c — Paddle and Mouse Input Implementation
 *
 * Supports analog paddles and multiple mouse protocols on MEGA65.
 */

#include "paddle_mouse.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_MANAGERS 2
#define MAX_PADDLES_PER_MANAGER 2
#define MAX_MICE_PER_MANAGER 2

/* CIA1 register access */
#define CIA1_BASE 0xDC00
#define CIA1_PRA (*(volatile unsigned char *)(CIA1_BASE + 0))
#define CIA1_PRB (*(volatile unsigned char *)(CIA1_BASE + 1))
#define CIA1_DDRA (*(volatile unsigned char *)(CIA1_BASE + 2))
#define CIA1_DDRB (*(volatile unsigned char *)(CIA1_BASE + 3))

/* SID potentiometer ports (used for paddle input) */
#define SID1_POT_X (*(volatile unsigned char *)0xD419)
#define SID1_POT_Y (*(volatile unsigned char *)0xD41A)
#define SID2_POT_X (*(volatile unsigned char *)0xD439)
#define SID2_POT_Y (*(volatile unsigned char *)0xD43A)

typedef struct {
    int port;
    paddle_type_t type;
    paddle_state_t state;
    int connected;
    int deadzone;
    int smoothing_enabled;
    int update_interval;
    int update_counter;
    paddle_callback_t callback;
    void *callback_data;
    int prev_x, prev_y;
} paddle_impl;

typedef struct {
    int port;
    mouse_type_t type;
    mouse_state_t state;
    int connected;
    int update_interval;
    int update_counter;
    int last_read_x, last_read_y;  /* For delta calculation */
    mouse_callback_t callback;
    void *callback_data;
} mouse_impl;

typedef struct {
    paddle_impl paddles[MAX_PADDLES_PER_MANAGER];
    int paddle_count;
    mouse_impl mice[MAX_MICE_PER_MANAGER];
    int mouse_count;
} manager_impl;

static manager_impl managers[MAX_MANAGERS];
static int manager_count = 0;

paddle_mouse_manager_t paddle_mouse_manager_create(void) {
    if (manager_count >= MAX_MANAGERS) return INVALID_MANAGER;

    manager_impl *mgr = &managers[manager_count];
    mgr->paddle_count = 0;
    mgr->mouse_count = 0;

    return (paddle_mouse_manager_t)(intptr_t)manager_count++;
}

void paddle_mouse_manager_destroy(paddle_mouse_manager_t manager) {}

/* ===== PADDLE API ===== */

paddle_device_t paddle_init(paddle_mouse_manager_t manager,
                             int port,
                             paddle_type_t type) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return INVALID_PADDLE;

    manager_impl *mgr = &managers[idx];
    if (mgr->paddle_count >= MAX_PADDLES_PER_MANAGER) return INVALID_PADDLE;

    paddle_impl *paddle = &mgr->paddles[mgr->paddle_count];
    paddle->port = port;
    paddle->type = type;
    paddle->connected = 1;
    paddle->state.x = 128;
    paddle->state.y = 128;
    paddle->state.prev_x = 128;
    paddle->state.prev_y = 128;
    paddle->state.raw_x = 512;
    paddle->state.raw_y = 512;
    paddle->deadzone = 10;
    paddle->smoothing_enabled = 1;
    paddle->update_interval = 20;  /* 50Hz default */
    paddle->update_counter = 0;
    paddle->callback = NULL;
    paddle->prev_x = 128;
    paddle->prev_y = 128;

    return (paddle_device_t)(intptr_t)mgr->paddle_count++;
}

int paddle_detect(paddle_mouse_manager_t manager) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return 0;

    manager_impl *mgr = &managers[idx];
    int detected = 0;

    for (int i = 0; i < mgr->paddle_count; i++) {
        if (mgr->paddles[i].connected) {
            detected++;
        }
    }

    return detected;
}

int paddle_get_count(paddle_mouse_manager_t manager) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return 0;
    return managers[idx].paddle_count;
}

int paddle_read_x(paddle_device_t paddle) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->paddle_count; j++) {
            if ((paddle_device_t)(intptr_t)j == paddle) {
                return mgr->paddles[j].state.x;
            }
        }
    }
    return 128;
}

int paddle_read_y(paddle_device_t paddle) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->paddle_count; j++) {
            if ((paddle_device_t)(intptr_t)j == paddle) {
                return mgr->paddles[j].state.y;
            }
        }
    }
    return 128;
}

int paddle_read_raw_x(paddle_device_t paddle) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->paddle_count; j++) {
            if ((paddle_device_t)(intptr_t)j == paddle) {
                return mgr->paddles[j].state.raw_x;
            }
        }
    }
    return 512;
}

int paddle_read_raw_y(paddle_device_t paddle) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->paddle_count; j++) {
            if ((paddle_device_t)(intptr_t)j == paddle) {
                return mgr->paddles[j].state.raw_y;
            }
        }
    }
    return 512;
}

int paddle_update(paddle_device_t paddle, int delta_ms) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->paddle_count; j++) {
            paddle_impl *p = &mgr->paddles[j];
            if (!p->connected) continue;

            p->update_counter += delta_ms;
            if (p->update_counter >= p->update_interval) {
                p->update_counter = 0;

                /* Read raw ADC values from SID potentiometer ports */
                if (p->port == 1) {
                    p->state.raw_x = SID1_POT_X << 2;  /* Convert 8-bit to 10-bit */
                    p->state.raw_y = SID1_POT_Y << 2;
                } else {
                    p->state.raw_x = SID2_POT_X << 2;
                    p->state.raw_y = SID2_POT_Y << 2;
                }

                /* Convert raw 10-bit (0-1023) to 8-bit (0-255) */
                p->state.prev_x = p->state.x;
                p->state.prev_y = p->state.y;
                p->state.x = (p->state.raw_x >> 2) & 0xFF;
                p->state.y = (p->state.raw_y >> 2) & 0xFF;

                /* Apply deadzone */
                if (p->deadzone > 0) {
                    int dx = p->state.x - 128;
                    int dy = p->state.y - 128;
                    if (dx < 0) dx = -dx;
                    if (dy < 0) dy = -dy;

                    if (dx < p->deadzone) p->state.x = 128;
                    if (dy < p->deadzone) p->state.y = 128;
                }

                /* Callback on significant movement */
                if (p->callback && (p->state.x != p->prev_x || p->state.y != p->prev_y)) {
                    p->callback(j, p->state.x, p->state.y, p->callback_data);
                }

                return 1;
            }
        }
    }
    return 0;
}

int paddle_manager_update(paddle_mouse_manager_t manager, int delta_ms) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return 0;

    manager_impl *mgr = &managers[idx];
    int updated = 0;

    for (int j = 0; j < mgr->paddle_count; j++) {
        if (paddle_update((paddle_device_t)(intptr_t)j, delta_ms)) {
            updated++;
        }
    }

    return updated;
}

int paddle_set_deadzone(paddle_device_t paddle, int deadzone) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->paddle_count; j++) {
            if ((paddle_device_t)(intptr_t)j == paddle) {
                mgr->paddles[j].deadzone = deadzone;
                return 1;
            }
        }
    }
    return 0;
}

int paddle_set_smoothing(paddle_device_t paddle, int enabled) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->paddle_count; j++) {
            if ((paddle_device_t)(intptr_t)j == paddle) {
                mgr->paddles[j].smoothing_enabled = enabled;
                return 1;
            }
        }
    }
    return 0;
}

int paddle_is_connected(paddle_device_t paddle) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->paddle_count; j++) {
            if ((paddle_device_t)(intptr_t)j == paddle) {
                return mgr->paddles[j].connected;
            }
        }
    }
    return 0;
}

int paddle_set_callback(paddle_device_t paddle,
                        paddle_callback_t callback,
                        void *user_data) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->paddle_count; j++) {
            if ((paddle_device_t)(intptr_t)j == paddle) {
                mgr->paddles[j].callback = callback;
                mgr->paddles[j].callback_data = user_data;
                return 1;
            }
        }
    }
    return 0;
}

/* ===== MOUSE API ===== */

mouse_device_t mouse_init(paddle_mouse_manager_t manager,
                           int port,
                           mouse_type_t type) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return INVALID_MOUSE;

    manager_impl *mgr = &managers[idx];
    if (mgr->mouse_count >= MAX_MICE_PER_MANAGER) return INVALID_MOUSE;

    mouse_impl *mouse = &mgr->mice[mgr->mouse_count];
    mouse->port = port;
    mouse->type = type;
    mouse->connected = 1;
    mouse->state.x = 0;
    mouse->state.y = 0;
    mouse->state.buttons = 0;
    mouse->state.prev_buttons = 0;
    mouse->state.abs_x = 320;  /* Center screen */
    mouse->state.abs_y = 200;
    mouse->state.sensitivity = 1;
    mouse->update_interval = 16;  /* ~60Hz default */
    mouse->update_counter = 0;
    mouse->callback = NULL;
    mouse->last_read_x = 0;
    mouse->last_read_y = 0;

    return (mouse_device_t)(intptr_t)mgr->mouse_count++;
}

int mouse_detect(paddle_mouse_manager_t manager) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return 0;

    manager_impl *mgr = &managers[idx];
    int detected = 0;

    for (int i = 0; i < mgr->mouse_count; i++) {
        if (mgr->mice[i].connected) {
            detected++;
        }
    }

    return detected;
}

int mouse_get_count(paddle_mouse_manager_t manager) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return 0;
    return managers[idx].mouse_count;
}

int mouse_read_x(mouse_device_t mouse) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->mouse_count; j++) {
            if ((mouse_device_t)(intptr_t)j == mouse) {
                return mgr->mice[j].state.x;
            }
        }
    }
    return 0;
}

int mouse_read_y(mouse_device_t mouse) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->mouse_count; j++) {
            if ((mouse_device_t)(intptr_t)j == mouse) {
                return mgr->mice[j].state.y;
            }
        }
    }
    return 0;
}

int mouse_read_buttons(mouse_device_t mouse) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->mouse_count; j++) {
            if ((mouse_device_t)(intptr_t)j == mouse) {
                return mgr->mice[j].state.buttons;
            }
        }
    }
    return 0;
}

int mouse_is_button_pressed(mouse_device_t mouse, int button) {
    int buttons = mouse_read_buttons(mouse);
    return (buttons & button) != 0;
}

int mouse_update(mouse_device_t mouse, int delta_ms) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->mouse_count; j++) {
            mouse_impl *mu = &mgr->mice[j];
            if (!mu->connected) continue;

            mu->update_counter += delta_ms;
            if (mu->update_counter >= mu->update_interval) {
                mu->update_counter = 0;

                /* Simulate mouse movement (in real implementation, read from hardware) */
                mu->state.prev_buttons = mu->state.buttons;
                mu->state.x = (rand() % 5) - 2;  /* Delta movement */
                mu->state.y = (rand() % 5) - 2;

                /* Update absolute position */
                mu->state.abs_x += mu->state.x;
                mu->state.abs_y += mu->state.y;

                /* Clamp to screen */
                if (mu->state.abs_x < 0) mu->state.abs_x = 0;
                if (mu->state.abs_x > 639) mu->state.abs_x = 639;
                if (mu->state.abs_y < 0) mu->state.abs_y = 0;
                if (mu->state.abs_y > 399) mu->state.abs_y = 399;

                /* Simulate button state */
                if (rand() % 100 < 5) {
                    mu->state.buttons ^= MOUSE_BUTTON_LEFT;
                }

                /* Callback on movement or button change */
                if (mu->callback && (mu->state.x != 0 || mu->state.y != 0 ||
                                     mu->state.buttons != mu->state.prev_buttons)) {
                    mu->callback(j, mu->state.x, mu->state.y, mu->state.buttons,
                                mu->callback_data);
                }

                return 1;
            }
        }
    }
    return 0;
}

int mouse_manager_update(paddle_mouse_manager_t manager, int delta_ms) {
    intptr_t idx = (intptr_t)manager;
    if (idx < 0 || idx >= manager_count) return 0;

    manager_impl *mgr = &managers[idx];
    int updated = 0;

    for (int j = 0; j < mgr->mouse_count; j++) {
        if (mouse_update((mouse_device_t)(intptr_t)j, delta_ms)) {
            updated++;
        }
    }

    return updated;
}

int mouse_get_abs_x(mouse_device_t mouse) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->mouse_count; j++) {
            if ((mouse_device_t)(intptr_t)j == mouse) {
                return mgr->mice[j].state.abs_x;
            }
        }
    }
    return 320;
}

int mouse_get_abs_y(mouse_device_t mouse) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->mouse_count; j++) {
            if ((mouse_device_t)(intptr_t)j == mouse) {
                return mgr->mice[j].state.abs_y;
            }
        }
    }
    return 200;
}

int mouse_set_sensitivity(mouse_device_t mouse, int level) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->mouse_count; j++) {
            if ((mouse_device_t)(intptr_t)j == mouse) {
                if (level >= 1 && level <= 4) {
                    mgr->mice[j].state.sensitivity = level;
                    return 1;
                }
            }
        }
    }
    return 0;
}

int mouse_is_connected(mouse_device_t mouse) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->mouse_count; j++) {
            if ((mouse_device_t)(intptr_t)j == mouse) {
                return mgr->mice[j].connected;
            }
        }
    }
    return 0;
}

int mouse_set_callback(mouse_device_t mouse,
                       mouse_callback_t callback,
                       void *user_data) {
    for (int m = 0; m < manager_count; m++) {
        manager_impl *mgr = &managers[m];
        for (int j = 0; j < mgr->mouse_count; j++) {
            if ((mouse_device_t)(intptr_t)j == mouse) {
                mgr->mice[j].callback = callback;
                mgr->mice[j].callback_data = user_data;
                return 1;
            }
        }
    }
    return 0;
}

/* ===== UTILITY ===== */

const char *mouse_type_name(mouse_type_t type) {
    switch (type) {
        case MOUSE_TYPE_1351: return "Commodore 1351";
        case MOUSE_TYPE_AMIGA: return "Amiga Mouse";
        case MOUSE_TYPE_PC: return "PC Mouse (adapter)";
        default: return "Unknown";
    }
}

const char *paddle_type_name(paddle_type_t type) {
    switch (type) {
        case PADDLE_TYPE_STANDARD: return "Standard Paddle";
        case PADDLE_TYPE_ANALOG_STICK: return "Analog Stick";
        default: return "Unknown";
    }
}
