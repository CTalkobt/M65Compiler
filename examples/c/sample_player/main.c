/* sample_player — a drum machine on the MEGA65's audio DMA (<audio_digi.h>)
 *
 * No sample files: at start the program synthesizes its sounds into memory
 * as 8-bit unsigned samples at 8 kHz:
 *   kick   a sine whose pitch falls from 160 to 45 Hz, fading out
 *   snare  a 190 Hz sine under a burst of noise
 *   hi-hat high-passed noise (the difference of two noise samples), short
 *   tone   one cycle of a sine wave, looped: its pitch is the playback rate
 * and plays them on the four audio DMA channels, each placed in the stereo
 * field with digi_set_volume: the kick in the middle, the snare a little
 * left, the hi-hat a little right, the tone on both sides.
 *
 * Keys:
 *   1 2 3      kick, snare, hi-hat
 *   SPACE      stop/start the 16-step pattern (it starts playing)
 *   P          next pattern
 *   + -        tempo                              T  tone on/off
 *   Q W        tone up/down a semitone
 *   RUN/STOP   quit
 *
 * Audio DMA reads the samples while the program runs, in the CPU's idle
 * cycles, so the CPU runs at full speed (40.5 MHz).
 *
 * Built with -DSP_TEST (make check), it synthesizes the samples, starts
 * each one and checks the sample data and the channel registers, writing
 * the results to $C0C0.
 */
/* Standard C signedness: the synthesis works with negative values */
#pragma cc45 signed_default

#include <audio_digi.h>
#include <conio.h>
#include <input.h>
#include <stdio.h>

#define REG(a) (*(volatile unsigned char *)(a))
#define RATE 8000U

#define KICK_LEN  2400
#define SNARE_LEN 1800
#define HAT_LEN   600
#define TONE_LEN  32

static unsigned char kick[KICK_LEN];
static unsigned char snare[SNARE_LEN];
static unsigned char hat[HAT_LEN];
static unsigned char tone[TONE_LEN];

/* ------------------------------------------------------------------------ */
/* Synthesis                                                                */

static signed char sine[256];   /* one cycle, -127..127 */
static unsigned int seed = 0x1234;

/* A parabola per half cycle: close enough to a sine for drums */
static void make_sine(void) {
    unsigned int i;
    for (i = 0; i < 128; i++) {
        signed char v = (signed char)((unsigned long)i * (128 - i) * 127UL / 4096UL);
        sine[i] = v;
        sine[i + 128] = (signed char)-v;
    }
}

static signed char noise(void) {
    seed = seed * 25173U + 13849U;
    return (signed char)(seed >> 8);
}

/* An amplitude 0-255 that falls from 255 to 0 over len samples */
static unsigned char fade(unsigned int i, unsigned int len) {
    return (unsigned char)(255U - (unsigned int)((unsigned long)i * 255UL / len));
}

/* A signed value scaled by an amplitude, as an unsigned 8-bit sample */
static unsigned char sample(int v, unsigned char amp) {
    int s = (int)((long)v * amp / 256);
    if (s > 127) s = 127;
    if (s < -128) s = -128;
    return (unsigned char)(s + 128);
}

static void synthesize(void) {
    unsigned int i, phase = 0;
    signed char prev = 0;
    make_sine();
    /* kick: the step (frequency * 65536 / RATE) falls from 160 to 45 Hz */
    for (i = 0; i < KICK_LEN; i++) {
        unsigned int hz = 45 + (unsigned int)((unsigned long)(KICK_LEN - i) * 115UL / KICK_LEN);
        phase += (unsigned int)((unsigned long)hz * 65536UL / RATE);
        kick[i] = sample(sine[phase >> 8], fade(i, KICK_LEN));
    }
    /* snare: a sine with noise over it, the noise fading faster */
    phase = 0;
    for (i = 0; i < SNARE_LEN; i++) {
        unsigned char a = fade(i, SNARE_LEN);
        phase += (unsigned int)(190UL * 65536UL / RATE);
        snare[i] = sample(sine[phase >> 8] / 2 + (int)noise() * a / 300, a);
    }
    /* hi-hat: high-passed noise, short */
    for (i = 0; i < HAT_LEN; i++) {
        signed char n = noise();
        hat[i] = sample(n - prev, fade(i, HAT_LEN) / 2);
        prev = n;
    }
    /* tone: one cycle of the sine */
    for (i = 0; i < TONE_LEN; i++) tone[i] = (unsigned char)(sine[i * 8] + 128);
}

/* ------------------------------------------------------------------------ */
/* Playback                                                                 */

enum { CH_KICK, CH_SNARE, CH_HAT, CH_TONE };

static void play_kick(void)  { digi_play(CH_KICK, kick, KICK_LEN, RATE, 255, 0);   digi_set_volume(CH_KICK, 220, 220); }
static void play_snare(void) { digi_play(CH_SNARE, snare, SNARE_LEN, RATE, 230, 0); digi_set_volume(CH_SNARE, 230, 120); }
static void play_hat(void)   { digi_play(CH_HAT, hat, HAT_LEN, RATE, 200, 0);       digi_set_volume(CH_HAT, 200, 100); }

/* tone: MIDI note to the playback rate (TONE_LEN samples a cycle) */
static unsigned char tone_note = 69, tone_on;
static const unsigned long note_hz256[12] = {  /* C4..B4 in 1/256 Hz */
    66976UL, 70959UL, 75178UL, 79649UL, 84385UL, 89402UL, 94719UL, 100351UL, 106318UL, 112640UL, 119338UL, 126434UL
};

static unsigned int tone_rate(void) {
    unsigned char oct = tone_note / 12;
    unsigned long hz256 = note_hz256[tone_note % 12];
    if (oct > 5) hz256 <<= oct - 5;
    else hz256 >>= 5 - oct;
    return (unsigned int)((hz256 * TONE_LEN + 128) >> 8);
}

static void tone_update(void) {
    if (!tone_on) { digi_stop(CH_TONE); return; }
    if (!digi_playing(CH_TONE)) {
        digi_play(CH_TONE, tone, TONE_LEN, tone_rate(), 90, DIGI_LOOP);
        digi_set_volume(CH_TONE, 90, 90);
    } else {
        digi_set_rate(CH_TONE, tone_rate());
    }
}

/* ------------------------------------------------------------------------ */
/* Patterns: 16 steps, a bit per drum (1 kick, 2 snare, 4 hat)              */

#define K 1
#define S 2
#define H 4
static const unsigned char patterns[4][16] = {
    { K|H, H, H, H, S|H, H, H, H, K|H, H, K|H, H, S|H, H, H, H },               /* rock */
    { K|H, 0, H, 0, S|H, 0, H, K, K|H, 0, H, 0, S|H, 0, H, H },                /* funk */
    { K, 0, H, 0, K, 0, H, 0, K, 0, H, 0, K, 0, H, H },                         /* four on the floor */
    { K|H, 0, H, S, H, K, H|S, 0, K|H, 0, H, S, H, K, S|H, S },                 /* breakbeat */
};
static const char *pattern_names[4] = { "rock      ", "funk      ", "four floor", "breakbeat " };

static unsigned char pattern, step, running, bpm = 110;

static void play_step(void) {
    unsigned char d = patterns[pattern][step];
    if (d & K) play_kick();
    if (d & S) play_snare();
    if (d & H) play_hat();
}

/* ------------------------------------------------------------------------ */
/* Screen                                                                   */

static void draw_static(void) {
    REG(0xD020) = 0;                /* border */
    textbackground(BLACK);
    textcolor(WHITE);
    clrscr();
    cputsxy(2, 1, "Sample Player - a drum machine on the MEGA65's audio DMA");
    textcolor(LIGHT_BLUE);
    cputsxy(2, 3, "Samples synthesized at start: 8-bit unsigned, 8000 Hz");
    cputsxy(2, 16, "1 2 3  kick snare hi-hat    SPACE  start/stop    P  pattern");
    cputsxy(2, 17, "+ -    tempo                T      tone          Q W  tone pitch");
    cputsxy(2, 18, "RUN/STOP  quit");
}

static const char *ch_names[4] = { "ch 0 kick   (mid)  ", "ch 1 snare  (left) ", "ch 2 hi-hat (right)", "ch 3 tone   (both) " };
static const char *note_letters[12] = { "C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B " };

static void draw_status(void) {
    unsigned char ch, i;
    for (ch = 0; ch < 4; ch++) {
        unsigned char on = digi_playing(ch);
        textcolor(on ? YELLOW : DARK_GRAY);
        revers(on);
        cputsxy(2, 5 + ch, ch_names[ch]);
        revers(0);
    }
    textcolor(WHITE);
    cprintfxy(2, 10, "pattern %s  tempo %3d bpm  %s", pattern_names[pattern], bpm, running ? "playing" : "stopped");
    cprintfxy(2, 11, "tone %s %s%d  %5u Hz rate", tone_on ? "on " : "off", note_letters[tone_note % 12],
              tone_note / 12 - 1, tone_rate());
    /* the pattern, the current step marked */
    for (i = 0; i < 16; i++) {
        unsigned char d = patterns[pattern][i];
        textcolor(running && i == step ? YELLOW : LIGHT_GRAY);
        cputch(2 + i * 3, 13, (d & K) ? 'K' : '.');
        cputch(3 + i * 3, 13, (d & S) ? 'S' : (d & H) ? 'h' : ' ');
    }
}

static void fast_cpu(void) {
    REG(0xD031) = REG(0xD031) | 0x40;   /* FAST */
    REG(0xD054) = REG(0xD054) | 0x40;   /* VFAST: 40.5 MHz */
}

#ifdef SP_TEST
#define OUT ((volatile unsigned char *)0xC0C0)
/* the samples: silence-centred, loud at the start and quiet at the end */
static unsigned char span(const unsigned char *s, unsigned int from, unsigned int n) {
    unsigned char lo = 255, hi = 0;
    unsigned int i;
    for (i = from; i < from + n; i++) {
        if (s[i] < lo) lo = s[i];
        if (s[i] > hi) hi = s[i];
    }
    return hi - lo;
}
int main(void) {
    fast_cpu();
    synthesize();
    OUT[0] = span(kick, 0, 400) > 150 && span(kick, KICK_LEN - 100, 100) < 30;
    OUT[1] = span(snare, 0, 400) > 100 && span(snare, SNARE_LEN - 100, 100) < 30 && span(hat, 0, 200) > 40;
    OUT[2] = tone[0] == 128 && tone[8] > 240 && tone[24] < 16;
    digi_init();
    play_kick();
    play_snare();
    play_hat();
    tone_on = 1;
    tone_update();
    OUT[3] = digi_playing(0) && digi_playing(1) && digi_playing(2) && digi_playing(3) &&
             REG(0xD750) == (0x80 | 0x40 | 0x20 | 2) && REG(0xD71C) == 220;
    tone_note = 69;
    OUT[4] = tone_rate() == 14080;   /* A4: 440 Hz x 32 samples */
    tone_on = 0;
    tone_update();
    OUT[5] = !digi_playing(3);
    digi_done();
    OUT[6] = 0xAA;
    return 0;
}
#else
int main(void) {
    unsigned char frames = 0;
    unsigned int acc = 0;
    fast_cpu();
    putchar(14);                    /* upper/lower case characters */
    draw_static();
    cputsxy(2, 5, "synthesizing...");
    synthesize();
    digi_init();
    draw_static();
    running = 1;                    /* the pattern plays from the start */
    step = 15;
    acc = 3000;
    for (;;) {
        /* one frame (50 Hz) */
        while (REG(0xD012) != 0xF0) ;
        while (REG(0xD012) == 0xF0) ;
        /* steps are sixteenth notes: bpm * 4 a minute, 3000 frames a minute */
        if (running) {
            acc += bpm * 4;
            if (acc >= 3000) {
                acc -= 3000;
                step = (step + 1) & 15;
                play_step();
            }
        }
        for (;;) {
            unsigned char c = input_key();  /* <input.h>: 0 when no key waits */
            if (!c) break;
            if (c == CH_STOP) goto quit;
            if (c == '1') play_kick();
            else if (c == '2') play_snare();
            else if (c == '3') play_hat();
            else if (c == ' ') { running = !running; step = 15; acc = 3000; }
            else if (c == 'p') pattern = (pattern + 1) & 3;
            else if (c == '+' && bpm < 200) bpm += 5;
            else if (c == '-' && bpm > 60) bpm -= 5;
            else if (c == 't') { tone_on = !tone_on; tone_update(); }
            else if (c == 'q' && tone_note < 95) { tone_note++; tone_update(); }
            else if (c == 'w' && tone_note > 36) { tone_note--; tone_update(); }
        }
        if (++frames >= 2) { frames = 0; draw_status(); }
    }
quit:
    digi_done();
    textcolor(WHITE);
    clrscr();
    return 0;
}
#endif
