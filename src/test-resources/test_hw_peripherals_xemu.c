/* test_hw_peripherals_xemu.c — <hw_peripherals.h> and <input.h>'s joystick
 * port functions (#645)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] hw_get_revision (xemu: MEGA65 R3), has_sdram (not an R6: 0),
 *       vic_version 4
 *   [1] hw_get_cpu_speed (crt0: 40); hw_set_cpu_speed to 3, 2, 1 and back
 *       to 40: the speed read back and the bits ($D031 bit 6, $D054 bit 6,
 *       $D030 bit 0), the hot register bit as it was; 7: 0
 *   [2] hw_debug_print: the characters sent ('\n' two); off: 0; on again
 *   [3] hw_sleep(100): 5 frames (PAL) gone; hw_wake, battery -1,
 *       breakpoints 0, hw_get_cycles (xemu: $FFFFFFFF, no counter)
 *   [4] hw_get_sysinfo: the parts above, ram_size 384 + 8192 (xemu has
 *       attic RAM), the attic byte probed left as it was
 *   [5] input_joyport_detect: ports 1 and 2 empty (xemu: no paddles, no
 *       joystick pressed), port 3 unknown; input_set_mouse_mode (port 3,
 *       mode 9: 0); input_joyport_name
 *   [6] $AA sentinel
 */
#include <hw_peripherals.h>
#include <input.h>
#include <dma.h>
#include <string.h>
#include <time.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

int main(void) {
    mega65_sysinfo_t si;
    unsigned char hot = REG(0xD05D) & 0x80, a0 = 0x3C, a1;
    clock_t t0;
    int ok;
    OUT[6] = 0;

    ok = hw_get_revision() == MEGA65_R3 && !hw_has_sdram() && hw_get_vic_version() == 4;
    OUT[0] = ok;

    ok = hw_get_cpu_speed() == 40;
    ok = ok && hw_set_cpu_speed(3) && hw_get_cpu_speed() == 3 && (REG(0xD031) & 0x40) && !(REG(0xD054) & 0x40);
    ok = ok && hw_set_cpu_speed(2) && hw_get_cpu_speed() == 2 && !(REG(0xD031) & 0x40) && (REG(0xD030) & 0x01);
    ok = ok && hw_set_cpu_speed(1) && hw_get_cpu_speed() == 1 && !(REG(0xD030) & 0x01);
    ok = ok && hw_set_cpu_speed(40) && hw_get_cpu_speed() == 40 && (REG(0xD054) & 0x40) && !(REG(0xD030) & 0x01);
    ok = ok && hw_set_cpu_speed(7) == 0 && hw_get_cpu_speed() == 40 && (REG(0xD05D) & 0x80) == hot;
    OUT[1] = ok;

    ok = hw_debug_print("cc45 ok\n") == 9;
    hw_debug_enable(0);
    ok = ok && hw_debug_print("off") == 0;
    hw_debug_enable(1);
    ok = ok && hw_debug_print("") == 0 && hw_debug_print(0) == 0;
    OUT[2] = ok;

    t0 = clock();
    hw_sleep(100);
    ok = (unsigned long)(clock() - t0) >= 5 && (unsigned long)(clock() - t0) <= 7;
    hw_wake();
    ok = ok && hw_battery_percent() == -1 && hw_set_breakpoint(0x2000) == 0 && hw_clear_breakpoint(0x2000) == 0;
    ok = ok && hw_get_cycles() == 0xFFFFFFFFUL;    /* (xemu: the counter not emulated, $FF bytes) */
    OUT[3] = ok;

    dma_copy_far(0x8000000UL, (unsigned long)(unsigned int)&a0, 1);
    si = hw_get_sysinfo();
    dma_copy_far((unsigned long)(unsigned int)&a1, 0x8000000UL, 1);
    ok = si.revision == MEGA65_R3 && si.cpu_speed == 40 && !si.has_sdram && si.vic_version == 4 &&
         si.ram_size == 384 + 8192 && a1 == 0x3C;
    OUT[4] = ok;

    ok = input_joyport_detect(1) == JOYPORT_EMPTY && input_joyport_detect(2) == JOYPORT_EMPTY &&
         input_joyport_detect(3) == JOYPORT_UNKNOWN;
    ok = ok && input_set_mouse_mode(1, INPUT_MOUSE_AUTO) && input_set_mouse_mode(1, INPUT_MOUSE_1351) &&
         input_set_mouse_mode(3, INPUT_MOUSE_AMIGA) == 0 && input_set_mouse_mode(2, 9) == 0;
    ok = ok && strcmp(input_joyport_name(JOYPORT_MOUSE_1351), "1351 mouse") == 0 &&
         strcmp(input_joyport_name(JOYPORT_EMPTY), "empty") == 0 &&
         strcmp(input_joyport_name((joyport_device_t)9), "unknown") == 0;
    OUT[5] = ok;
    OUT[6] = 0xAA;
    return 0;
}
