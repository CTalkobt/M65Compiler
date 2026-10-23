/* audio_sid.c — the MEGA65's four SID chips (see <audio_sid.h>)
 *
 * Each register write goes to the chip and to a shadow copy (the chips'
 * registers can't be read back), and with the mirror on to SIDs 3-4 too:
 * stdlib_asm/sid_core.s45, which the tune player (<audio_tune.h>) shares.
 * Voice v: SID v / 3, voice v % 3.
 */

#include <audio_sid.h>

__fastcall__ void __sid_write_reg(unsigned char index, unsigned char value);
extern unsigned char __sid_shadow[SID_CHIPS * 0x20];
extern unsigned char __sid_mirror;     /* $80: on */

#define shadow(chip, reg) __sid_shadow[(chip) * 0x20 + (reg)]

void sid_write_chip(unsigned char chip, unsigned char reg, unsigned char val) {
    if (chip >= SID_CHIPS || reg >= 0x20) return;
    __sid_write_reg((unsigned char)(chip * 0x20 + reg), val);
}

void sid_write(unsigned char v, unsigned char reg, unsigned char val) {
    unsigned char chip = v / 3;
    if (v >= SID_VOICES || reg > 6) return;
    sid_write_chip(chip, (unsigned char)((v - chip * 3) * 7 + reg), val);
}

unsigned char sid_get_reg(unsigned char chip, unsigned char reg) {
    if (chip >= SID_CHIPS || reg >= 0x20) return 0;
    return shadow(chip, reg);
}

/* A voice register's shadow value */
static unsigned char vreg(unsigned char v, unsigned char reg) {
    unsigned char chip = v / 3;
    return shadow(chip, (v - chip * 3) * 7 + reg);
}

void sid_set_mirror(unsigned char on) {
    __sid_mirror = on ? 0x80 : 0;
}

void sid_init(void) {
    unsigned char c, r;
    __sid_mirror = 0;
    for (c = 0; c < SID_CHIPS; c++) {
        for (r = 0; r < 0x18; r++) __sid_write_reg((unsigned char)(c * 0x20 + r), 0);
        __sid_write_reg((unsigned char)(c * 0x20 + 0x18), 15);
    }
}

/* ============================================================================
 * Frequencies
 * ========================================================================== */

/* $D06F bit 7: NTSC */
static unsigned char ntsc(void) {
    return (*(volatile unsigned char *)0xD06F & 0x80) != 0;
}

unsigned int sid_hz_to_reg(unsigned int hz) {
    /* hz * 2^24 / clock: PAL 17.0284, NTSC 16.4044 per Hz */
    unsigned long r = (unsigned long)hz * (ntsc() ? 16404UL : 17028UL) / 1000UL;
    return r > 65535UL ? 65535U : (unsigned int)r;
}

/* The register values of MIDI notes 84-95 (C6-B6) at the PAL and NTSC
 * clocks: 440 Hz * 2^((note - 69) / 12) * 2^24 / clock (985248 Hz PAL,
 * 1022727 Hz NTSC); the other octaves shift them (#322: 16 bits, no
 * multiply) */
static const unsigned int octave7[24] = {
    17820, 18880, 20003, 21192, 22452, 23787, 25202, 26700, 28288, 29970, 31752, 33640,   /* PAL */
    17167, 18188, 19270, 20415, 21629, 22916, 24278, 25722, 27251, 28872, 30589, 32407    /* NTSC */
};

unsigned int sid_note_to_reg(unsigned char note) {
    unsigned char octave = 0;
    unsigned int r;
    while (note >= 12) { note -= 12; octave++; }
    if (ntsc()) note += 12;
    r = octave7[note];
    /* past the register's range (from B7 at the PAL clock): clamped */
    if (octave > 8 || (octave == 8 && (r & 0x8000))) return 65535U;
    if (octave == 8) return r << 1;
    while (octave < 7) { r >>= 1; octave++; }
    return r;
}

void sid_set_freq_reg(unsigned char v, unsigned int reg) {
    sid_write(v, SID_REG_FREQ_LO, (unsigned char)reg);
    sid_write(v, SID_REG_FREQ_HI, (unsigned char)(reg >> 8));
}

void sid_set_freq(unsigned char v, unsigned int hz) {
    sid_set_freq_reg(v, sid_hz_to_reg(hz));
}

void sid_set_note(unsigned char v, unsigned char note) {
    sid_set_freq_reg(v, sid_note_to_reg(note));
}

/* ============================================================================
 * Sound shape
 * ========================================================================== */

void sid_set_waveform(unsigned char v, unsigned char wave) {
    if (v >= SID_VOICES) return;
    sid_write(v, SID_REG_CTRL, (unsigned char)((wave & 0xFE) | (vreg(v, SID_REG_CTRL) & SID_GATE)));
}

void sid_set_pulse(unsigned char v, unsigned int width) {
    if (width > 4095) width = 4095;
    sid_write(v, SID_REG_PW_LO, (unsigned char)width);
    sid_write(v, SID_REG_PW_HI, (unsigned char)(width >> 8));
}

void sid_set_adsr(unsigned char v, unsigned char attack, unsigned char decay,
                  unsigned char sustain, unsigned char release) {
    sid_write(v, SID_REG_AD, (unsigned char)(((attack & 15) << 4) | (decay & 15)));
    sid_write(v, SID_REG_SR, (unsigned char)(((sustain & 15) << 4) | (release & 15)));
}

void sid_set_instrument(unsigned char v, const sid_instrument_t *inst) {
    if (!inst) return;
    sid_set_pulse(v, inst->pw);
    sid_write(v, SID_REG_AD, inst->ad);
    sid_write(v, SID_REG_SR, inst->sr);
    sid_set_waveform(v, inst->wave);
}

/* ============================================================================
 * Notes
 * ========================================================================== */

void sid_gate(unsigned char v, unsigned char on) {
    unsigned char c;
    if (v >= SID_VOICES) return;
    c = vreg(v, SID_REG_CTRL);
    sid_write(v, SID_REG_CTRL, on ? (unsigned char)(c | SID_GATE) : (unsigned char)(c & ~SID_GATE));
}

void sid_note_on(unsigned char v, unsigned char note) {
    sid_set_note(v, note);
    sid_gate(v, 0);     /* the envelope starts again from the attack */
    sid_gate(v, 1);
}

void sid_note_off(unsigned char v) {
    sid_gate(v, 0);
}

void sid_play(unsigned char v, const sid_instrument_t *inst, unsigned char note) {
    sid_gate(v, 0);
    sid_set_instrument(v, inst);
    sid_set_note(v, note);
    sid_gate(v, 1);
}

void sid_all_off(void) {
    unsigned char v;
    for (v = 0; v < SID_VOICES; v++) sid_gate(v, 0);
}

/* ============================================================================
 * Volume and filter
 * ========================================================================== */

void sid_set_volume(unsigned char volume) {
    unsigned char c;
    for (c = 0; c < SID_CHIPS; c++)
        __sid_write_reg((unsigned char)(c * 0x20 + 0x18), (unsigned char)((shadow(c, 0x18) & 0xF0) | (volume & 15)));
}

void sid_set_filter(unsigned char chip, unsigned int cutoff, unsigned char resonance,
                    unsigned char mode, unsigned char voices) {
    if (chip >= SID_CHIPS) return;
    if (cutoff > 2047) cutoff = 2047;
    sid_write_chip(chip, 0x15, (unsigned char)(cutoff & 7));
    sid_write_chip(chip, 0x16, (unsigned char)(cutoff >> 3));
    sid_write_chip(chip, 0x17, (unsigned char)(((resonance & 15) << 4) | (voices & 7)));
    sid_write_chip(chip, 0x18, (unsigned char)((mode & 0xF0) | (shadow(chip, 0x18) & 15)));
}
