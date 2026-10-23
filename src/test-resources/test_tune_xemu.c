/* test_tune_xemu.c — the tune player (<audio_tune.h>)
 *
 * The player's SID writes go to a RAM buffer here: this file defines
 * __sid_regs (weak in the library, $D400), so every register it writes can
 * be read back. Tunes are hand-made blobs (tune45 makes them otherwise).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a tune frame by frame (a tick a frame): notes (gate, waveform,
 *       frequency at the PAL clock, pulse width, envelope), a rest, the
 *       staccato gap, the end (tune_playing 0, tune_ticks)
 *   [1] the same tune from far memory (bank 4, tune_play_far)
 *   [2] a loop ($FE) and a tie ($82); tune_stop silences
 *   [3] tune_voices (SID 2's voices) and the stereo mirror (SID 4 too)
 *   [4] tempo: 2.5 frames a tick; tune_tempo(0x200) twice as fast
 *   [5] vibrato (its delay, the triangle), pulse-width sweep, pitch slide
 *   [6] arpeggio ($83), transpose, detune
 *   [7] 01 (and the program links with tune_update called from an
 *       __interrupt handler: the player has no static allocation)
 *   [8] $AA sentinel
 */
#include <audio_tune.h>
#include <audio_sid.h>
#include <dma.h>

#define OUT ((volatile unsigned char *)0xC0C0)

unsigned char __sid_regs[128];   /* (the player writes here, not to $D400) */

#define R(v, r) __sid_regs[(v) / 3 * 0x20 + (v) % 3 * 7 + (r)]
#define FREQ(v) (R(v, 0) | (R(v, 1) << 8))
#define PW(v) (R(v, 2) | (R(v, 3) << 8))

/* instrument: wave, AD, SR, pw lo, pw hi, vib, gap, pwm, vdelay, slide */
#define INST(w, ad, sr, pw, vib, gap, pwm, vdel, sl) w, ad, sr, (pw) & 0xFF, (pw) >> 8, vib, gap, pwm, vdel, sl

/* two voices, a tick a frame, one instrument (pulse, staccato gap 1) */
static const unsigned char t1[] = {
    0x00, 2, 0x00, 0x01, 1, 0, 12, 0,   /* flags, voices, fpt 1.0, 1 inst, at 12 */
    22, 0, 30, 0,                       /* voice streams at 22, 30 */
    INST(0x41, 0x09, 0xA6, 0x0600, 0, 1, 0, 0, 0),
    0x81, 0, 60, 2, 64, 2, 0xFF, 0,     /* v0: inst 0, C4 2, E4 2, end */
    0x80, 1, 48, 3, 0xFF,               /* v1: rest 1, C3 3, end */
};

/* one voice that loops: C4 1, tied 2 more, rest 1 */
static const unsigned char t2[] = {
    0x01, 1, 0x00, 0x01, 1, 0, 10, 0,
    20, 0,
    INST(0x21, 0x00, 0xF0, 0x0800, 0, 0, 0, 0, 0),
    60, 1, 0x82, 2, 0x80, 1, 0xFE,
};

/* 2.5 frames a tick */
static const unsigned char t3[] = {
    0x00, 1, 0x80, 0x02, 1, 0, 10, 0,
    20, 0,
    INST(0x11, 0x00, 0xF0, 0x0800, 0, 0, 0, 0, 0),
    60, 4, 62, 4, 0xFF,
};

/* effects: vibrato depth 2 after 3 frames; a pulse sweep of 32 a frame;
 * a slide of -2 (x16) a frame; an arpeggio */
static const unsigned char t4[] = {
    0x00, 3, 0x00, 0x01, 3, 0, 14, 0,
    44, 0, 48, 0, 54, 0,
    INST(0x41, 0x00, 0xF0, 0x0800, 2, 0, 0, 3, 0),
    INST(0x41, 0x00, 0xF0, 0x0DF0, 0, 0, 32, 0, 0),
    INST(0x11, 0x00, 0xF0, 0x0800, 0, 0, 0, 0, 0xFE),
    69, 40, 0xFF, 0,                    /* v0: A4 (inst 0: vibrato) */
    0x81, 1, 60, 40, 0xFF, 0,           /* v1: inst 1, C4 (sweep) */
    0x81, 2, 0x83, 4, 7, 48, 40, 0xFF,  /* v2: inst 2, arpeggio 0/4/7 over C3, slide */
};

/* never installed: ln45 rejects static-allocation functions reachable from
 * an __interrupt handler, so this links only while the player has none */
__interrupt void music_irq(void) {
    tune_update();
}

/* PAL register values: C4 4455, E4 5613, C3 2227, D4 5000, A4 7492, G4 6674 */
static int frames(unsigned char n) {
    while (n--) tune_update();
    return 1;
}

int main(void) {
    int ok;
    unsigned int f, f2;
    unsigned char i, up, dn;

    sid_init();

    /* [0] */
    tune_play(t1);
    ok = tune_playing() && frames(1) &&
         R(0, 4) == 0x41 && FREQ(0) == 4455 && PW(0) == 0x0600 && R(0, 5) == 0x09 && R(0, 6) == 0xA6 &&
         R(1, 4) == 0x40;
    ok = ok && frames(1) && R(0, 4) == 0x40 && R(1, 4) == 0x41 && FREQ(1) == 2227;   /* gap; C3 */
    ok = ok && frames(1) && R(0, 4) == 0x41 && FREQ(0) == 5613 && R(1, 4) == 0x41;
    ok = ok && frames(1) && R(0, 4) == 0x40 && R(1, 4) == 0x40;                        /* gaps */
    ok = ok && frames(1) && !tune_playing() && tune_ticks() == 5;
    OUT[0] = ok;

    /* [1] */
    for (i = 0; i < 128; i++) __sid_regs[i] = 0;
    dma_copy_far(0x40000UL, (unsigned long)(unsigned int)t1, sizeof t1);
    tune_play_far(0x40000UL);
    ok = frames(1) && R(0, 4) == 0x41 && FREQ(0) == 4455 && frames(1) && FREQ(1) == 2227 &&
         frames(1) && FREQ(0) == 5613 && frames(2) && !tune_playing();
    OUT[1] = ok;

    /* [2] */
    tune_play(t2);
    ok = frames(1) && R(0, 4) == 0x21 && frames(2) && R(0, 4) == 0x21 &&   /* tied: held */
         frames(1) && R(0, 4) == 0x20 &&                                   /* the rest */
         frames(1) && R(0, 4) == 0x21 && tune_playing() && tune_ticks() == 5;   /* round again */
    tune_stop();
    ok = ok && R(0, 4) == 0x20 && !tune_playing();
    OUT[2] = ok;

    /* [3] */
    for (i = 0; i < 128; i++) __sid_regs[i] = 0;
    tune_voices(3);
    sid_set_mirror(1);
    tune_play(t1);
    ok = frames(1) && R(3, 4) == 0x41 && FREQ(3) == 4455 && !(R(0, 4) & 1) && FREQ(0) == 0 &&
         R(9, 4) == 0x41 && FREQ(9) == 4455;                               /* mirrored on SID 4 */
    tune_stop();
    sid_set_mirror(0);
    tune_voices(0);
    OUT[3] = ok;

    /* [4] 2.5 frames a tick: notes at frames 1 and 11 (4 ticks = 10 frames) */
    tune_play(t3);
    ok = frames(1) && FREQ(0) == 4455 && frames(9) && FREQ(0) == 4455 && frames(1) && FREQ(0) == 5000;
    tune_tempo(0x200);
    tune_play(t3);
    ok = ok && frames(1) && FREQ(0) == 4455 && frames(4) && FREQ(0) == 4455 && frames(1) && FREQ(0) == 5000;
    tune_tempo(0x100);
    OUT[4] = ok;

    /* [5] */
    tune_play(t4);
    frames(1);
    ok = FREQ(0) == 7492;                     /* (frame 1: within the delay) */
    frames(2);
    ok = ok && FREQ(0) == 7492;               /* t = 3: still the delay */
    frames(1);                                /* t = 4: triangle 0 */
    ok = ok && FREQ(0) == 7492;
    frames(1);                                /* t = 5: triangle -1: - (29 * 2) / 2 */
    ok = ok && FREQ(0) == 7492 - 29;
    frames(1);                                /* t = 6: triangle -2: - 29 * 2 */
    ok = ok && FREQ(0) == 7492 - 58;
    frames(3);                                /* t = 9: triangle 1 */
    ok = ok && FREQ(0) == 7492 + 29;
    /* the sweep: from $0DF0 up 32 a frame, turning at $0E00 */
    up = dn = 0;
    f = PW(1);
    for (i = 0; i < 10; i++) {
        frames(1);
        f2 = PW(1);
        if (f2 > f) up++;
        if (f2 < f) dn++;
        f = f2;
    }
    ok = ok && dn >= 9 && f < 0x0DF0;
    /* the slide: C3 + arpeggio... */
    OUT[5] = ok;

    /* [6] arpeggio: C3, E3, G3 a frame each (slide: 2 x 16 less a frame) */
    tune_play(t4);
    frames(1);
    f = FREQ(2);                               /* C3 (2227), arpeggio index 1 on frame 1: E3 */
    ok = f == 2806 || f == 2227 || f == 3337;
    frames(1);
    f2 = FREQ(2);
    ok = ok && f2 != f;
    /* transpose and detune */
    tune_transpose(0, 12);
    tune_play(t1);
    frames(1);
    ok = ok && FREQ(0) == 8910;               /* C5 */
    tune_transpose(0, 0);
    tune_detune(0, 1);
    tune_play(t1);
    frames(1);
    ok = ok && FREQ(0) == 4455 + (4455 >> 6);
    tune_detune(0, 0);
    tune_stop();
    OUT[6] = ok;

    OUT[7] = (unsigned int)music_irq != 0;
    OUT[8] = 0xAA;
    return 0;
}
