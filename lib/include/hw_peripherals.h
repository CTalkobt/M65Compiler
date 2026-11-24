/* hw_peripherals.h — the MEGA65's CPU speed, identification, debug output
 * and system information (#645)
 *
 * (The joystick ports: what is on them and the mouse modes are <input.h>'s
 * input_joyport_detect / input_set_mouse_mode / input_joyport_name.)
 *
 *   hw_get_revision     mega65_get_revision() (<mega65_version.h>: $D629)
 *   hw_get_cpu_speed    1, 2 (C128 FAST, $D030 bit 0), 3 (C65 FAST, 3.5 MHz,
 *                       $D031 bit 6) or 40 (and VFAST, $D054 bit 6)
 *   hw_set_cpu_speed    1, 2, 3 or 40 (the bits as above, the VIC-IV's hot
 *                       registers off while they change): 1; else 0
 *   hw_has_sdram        1 on a MEGA65 R6 (its SDRAM)
 *   hw_get_vic_version  4 (the VIC-IV: crt0 makes its registers visible)
 *   hw_debug_enable     hw_debug_print on (1) or off (0; at first: on)
 *   hw_debug_print      the text (PETSCII to ASCII, '\n' as CR LF) to the
 *                       serial monitor's UART, a hypervisor trap ($D643) a
 *                       character; the characters sent (0 while off)
 *   hw_get_cycles       the CPU's cycle counter ($D7E4-$D7E7, MATH:ITERCNT;
 *                       xemu does not emulate it: $FFFFFFFF)
 *   hw_set_breakpoint,  0: the MEGA65's breakpoints are the serial
 *   hw_clear_breakpoint monitor's, set from the computer it is connected to
 *                       (dbg45, m65 -b)
 *   hw_sleep            waits (the frame counter, clock()); the MEGA65 has
 *                       no low-power mode, so hw_wake does nothing
 *   hw_battery_percent  -1: no battery to measure (a MEGAphone's is not
 *                       read here)
 *   hw_get_sysinfo      all of it, and ram_size: KB of chip RAM (384) and
 *                       attic RAM (8192, when the HyperRAM answers at
 *                       $8000000)
 *
 * Library: stdlib/hw_peripherals.c (hw_debug_print's trap:
 * stdlib_asm/hw_hyper.s45). Test: test_hw_peripherals_xemu.c.
 */

#ifndef HW_PERIPHERALS_H
#define HW_PERIPHERALS_H

#include <mega65_version.h>

/* ===== Hardware Identification ===== */

/* The model (mega65_get_revision) */
mega65_revision_t hw_get_revision(void);

/* Get CPU speed (1, 2, 3 = 3.5 MHz, 40) */
int hw_get_cpu_speed(void);

/* Set CPU speed (1, 2, 3 or 40): 1, or 0 for another */
int hw_set_cpu_speed(int mhz);

/* Check for SDRAM (R6 feature) */
int hw_has_sdram(void);

/* Get VIC version (4) */
int hw_get_vic_version(void);

/* ===== Debug Features ===== */

/* hw_debug_print on / off */
void hw_debug_enable(int enable);

/* Print to the serial monitor's UART: the characters sent */
int hw_debug_print(const char *message);

/* The CPU's cycle counter */
unsigned long hw_get_cycles(void);

/* Hardware breakpoints: 0 (the serial monitor's only) */
int hw_set_breakpoint(unsigned int address);
int hw_clear_breakpoint(unsigned int address);

/* ===== Power Management ===== */

/* Wait duration_ms (no low-power mode) */
void hw_sleep(int duration_ms);

/* Nothing (no low-power mode to wake from) */
void hw_wake(void);

/* Get battery status: -1, none */
int hw_battery_percent(void);

/* ===== System Info ===== */
typedef struct {
    mega65_revision_t revision;
    int cpu_speed;
    int has_sdram;
    int vic_version;
    int ram_size;               /* KB: chip RAM and attic RAM */
} mega65_sysinfo_t;

/* Get complete system info */
mega65_sysinfo_t hw_get_sysinfo(void);

#endif
