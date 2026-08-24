/* lightpen.h — Light Pen Input for MEGA65
 *
 * Provides light pen sensor support for MEGA65, including position detection,
 * button input, and event callbacks for interactive applications.
 *
 * The light pen detects the position of a light source (typically the CRT beam)
 * and allows pixel-accurate pointing on the screen. Can be used for:
 * - Painting/drawing applications
 * - Selection/menu interfaces
 * - Pointing devices for games
 * - Screen coordinate input
 *
 * Usage:
 *   #include <lightpen.h>
 *   lightpen_init();
 *   if (lightpen_is_connected()) {
 *       int x = lightpen_get_x();
 *       int y = lightpen_get_y();
 *   }
 */

#ifndef LIGHTPEN_H
#define LIGHTPEN_H

#include <stdint.h>

/* ============================================================================
 * LIGHT PEN STATE ENUMERATION
 * ========================================================================== */

typedef enum {
    LIGHTPEN_IDLE = 0,        /* No button pressed */
    LIGHTPEN_PRESSED = 1,     /* Button pressed */
    LIGHTPEN_RELEASED = 2,    /* Button just released */
} lightpen_state_t;

/* ============================================================================
 * LIGHT PEN POSITION STRUCTURE
 * ========================================================================== */

typedef struct {
    int x;      /* X coordinate (0-319 for standard, 0-639 for H640) */
    int y;      /* Y coordinate (0-199 for NTSC, 0-223 for PAL) */
    int valid;  /* 1 if position is valid, 0 if out of range */
} lightpen_pos_t;

/* ============================================================================
 * LIGHT PEN EVENT STRUCTURE
 * ========================================================================== */

typedef struct {
    lightpen_pos_t pos;     /* Position when event occurred */
    lightpen_state_t state; /* Button state */
    int frame;              /* Frame number when event occurred */
} lightpen_event_t;

/* ============================================================================
 * LIGHT PEN CALLBACK
 * ========================================================================== */

/**
 * lightpen_callback_t - Callback function type for light pen events
 *
 * Called whenever the light pen state changes (position or button).
 * Application should handle the event and return quickly.
 *
 * Parameters:
 *   event — Pointer to lightpen_event_t with current state
 */
typedef void (*lightpen_callback_t)(const lightpen_event_t *event);

/* ============================================================================
 * INITIALIZATION & CONTROL
 * ========================================================================== */

/**
 * lightpen_init - Initialize light pen subsystem
 *
 * Sets up light pen hardware and initializes internal state.
 * Must be called before any other lightpen functions.
 *
 * Returns:
 *   1 if initialized successfully, 0 if hardware unavailable
 */
int lightpen_init(void);

/**
 * lightpen_shutdown - Disable light pen and release resources
 *
 * Disables hardware polling and clears internal state.
 */
void lightpen_shutdown(void);

/**
 * lightpen_is_initialized - Check if light pen is initialized
 *
 * Returns:
 *   1 if initialized, 0 otherwise
 */
int lightpen_is_initialized(void);

/**
 * lightpen_is_connected - Check if light pen is physically connected
 *
 * Queries hardware to detect light pen presence.
 *
 * Returns:
 *   1 if light pen detected, 0 if not connected or unavailable
 */
int lightpen_is_connected(void);

/* ============================================================================
 * POSITION READING
 * ========================================================================== */

/**
 * lightpen_get_x - Get current X coordinate
 *
 * Returns the current X position of the light pen.
 *
 * Returns:
 *   X coordinate (0-319 standard, 0-639 H640 mode)
 *   -1 if light pen not connected or position invalid
 */
int lightpen_get_x(void);

/**
 * lightpen_get_y - Get current Y coordinate
 *
 * Returns the current Y position of the light pen.
 *
 * Returns:
 *   Y coordinate (0-199 NTSC, 0-223 PAL)
 *   -1 if light pen not connected or position invalid
 */
int lightpen_get_y(void);

/**
 * lightpen_get_position - Get both coordinates at once
 *
 * More efficient than separate get_x/get_y calls.
 *
 * Parameters:
 *   pos — Pointer to lightpen_pos_t to receive position
 *
 * Returns:
 *   1 if position valid, 0 if light pen not connected
 */
int lightpen_get_position(lightpen_pos_t *pos);

/**
 * lightpen_is_valid - Check if current position is valid
 *
 * Position is invalid if light pen is outside screen bounds
 * or hardware returns unreliable data.
 *
 * Returns:
 *   1 if position valid, 0 if out of bounds or invalid
 */
int lightpen_is_valid(void);

/* ============================================================================
 * BUTTON INPUT
 * ========================================================================== */

/**
 * lightpen_get_button - Get button state
 *
 * Returns the current state of the light pen button (fire button).
 *
 * Returns:
 *   LIGHTPEN_IDLE — Button not pressed
 *   LIGHTPEN_PRESSED — Button currently held down
 *   LIGHTPEN_RELEASED — Button just released this frame
 */
lightpen_state_t lightpen_get_button(void);

/**
 * lightpen_is_pressed - Check if button is currently pressed
 *
 * Returns:
 *   1 if button is held down, 0 if released
 */
int lightpen_is_pressed(void);

/**
 * lightpen_was_pressed - Check if button was pressed this frame
 *
 * Detects button press edge (transition from released to pressed).
 *
 * Returns:
 *   1 if button pressed this frame, 0 otherwise
 */
int lightpen_was_pressed(void);

/**
 * lightpen_was_released - Check if button was released this frame
 *
 * Detects button release edge (transition from pressed to released).
 *
 * Returns:
 *   1 if button released this frame, 0 otherwise
 */
int lightpen_was_released(void);

/* ============================================================================
 * EVENT CALLBACKS
 * ========================================================================== */

/**
 * lightpen_set_callback - Register event callback function
 *
 * Sets a callback to be invoked on light pen state changes.
 * Callback is called from interrupt or polling context.
 *
 * Parameters:
 *   callback — Function to call on light pen events
 *              Pass NULL to disable callbacks
 *
 * Returns:
 *   1 if callback registered, 0 on error
 */
int lightpen_set_callback(lightpen_callback_t callback);

/**
 * lightpen_get_callback - Get current callback function
 *
 * Returns:
 *   Pointer to current callback, or NULL if none set
 */
lightpen_callback_t lightpen_get_callback(void);

/* ============================================================================
 * CALIBRATION
 * ========================================================================== */

/**
 * lightpen_calibrate - Perform light pen calibration
 *
 * Guides user through calibration sequence to improve accuracy.
 * Displays on-screen prompts for user to point at specific positions.
 *
 * Returns:
 *   1 if calibration completed, 0 if cancelled or failed
 */
int lightpen_calibrate(void);

/**
 * lightpen_reset_calibration - Reset to factory calibration
 *
 * Clears any user calibration and returns to default settings.
 */
void lightpen_reset_calibration(void);

/**
 * lightpen_get_calibration - Read current calibration parameters
 *
 * Returns calibration offset and scaling factors.
 *
 * Parameters:
 *   offset_x — Pointer to receive X offset
 *   offset_y — Pointer to receive Y offset
 *   scale_x — Pointer to receive X scale factor
 *   scale_y — Pointer to receive Y scale factor
 *
 * Returns:
 *   1 if calibration data available, 0 otherwise
 */
int lightpen_get_calibration(int *offset_x, int *offset_y,
                             float *scale_x, float *scale_y);

/* ============================================================================
 * POLLING & UPDATES
 * ========================================================================== */

/**
 * lightpen_update - Poll light pen hardware (call once per frame)
 *
 * Updates internal state by reading hardware registers.
 * Should be called once per frame for consistent input handling.
 *
 * Returns:
 *   1 if light pen connected and data valid, 0 otherwise
 */
int lightpen_update(void);

/**
 * lightpen_is_polling - Check if polling is active
 *
 * Returns:
 *   1 if lightpen_update() should be called, 0 if using interrupt mode
 */
int lightpen_is_polling(void);

/**
 * lightpen_set_polling - Enable or disable polling mode
 *
 * When polling is enabled, application must call lightpen_update()
 * regularly. When disabled, interrupt handler updates state.
 *
 * Parameters:
 *   enabled — 1 to enable polling, 0 to use interrupt mode
 *
 * Returns:
 *   1 if mode set successfully, 0 on error
 */
int lightpen_set_polling(int enabled);

/* ============================================================================
 * COORDINATE TRANSLATION
 * ========================================================================== */

/**
 * lightpen_get_char_x - Get character column from light pen X
 *
 * Converts pixel X coordinate to character column (0-39 or 0-79).
 * Useful for text-based selection.
 *
 * Returns:
 *   Character column (0-39 standard, 0-79 H640 mode)
 *   -1 if position invalid
 */
int lightpen_get_char_x(void);

/**
 * lightpen_get_char_y - Get character row from light pen Y
 *
 * Converts pixel Y coordinate to character row (0-24 or 0-27).
 * Useful for text-based selection.
 *
 * Returns:
 *   Character row (0-24 standard)
 *   -1 if position invalid
 */
int lightpen_get_char_y(void);

/**
 * lightpen_get_char_pos - Get character position (x, y)
 *
 * More efficient than separate get_char_x/get_char_y calls.
 *
 * Parameters:
 *   char_x — Pointer to receive column
 *   char_y — Pointer to receive row
 *
 * Returns:
 *   1 if position valid, 0 otherwise
 */
int lightpen_get_char_pos(int *char_x, int *char_y);

/* ============================================================================
 * FILTERING & SMOOTHING
 * ========================================================================== */

/**
 * lightpen_set_filter - Enable position filtering/smoothing
 *
 * Reduces jitter from light pen noise using exponential moving average.
 *
 * Parameters:
 *   strength — Filter strength (0.0=no filter, 1.0=max smoothing)
 *              Typical: 0.3-0.6 for balanced response
 *
 * Returns:
 *   1 if filter set, 0 on error
 */
int lightpen_set_filter(float strength);

/**
 * lightpen_get_filter - Get current filter strength
 *
 * Returns:
 *   Current filter strength (0.0-1.0)
 */
float lightpen_get_filter(void);

/**
 * lightpen_clear_filter - Disable position filtering
 */
void lightpen_clear_filter(void);

/* ============================================================================
 * DEBUGGING & INSPECTION
 * ========================================================================== */

/**
 * lightpen_print_status - Print light pen status to console (debug)
 *
 * Displays current position, button state, and connection status.
 */
void lightpen_print_status(void);

/**
 * lightpen_print_event - Print event details to console (debug)
 *
 * Parameters:
 *   event — Event to print
 */
void lightpen_print_event(const lightpen_event_t *event);

#endif
