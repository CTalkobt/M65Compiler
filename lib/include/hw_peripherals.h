/* hw_peripherals.h — Phase 89: Advanced Peripherals
 *
 * Control for advanced MEGA65 peripherals: joystick port detection,
 * mouse/paddle modes, hardware identification, and debug features.
 */

#ifndef HW_PERIPHERALS_H
#define HW_PERIPHERALS_H

/* Joystick port detection */
typedef enum {
    JOYPORT_EMPTY = 0,
    JOYPORT_JOYSTICK = 1,
    JOYPORT_MOUSE_1351 = 2,
    JOYPORT_MOUSE_AMIGA = 3,
    JOYPORT_PADDLE = 4,
    JOYPORT_UNKNOWN = 5,
} joyport_device_t;

/* Hardware identification */
typedef enum {
    MEGA65_R3 = 3,
    MEGA65_R4 = 4,
    MEGA65_R5 = 5,
    MEGA65_R6 = 6,
} mega65_revision_t;

/* ===== Joystick Port Detection ===== */

/* Detect device on joystick port 1 or 2 */
joyport_device_t hw_detect_joyport(int port);

/* Enable mouse mode (1351/Amiga) on port */
int hw_set_mouse_mode(int port, int mode);

/* Get port detection status */
const char *hw_joyport_name(joyport_device_t device);

/* ===== Hardware Identification ===== */

/* Get MEGA65 revision (R3, R4, R5, R6) */
mega65_revision_t hw_get_revision(void);

/* Get CPU speed (1 = 1MHz, 40 = 40MHz) */
int hw_get_cpu_speed(void);

/* Set CPU speed (1 or 40) */
int hw_set_cpu_speed(int mhz);

/* Check for SDRAM (R6 feature) */
int hw_has_sdram(void);

/* Get VIC-IV version */
int hw_get_vic_version(void);

/* ===== Debug Features ===== */

/* Enable/disable debug mode (affects display) */
void hw_debug_enable(int enable);

/* Print to debug output (if available) */
int hw_debug_print(const char *message);

/* Get CPU cycle count for profiling */
unsigned int hw_get_cycles(void);

/* Hardware breakpoint support */
int hw_set_breakpoint(unsigned int address);
int hw_clear_breakpoint(unsigned int address);

/* ===== Power Management ===== */

/* Put system in low-power mode */
void hw_sleep(int duration_ms);

/* Wake from sleep */
void hw_wake(void);

/* Get battery status (if applicable) */
int hw_battery_percent(void);

/* ===== System Info ===== */

typedef struct {
    mega65_revision_t revision;
    int cpu_speed;
    int has_sdram;
    int vic_version;
    int ram_size;
} mega65_sysinfo_t;

/* Get complete system info */
mega65_sysinfo_t hw_get_sysinfo(void);

#endif
