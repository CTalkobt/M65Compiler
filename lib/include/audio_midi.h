/* audio_midi.h — MEGA65 MIDI Input Handler (Phase 11)
 *
 * MIDI message parsing and SID synthesizer control.
 * Converts MIDI note/control messages to synthesis parameters.
 */

#pragma once

#include <audio_sid.h>

/* ============================================================================
 * MIDI MESSAGE TYPES
 * ============================================================================ */

/**
 * MIDI message status bytes
 */
typedef enum {
    MIDI_NOTE_OFF = 0x80,           /* Note off (2 bytes) */
    MIDI_NOTE_ON = 0x90,            /* Note on (2 bytes) */
    MIDI_POLYTOUCH = 0xA0,          /* Polyphonic key pressure (2 bytes) */
    MIDI_CONTROL_CHANGE = 0xB0,     /* Control change (2 bytes) */
    MIDI_PROGRAM_CHANGE = 0xC0,     /* Program change (1 byte) */
    MIDI_CHANNEL_PRESSURE = 0xD0,   /* Channel pressure (1 byte) */
    MIDI_PITCH_BEND = 0xE0,         /* Pitch bend (2 bytes) */
    MIDI_SYSTEM_EXCLUSIVE = 0xF0    /* System exclusive */
} midi_status_t;

/**
 * Standard MIDI Control Change (CC) messages
 */
typedef enum {
    MIDI_CC_MODULATION = 1,         /* Vibrato depth */
    MIDI_CC_BREATH = 2,             /* Breath controller */
    MIDI_CC_FOOT = 4,               /* Foot pedal */
    MIDI_CC_PORTAMENTO_TIME = 5,    /* Portamento/glide time */
    MIDI_CC_DATA_ENTRY = 6,         /* Data entry slider */
    MIDI_CC_CHANNEL_VOLUME = 7,     /* Channel volume */
    MIDI_CC_BALANCE = 8,            /* Balance */
    MIDI_CC_PAN = 10,               /* Stereo pan */
    MIDI_CC_EXPRESSION = 11,        /* Expression */
    MIDI_CC_EFFECT1 = 12,           /* Effect control 1 */
    MIDI_CC_EFFECT2 = 13,           /* Effect control 2 */
    MIDI_CC_SUSTAIN = 64,           /* Sustain pedal */
    MIDI_CC_FILTER_CUTOFF = 74,     /* Filter cutoff frequency */
    MIDI_CC_FILTER_RESONANCE = 71,  /* Filter resonance/Q */
    MIDI_CC_REVERB_SEND = 91,       /* Reverb send level */
    MIDI_CC_CHORUS_SEND = 93,       /* Chorus send level */
    MIDI_CC_ALL_NOTES_OFF = 123,    /* All notes off */
    MIDI_CC_OMNI_OFF = 124,         /* Omni mode off */
    MIDI_CC_OMNI_ON = 125,          /* Omni mode on */
    MIDI_CC_MONO_MODE = 126         /* Mono mode */
} midi_cc_t;

/* ============================================================================
 * MIDI INPUT HANDLER
 * ============================================================================ */

/**
 * MIDI input state machine
 *
 * Parses incoming MIDI bytes and generates complete messages.
 */
struct midi_input {
    /* Input state */
    int midi_fd;                    /* MIDI input device file/port */
    unsigned char running_status;   /* Running status byte (for compression) */
    unsigned char message_buffer[3];/* Message being accumulated */
    int buffer_index;               /* Current position in buffer */
    int expected_length;            /* Expected message length */

    /* Receiving */
    int message_ready;              /* 1 = complete message in buffer */
    unsigned char last_channel;     /* Last MIDI channel (0-15) */

    /* Methods */
    void (*init)(struct midi_input *this);
    void (*done)(struct midi_input *this);
    int (*read_byte)(struct midi_input *this);
    int (*has_message)(struct midi_input *this);
    void (*clear_message)(struct midi_input *this);
};

typedef struct midi_input midi_input_t;

/* ============================================================================
 * MIDI-TO-SID VOICE ROUTER
 * ============================================================================ */

/**
 * MIDI voice allocation - tracks which MIDI channel uses which SID voice
 */
struct midi_voice_alloc {
    int sid_voice;                  /* Allocated SID voice (0-2) */
    int midi_channel;               /* MIDI channel (0-15) */
    int active;                     /* 1 = currently playing a note */
    int current_note;               /* Current note being played */
};

/**
 * MIDI to SID synthesizer bridge
 *
 * Routes MIDI messages to SID voices (up to 12 voices across 4 SID chips).
 * with parameter mapping.
 */
struct midi_to_sid {
    /* Synthesizer reference */
    sid_synth_t *synth;             /* Pointer to SID synthesizer */

    /* Voice allocation (up to 12 voices) */
    struct midi_voice_alloc voice_alloc[12];

    /* MIDI settings */
    int omni_mode;                  /* 1 = omni mode (all channels) */
    int mono_mode;                  /* 1 = monophonic mode */
    int sustain_pedal;              /* Sustain pedal state (0-127) */

    /* Per-channel parameters */
    int channel_volume[16];         /* Channel volume (0-127) */
    int channel_pan[16];            /* Channel pan (0-127) */
    int channel_portamento[16];     /* Portamento time per channel (ms) */
    int channel_vibrato_depth[16];  /* Vibrato depth per channel */
    int channel_vibrato_rate[16];   /* Vibrato rate per channel */

    /* Filter parameters */
    int filter_cutoff[12];          /* Per-voice filter cutoff */
    int filter_resonance[12];       /* Per-voice filter resonance */
    int num_voices;                 /* Total voices available (3-12) */

    /* Effect levels */
    int reverb_send;                /* Reverb send level (0-127) */
    int chorus_send;                /* Chorus send level (0-127) */
    int delay_time;                 /* Delay time (0-1000ms) */

    /* Methods */
    void (*init)(struct midi_to_sid *this, sid_synth_t *synth);
    void (*done)(struct midi_to_sid *this);

    /* MIDI message handling */
    void (*note_on)(struct midi_to_sid *this, int channel, int note, int velocity);
    void (*note_off)(struct midi_to_sid *this, int channel, int note);
    void (*control_change)(struct midi_to_sid *this, int channel, int cc, int value);
    void (*pitch_bend)(struct midi_to_sid *this, int channel, int bend);
    void (*program_change)(struct midi_to_sid *this, int channel, int program);
    void (*channel_pressure)(struct midi_to_sid *this, int channel, int pressure);

    /* Utilities */
    void (*set_omni_mode)(struct midi_to_sid *this, int enabled);
    void (*set_mono_mode)(struct midi_to_sid *this, int enabled);
    void (*reset_all_controllers)(struct midi_to_sid *this);
    int (*get_active_voices)(struct midi_to_sid *this);
};

typedef struct midi_to_sid midi_to_sid_t;

/* ============================================================================
 * MIDI RECEIVER - High-level interface combining input + router
 * ============================================================================ */

/**
 * Complete MIDI receiver system - input + SID control
 */
struct midi_receiver {
    /* Components */
    midi_input_t input;             /* MIDI input parser */
    midi_to_sid_t router;           /* MIDI-to-SID voice router */
    sid_synth_t *synth;             /* Reference to SID synth */

    /* Processing */
    int running;                    /* 1 = receiver is active */
    int messages_received;          /* Message counter */

    /* Methods */
    void (*init)(struct midi_receiver *this, sid_synth_t *synth);
    void (*done)(struct midi_receiver *this);
    void (*process)(struct midi_receiver *this);
    void (*handle_message)(struct midi_receiver *this,
                          unsigned char status,
                          unsigned char byte1,
                          unsigned char byte2);
};

typedef struct midi_receiver midi_receiver_t;

/* ============================================================================
 * MIDI RECEIVER API FUNCTIONS
 * ============================================================================ */

/**
 * Create MIDI receiver connected to SID synthesizer
 *
 * Returns:
 *   Initialized midi_receiver_t ready for MIDI message processing
 *
 * Example:
 *   midi_receiver_t receiver = midi_receiver_create(&synth);
 *   receiver.init(&receiver, &synth);
 *   while (receiver.running) {
 *       receiver.process(&receiver);
 *   }
 */
midi_receiver_t midi_receiver_create(void);

/* ============================================================================
 * MIDI NOTE CONVERSION
 * ============================================================================ */

/**
 * Convert MIDI note number to frequency in Hz
 *
 * Parameters:
 *   note — MIDI note (0-127)
 *
 * Returns:
 *   Frequency in Hz
 */
int midi_note_to_frequency(int note);

/**
 * Convert frequency in Hz to MIDI note number
 *
 * Parameters:
 *   freq — Frequency in Hz
 *
 * Returns:
 *   MIDI note (0-127)
 */
int frequency_to_midi_note(int freq);

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

/* MIDI byte ranges */
#define MIDI_NOTE_MIN               0       /* Lowest MIDI note */
#define MIDI_NOTE_MAX               127     /* Highest MIDI note */
#define MIDI_VELOCITY_MIN           0       /* No velocity */
#define MIDI_VELOCITY_MAX           127     /* Maximum velocity */
#define MIDI_CC_MIN                 0       /* Minimum CC value */
#define MIDI_CC_MAX                 127     /* Maximum CC value */

/* MIDI channels */
#define MIDI_CHANNEL_1              0       /* First MIDI channel */
#define MIDI_CHANNEL_16             15      /* Last MIDI channel */
#define MIDI_CHANNELS               16      /* Total channels */

/* Voice allocation strategies */
#define MIDI_ALLOC_ROUND_ROBIN      0       /* Round-robin voice allocation */
#define MIDI_ALLOC_CHANNEL_TO_VOICE 1       /* Map MIDI channels to voices */
#define MIDI_ALLOC_LOWEST_NOTE      2       /* Allocate to lowest note voice */

/* Default values */
#define MIDI_DEFAULT_VOLUME         100     /* Default MIDI volume */
#define MIDI_DEFAULT_PAN            64      /* Center pan */
#define MIDI_DEFAULT_PORTAMENTO     0       /* No glide */
#define MIDI_DEFAULT_REVERB         50      /* Default reverb send */
