/* audio_digi.c — sample playback through the MEGA65's audio DMA (see
 * <audio_digi.h>)
 *
 * Channel ch's registers at $D720 + ch * $10: 0 control (bit 7 enable,
 * 6 loop, 5 unsigned, 3 stopped, 1-0 sample bits), 1-3 loop start address,
 * 4-6 rate, 7-8 end address (low 16 bits, one past the last sample),
 * 9 volume, A-C current address. $D711 bit 7 enables audio DMA; $D71C-$D71F
 * are the volumes on the other side.
 */

#include <audio_digi.h>

#define REG(a) (*(volatile unsigned char *)(a))
#define CH(ch, r) REG(0xD720 + (ch) * 0x10 + (r))

void digi_init(void) {
    unsigned char ch;
    for (ch = 0; ch < DIGI_CHANNELS; ch++) {
        CH(ch, 0) = 0;
        REG(0xD71C + ch) = 0;
    }
    REG(0xD711) = 0x80;
}

void digi_done(void) {
    unsigned char ch;
    for (ch = 0; ch < DIGI_CHANNELS; ch++) CH(ch, 0) = 0;
    REG(0xD711) = 0;
}

unsigned long digi_rate_to_reg(unsigned int hz) {
    /* hz * 2^24 / 40500000 = hz * 0.414252: hz * 27149 / 65536 */
    return ((unsigned long)hz * 27149UL) >> 16;
}

void digi_play_far(unsigned char ch, unsigned long addr, unsigned int len, unsigned int rate,
                   unsigned char volume, unsigned char flags) {
    unsigned long r = digi_rate_to_reg(rate);
    unsigned int end = (unsigned int)addr + len;
    unsigned char ctrl;
    if (ch >= DIGI_CHANNELS || len == 0) return;
    CH(ch, 0) = 0;                              /* stop while it is set up */
    CH(ch, 1) = (unsigned char)addr;            /* loop start */
    CH(ch, 2) = (unsigned char)(addr >> 8);
    CH(ch, 3) = (unsigned char)(addr >> 16);
    CH(ch, 0xA) = (unsigned char)addr;          /* the first sample */
    CH(ch, 0xB) = (unsigned char)(addr >> 8);
    CH(ch, 0xC) = (unsigned char)(addr >> 16);
    CH(ch, 4) = (unsigned char)r;
    CH(ch, 5) = (unsigned char)(r >> 8);
    CH(ch, 6) = (unsigned char)(r >> 16);
    CH(ch, 7) = (unsigned char)end;
    CH(ch, 8) = (unsigned char)(end >> 8);
    CH(ch, 9) = volume;
    REG(0xD71C + ch) = 0;
    if (flags & DIGI_16BIT) ctrl = 3;
    else if (flags & DIGI_NYBBLE_HI) ctrl = 1;
    else if (flags & DIGI_NYBBLE_LO) ctrl = 0;
    else ctrl = 2;
    if (!(flags & DIGI_SIGNED)) ctrl |= 0x20;
    if (flags & DIGI_LOOP) ctrl |= 0x40;
    CH(ch, 0) = (unsigned char)(ctrl | 0x80);
}

void digi_play(unsigned char ch, const void *sample, unsigned int len, unsigned int rate,
               unsigned char volume, unsigned char flags) {
    digi_play_far(ch, (unsigned long)(unsigned int)sample, len, rate, volume, flags);
}

void digi_stop(unsigned char ch) {
    if (ch < DIGI_CHANNELS) CH(ch, 0) = 0;
}

unsigned char digi_playing(unsigned char ch) {
    unsigned char c;
    if (ch >= DIGI_CHANNELS) return 0;
    c = CH(ch, 0);
    return (c & 0x80) && !(c & 0x08);
}

void digi_set_volume(unsigned char ch, unsigned char volume, unsigned char other_side) {
    if (ch >= DIGI_CHANNELS) return;
    CH(ch, 9) = volume;
    REG(0xD71C + ch) = other_side;
}

void digi_set_rate(unsigned char ch, unsigned int rate) {
    unsigned long r = digi_rate_to_reg(rate);
    if (ch >= DIGI_CHANNELS) return;
    CH(ch, 4) = (unsigned char)r;
    CH(ch, 5) = (unsigned char)(r >> 8);
    CH(ch, 6) = (unsigned char)(r >> 16);
}

unsigned long digi_position(unsigned char ch) {
    if (ch >= DIGI_CHANNELS) return 0;
    return CH(ch, 0xA) | ((unsigned int)CH(ch, 0xB) << 8) | ((unsigned long)CH(ch, 0xC) << 16);
}
