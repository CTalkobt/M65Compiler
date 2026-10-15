/* audio_midi.h — MIDI messages played on the SIDs (lib/stdlib/audio_midi.c)
 *
 * A parser turns a MIDI byte stream into messages (running status,
 * real-time bytes and SysEx skipped); a synth plays the channel messages on
 * a range of SID voices (<audio_sid.h>): voice allocation (a free voice, else
 * the oldest), velocity 0 as note off, the sustain pedal, pitch bend
 * (+-2 semitones), program change (an instrument table), channel volume
 * (the SID master volume) and all notes off.
 *
 *   midi_parser_t p;  midi_synth_t s;  midi_msg_t m;
 *   sid_init();
 *   midi_parser_init(&p);
 *   midi_synth_init(&s, 0, 6, &piano);       // voices 0-5
 *   for each byte b: if (midi_parse(&p, b, &m)) midi_synth_message(&s, &m);
 */
#pragma once

#include <audio_sid.h>

/* Status bytes (high nybble; the low one is the channel) */
#define MIDI_NOTE_OFF           0x80
#define MIDI_NOTE_ON            0x90
#define MIDI_POLY_PRESSURE      0xA0
#define MIDI_CONTROL_CHANGE     0xB0
#define MIDI_PROGRAM_CHANGE     0xC0
#define MIDI_CHANNEL_PRESSURE   0xD0
#define MIDI_PITCH_BEND         0xE0

/* Controllers the synth uses */
#define MIDI_CC_VOLUME          7
#define MIDI_CC_SUSTAIN         64
#define MIDI_CC_ALL_SOUND_OFF   120
#define MIDI_CC_ALL_NOTES_OFF   123

/* A channel message */
typedef struct {
    unsigned char status;   /* type | channel */
    unsigned char data1;    /* note, controller, program... */
    unsigned char data2;    /* velocity, value (0 for 1-byte messages) */
} midi_msg_t;

#define MIDI_TYPE(m)    ((m)->status & 0xF0)
#define MIDI_CHANNEL(m) ((m)->status & 0x0F)

typedef struct {
    unsigned char running;  /* running status, 0 = none */
    unsigned char data[2];
    unsigned char have;     /* data bytes so far */
    unsigned char sysex;    /* inside a SysEx message */
} midi_parser_t;

void midi_parser_init(midi_parser_t *p);
/* Feed one byte; 1 when *msg holds a complete channel message */
unsigned char midi_parse(midi_parser_t *p, unsigned char b, midi_msg_t *msg);

#define MIDI_SYNTH_VOICES 12

typedef struct {
    unsigned char first_voice;              /* SID voices used */
    unsigned char num_voices;
    unsigned char note[MIDI_SYNTH_VOICES];  /* per voice: its note, */
    unsigned char channel[MIDI_SYNTH_VOICES];   /* channel, */
    unsigned char state[MIDI_SYNTH_VOICES];     /* 0 free, 1 playing, 2 held by the pedal */
    unsigned char age[MIDI_SYNTH_VOICES];   /* when it started (oldest is stolen) */
    unsigned char clock;
    unsigned char sustain[16];              /* pedal down per channel */
    int bend[16];                           /* -8192..8191 per channel */
    const sid_instrument_t *inst[16];       /* instrument per channel */
    const sid_instrument_t *programs;       /* program change table (or NULL) */
    unsigned char num_programs;
} midi_synth_t;

/* Voices first_voice .. first_voice + num_voices - 1, every channel playing
 * inst */
void midi_synth_init(midi_synth_t *s, unsigned char first_voice, unsigned char num_voices,
                     const sid_instrument_t *inst);
/* The instrument of a channel */
void midi_synth_set_instrument(midi_synth_t *s, unsigned char channel, const sid_instrument_t *inst);
/* Program change n selects programs[n % count] */
void midi_synth_set_programs(midi_synth_t *s, const sid_instrument_t *programs, unsigned char count);
/* Play a message */
void midi_synth_message(midi_synth_t *s, const midi_msg_t *m);
/* Voices sounding (playing or held by the pedal) */
unsigned char midi_synth_active(const midi_synth_t *s);
/* Release every note */
void midi_synth_all_off(midi_synth_t *s);

/* The frequency in Hz of a MIDI note (69 = A4 = 440 Hz), rounded */
unsigned int midi_note_to_hz(unsigned char note);
