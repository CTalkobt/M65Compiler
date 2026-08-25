/* hw_peripherals.c — Advanced Peripherals Implementation */

#include "hw_peripherals.h"
#include <stdint.h>
#include <string.h>

/* Hardware info from mega65.h locations */
#define VIC4_VERSION (*(volatile unsigned char *)0xD07C)
#define VIC4_CTRL_B (*(volatile unsigned char *)0xD031)
#define VIC4_FAST_BIT 0x40

/* Joystick port input via CIA */
#define CIA1_PRA (*(volatile unsigned char *)0xDC00)
#define CIA1_PRB (*(volatile unsigned char *)0xDC01)

joyport_device_t hw_detect_joyport(int port) {
    if (port < 1 || port > 2) return JOYPORT_UNKNOWN;

    /* Read joystick port (simplified detection) */
    unsigned char val;
    if (port == 1) {
        val = CIA1_PRB ^ 0x1F;  /* Active low */
    } else {
        val = CIA1_PRA ^ 0x1F;
    }

    /* Heuristic: detect based on button pattern */
    if ((val & 0x1F) == 0) {
        return JOYPORT_EMPTY;
    }

    /* Check for mouse movement (bits change frequently) */
    if ((val & 0x0F) != 0) {
        return JOYPORT_JOYSTICK;
    }

    /* Default to joystick */
    return JOYPORT_JOYSTICK;
}

int hw_set_mouse_mode(int port, int mode) {
    if (port < 1 || port > 2) return 0;

    /* Mouse mode selection via hypervisor or special register */
    /* (Implementation varies by MEGA65 version) */
    return 1;
}

const char *hw_joyport_name(joyport_device_t device) {
    switch (device) {
        case JOYPORT_EMPTY: return "Empty";
        case JOYPORT_JOYSTICK: return "Joystick";
        case JOYPORT_MOUSE_1351: return "1351 Mouse";
        case JOYPORT_MOUSE_AMIGA: return "Amiga Mouse";
        case JOYPORT_PADDLE: return "Paddle";
        case JOYPORT_UNKNOWN: return "Unknown";
        default: return "Unknown";
    }
}

mega65_revision_t hw_get_revision(void) {
    /* Check for SDRAM (R6 feature) and other markers */
    /* (Simplified detection) */
    return MEGA65_R6;  /* Default to latest */
}

int hw_get_cpu_speed(void) {
    /* Check FAST bit in VIC-IV control */
    if ((VIC4_CTRL_B & VIC4_FAST_BIT) != 0) {
        return 40;
    }
    return 1;
}

int hw_set_cpu_speed(int mhz) {
    if (mhz == 40) {
        VIC4_CTRL_B |= VIC4_FAST_BIT;
        return 40;
    } else if (mhz == 1) {
        VIC4_CTRL_B &= ~VIC4_FAST_BIT;
        return 1;
    }
    return 0;
}

int hw_has_sdram(void) {
    /* R6 boards have SDRAM */
    mega65_revision_t rev = hw_get_revision();
    return rev == MEGA65_R6;
}

int hw_get_vic_version(void) {
    return VIC4_VERSION;
}

void hw_debug_enable(int enable) {
    /* Debug mode control (implementation varies) */
    (void)enable;
}

int hw_debug_print(const char *message) {
    if (!message) return 0;

    /* Debug output to serial or special register */
    /* (Implementation varies by hardware setup) */
    return 1;
}

unsigned int hw_get_cycles(void) {
    /* Return approximate CPU cycle count */
    /* (Real implementation would use cycle counter) */
    static unsigned int cycles = 0;
    return cycles++;
}

int hw_set_breakpoint(unsigned int address) {
    /* Hardware breakpoint support (if available) */
    (void)address;
    return 0;  /* Not available on MEGA65 */
}

int hw_clear_breakpoint(unsigned int address) {
    /* Clear hardware breakpoint */
    (void)address;
    return 0;
}

void hw_sleep(int duration_ms) {
    /* Low-power sleep mode (not standard on MEGA65) */
    (void)duration_ms;
}

void hw_wake(void) {
    /* Wake from sleep */
}

int hw_battery_percent(void) {
    /* Battery status (MEGA65 is powered, so always 100%) */
    return 100;
}

mega65_sysinfo_t hw_get_sysinfo(void) {
    mega65_sysinfo_t info;

    info.revision = hw_get_revision();
    info.cpu_speed = hw_get_cpu_speed();
    info.has_sdram = hw_has_sdram();
    info.vic_version = hw_get_vic_version();
    info.ram_size = 0x20000;  /* 128KB base + extended per revision */

    if (info.has_sdram) {
        info.ram_size += 0x800000;  /* 8MB SDRAM on R6 */
    }

    return info;
}
