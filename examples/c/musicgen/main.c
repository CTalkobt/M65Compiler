/*
 * musicgen — a random music generator for the MEGA65
 *
 * Composes and plays endless songs in seven styles on the SIDs, using
 * common rules of melody writing and harmony:
 *
 *   - harmony: each style has its own chords and a Markov table of how
 *     likely one chord follows another (I-V-vi-IV pop, mixolydian rock,
 *     ii-V-I jazz, functional classical...). The chords of a section are
 *     drawn from it with the cadences fixed: a half cadence on the dominant
 *     at bar 4, a full cadence (dominant, tonic) at bars 7-8. A chord on
 *     each side of a fixed one is drawn from both transitions (the
 *     constrained Markov chain); the blues uses the 12-bar form with a
 *     random quick change and turnaround.
 *   - form: AABA (8-bar sections, the song repeats twice, then a new one);
 *     a section is a parallel period: a 4-bar antecedent ending on the
 *     half cadence and a consequent that repeats its first two bars
 *     (the motif) and ends on the tonic. The blues uses call and response:
 *     a 2-bar phrase, repeated over the IV chord, then an answer.
 *   - melody: rhythm from the style's one-beat cells, a motif reused;
 *     notes on the strong beats (1 and 3) are chord tones, the others
 *     mostly steps in the scale (passing and neighbour notes); a leap is
 *     followed by a step back the other way; each phrase rises to a high
 *     point and falls back (an arch contour); the last note is the tonic,
 *     approached by step.
 *   - accompaniment: chords voice-led (each chord's inversion is the one
 *     closest to the previous chord), per-style rhythms (strummed piano,
 *     power chords, jazz comping, sustained strings or pads, a chiptune
 *     arpeggio), bass lines (root/fifth, boogie, walking bass with
 *     chromatic approach notes, Alberti), drum patterns (kick, snare and
 *     hi-hat on one SID voice) with a fill at the end of a section and
 *     swing (long-short eighths) for blues and jazz.
 *
 * Sound: SID 1 voice 1 lead, voice 2 bass, voice 3 drums; SID 2 the
 * chords. SID 3 and 4 get the same writes, so the music plays on both
 * stereo channels. 14 instruments (ADSR, waveform, pulse width with
 * modulation, delayed vibrato).
 *
 * Keys (the hardware typing queue at $D610):
 *   1-7 style    + - tempo    N new song    SPACE pause
 *   L C B change the lead, chord and bass instrument
 *   D drums on/off            RUN/STOP quit
 *
 * The screen is our own buffer: the VIC-IV screen and colour RAM pointers
 * are moved to it, so BASIC's screen comes back at the end. Interrupts are
 * off while it runs; the music is timed by the raster (50 or 60 frames a
 * second, tempo from a fractional frame counter).
 *
 * Build with -DMG_TEST for the generator test: songs of every style are
 * composed, logged at $A000 and the program exits (check_songs.py
 * checks the melodies against the chords and scales).
 */
#include <mega65.h>
#include <dma.h>
#include "notes.h"

#define REG(a) (*(volatile unsigned char *)(a))
#define ASCIIKEY REG(0xD610)

/* ------------------------------------------------------------------------ */
/* Chords                                                                   */

enum { MAJ, MIN, DIM, DOM7, MAJ7, MIN7, M7B5, POW };
/* intervals above the root; 255 ends a 3-note chord */
static const unsigned char qual_iv[8][4] = {
    {0, 4, 7, 255}, {0, 3, 7, 255}, {0, 3, 6, 255}, {0, 4, 7, 10},
    {0, 4, 7, 11},  {0, 3, 7, 10},  {0, 3, 6, 10},  {0, 7, 12, 255},
};
/* (the screen shows capitals: "min", not "m", which would read as major) */
static const char *qual_name[8] = { "", "min", "dim", "7", "maj7", "min7", "min7b5", "5" };
static const char *sharp_name[12] = { "c", "c#", "d", "d#", "e", "f", "f#", "g", "g#", "a", "a#", "b" };
static const char *flat_name[12] = { "c", "db", "d", "eb", "e", "f", "gb", "g", "ab", "a", "bb", "b" };

/* A chord of a style: its root (semitones above the key), quality and the
 * scale notes it changes for the melody: alt toggles them (the V chord of
 * a minor key raises the 7th: Bb off, B on) */
struct chord {
    unsigned char root, qual;
    unsigned int alt;
};

/* Scales as masks of semitones above the key */
#define SC_MAJOR  0x0AB5   /* 0 2 4 5 7 9 11 */
#define SC_MINOR  0x05AD   /* 0 2 3 5 7 8 10 */
#define SC_MIXO   0x06B5   /* 0 2 4 5 7 9 10 */
#define SC_BLUES  0x04E9   /* 0 3 5 6 7 10 */

/* ------------------------------------------------------------------------ */
/* Instruments                                                              */

struct inst {
    const char *name;
    unsigned char wave, ad, sr;
    unsigned int pw;
    signed char pwm;            /* pulse width change a frame */
    unsigned char vib;          /* vibrato depth (0 none) */
    unsigned char vdelay;       /* frames before the vibrato starts */
};

enum { I_PIANO, I_ORGAN, I_FLUTE, I_BRASS, I_STRINGS, I_HARPSI, I_SYNTH, I_SQUARE,
       I_HARMONICA, I_VIBES, I_UPRIGHT, I_SYNBASS, I_PLUCK, I_PAD, N_INST };

static const struct inst insts[N_INST] = {
    { "piano",        SID_PULSE,    0x0A, 0x0A, 0x0500,  0, 0,  0 },
    { "organ",        SID_PULSE,    0x00, 0xF3, 0x0800,  0, 0,  0 },
    { "flute",        SID_TRIANGLE, 0x60, 0xB7, 0x0800,  0, 2, 12 },
    { "brass",        SID_SAWTOOTH, 0x52, 0xA5, 0x0800,  0, 1, 20 },
    { "strings",      SID_SAWTOOTH, 0x96, 0xC9, 0x0800,  0, 1, 20 },
    { "harpsichord",  SID_PULSE,    0x09, 0x07, 0x0300,  0, 0,  0 },
    { "synth lead",   SID_PULSE,    0x23, 0xB6, 0x0400, 24, 2, 15 },
    { "square",       SID_PULSE,    0x02, 0xA4, 0x0800,  0, 0,  0 },
    { "harmonica",    SID_PULSE,    0x43, 0xB5, 0x0280,  0, 3,  8 },
    { "vibes",        SID_TRIANGLE, 0x0A, 0x0A, 0x0800,  0, 1,  2 },
    { "upright bass", SID_TRIANGLE, 0x0A, 0x58, 0x0800,  0, 0,  0 },
    { "synth bass",   SID_SAWTOOTH, 0x08, 0x98, 0x0800,  0, 0,  0 },
    { "pluck bass",   SID_PULSE,    0x08, 0x28, 0x0600,  0, 0,  0 },
    { "pad",          SID_PULSE,    0xB8, 0xCA, 0x0800,  6, 0,  0 },
};

/* the choices of the L, C and B keys */
static const unsigned char lead_insts[10] = { I_SYNTH, I_FLUTE, I_BRASS, I_HARMONICA, I_VIBES,
                                              I_SQUARE, I_PIANO, I_ORGAN, I_STRINGS, I_HARPSI };
static const unsigned char chord_insts[8] = { I_PIANO, I_ORGAN, I_STRINGS, I_PAD, I_HARPSI,
                                              I_BRASS, I_SQUARE, I_VIBES };
static const unsigned char bass_insts[4] = { I_SYNBASS, I_UPRIGHT, I_PLUCK, I_ORGAN };

/* ------------------------------------------------------------------------ */
/* Styles                                                                   */

enum { ACC_HIT, ACC_POWER, ACC_ARP };   /* how the chord voices play */
enum { FORM_AABA, FORM_BLUES };

struct style {
    const char *name;
    const char *mode;                 /* the scale's name */
    unsigned char rel;                /* semitones up to the major key with the same notes */
    unsigned char bpm, swing, form;
    unsigned int scale;
    unsigned char nch;                /* chords (up to 8) */
    const struct chord *ch;
    const unsigned char *tr;          /* nch x nch: weight of chord j after chord i */
    unsigned char half, dom;          /* the chords at bar 4 and bar 7 */
    const char *kick, *snare, *hat;   /* 16 steps: x hit, o open hi-hat */
    const char *bass;                 /* 16 steps, see bass_step() */
    unsigned char acc;
    const char *comp1, *comp2;        /* chord rhythms: x strike, - release */
    const unsigned char *cells;       /* one-beat rhythms: bit n = a note on 16th n */
    unsigned char ncells;
    unsigned char lead, chords, bassi, drums;
    unsigned char lo, hi;             /* the melody's range */
};

/* Pop: I V vi IV ii iii */
static const struct chord pop_ch[6] = {
    {0, MAJ, 0}, {7, MAJ, 0}, {9, MIN, 0}, {5, MAJ, 0}, {2, MIN, 0}, {4, MIN, 0} };
static const unsigned char pop_tr[36] = {
    0, 4, 3, 5, 2, 1,
    5, 0, 4, 2, 0, 1,
    1, 2, 0, 5, 3, 1,
    4, 5, 2, 0, 1, 1,
    1, 6, 1, 2, 0, 0,
    0, 1, 4, 4, 0, 0 };
static const unsigned char pop_cells[10] = { 1, 5, 5, 9, 4, 0, 1, 5, 13, 4 };

/* Rock (mixolydian): I V bVII IV vi bIII */
static const struct chord rock_ch[6] = {
    {0, MAJ, 0}, {7, MAJ, 0}, {10, MAJ, 0}, {5, MAJ, 0}, {9, MIN, 0}, {3, MAJ, 0x0018} };
static const unsigned char rock_tr[36] = {
    0, 2, 4, 5, 1, 2,
    4, 0, 1, 3, 1, 0,
    3, 0, 0, 5, 0, 1,
    5, 2, 2, 0, 1, 0,
    1, 1, 1, 4, 0, 1,
    2, 0, 3, 3, 0, 0 };
static const unsigned char rock_cells[8] = { 5, 5, 1, 13, 5, 0, 4, 1 };

/* Blues: I7 IV7 V7 in the 12-bar form (the table is not used) */
static const struct chord blues_ch[3] = { {0, DOM7, 0}, {5, DOM7, 0}, {7, DOM7, 0} };
static const unsigned char blues_tr[9] = { 0, 1, 1, 1, 0, 1, 1, 1, 0 };
static const unsigned char blues_cells[8] = { 1, 5, 5, 4, 0, 1, 5, 4 };

/* Jazz: Imaj7 V7 ii7 vi7 IVmaj7 iii7 VI7 II7 (secondary dominants) */
static const struct chord jazz_ch[8] = {
    {0, MAJ7, 0}, {7, DOM7, 0}, {2, MIN7, 0}, {9, MIN7, 0}, {5, MAJ7, 0}, {4, MIN7, 0},
    {9, DOM7, 0x0003}, {2, DOM7, 0x0060} };
static const unsigned char jazz_tr[64] = {
    0, 1, 4, 3, 2, 2, 3, 1,
    8, 0, 1, 1, 0, 0, 0, 0,
    1, 9, 0, 0, 0, 0, 0, 0,
    0, 1, 6, 0, 1, 0, 0, 2,
    2, 1, 2, 1, 0, 3, 0, 1,
    0, 0, 1, 5, 1, 0, 3, 0,
    0, 0, 7, 0, 0, 0, 0, 1,
    0, 6, 1, 0, 0, 0, 0, 0 };
static const unsigned char jazz_cells[8] = { 5, 5, 4, 1, 5, 0, 4, 5 };

/* Classical: I V IV ii vi V7 vii-dim */
static const struct chord class_ch[7] = {
    {0, MAJ, 0}, {7, MAJ, 0}, {5, MAJ, 0}, {2, MIN, 0}, {9, MIN, 0}, {7, DOM7, 0}, {11, DIM, 0} };
static const unsigned char class_tr[49] = {
    0, 3, 4, 3, 3, 2, 1,
    6, 0, 0, 0, 2, 2, 0,
    2, 4, 0, 2, 0, 3, 1,
    0, 4, 0, 0, 0, 5, 1,
    0, 1, 3, 5, 0, 0, 0,
    9, 0, 0, 0, 1, 0, 0,
    8, 0, 0, 0, 0, 0, 0 };
static const unsigned char class_cells[8] = { 1, 5, 5, 15, 1, 5, 9, 13 };

/* Ambient (aeolian): i v bVImaj7 bVII iv7 bIIImaj7 */
static const struct chord amb_ch[6] = {
    {0, MIN, 0}, {7, MIN, 0}, {8, MAJ7, 0}, {10, MAJ, 0}, {5, MIN7, 0}, {3, MAJ7, 0} };
static const unsigned char amb_tr[36] = {
    0, 1, 4, 2, 3, 3,
    3, 0, 3, 1, 1, 0,
    2, 1, 0, 4, 2, 2,
    4, 1, 2, 0, 1, 3,
    3, 1, 2, 3, 0, 1,
    2, 0, 3, 2, 3, 0 };
static const unsigned char amb_cells[8] = { 1, 1, 0, 1, 0, 4, 1, 0 };

/* Chiptune (minor, a major V): i V bVI bVII iv bIII */
static const struct chord chip_ch[6] = {
    {0, MIN, 0}, {7, MAJ, 0x0C00}, {8, MAJ, 0}, {10, MAJ, 0}, {5, MIN, 0}, {3, MAJ, 0} };
static const unsigned char chip_tr[36] = {
    0, 2, 4, 3, 3, 1,
    7, 0, 1, 0, 1, 0,
    1, 1, 0, 6, 1, 1,
    5, 2, 1, 0, 0, 2,
    2, 4, 1, 2, 0, 0,
    1, 0, 3, 2, 3, 0 };
static const unsigned char chip_cells[8] = { 5, 15, 5, 13, 11, 1, 5, 15 };

/*
 * Bass steps: R root, r root an octave up, 5 fifth, 3 the chord's third,
 * 6 sixth, 7 minor seventh, W walking (a beat's note), L Alberti (the
 * chord's notes low-high-middle-high), - release, . nothing
 */
#define N_STYLES 7
static const struct style styles[N_STYLES] = {
    { "pop", "major", 0, 112, 0, FORM_AABA, SC_MAJOR, 6, pop_ch, pop_tr, 1, 1,
      "x.......x.x.....", "....x.......x...", "x.x.x.x.x.x.x.x.",
      "R..R..R.r..R.5..", ACC_HIT, "x..x..x...x.x...", "x.....x.x...x...",
      pop_cells, 10, I_SYNTH, I_PIANO, I_SYNBASS, 1, 62, 81 },
    { "rock", "mixolydian", 5, 128, 0, FORM_AABA, SC_MIXO, 6, rock_ch, rock_tr, 1, 1,
      "x.....x.x.x.....", "....x.......x...", "x.x.x.x.x.x.x.x.",
      "R.R.R.R.R.R.5.r.", ACC_POWER, "x.-.x.-.x.-.x.x-", "x.-.x.-.x.-.x.-.",
      rock_cells, 8, I_BRASS, I_SQUARE, I_PLUCK, 1, 60, 79 },
    { "blues", "blues", 5, 92, 1, FORM_BLUES, SC_BLUES, 3, blues_ch, blues_tr, 2, 2,
      "x.......x.......", "....x.......x...", "x.x.x.x.x.x.x.x.",
      "R.3.5.6.7.6.5.3.", ACC_HIT, "....x.-.....x.-.", "..x.....x.-.....",
      blues_cells, 8, I_HARMONICA, I_PIANO, I_UPRIGHT, 1, 60, 79 },
    { "jazz", "major", 0, 132, 1, FORM_AABA, SC_MAJOR, 8, jazz_ch, jazz_tr, 1, 1,
      "x...............", "..............x.", "x...x.x.x...x.x.",
      "W...W...W...W...", ACC_HIT, "..x.-.....x.-...", "x.-.....x.-..x-.",
      jazz_cells, 8, I_VIBES, I_PIANO, I_UPRIGHT, 1, 62, 81 },
    { "classical", "major", 0, 100, 0, FORM_AABA, SC_MAJOR, 7, class_ch, class_tr, 1, 5,
      "................", "................", "................",
      "LLLLLLLLLLLLLLLL", ACC_HIT, "x...............", "x.......x.......",
      class_cells, 8, I_FLUTE, I_STRINGS, I_HARPSI, 0, 62, 84 },
    { "ambient", "minor", 3, 70, 0, FORM_AABA, SC_MINOR, 6, amb_ch, amb_tr, 1, 3,
      "x...............", "................", "..x...x...x...x.",
      "R...............", ACC_HIT, "x...............", "x...............",
      amb_cells, 8, I_VIBES, I_PAD, I_SYNBASS, 0, 60, 79 },
    { "chiptune", "minor", 3, 150, 0, FORM_AABA, SC_MINOR, 6, chip_ch, chip_tr, 1, 1,
      "x...x...x...x...", "....x.......x..x", "..x...x...x...x.",
      "R.r.R.r.R.r.R.r.", ACC_ARP, "x...x...x...x...", "x.......x.x.x...",
      chip_cells, 8, I_SQUARE, I_SQUARE, I_PLUCK, 1, 64, 86 },
};

/* ------------------------------------------------------------------------ */
/* Random numbers: 16-bit xorshift                                           */

static unsigned int rs = 0xACE1;
static unsigned int rnd16(void) {
    rs ^= rs << 7;
    rs ^= rs >> 9;
    rs ^= rs << 8;
    return rs;
}
/* 0 .. n-1 */
static unsigned char rnd_n(unsigned char n) {
    return (unsigned char)(((unsigned long)(rnd16() & 0xFF) * n) >> 8);
}

/* ------------------------------------------------------------------------ */
/* The song                                                                 */

#define MAXBARS 12
#define STEPS (MAXBARS * 16)
#define NONE 255
#define REST 1

static const struct style *sty;
static unsigned char style_n;
static unsigned char key;                       /* 0 = C .. 11 = B */
static unsigned char nbars;                     /* of a section: 8 or 12 */
static unsigned char sec_ch[2][MAXBARS];        /* the chords of sections A and B */
static unsigned char sec_mel[2][STEPS];         /* 0 hold, REST, else a MIDI note */
static const unsigned char plan[4] = { 0, 0, 1, 0 };   /* AABA */

/* pitch class of a note relative to the key */
static unsigned char pc(unsigned char n) {
    return (unsigned char)(n + 120 - key) % 12;
}
static unsigned char in_mask(unsigned char n, unsigned int m) {
    return (unsigned char)((m >> pc(n)) & 1);
}
/* the next note above (dir 1) or below (dir -1) in the mask */
static unsigned char step_in(unsigned char n, signed char dir, unsigned int m) {
    unsigned char i;
    for (i = 0; i < 12; i++) {
        n = (unsigned char)(n + dir);
        if (in_mask(n, m)) break;
    }
    return n;
}
/* the note in the mask nearest to n (ties go dir's way) */
static unsigned char near_in(unsigned char n, unsigned int m, signed char dir) {
    unsigned char d;
    if (in_mask(n, m)) return n;
    for (d = 1; d < 12; d++) {
        if (in_mask((unsigned char)(n + dir * (signed char)d), m)) return (unsigned char)(n + dir * (signed char)d);
        if (in_mask((unsigned char)(n - dir * (signed char)d), m)) return (unsigned char)(n - dir * (signed char)d);
    }
    return n;
}
static unsigned int chord_mask(unsigned char c) {
    const struct chord *ch = &sty->ch[c];
    unsigned int m = 0;
    unsigned char i, iv;
    for (i = 0; i < 4; i++) {
        iv = qual_iv[ch->qual][i];
        if (iv == 255) break;
        m |= 1 << ((ch->root + iv) % 12);
    }
    return m;
}
/* the melody's notes over a chord: the scale with the chord's changes */
static unsigned int scale_mask(unsigned char c) {
    return (sty->scale ^ sty->ch[c].alt) | chord_mask(c);
}

/* a chord after prev (and, unless next is NONE, leading to next): drawn
 * with the weight tr[prev][c] x tr[c][next] */
static unsigned char pick_chord(unsigned char prev, unsigned char next) {
    unsigned char n = sty->nch, i, w[8];
    unsigned int tot = 0, r;
    for (i = 0; i < n; i++) {
        w[i] = sty->tr[prev * n + i];
        if (next != NONE) w[i] = w[i] * sty->tr[i * n + next];
        tot += w[i];
    }
    if (tot == 0) {
        for (i = 0; i < n; i++) { w[i] = sty->tr[prev * n + i]; tot += w[i]; }
    }
    r = rnd16() % tot;
    for (i = 0; i < n; i++) {
        if (r < w[i]) return i;
        r -= w[i];
    }
    return 0;
}

static void gen_chords(void) {
    unsigned char *a = sec_ch[0], *b = sec_ch[1], i;
    if (sty->form == FORM_BLUES) {
        /* I7 (IV7) I7 I7 | IV7 IV7 I7 I7 | V7 IV7 I7 V7, the quick change
         * and the turnaround drawn */
        static const unsigned char twelve[12] = { 0, 0, 0, 0, 1, 1, 0, 0, 2, 1, 0, 2 };
        for (i = 0; i < 2; i++) {
            unsigned char *c = sec_ch[i], k;
            for (k = 0; k < 12; k++) c[k] = twelve[k];
            if (rnd_n(2)) c[1] = 1;         /* quick change */
            if (rnd_n(3) == 0) c[9] = 2;    /* V7 V7 instead of V7 IV7 */
        }
        return;
    }
    /* A: a parallel period, I .. half cadence | I (the same) .. V I */
    a[0] = 0;
    a[1] = pick_chord(0, NONE);
    a[2] = pick_chord(a[1], sty->half);
    a[3] = sty->half;
    a[4] = 0;
    a[5] = sty->tr[a[1] * sty->nch + sty->dom] ? a[1] : pick_chord(0, sty->dom);
    a[6] = sty->dom;
    a[7] = 0;
    /* B: away from the tonic, back to the half cadence that leads into A */
    b[0] = pick_chord(0, NONE);
    for (i = 1; i < 6; i++) b[i] = pick_chord(b[i - 1], NONE);
    b[6] = pick_chord(b[5], sty->half);
    b[7] = sty->half;
}

/* The rhythm of a section: 1 a note, 0 hold, REST (2) a rest */
static unsigned char rhythm[STEPS];
#define R_NOTE 1
#define R_REST 2

static void put_cell(unsigned char beat, unsigned char cell) {
    unsigned char s = beat * 4, i;
    for (i = 0; i < 4; i++) rhythm[s + i] = (cell >> i) & 1 ? R_NOTE : 0;
    if (cell == 0 && rnd_n(2)) rhythm[s] = R_REST;   /* an empty beat: a rest or the note held */
}
static unsigned char cell(void) {
    return sty->cells[rnd_n(sty->ncells)];
}
/* a cadence bar: a long note on beat 1, a rest; a pickup into the next phrase */
static void cadence_bar(unsigned char bar, unsigned char pickup) {
    unsigned char s = bar * 16, i;
    for (i = 0; i < 16; i++) rhythm[s + i] = 0;
    rhythm[s] = R_NOTE;
    rhythm[s + (sty->bpm < 90 ? 12 : 8)] = R_REST;
    if (pickup && rnd_n(2)) {
        rhythm[s + 12] = R_NOTE;
        if (rnd_n(2)) rhythm[s + 14] = R_NOTE;
    }
}

static void gen_rhythm(unsigned char sec) {
    unsigned char motif[8], i, b;
    for (i = 0; i < 8; i++) motif[i] = cell();
    if (motif[0] == 0) motif[0] = 1;          /* the motif starts with a note */
    if (sty->form == FORM_BLUES) {
        /* call (bars 1-2), the call again (5-6), the answer (9-10); the
         * bars between: a rest or a short fill */
        for (b = 0; b < 12; b++) {
            unsigned char k;
            if (b == 0 || b == 1 || b == 4 || b == 5) {
                for (k = 0; k < 4; k++) put_cell(b * 4 + k, motif[(b & 1) * 4 + k]);
            } else if (b == 8 || b == 9) {
                for (k = 0; k < 4; k++) put_cell(b * 4 + k, cell());
            } else if (b == 10) {
                cadence_bar(b, 0);
            } else {
                for (k = 0; k < 16; k++) rhythm[b * 16 + k] = 0;
                rhythm[b * 16] = R_REST;
                if (b != 11 && rnd_n(3) == 0) put_cell(b * 4 + 2, cell());   /* a fill */
                if (b == 11 && rnd_n(2)) rhythm[b * 16 + 12] = R_NOTE;      /* a pickup */
            }
        }
        return;
    }
    for (b = 0; b < 8; b++) {
        unsigned char k;
        if (b == 3 || b == 7) {
            cadence_bar(b, b == 3 || sec == 1);
            continue;
        }
        for (k = 0; k < 4; k++) {
            unsigned char c = motif[(b & 1) * 4 + k];
            if (b == 2 && k == rnd_n(4)) c = cell();     /* the motif varied */
            if (b == 6) c = cell();                        /* new material before the cadence */
            if (sec == 1 && b >= 4 && b < 6) c = cell();   /* B: less repetition */
            put_cell(b * 4 + k, c);
        }
    }
}

/* the arch: scale steps above the centre by bar of a 4-bar phrase */
static const unsigned char arch[4][4] = {
    { 0, 2, 4, 1 }, { 1, 3, 2, 0 }, { 0, 3, 5, 1 }, { 2, 4, 3, 0 } };

static unsigned char centre;

static void gen_melody(unsigned char sec) {
    unsigned char *mel = sec_mel[sec];
    unsigned char s, p, prev, bar, shape, last_i = 0, prev_i = 0, reps = 0, tgt;
    signed char dir, resolve = 0;
    unsigned int cm, sm;
    unsigned char c0 = sec_ch[sec][0];
    unsigned char steps = nbars * 16;

    gen_rhythm(sec);
    shape = rnd_n(4);
    p = near_in(centre + (sec ? 3 : 0), chord_mask(c0), 1);
    prev = p;
    for (s = 0; s < steps; s++) {
        bar = s >> 4;
        mel[s] = 0;
        if (rhythm[s] == R_REST) { mel[s] = REST; continue; }
        if (rhythm[s] != R_NOTE) continue;
        /* the consequent repeats the antecedent's first two bars when the
         * chords are the same */
        if (sty->form == FORM_AABA && bar >= 4 && bar < 6 && sec_ch[sec][bar] == sec_ch[sec][bar - 4] &&
            rhythm[s - 64] == R_NOTE && (sec == 0 || bar == 4)) {
            p = mel[s - 64];
            mel[s] = p;
            prev_i = last_i; last_i = s; prev = p; resolve = 0;
            continue;
        }
        if (sty->form == FORM_BLUES && (bar == 4 || bar == 5) && rhythm[s - 64] == R_NOTE) {
            p = mel[s - 64];   /* the call repeated (over the IV chord) */
            mel[s] = p;
            prev_i = last_i; last_i = s; prev = p; resolve = 0;
            continue;
        }
        cm = chord_mask(sec_ch[sec][bar]);
        sm = scale_mask(sec_ch[sec][bar]);
        /* the contour's target */
        tgt = centre + (sec ? 2 : 0);
        {
            unsigned char k, up = arch[shape][(bar & 3)];
            if (sty->form == FORM_BLUES) up = arch[shape][(bar >> 1) & 3];
            for (k = 0; k < up; k++) tgt = step_in(tgt, 1, sm);
        }
        dir = tgt > p ? 1 : (tgt < p ? -1 : (rnd_n(2) ? 1 : -1));
        if (resolve) {
            /* after a leap: a step back the other way */
            p = step_in(p, (signed char)-resolve, sm);
            if ((s & 7) == 0 && !in_mask(p, cm)) p = near_in(p, cm, (signed char)-resolve);
            resolve = 0;
        } else if ((s & 7) == 0 || (s & 15) == 0) {
            /* a strong beat: a chord tone near the last note */
            unsigned char q = rnd_n(4) ? step_in(p, dir, sm) : p;
            unsigned char p2 = near_in(q, cm, dir);
            if ((p2 > p ? p2 - p : p - p2) > 7) p2 = near_in(p, cm, dir);
            if (p2 == p && reps >= 1) p2 = step_in(p, dir, cm);
            p = p2;
        } else {
            unsigned char r = rnd_n(10);
            if (r < 6) p = step_in(p, dir, sm);                       /* a step */
            else if (r < 8) p = step_in(p, (signed char)-dir, sm);    /* a neighbour */
            else if (r < 9) ;                                          /* the same note */
            else p = step_in(step_in(p, dir, cm), dir, cm);            /* a leap to a chord tone */
        }
        /* the range */
        while (p > sty->hi) p = step_in(p, -1, sm);
        while (p < sty->lo) p = step_in(p, 1, sm);
        /* cadences: bar 4 ends on the half cadence's chord, the last bar of
         * A on the tonic, approached by step */
        if (sty->form == FORM_AABA && (s == 48 || s == 112)) {
            if (s == 112 && sec == 0) {
                p = near_in(p, 1, 1);
                if (p > sty->hi) p -= 12;
                if (mel[last_i] > 1) {
                    unsigned char a = step_in(p, mel[last_i] >= p ? 1 : -1, scale_mask(sec_ch[sec][last_i >> 4]));
                    mel[last_i] = a;
                }
            } else {
                p = near_in(p, cm, -1);
            }
        }
        if (sty->form == FORM_BLUES && s == 160) {
            p = near_in(p, 1, -1);
            if (p < sty->lo) p += 12;
        }
        if (p == prev) reps++; else reps = 0;
        if ((p > prev ? p - prev : prev - p) >= 5) resolve = p > prev ? 1 : -1;
        mel[s] = p;
        prev = p;
        prev_i = last_i;
        last_i = s;
    }
    (void)prev_i;
}

static void gen_song(void) {
    nbars = sty->form == FORM_BLUES ? 12 : 8;
    key = rnd_n(12);
    centre = sty->lo + 7;
    gen_chords();
    gen_melody(0);
    gen_melody(1);
}

/* ------------------------------------------------------------------------ */
/* Sound                                                                    */

/* voices: 0 lead, 1 bass, 2 drums (SID 1), 3-5 chords (SID 2) */
#define V_LEAD 0
#define V_BASS 1
#define V_DRUM 2
#define V_CH   3
static const unsigned char voff[6] = { 0x00, 0x07, 0x0E, 0x20, 0x27, 0x2E };

struct voice {
    unsigned char inst, note, gate, t, level;
    unsigned int freq, pw;
    signed char pwdir;
};
static struct voice voices[6];

/* a SID register, on both stereo sides (SID 1/2 and SID 3/4) */
static void sid_w(unsigned char v, unsigned char reg, unsigned char val) {
    unsigned char o = voff[v] + reg;
    REG(0xD400 + o) = val;
    REG(0xD440 + o) = val;
}
static void sid_freq(unsigned char v, unsigned int f) {
    sid_w(v, 0, (unsigned char)f);
    sid_w(v, 1, (unsigned char)(f >> 8));
}

static void note_on(unsigned char v, unsigned char n) {
    struct voice *vo = &voices[v];
    const struct inst *in = &insts[vo->inst];
    if (n < NOTE_LO) n += 12;
    if (n > NOTE_HI) n -= 12;
    vo->note = n;
    vo->freq = note_freq[n - NOTE_LO];
    vo->t = 0;
    vo->gate = 1;
    vo->level = 15;
    sid_freq(v, vo->freq);
    sid_w(v, 2, (unsigned char)vo->pw);
    sid_w(v, 3, (unsigned char)(vo->pw >> 8));
    sid_w(v, 5, in->ad);
    sid_w(v, 6, in->sr);
    sid_w(v, 4, in->wave);              /* gate off, then on: the envelope restarts */
    sid_w(v, 4, in->wave | SID_GATE);
}
static void note_off(unsigned char v) {
    struct voice *vo = &voices[v];
    if (!vo->gate) return;
    vo->gate = 0;
    sid_w(v, 4, insts[vo->inst].wave);
}
static void set_inst(unsigned char v, unsigned char i) {
    note_off(v);
    voices[v].inst = i;
    voices[v].pw = insts[i].pw;
    voices[v].pwdir = insts[i].pwm;
}

/* Drums on voice 2: kick (a pulse falling in pitch), snare (noise), hi-hat
 * (high noise, closed or open) */
enum { D_NONE, D_KICK, D_SNARE, D_HAT, D_OPEN };
static unsigned char drum, drum_t;
static const unsigned char kick_f[6] = { 0x18, 0x10, 0x0B, 0x08, 0x06, 0x05 };

static void drum_hit(unsigned char d) {
    drum = d;
    drum_t = 0;
    voices[V_DRUM].level = 15;
    sid_w(V_DRUM, 2, 0);
    sid_w(V_DRUM, 3, 8);
    sid_w(V_DRUM, 4, 0);
    if (d == D_KICK) {
        sid_w(V_DRUM, 5, 0x08); sid_w(V_DRUM, 6, 0x00);
        sid_w(V_DRUM, 0, 0); sid_w(V_DRUM, 1, kick_f[0]);
        sid_w(V_DRUM, 4, SID_PULSE | SID_GATE);
    } else if (d == D_SNARE) {
        sid_w(V_DRUM, 5, 0x08); sid_w(V_DRUM, 6, 0x00);
        sid_w(V_DRUM, 0, 0); sid_w(V_DRUM, 1, 0x22);
        sid_w(V_DRUM, 4, SID_NOISE | SID_GATE);
    } else {
        sid_w(V_DRUM, 5, d == D_OPEN ? 0x06 : 0x02); sid_w(V_DRUM, 6, 0x00);
        sid_w(V_DRUM, 0, 0); sid_w(V_DRUM, 1, 0xE0);
        sid_w(V_DRUM, 4, SID_NOISE | SID_GATE);
    }
}
static void drum_frame(void) {
    if (drum == D_NONE) return;
    drum_t++;
    if (drum == D_KICK) {
        if (drum_t < 6) sid_w(V_DRUM, 1, kick_f[drum_t]);
        if (drum_t == 4) sid_w(V_DRUM, 4, SID_PULSE);
    } else if (drum == D_SNARE) {
        if (drum_t == 1) sid_w(V_DRUM, 1, 0x1C);
        if (drum_t == 3) sid_w(V_DRUM, 4, SID_NOISE);
    } else if (drum_t == (drum == D_OPEN ? 3 : 1)) {
        sid_w(V_DRUM, 4, SID_NOISE);
    }
    if (drum_t >= 8) drum = D_NONE;
}

/* the chord voices */
static unsigned char voicing[3];
static unsigned char arp_on, arp_i;

/* the chord's inversion (in close position, 3 notes from E3 up) closest to
 * the last one; a 4-note chord drops its fifth */
static void voice_chord(unsigned char c) {
    const struct chord *ch = &sty->ch[c];
    unsigned int m = 0;
    unsigned char b, best = 255, i, v[3];
    for (i = 0; i < 4; i++) {
        unsigned char iv = qual_iv[ch->qual][i];
        if (iv == 255) break;
        if (i == 2 && qual_iv[ch->qual][3] != 255) continue;   /* no fifth in a 7th chord */
        m |= 1 << ((ch->root + iv) % 12);
    }
    if (sty->acc == ACC_POWER || ch->qual == POW) {
        b = 40 + (key + ch->root) % 12;
        voicing[0] = b; voicing[1] = b + 7; voicing[2] = b + 12;
        return;
    }
    for (b = 52; b < 64; b++) {
        unsigned char cost;
        if (!in_mask(b, m)) continue;
        v[0] = b;
        v[1] = step_in(b, 1, m);
        v[2] = step_in(v[1], 1, m);
        if (voicing[0] == 0) {
            cost = b > 57 ? b - 57 : 57 - b;
        } else {
            cost = 0;
            for (i = 0; i < 3; i++) cost += v[i] > voicing[i] ? v[i] - voicing[i] : voicing[i] - v[i];
        }
        if (cost < best) {
            best = cost;
            voicing[0] = v[0]; voicing[1] = v[1]; voicing[2] = v[2];
            if (best == 0) break;
        }
    }
    /* first time: remember the start (cost from E3) */
    if (best == 255) { voicing[0] = 52; voicing[1] = 55; voicing[2] = 59; }
}

static void chords_strike(void) {
    unsigned char i;
    if (sty->acc == ACC_ARP) {
        arp_on = 1;
        arp_i = 0;
        note_on(V_CH, voicing[0] + 12);
        return;
    }
    for (i = 0; i < 3; i++) note_on(V_CH + i, voicing[i]);
}
static void chords_release(void) {
    unsigned char i;
    for (i = 0; i < 3; i++) note_off(V_CH + i);
    arp_on = 0;
}

/* The bass */
static unsigned char bass_root(unsigned char c) {
    unsigned char n = 36 + (key + sty->ch[c].root) % 12;
    if (n > 44) n -= 12;
    return n;
}
static void bass_step(unsigned char step, unsigned char c, unsigned char next) {
    unsigned char r = bass_root(c), code = sty->bass[step], n = 0, q = sty->ch[c].qual;
    switch (code) {
        case 'R': n = r; break;
        case 'r': n = r + 12; break;
        case '5': n = r + 7; break;
        case '3': n = r + qual_iv[q][1]; break;
        case '6': n = r + 9; break;
        case '7': n = r + 10; break;
        case 'L': {          /* Alberti: low, high, middle, high */
            static const unsigned char alb[4] = { 0, 2, 1, 2 };
            n = voicing[alb[step & 3]] - 12;
            break;
        }
        case 'W': {          /* walking: root, a chord tone, a chord tone, an approach */
            unsigned char beat = step >> 2, nr = bass_root(next);
            if (beat == 0) n = r;
            else if (beat == 1) n = r + (rnd_n(2) ? qual_iv[q][1] : 2);
            else if (beat == 2) n = r + (rnd_n(3) ? 7 : qual_iv[q][1]);
            else n = rnd_n(2) ? nr - 1 : nr + 1;   /* a chromatic approach to the next root */
            break;
        }
        case '-': note_off(V_BASS); return;
        default: return;
    }
    note_on(V_BASS, n);
}

/* ------------------------------------------------------------------------ */
/* Playing                                                                  */

static unsigned char tempo, drums_on, paused;
static unsigned char lead_sel, chord_sel, bass_sel;
static unsigned char plan_i, bar, step, rounds;
static unsigned int acc, cur_len, len_long, len_short;
static unsigned char ntsc;
static const char *comp;
static unsigned char frame;

static void set_tempo(void) {
    unsigned int base = (unsigned int)((ntsc ? 230400UL : 192000UL) / tempo);
    if (sty->swing) {
        len_long = base / 3 * 4;      /* swing: long-short eighths */
        len_short = base / 3 * 2;
    } else {
        len_long = len_short = base;
    }
}
static unsigned int step_len(void) {
    return (step & 3) < 2 ? len_long : len_short;
}

static void draw_all(void);
static void draw_bar(void);

static void start_song(void) {
    unsigned char i;
    for (i = 0; i < 6; i++) note_off(i);
    voicing[0] = 0;
    gen_song();
    plan_i = 0; bar = 0; step = 0; rounds = 0; acc = 0; cur_len = 0;
    draw_all();
}

static void choose_style(unsigned char n) {
    unsigned char i;
    style_n = n;
    sty = &styles[n];
    tempo = sty->bpm;
    drums_on = sty->drums;
    for (i = 0; i < 10; i++) if (lead_insts[i] == sty->lead) lead_sel = i;
    for (i = 0; i < 8; i++) if (chord_insts[i] == sty->chords) chord_sel = i;
    for (i = 0; i < 4; i++) if (bass_insts[i] == sty->bassi) bass_sel = i;
    set_inst(V_LEAD, sty->lead);
    set_inst(V_BASS, sty->bassi);
    for (i = 0; i < 3; i++) set_inst(V_CH + i, sty->chords);
    set_tempo();
    start_song();
}

static void do_step(void) {
    unsigned char sec = plan[plan_i];
    unsigned char c = sec_ch[sec][bar], next, m, last_bar = bar == nbars - 1;
    next = bar + 1 < nbars ? sec_ch[sec][bar + 1] : sec_ch[plan[(plan_i + 1) & 3]][0];
    if (step == 0) {
        voice_chord(c);
        comp = rnd_n(3) ? sty->comp1 : sty->comp2;
        draw_bar();
    }
    /* melody */
    m = sec_mel[sec][bar * 16 + step];
    if (m == REST) note_off(V_LEAD);
    else if (m) note_on(V_LEAD, m);
    /* bass and chords */
    bass_step(step, c, next);
    if (comp[step] == 'x') chords_strike();
    else if (comp[step] == '-') chords_release();
    /* drums, with a fill in the last bar of a section */
    if (drums_on) {
        if (last_bar && step >= 12 && sty->snare[4] == 'x' && (step == 12 || step == 14 || step == 15))
            drum_hit(D_SNARE);
        else if (sty->kick[step] == 'x') drum_hit(D_KICK);
        else if (sty->snare[step] == 'x') drum_hit(D_SNARE);
        else if (sty->hat[step] == 'x') drum_hit(bar == 0 && step == 0 ? D_OPEN : D_HAT);
    }
    /* next step */
    if (++step == 16) {
        step = 0;
        if (++bar == nbars) {
            bar = 0;
            if (++plan_i == 4) {
                plan_i = 0;
                if (++rounds == 2) start_song();   /* twice through: a new song */
            }
        }
    }
}

/* every frame: envelopes' meters, vibrato, pulse width, the arpeggio */
static void voices_frame(void) {
    unsigned char v;
    for (v = 0; v < 6; v++) {
        struct voice *vo = &voices[v];
        const struct inst *in;
        if (v == V_DRUM) {
            if (vo->level) vo->level--;
            continue;
        }
        in = &insts[vo->inst];
        if (vo->level > (vo->gate ? (in->sr >> 4) : 0)) vo->level--;
        if (!vo->gate) continue;
        if (vo->t < 255) vo->t++;
        if (vo->pwdir) {
            vo->pw += vo->pwdir;
            if (vo->pw > 0x0E00 || vo->pw < 0x0200) vo->pwdir = -vo->pwdir;
            sid_w(v, 2, (unsigned char)vo->pw);
            sid_w(v, 3, (unsigned char)(vo->pw >> 8));
        }
        if (in->vib && vo->t > in->vdelay) {
            static const signed char tri[8] = { 0, 1, 2, 1, 0, -1, -2, -1 };
            signed int d = (signed int)(vo->freq >> 8) * in->vib * tri[(vo->t >> 1) & 7];
            sid_freq(v, vo->freq + d);
        }
    }
    if (arp_on) {
        /* the chord's notes in turn, one a frame */
        arp_i = arp_i == 2 ? 0 : arp_i + 1;
        sid_freq(V_CH, note_freq[voicing[arp_i] + 12 - NOTE_LO]);
        voices[V_CH + 1].level = voices[V_CH + 2].level = voices[V_CH].level;
    }
    drum_frame();
}

static void all_off(void) {
    unsigned char v;
    for (v = 0; v < 6; v++) note_off(v);
    arp_on = 0;
    for (v = 0; v < 6; v++) sid_w(v, 4, 0);
}

/* ------------------------------------------------------------------------ */
/* The screen: our own 80x25 buffer, colour RAM from offset $0800           */

#define COLOUR_LIN 0xFF80800UL
static unsigned char scr[2000];
static unsigned char cols, x0;
static unsigned int stride;

static unsigned char scode(unsigned char c) {
    if (c >= 0x40 && c < 0x60) return c - 0x40;
    if (c >= 0x60 && c < 0x80) return c - 0x20;
    if (c >= 0xC0 && c < 0xE0) return c - 0x80;
    return c;
}
static unsigned char *at(unsigned char x, unsigned char y) {
    return &scr[y * stride + x0 + x];
}
static void put(unsigned char x, unsigned char y, const char *s) {
    unsigned char *d = at(x, y);
    while (*s) *d++ = scode((unsigned char)*s++);
}
/* text padded with spaces to w characters */
static void putw(unsigned char x, unsigned char y, const char *s, unsigned char w) {
    unsigned char *d = at(x, y), i = 0;
    while (*s && i < w) { *d++ = scode((unsigned char)*s++); i++; }
    while (i++ < w) *d++ = ' ';
}
static void putnum(unsigned char x, unsigned char y, unsigned int v, unsigned char digits) {
    unsigned char *d = at(x, y) + digits;
    while (digits--) { *--d = '0' + v % 10; v /= 10; }
}
static void colour(unsigned char x, unsigned char y, unsigned char w, unsigned char c) {
    dma_fill_far(COLOUR_LIN + y * stride + x0 + x, w, c);
}

static const char *style_names = "1 pop  2 rock  3 blues  4 jazz";

/* a note's name, spelled with sharps in the sharp keys (G D A E B F#
 * major and their modes), else with flats */
static const char *note_name(unsigned char n) {
    unsigned char maj = (key + sty->rel) % 12;
    if (maj == 7 || maj == 2 || maj == 9 || maj == 4 || maj == 11 || maj == 6) return sharp_name[n % 12];
    return flat_name[n % 12];
}

static void chord_text(char *buf, unsigned char c) {
    const struct chord *ch = &sty->ch[c];
    const char *a = note_name(key + ch->root), *b = qual_name[ch->qual];
    unsigned char i = 0;
    while (*a) buf[i++] = *a++;
    while (*b) buf[i++] = *b++;
    buf[i] = 0;
}

static void draw_status(void) {
    putw(8, 2, sty->name, 10);
    putnum(27, 2, tempo, 3);
    put(31, 2, "bpm");
    putw(8, 6, insts[voices[V_LEAD].inst].name, 13);
    putw(8, 7, insts[voices[V_CH].inst].name, 13);
    putw(8, 8, insts[voices[V_BASS].inst].name, 13);
    putw(8, 9, drums_on ? "on" : "off", 4);
    putw(22, 9, paused ? "paused" : "", 6);
}

/* the progression of the playing section, the current bar highlighted */
static void draw_prog(void) {
    unsigned char sec = plan[plan_i], b, i;
    char buf[10];
    unsigned char *d;
    for (b = 0; b < 3; b++) putw(0, 16 + b, "", 40);
    for (b = 0; b < nbars; b++) {
        unsigned char x = (b & 3) * 10 + 1, y = 16 + (b >> 2);
        chord_text(buf, sec_ch[sec][b]);
        putw(x, y, buf, 9);
        if (b == bar) {
            d = at(x - 1, y);
            for (i = 0; i < 10; i++) d[i] |= 0x80;   /* reverse */
        }
    }
}

static void draw_bar(void) {
    char buf[10];
    unsigned char bars = nbars * 4;
    putnum(27, 3, plan_i * nbars + bar + 1, 2);
    put(29, 3, "/");
    putnum(30, 3, bars, 2);
    put(33, 3, plan[plan_i] ? "b" : "a");
    chord_text(buf, sec_ch[plan[plan_i]][bar]);
    putw(8, 4, buf, 10);
    draw_prog();
}

static void draw_all(void) {
    char buf[12];
    unsigned char i = 0;
    const char *a = note_name(key);
    putw(8, 3, "", 16);
    while (*a) buf[i++] = *a++;
    buf[i] = 0;
    put(8, 3, buf);
    put(9 + i, 3, sty->mode);
    draw_status();
    draw_bar();
}

static void draw_meters(void) {
    static const unsigned char order[4] = { V_LEAD, V_BASS, V_DRUM, V_CH };
    unsigned char m, i, l, *d;
    for (m = 0; m < 4; m++) {
        l = voices[order[m]].level;
        d = at(8, 11 + m);
        for (i = 0; i < 15; i++) d[i] = i < l ? 0xA0 : 0x2E;   /* reverse space or a dot */
    }
}

static void draw_frame(void) {
    unsigned char y;
    for (y = 0; y < 25; y++) putw(0, y, "", 40);
    put(0, 0, "musicgen - random music for the mega65");
    put(0, 2, "style");
    put(21, 2, "tempo");
    put(0, 3, "key");
    put(21, 3, "bar");
    put(0, 4, "chord");
    put(0, 6, "lead");
    put(0, 7, "chords");
    put(0, 8, "bass");
    put(0, 9, "drums");
    put(0, 11, "lead");
    put(0, 12, "bass");
    put(0, 13, "drums");
    put(0, 14, "chords");
    put(0, 20, style_names);
    put(0, 21, "5 classical  6 ambient  7 chiptune");
    put(0, 22, "+ - tempo   n new song   space pause");
    put(0, 23, "l c b instruments   d drums");
    put(0, 24, "run/stop quit");
    for (y = 0; y < 25; y++) colour(0, y, 40, 1);   /* white */
    colour(0, 0, 40, 3);                            /* cyan title */
    for (y = 2; y < 10; y++) colour(0, y, 7, 15);   /* light grey labels */
    for (y = 11; y < 15; y++) { colour(0, y, 7, 15); colour(8, y, 15, 5 + (y & 1) * 2); }
    for (y = 16; y < 19; y++) colour(0, y, 40, 7);  /* yellow chords */
    for (y = 20; y < 25; y++) colour(0, y, 40, 14); /* light blue help */
}

static unsigned char saved[8];

static void video_on(void) {
    saved[0] = REG(0xD060); saved[1] = REG(0xD061); saved[2] = REG(0xD062); saved[3] = REG(0xD063);
    saved[4] = REG(0xD064); saved[5] = REG(0xD065);
    saved[6] = REG(0xD020); saved[7] = REG(0xD021);
    cols = REG(0xD05E);
    stride = REG(0xD058) | (REG(0xD059) << 8);
    if (cols < 40) cols = 40;
    if (stride > 80 || stride < cols) stride = cols;
    x0 = (cols - 40) / 2;
    __dma_fill(scr, 2000, ' ');
    dma_fill_far(COLOUR_LIN, 2000, 1);
    draw_frame();
    REG(0xD060) = (unsigned char)(unsigned int)scr;
    REG(0xD061) = (unsigned char)((unsigned int)scr >> 8);
    REG(0xD062) = 0;
    REG(0xD063) = REG(0xD063) & 0xF0;
    REG(0xD064) = 0x00;
    REG(0xD065) = 0x08;
    REG(0xD020) = 0;
    REG(0xD021) = 0;
}
static void video_off(void) {
    REG(0xD060) = saved[0]; REG(0xD061) = saved[1]; REG(0xD062) = saved[2]; REG(0xD063) = saved[3];
    REG(0xD064) = saved[4]; REG(0xD065) = saved[5];
    REG(0xD020) = saved[6]; REG(0xD021) = saved[7];
}

static void wait_frame(void) {
    while (REG(0xD012) == 0) ;
    while (REG(0xD012) != 0 || (REG(0xD011) & 0x80)) ;
}

static unsigned char keys(void) {
    unsigned char c, quit = 0;
    while ((c = ASCIIKEY) != 0) {
        ASCIIKEY = 0;
        rs ^= frame;                       /* the player's timing stirs the random numbers */
        if (c >= '1' && c <= '7') { choose_style(c - '1'); continue; }
        switch (c) {
            case '+': if (tempo < 240) tempo += 4; set_tempo(); break;
            case '-': if (tempo > 40) tempo -= 4; set_tempo(); break;
            case 'n': case 'N': start_song(); break;
            case ' ': paused = !paused; if (paused) all_off(); break;
            case 'l': case 'L':
                lead_sel = lead_sel == 9 ? 0 : lead_sel + 1;
                set_inst(V_LEAD, lead_insts[lead_sel]);
                break;
            case 'c': case 'C': {
                unsigned char i;
                chord_sel = chord_sel == 7 ? 0 : chord_sel + 1;
                for (i = 0; i < 3; i++) set_inst(V_CH + i, chord_insts[chord_sel]);
                arp_on = 0;
                break;
            }
            case 'b': case 'B':
                bass_sel = bass_sel == 3 ? 0 : bass_sel + 1;
                set_inst(V_BASS, bass_insts[bass_sel]);
                break;
            case 'd': case 'D': drums_on = !drums_on; break;
            case 0x03: quit = 1; break;    /* RUN/STOP */
        }
        draw_status();
    }
    return quit;
}

#ifdef MG_TEST
/* The generator test: each style's song logged at $A000:
 * style, key, bars, 0, then per section: per bar the chord mask and scale
 * mask (2 bytes each, 12 bars), the 192 melody steps */
int main(void) {
    unsigned char *log = (unsigned char *)0xA000;
    unsigned char n, sec, b, k;
    unsigned int s;
    for (n = 0; n < 2 * N_STYLES; n++) {
        sty = &styles[n % N_STYLES];
        rs = 0x1234 + n * 977;
        gen_song();
        *log++ = n % N_STYLES; *log++ = key; *log++ = nbars; *log++ = 0;
        for (sec = 0; sec < 2; sec++) {
            for (b = 0; b < MAXBARS; b++) {
                unsigned int cm = b < nbars ? chord_mask(sec_ch[sec][b]) : 0;
                unsigned int sm = b < nbars ? scale_mask(sec_ch[sec][b]) : 0;
                *log++ = (unsigned char)cm; *log++ = (unsigned char)(cm >> 8);
                *log++ = (unsigned char)sm; *log++ = (unsigned char)(sm >> 8);
            }
            for (s = 0; s < STEPS; s++) *log++ = s < nbars * 16 ? sec_mel[sec][s] : 0;
        }
    }
    (void)k;
    REG(0xC0C0) = 0xAA;
    return 0;
}
#else
int main(void) {
    unsigned char i;
    __asm__("sei");
    ntsc = REG(0xD06F) & 0x80;
    rs ^= REG(0xD012) | (REG(0xDC04) << 8);
    if (rs == 0) rs = 1;
    for (i = 0; i < 6; i++) voices[i].inst = I_PIANO;
    REG(0xD418) = 15;              /* volume */
    REG(0xD438) = 15;
    REG(0xD458) = 15;
    REG(0xD478) = 15;
    video_on();
    while (ASCIIKEY) ASCIIKEY = 0;
    choose_style(0);
    for (;;) {
        wait_frame();
        frame++;
        if (!paused) {
            acc += 256;
            while (acc >= cur_len) {
                acc -= cur_len;
                cur_len = step_len();     /* the length of the step about to play */
                do_step();
            }
        }
        voices_frame();
        draw_meters();
        if (keys()) break;
    }
    all_off();
    video_off();
    while (ASCIIKEY) ASCIIKEY = 0;
    __asm__("cli");
    return 0;
}
#endif
