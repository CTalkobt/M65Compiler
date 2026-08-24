/* lightpen.c — Light Pen Input Implementation for MEGA65
 *
 * Implements light pen sensor input with position tracking, button detection,
 * calibration, and event callbacks.
 */

#include <lightpen.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * HARDWARE DEFINITIONS
 * ========================================================================== */

/* VIC-IV light pen registers */
#define LP_X_REG            0xD010  /* Light pen X coordinate (read-only) */
#define LP_Y_REG            0xD011  /* Light pen Y coordinate (read-only) */
#define LP_BUTTON_REG       0xD012  /* Light pen button state (read-only) */
#define LP_STATUS_REG       0xD013  /* Light pen status/presence */

/* ============================================================================
 * STATIC STATE
 * ========================================================================== */

static int initialized = 0;
static int connected = 0;
static int polling_enabled = 1;

/* Current state */
static lightpen_pos_t current_pos = {0, 0, 0};
static lightpen_state_t button_state = LIGHTPEN_IDLE;
static lightpen_state_t prev_button_state = LIGHTPEN_IDLE;
static int frame_count = 0;

/* Calibration state */
static int calib_offset_x = 0;
static int calib_offset_y = 0;
static float calib_scale_x = 1.0f;
static float calib_scale_y = 1.0f;

/* Filtering */
static float filter_strength = 0.0f;
static int filtered_x = 0;
static int filtered_y = 0;

/* Callback */
static lightpen_callback_t event_callback = NULL;

/* ============================================================================
 * INTERNAL HELPERS
 * ========================================================================== */

/**
 * _read_x - Read X coordinate from hardware
 */
static int _read_x(void) {
    int raw_x = (int)(*(volatile unsigned char *)LP_X_REG);
    raw_x |= ((int)(*(volatile unsigned char *)LP_X_REG) << 8);  /* 9-bit value */

    /* Apply calibration */
    int calibrated = (int)(raw_x * calib_scale_x) + calib_offset_x;

    return calibrated;
}

/**
 * _read_y - Read Y coordinate from hardware
 */
static int _read_y(void) {
    int raw_y = (int)(*(volatile unsigned char *)LP_Y_REG);
    raw_y |= ((int)(*(volatile unsigned char *)LP_Y_REG) << 8);  /* 9-bit value */

    /* Apply calibration */
    int calibrated = (int)(raw_y * calib_scale_y) + calib_offset_y;

    return calibrated;
}

/**
 * _read_button - Read button state from hardware
 */
static int _read_button(void) {
    return (int)(*(volatile unsigned char *)LP_BUTTON_REG) & 1;
}

/**
 * _check_connected - Check if light pen is physically connected
 */
static int _check_connected(void) {
    int status = (int)(*(volatile unsigned char *)LP_STATUS_REG);
    return (status & 0x01) ? 1 : 0;
}

/**
 * _is_in_bounds - Check if position is within valid screen bounds
 */
static int _is_in_bounds(int x, int y) {
    /* Standard NTSC: 320×200, standard PAL: 320×224 */
    if (x < 0 || x >= 320) return 0;
    if (y < 0 || y >= 240) return 0;  /* Conservative bound */

    return 1;
}

/**
 * _apply_filter - Apply exponential moving average filter
 */
static void _apply_filter(int raw_x, int raw_y) {
    if (filter_strength <= 0.0f) {
        filtered_x = raw_x;
        filtered_y = raw_y;
        return;
    }

    /* EMA: new = old + strength * (raw - old) */
    filtered_x = (int)(filtered_x + filter_strength * (raw_x - filtered_x));
    filtered_y = (int)(filtered_y + filter_strength * (raw_y - filtered_y));
}

/* ============================================================================
 * INITIALIZATION & CONTROL
 * ========================================================================== */

int lightpen_init(void) {
    if (initialized) {
        return 1;
    }

    /* Check hardware availability */
    connected = _check_connected();

    /* Reset state */
    current_pos.x = 0;
    current_pos.y = 0;
    current_pos.valid = 0;
    button_state = LIGHTPEN_IDLE;
    prev_button_state = LIGHTPEN_IDLE;
    frame_count = 0;

    /* Reset calibration to defaults */
    calib_offset_x = 0;
    calib_offset_y = 0;
    calib_scale_x = 1.0f;
    calib_scale_y = 1.0f;

    /* Clear filter */
    filter_strength = 0.0f;
    filtered_x = 0;
    filtered_y = 0;

    initialized = 1;
    return 1;
}

void lightpen_shutdown(void) {
    initialized = 0;
    connected = 0;
    event_callback = NULL;
}

int lightpen_is_initialized(void) {
    return initialized;
}

int lightpen_is_connected(void) {
    if (!initialized) return 0;
    return _check_connected();
}

/* ============================================================================
 * POSITION READING
 * ========================================================================== */

int lightpen_get_x(void) {
    if (!initialized || !connected) return -1;
    if (!current_pos.valid) return -1;

    return current_pos.x;
}

int lightpen_get_y(void) {
    if (!initialized || !connected) return -1;
    if (!current_pos.valid) return -1;

    return current_pos.y;
}

int lightpen_get_position(lightpen_pos_t *pos) {
    if (!pos || !initialized || !connected) return 0;

    *pos = current_pos;
    return current_pos.valid;
}

int lightpen_is_valid(void) {
    return current_pos.valid;
}

/* ============================================================================
 * BUTTON INPUT
 * ========================================================================== */

lightpen_state_t lightpen_get_button(void) {
    if (!initialized || !connected) return LIGHTPEN_IDLE;

    return button_state;
}

int lightpen_is_pressed(void) {
    return (button_state == LIGHTPEN_PRESSED) ? 1 : 0;
}

int lightpen_was_pressed(void) {
    return (prev_button_state == LIGHTPEN_IDLE &&
            button_state == LIGHTPEN_PRESSED) ? 1 : 0;
}

int lightpen_was_released(void) {
    return (prev_button_state == LIGHTPEN_PRESSED &&
            button_state == LIGHTPEN_IDLE) ? 1 : 0;
}

/* ============================================================================
 * EVENT CALLBACKS
 * ========================================================================== */

int lightpen_set_callback(lightpen_callback_t callback) {
    event_callback = callback;
    return 1;
}

lightpen_callback_t lightpen_get_callback(void) {
    return event_callback;
}

/* ============================================================================
 * CALIBRATION
 * ========================================================================== */

int lightpen_calibrate(void) {
    if (!initialized || !connected) return 0;

    /* Simple two-point calibration */
    int x1, y1, x2, y2;
    int raw_x1, raw_y1, raw_x2, raw_y2;

    /* Point 1: Top-left corner (0, 0) */
    printf("Point at top-left corner (0, 0) and press button...\n");
    while (!lightpen_was_pressed()) {
        lightpen_update();
    }
    raw_x1 = _read_x();
    raw_y1 = _read_y();
    x1 = 0;
    y1 = 0;

    /* Wait for release */
    while (lightpen_is_pressed()) {
        lightpen_update();
    }

    /* Point 2: Bottom-right corner */
    printf("Point at bottom-right corner (319, 199) and press button...\n");
    while (!lightpen_was_pressed()) {
        lightpen_update();
    }
    raw_x2 = _read_x();
    raw_y2 = _read_y();
    x2 = 319;
    y2 = 199;

    /* Calculate scale and offset */
    if (raw_x2 != raw_x1) {
        calib_scale_x = (float)(x2 - x1) / (float)(raw_x2 - raw_x1);
        calib_offset_x = x1 - (int)(raw_x1 * calib_scale_x);
    }

    if (raw_y2 != raw_y1) {
        calib_scale_y = (float)(y2 - y1) / (float)(raw_y2 - raw_y1);
        calib_offset_y = y1 - (int)(raw_y1 * calib_scale_y);
    }

    printf("Calibration complete!\n");
    return 1;
}

void lightpen_reset_calibration(void) {
    calib_offset_x = 0;
    calib_offset_y = 0;
    calib_scale_x = 1.0f;
    calib_scale_y = 1.0f;
}

int lightpen_get_calibration(int *offset_x, int *offset_y,
                             float *scale_x, float *scale_y) {
    if (!offset_x || !offset_y || !scale_x || !scale_y) return 0;

    *offset_x = calib_offset_x;
    *offset_y = calib_offset_y;
    *scale_x = calib_scale_x;
    *scale_y = calib_scale_y;

    return 1;
}

/* ============================================================================
 * POLLING & UPDATES
 * ========================================================================== */

int lightpen_update(void) {
    if (!initialized) return 0;

    /* Check connection */
    connected = _check_connected();
    if (!connected) {
        current_pos.valid = 0;
        return 0;
    }

    /* Read raw coordinates */
    int raw_x = _read_x();
    int raw_y = _read_y();

    /* Apply filter if enabled */
    _apply_filter(raw_x, raw_y);

    /* Use filtered or raw coordinates */
    int x = (filter_strength > 0.0f) ? filtered_x : raw_x;
    int y = (filter_strength > 0.0f) ? filtered_y : raw_y;

    /* Check bounds */
    current_pos.valid = _is_in_bounds(x, y) ? 1 : 0;
    current_pos.x = x;
    current_pos.y = y;

    /* Read button state */
    prev_button_state = button_state;
    int button = _read_button();

    if (button) {
        if (button_state == LIGHTPEN_IDLE) {
            button_state = LIGHTPEN_PRESSED;
        } else {
            button_state = LIGHTPEN_PRESSED;
        }
    } else {
        if (button_state == LIGHTPEN_PRESSED) {
            button_state = LIGHTPEN_RELEASED;
        } else {
            button_state = LIGHTPEN_IDLE;
        }
    }

    /* Invoke callback if registered */
    if (event_callback) {
        lightpen_event_t event;
        event.pos = current_pos;
        event.state = button_state;
        event.frame = frame_count;
        event_callback(&event);
    }

    frame_count++;
    return connected;
}

int lightpen_is_polling(void) {
    return polling_enabled;
}

int lightpen_set_polling(int enabled) {
    polling_enabled = enabled ? 1 : 0;
    return 1;
}

/* ============================================================================
 * COORDINATE TRANSLATION
 * ========================================================================== */

int lightpen_get_char_x(void) {
    if (!current_pos.valid) return -1;

    /* 8 pixels per character in standard mode */
    return current_pos.x / 8;
}

int lightpen_get_char_y(void) {
    if (!current_pos.valid) return -1;

    /* 8 pixels per character in standard mode */
    return current_pos.y / 8;
}

int lightpen_get_char_pos(int *char_x, int *char_y) {
    if (!char_x || !char_y || !current_pos.valid) return 0;

    *char_x = current_pos.x / 8;
    *char_y = current_pos.y / 8;

    return 1;
}

/* ============================================================================
 * FILTERING & SMOOTHING
 * ========================================================================== */

int lightpen_set_filter(float strength) {
    if (strength < 0.0f || strength > 1.0f) return 0;

    filter_strength = strength;
    return 1;
}

float lightpen_get_filter(void) {
    return filter_strength;
}

void lightpen_clear_filter(void) {
    filter_strength = 0.0f;
}

/* ============================================================================
 * DEBUGGING & INSPECTION
 * ========================================================================== */

void lightpen_print_status(void) {
    printf("\n=== Light Pen Status ===\n\n");
    printf("Initialized:     %s\n", initialized ? "Yes" : "No");
    printf("Connected:       %s\n", connected ? "Yes" : "No");
    printf("Polling Mode:    %s\n", polling_enabled ? "Enabled" : "Disabled");
    printf("Position Valid:  %s\n", current_pos.valid ? "Yes" : "No");
    printf("X Coordinate:    %d\n", current_pos.x);
    printf("Y Coordinate:    %d\n", current_pos.y);
    printf("Button State:    %s\n",
           button_state == LIGHTPEN_IDLE ? "Idle" :
           button_state == LIGHTPEN_PRESSED ? "Pressed" : "Released");
    printf("Frames Polled:   %d\n", frame_count);
    printf("Filter Strength: %.2f\n", filter_strength);
    printf("\n");
}

void lightpen_print_event(const lightpen_event_t *event) {
    if (!event) return;

    printf("Event #%d:\n", event->frame);
    printf("  Position: (%d, %d) %s\n",
           event->pos.x, event->pos.y,
           event->pos.valid ? "[valid]" : "[invalid]");
    printf("  Button:   %s\n",
           event->state == LIGHTPEN_IDLE ? "Idle" :
           event->state == LIGHTPEN_PRESSED ? "Pressed" : "Released");
}
