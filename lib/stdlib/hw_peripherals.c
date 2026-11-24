/* hw_peripherals.c — CPU speed, identification, debug output, system
 * information (<hw_peripherals.h>, #645) */

#include <hw_peripherals.h>
#include <dma.h>
#include <time.h>

#define REG(a) (*(volatile unsigned char *)(a))

/* stdlib_asm/hw_hyper.s45: a character to the serial monitor (trap $03) */
__fastcall__ void __hw_debug_putc(unsigned char c);

static unsigned char debug_off;

mega65_revision_t hw_get_revision(void) {
    return mega65_get_revision();
}

int hw_get_cpu_speed(void) {
    if (REG(0xD031) & 0x40) return (REG(0xD054) & 0x40) ? 40 : 3;
    if (REG(0xD030) & 0x01) return 2;
    return 1;
}

int hw_set_cpu_speed(int mhz) {
    unsigned char hot, fast, vfast, c128;
    if (mhz != 1 && mhz != 2 && mhz != 3 && mhz != 40) return 0;
    fast = mhz >= 3 ? 0x40 : 0;
    vfast = mhz == 40 ? 0x40 : 0;
    c128 = mhz == 2 ? 0x01 : 0;
    /* (the VIC-IV's hot registers off meanwhile: a $D031 write with them
     * on sets the VIC-IV's modes again, as crt0_mega65.s45 does) */
    hot = REG(0xD05D) & 0x80;
    REG(0xD05D) &= 0x7F;
    REG(0xD054) = (unsigned char)((REG(0xD054) & 0xBF) | vfast);
    REG(0xD031) = (unsigned char)((REG(0xD031) & 0xBF) | fast);
    REG(0xD030) = (unsigned char)((REG(0xD030) & 0xFE) | c128);
    REG(0xD05D) |= hot;
    return 1;
}

int hw_has_sdram(void) {
    return mega65_get_revision() == MEGA65_R6;
}

int hw_get_vic_version(void) {
    return 4;
}

void hw_debug_enable(int enable) {
    debug_off = !enable;
}

int hw_debug_print(const char *message) {
    int n = 0;
    unsigned char c;
    if (debug_off || !message) return 0;
    while ((c = (unsigned char)*message++) != 0) {
        if (c == 0x0D) {
            __hw_debug_putc(0x0D);
            __hw_debug_putc(0x0A);
            n += 2;
            continue;
        }
        /* PETSCII to ASCII: $41-$5A lower case, $C1-$DA upper case */
        if (c >= 0x41 && c <= 0x5A) c |= 0x20;
        else if (c >= 0xC1 && c <= 0xDA) c &= 0x7F;
        __hw_debug_putc(c);
        n++;
    }
    return n;
}

unsigned long hw_get_cycles(void) {
    unsigned long a, b;
    /* (the low byte read last; the high bytes again if they moved on) */
    do {
        a = (unsigned long)REG(0xD7E7) << 24 | (unsigned long)REG(0xD7E6) << 16 | (unsigned long)REG(0xD7E5) << 8;
        a |= REG(0xD7E4);
        b = (unsigned long)REG(0xD7E7) << 24 | (unsigned long)REG(0xD7E6) << 16 | (unsigned long)REG(0xD7E5) << 8;
    } while ((a & 0xFFFFFF00UL) != b);
    return a;
}

int hw_set_breakpoint(unsigned int address) {
    (void)address;
    return 0;
}

int hw_clear_breakpoint(unsigned int address) {
    (void)address;
    return 0;
}

void hw_sleep(int duration_ms) {
    clock_t t0 = clock();
    unsigned long frames = ((unsigned long)(unsigned int)duration_ms * CLOCKS_PER_SEC + 999) / 1000;
    while ((unsigned long)(clock() - t0) < frames) ;
}

void hw_wake(void) {
}

int hw_battery_percent(void) {
    return -1;
}

/* attic RAM answering at $8000000: a byte changed, read back, restored */
static int attic_present(void) {
    unsigned char save, probe, back;
    int ok;
    dma_copy_far((unsigned long)(unsigned int)&save, 0x8000000UL, 1);
    probe = (unsigned char)(save ^ 0x5A);
    dma_copy_far(0x8000000UL, (unsigned long)(unsigned int)&probe, 1);
    dma_copy_far((unsigned long)(unsigned int)&back, 0x8000000UL, 1);
    ok = back == probe;
    dma_copy_far(0x8000000UL, (unsigned long)(unsigned int)&save, 1);
    return ok;
}

mega65_sysinfo_t hw_get_sysinfo(void) {
    mega65_sysinfo_t s;
    s.revision = hw_get_revision();
    s.cpu_speed = hw_get_cpu_speed();
    s.has_sdram = hw_has_sdram();
    s.vic_version = hw_get_vic_version();
    s.ram_size = 384 + (attic_present() ? 8192 : 0);
    return s;
}
