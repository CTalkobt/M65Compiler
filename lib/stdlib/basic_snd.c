/* basic_snd.c — basic45 --compile's PLAY, RPLAY, TEMPO, VOL, ENVELOPE and
 * FILTER (#369)
 *
 * PLAY's strings are converted when the program is compiled (BasicPlay.cpp)
 * into tunes for the library's tune player (<audio_tune.h>), which the
 * interrupt hook (basic_spr.s45) updates each frame through _bas_musv. The
 * state is BASIC 65's own (b65.src V920413), in its variables in bank 0:
 * the envelopes ($1289-$12BA: atktab, sustab, wavtab, pulslw, pulshi), the
 * tempo ($1273), the volume / filter copies (filters1 $1278, filters2
 * $127C), so a program and the interpreter share them. A PLAY puts the
 * envelopes' current values into the tune's instruments and plays it at
 * the tempo's speed (tune_tempo: TEMPO n is n / 12 of the speed it was
 * written for); all of a program's strings are one tune, a PLAY starting the
 * voices it gives. Arguments in _bas_sa (PLAY's descriptor), _bas_n, _bas_g.
 */

#include <audio_tune.h>

extern int _bas_n, _bas_m;
extern unsigned char *_bas_sa;
extern unsigned char _bas_g[20];
extern void _bas_error(void);

#define TEMPO_RATE (*(volatile unsigned char *)0x1273)
#define OCTAVE (*(volatile unsigned char *)0x1270)
#define NTIME ((volatile unsigned char *)0x1276)
#define FILTERS1 ((volatile unsigned char *)0x1278)
#define FILTERS2 ((volatile unsigned char *)0x127C)
#define ATKTAB ((volatile unsigned char *)0x1289)
#define SUSTAB ((volatile unsigned char *)0x1293)
#define WAVTAB ((volatile unsigned char *)0x129D)
#define PULSLW ((volatile unsigned char *)0x12A7)
#define PULSHI ((volatile unsigned char *)0x12B1)
#define VOICES ((volatile unsigned char *)0x125D)
#define SOUND_TIME_HI ((volatile unsigned char *)0x12C4)
#define SID ((volatile unsigned char *)0xD400)
#define VIC_PAL (*(volatile unsigned char *)0xD06F)

static void raise(int n) {
    _bas_n = n;
    _bas_error();
}

/* the interrupt hook's music routine (0: none) */
void (*_bas_musv)(void);

static void speed(void) {
    tune_tempo((unsigned int)TEMPO_RATE * 256 / 12);
}

/* PLAY "...", ...: the descriptor _bas_sa (the voices given, bit n voice n +
 * 1; each one's stream in the program's tune _bas_ptune: BasicPlay.cpp). The
 * envelopes' current values into the tune's ten instruments; the tune
 * started when nothing plays; each voice given started on its stream at the
 * next tick (tune_voice_play), the others going on (#478). Voices 4-6 are
 * the tune's 7-9 (SID 3). */
extern unsigned char _bas_ptune[];
#define PT_INS 26           /* (BasicPlay.hpp: the header's bytes, an instrument's) */
#define PT_ISIZE 17
void _bas_play(void) {
    unsigned char *d = _bas_sa, k;
    for (k = 0; k < 10; k++) {
        unsigned char *p = _bas_ptune + PT_INS + (unsigned int)k * PT_ISIZE;
        p[0] = WAVTAB[k] & 0xFE;
        p[1] = ATKTAB[k];
        p[2] = SUSTAB[k];
        p[3] = PULSLW[k];
        p[4] = PULSHI[k] & 0x0F;
    }
    if (!_bas_musv || !tune_playing()) {
        tune_voices(0);
        speed();
        tune_play(_bas_ptune);
        _bas_musv = tune_update;
    }
    for (k = 0; k < 6; k++)
        if (d[0] & (1 << k))
            tune_voice_play(k < 3 ? k : k + 3, d[1 + k * 2] | (unsigned int)d[2 + k * 2] << 8);
}

/* RPLAY(v), v 1-6: 1 while voice v has its string to play */
void _bas_rplay(void) {
    unsigned char v = (unsigned char)(_bas_n - 1);
    if ((unsigned int)_bas_n - 1 >= 6) raise(14);
    _bas_n = _bas_musv ? tune_voice_busy(v < 3 ? v : v + 3) : 0;
}

static const unsigned char atkmus[10] = {0x09, 0xC0, 0x00, 0x05, 0x94, 0x09, 0x09, 0x09, 0x89, 0x09};
static const unsigned char susmus[10] = {0x00, 0xC0, 0xF0, 0x50, 0x40, 0x21, 0x00, 0x90, 0x41, 0x00};
static const unsigned char wavmus[10] = {0x41, 0x21, 0x11, 0x81, 0x11, 0x21, 0x41, 0x41, 0x41, 0x11};
static const unsigned char pwhmus[10] = {0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x08, 0x02, 0x00};

/* PLAY alone (sound_defaults): the music stopped, the four SIDs cleared
 * (volume 8), filters off, octave 4, quarter notes, TEMPO 12, the default
 * envelopes */
void _bas_snddef(void) {
    unsigned char k, c;
    if (_bas_musv) tune_stop();
    __asm__("php\n sei");
    for (c = 0; c < 4; c++) {
        volatile unsigned char *s = SID + c * 0x20;
        for (k = 0; k < 24; k++) s[k] = 0;
        s[24] = 8;
    }
    FILTERS1[2] = 0;
    FILTERS2[2] = 0;
    FILTERS1[3] = 8;
    FILTERS2[3] = 8;
    {
        unsigned int q = (VIC_PAL & 0x80 ? 1440 : 1200) / 4;
        NTIME[0] = (unsigned char)q;
        NTIME[1] = (unsigned char)(q >> 8);
    }
    OCTAVE = 4;
    TEMPO_RATE = 12;
    for (k = 0; k < 10; k++) {
        ATKTAB[k] = atkmus[k];
        SUSTAB[k] = susmus[k];
        WAVTAB[k] = wavmus[k];
        PULSHI[k] = pwhmus[k];
    }
    for (k = 0; k < 6; k++) {
        SOUND_TIME_HI[k] = 0xFF;
        VOICES[k * 2] = 0xFF;
        VOICES[k * 2 + 1] = 0xFF;
    }
    __asm__("plp");
    if (_bas_musv) speed();
}

/* TEMPO n (1-255; 0: ?ILLEGAL QUANTITY): a whole note 24 / n seconds */
void _bas_tempo(void) {
    if (!(unsigned char)_bas_n) raise(14);
    TEMPO_RATE = (unsigned char)_bas_n;
    if (_bas_musv) speed();
}

/* VOL [right] [, left]: _bas_g[0] bit 0 / 1 given, _bas_g[1] / [2] the
 * volumes (0-15); the right SIDs (1, 2) and the left (3, 4), and PLAY's
 * copies */
void _bas_vol(void) {
    if (_bas_g[0] & 1) {
        if (_bas_g[1] > 15) raise(14);
        FILTERS1[3] = (FILTERS1[3] & 0xF0) | _bas_g[1];
        SID[0x18] = FILTERS1[3];
        SID[0x38] = FILTERS1[3];
    }
    if (_bas_g[0] & 2) {
        if (_bas_g[2] > 15) raise(14);
        FILTERS2[3] = (FILTERS2[3] & 0xF0) | _bas_g[2];
        SID[0x58] = FILTERS2[3];
        SID[0x78] = FILTERS2[3];
    }
}

/* ENVELOPE n, attack, decay, sustain, release, waveform, pulse width:
 * _bas_g[0] n (0-9), [1] the given ones (bit 0 attack ... bit 5 pulse
 * width), [2-6] their values, [7-8] the pulse width (0-65535) */
void _bas_envelope(void) {
    unsigned char n = _bas_g[0], f = _bas_g[1], t[3], k;
    if (n >= 10) raise(14);
    t[0] = ATKTAB[n];
    t[1] = SUSTAB[n];
    t[2] = WAVTAB[n];
    for (k = 0; k < 2; k++) {
        if (f & (1 << (k * 2))) t[k] = (unsigned char)((t[k] & 0x0F) | (_bas_g[2 + k * 2] << 4));
        if (f & (2 << (k * 2))) t[k] = (unsigned char)((t[k] & 0xF0) | (_bas_g[3 + k * 2] & 0x0F));
    }
    if (f & 16) {
        unsigned char w = _bas_g[6];
        if (w > 4) raise(14);
        t[2] = w == 4 ? 0x15 : (unsigned char)((0x10 << w) | 1);
    }
    if ((f & 32) && (t[2] & 0x40)) {
        PULSHI[n] = _bas_g[8];
        PULSLW[n] = _bas_g[7];
    }
    ATKTAB[n] = t[0];
    SUSTAB[n] = t[1];
    WAVTAB[n] = t[2];
}

/* FILTER sid, freq, lp, bp, hp, res: _bas_g[0] the SID (1-2), [1] the given
 * ones (bit 0 freq, 1-3 lp bp hp, 4 res), [2-3] freq (0-2047), [4-6] the
 * types (0-1), [7] res (0-15): PLAY's copy of that SID's filter, as the
 * ROM's FILTER, and the chip's filter at once (tune_filter: the routing and
 * volume kept; the ROM's chips get it at a voice's X1, which here routes the
 * voice only: #475). SID 2 is chip 2, where PLAY's voices 4-6 play. */
void _bas_filter(void) {
    unsigned char s = (unsigned char)(_bas_g[0] - 1), f = _bas_g[1], k, fl = 0x10;
    volatile unsigned char *p;
    unsigned char sv[4];
    if (s >= 2) raise(14);
    p = s ? FILTERS2 : FILTERS1;
    for (k = 0; k < 4; k++) sv[k] = p[k];
    if (f & 1) {
        unsigned int q = _bas_g[2] | (unsigned int)_bas_g[3] << 8;
        if (q > 2047) raise(14);
        sv[0] = (unsigned char)q;
        sv[1] = (unsigned char)(q >> 3);
    }
    for (k = 0; k < 3; k++, fl <<= 1)
        if (f & (2 << k)) {
            if (_bas_g[4 + k] > 1) raise(14);
            sv[3] = _bas_g[4 + k] ? (unsigned char)(sv[3] | fl) : (unsigned char)(sv[3] & ~fl);
        }
    if (f & 16) {
        if (_bas_g[7] > 15) raise(14);
        sv[2] = (unsigned char)((sv[2] & 0x0F) | (_bas_g[7] << 4));
    }
    for (k = 0; k < 4; k++) p[k] = sv[k];
    tune_filter(s ? 2 : 0, (sv[0] & 7) | (unsigned int)sv[1] << 3, sv[2] >> 4, (sv[3] >> 4) & 7);
}

/* SOUND voice, freq, duration [, dir [, min [, step [, waveform [, pulse]]]]]
 * (b65.src sound): _bas_g[0] the voice (1-6), [1-2] the frequency, [3-4]
 * the duration (frames, 0-32767; 0: off at the next frame), [5] the
 * direction (0 up, 1 down, 2 oscillating), [6-7] the minimum, [8-9] the
 * step, [10] the waveform (0-3; 2 by default), [11-12] the pulse width
 * (0-4095; $800), [13] bit 0 / 1: the waveform / pulse width given. On
 * SIDs 2 and 4 ($D420, $D460); the sweep and the time run in the interrupt
 * hook (basic_spr.s45's soundirq) on BASIC's own variables */
#define SOUND_VOICE (*(volatile unsigned char *)0x12BD)
#define S_ARR(k) ((volatile unsigned char *)(0x12BE + (k) * 6))   /* time lo, time hi, max lo ... freq hi */
void _bas_sound(void) {
    static const unsigned char sidoff[6] = {0x00, 0x07, 0x0E, 0x40, 0x47, 0x4E};
    unsigned char v = (unsigned char)(_bas_g[0] - 1), dir = _bas_g[5], w = 2, k;
    unsigned int f = _bas_g[1] | (unsigned int)_bas_g[2] << 8, t = _bas_g[3] | (unsigned int)_bas_g[4] << 8;
    unsigned int mn = _bas_g[6] | (unsigned int)_bas_g[7] << 8, st = _bas_g[8] | (unsigned int)_bas_g[9] << 8;
    unsigned int pw = 0x800;
    unsigned char tv[9];
    if (v >= 6 || (t & 0x8000) || dir >= 3) raise(14);
    if (dir & 1) st = 0 - st;
    if (_bas_g[13] & 1) w = _bas_g[10];
    if (w >= 4) raise(14);
    if (_bas_g[13] & 2) pw = _bas_g[11] | (unsigned int)_bas_g[12] << 8;
    if (pw >= 0x1000) raise(14);
    SOUND_VOICE = v;
    if (t) {
        volatile unsigned char *sid = (volatile unsigned char *)0xD420 + sidoff[v];
        tv[0] = (unsigned char)f; tv[1] = (unsigned char)(f >> 8);
        tv[2] = (unsigned char)mn; tv[3] = (unsigned char)(mn >> 8);
        tv[4] = dir;
        tv[5] = (unsigned char)st; tv[6] = (unsigned char)(st >> 8);
        tv[7] = (unsigned char)f; tv[8] = (unsigned char)(f >> 8);
        for (k = 0; k < 9; k++) S_ARR(2 + k)[v] = tv[k];
        sid[5] = 0;
        sid[6] = 0xF0;
        sid[0] = (unsigned char)f;
        sid[1] = (unsigned char)(f >> 8);
        sid[2] = (unsigned char)pw;
        sid[3] = (unsigned char)(pw >> 8);
        sid[4] = (unsigned char)((0x10 << w) | 1);
    }
    __asm__("sei");
    S_ARR(1)[v] = (unsigned char)(t >> 8);
    S_ARR(0)[v] = (unsigned char)t;
    __asm__("cli");
}
