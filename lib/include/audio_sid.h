/* audio_sid.h — the MEGA65's four SID chips (lib/stdlib/audio_sid.c)
 *
 * Twelve voices: voice v is voice v % 3 of SID v / 3 (SID 1 $D400, SID 2
 * $D420 on the left, SID 3 $D440, SID 4 $D460 on the right). Every register
 * written is also kept in a shadow copy (the SID's registers are write-only),
 * which sid_get_reg reads and which lets the waveform and gate be changed
 * separately. With sid_set_mirror(1), writes to voices 0-5 (SIDs 1-2) are
 * repeated on voices 6-11 (SIDs 3-4), so they play on both sides.
 *
 *   sid_init();
 *   sid_set_instrument(0, &piano);
 *   sid_note_on(0, 60);          // middle C
 *   ...
 *   sid_note_off(0);             // release
 *
 * Frequencies follow the SID clock of the video standard (PAL 985248 Hz,
 * NTSC 1022727 Hz, $D06F bit 7).
 */
#pragma once

#define SID_VOICES      12
#define SID_CHIPS       4

/* Control register bits (as in <mega65.h>) */
#ifndef SID_GATE
#define SID_GATE        0x01    /* gate: attack-decay-sustain while set, release when cleared */
#define SID_SYNC        0x02    /* hard sync with the previous voice */
#define SID_RINGMOD     0x04    /* ring modulation with the previous voice */
#define SID_TEST        0x08    /* test bit: resets the oscillator */
#define SID_TRIANGLE    0x10
#define SID_SAWTOOTH    0x20
#define SID_PULSE       0x40
#define SID_NOISE       0x80
#endif

/* Filter modes (sid_set_filter) */
#define SID_FILTER_LP   0x10    /* low pass */
#define SID_FILTER_BP   0x20    /* band pass */
#define SID_FILTER_HP   0x40    /* high pass */
#define SID_FILTER_3OFF 0x80    /* voice 3 of the chip silent (for modulation) */

/* Voice registers (sid_write) */
#define SID_REG_FREQ_LO 0
#define SID_REG_FREQ_HI 1
#define SID_REG_PW_LO   2
#define SID_REG_PW_HI   3
#define SID_REG_CTRL    4
#define SID_REG_AD      5
#define SID_REG_SR      6

/* A sound: waveform (SID_PULSE...), attack/decay and sustain/release
 * nibble pairs, pulse width (0-4095) */
typedef struct {
    unsigned char wave;
    unsigned char ad;
    unsigned char sr;
    unsigned int pw;
} sid_instrument_t;

/* All four SIDs silent (gates off, registers 0), volume 15, no mirroring */
void sid_init(void);

/* Writes to voices 0-5 also go to voices 6-11 (on: 1) */
void sid_set_mirror(unsigned char on);

/* Register reg (0-6) of voice v; register reg (0-$18) of SID chip (0-3) */
void sid_write(unsigned char v, unsigned char reg, unsigned char val);
void sid_write_chip(unsigned char chip, unsigned char reg, unsigned char val);
/* The value last written to register reg (0-$18) of SID chip (0-3) */
unsigned char sid_get_reg(unsigned char chip, unsigned char reg);

/* Frequency register values: of a frequency in Hz (up to about 3800 Hz),
 * of a MIDI note (69 = A4 = 440 Hz; 0-106) */
unsigned int sid_hz_to_reg(unsigned int hz);
unsigned int sid_note_to_reg(unsigned char note);

void sid_set_freq_reg(unsigned char v, unsigned int reg);
void sid_set_freq(unsigned char v, unsigned int hz);
void sid_set_note(unsigned char v, unsigned char note);

/* Waveform bits (SID_PULSE | SID_RINGMOD...); the gate stays as it is */
void sid_set_waveform(unsigned char v, unsigned char wave);
/* Pulse width 0-4095 (2048: square) */
void sid_set_pulse(unsigned char v, unsigned int width);
/* Envelope, each 0-15 */
void sid_set_adsr(unsigned char v, unsigned char attack, unsigned char decay,
                  unsigned char sustain, unsigned char release);
/* Waveform, envelope and pulse width of an instrument */
void sid_set_instrument(unsigned char v, const sid_instrument_t *inst);

/* Gate on (1: attack) or off (0: release) */
void sid_gate(unsigned char v, unsigned char on);
/* The note (MIDI) with the envelope restarted (gate off, then on) */
void sid_note_on(unsigned char v, unsigned char note);
/* Release the note */
void sid_note_off(unsigned char v);
/* Instrument, note and a restarted envelope */
void sid_play(unsigned char v, const sid_instrument_t *inst, unsigned char note);
/* Every voice released */
void sid_all_off(void);

/* Master volume 0-15 of all four SIDs (the filter mode bits are kept) */
void sid_set_volume(unsigned char volume);
/* A SID's filter: cutoff 0-2047, resonance 0-15, mode (SID_FILTER_LP...),
 * the voices it filters (bits 0-2) */
void sid_set_filter(unsigned char chip, unsigned int cutoff, unsigned char resonance,
                    unsigned char mode, unsigned char voices);
