/* audio_sid.h — MEGA65 SID Audio Chip (Phase 10)
 *
 * Sound synthesis system with oscillators, ADSR envelopes, and filters.
 * Full struct method API for synthesizer control.
 */

#pragma once

/* ============================================================================
 * SID AUDIO DEFINITIONS
 * ============================================================================ */

/**
 * Oscillator waveform types
 */
typedef enum {
    SID_WAVE_TRIANGLE,      /* Triangle wave */
    SID_WAVE_SAWTOOTH,      /* Sawtooth wave */
    SID_WAVE_PULSE,         /* Pulse/square wave (PWM) */
    SID_WAVE_NOISE          /* Pseudo-random noise */
} sid_waveform_t;

/**
 * Filter type
 */
typedef enum {
    SID_FILTER_LOWPASS,     /* Low-pass filter */
    SID_FILTER_BANDPASS,    /* Band-pass filter */
    SID_FILTER_HIGHPASS,    /* High-pass filter */
    SID_FILTER_NOTCH        /* Notch/band-reject filter */
} sid_filter_type_t;

/**
 * ADSR Envelope state
 */
typedef enum {
    ADSR_ATTACK,            /* Attack phase */
    ADSR_DECAY,             /* Decay phase */
    ADSR_SUSTAIN,           /* Sustain phase */
    ADSR_RELEASE            /* Release phase */
} adsr_phase_t;

/**
 * ADSR Envelope - Attack, Decay, Sustain, Release
 *
 * Controls amplitude envelope for dynamic sound shaping.
 */
struct sid_adsr {
    int attack;             /* Attack time (0-255, ms) */
    int decay;              /* Decay time (0-255, ms) */
    int sustain;            /* Sustain level (0-255) */
    int release;            /* Release time (0-255, ms) */

    /* State tracking */
    adsr_phase_t phase;     /* Current ADSR phase */
    int elapsed;            /* Elapsed time in current phase */
    int current_level;      /* Current amplitude (0-255) */
    int active;             /* 1 = envelope is running */

    /* Methods */
    void (*note_on)(struct sid_adsr *this);
    void (*note_off)(struct sid_adsr *this);
    void (*update)(struct sid_adsr *this);
    int (*get_level)(struct sid_adsr *this);
};

typedef struct sid_adsr sid_adsr_t;

/**
 * Oscillator - generates waveforms
 *
 * Produces different waveforms at specified frequency.
 */
struct sid_oscillator {
    int frequency;          /* Frequency in Hz (0-20000) */
    int phase;              /* Current phase accumulator */
    sid_waveform_t waveform;/* Waveform type */
    int pulse_width;        /* Pulse width for PWM (0-255) */
    int detune;             /* Detune amount (-100 to +100 cents) */

    /* Methods */
    void (*set_frequency)(struct sid_oscillator *this, int freq);
    void (*set_waveform)(struct sid_oscillator *this, sid_waveform_t wave);
    void (*set_pulse_width)(struct sid_oscillator *this, int width);
    int (*generate_sample)(struct sid_oscillator *this);
};

typedef struct sid_oscillator sid_oscillator_t;

/**
 * Filter - resonant filter for timbre shaping
 *
 * Applies low-pass, high-pass, band-pass, or notch filtering.
 */
struct sid_filter {
    sid_filter_type_t type; /* Filter type */
    int cutoff;             /* Cutoff frequency (0-20000 Hz) */
    int resonance;          /* Resonance/Q factor (0-255) */
    int output;             /* Filtered output sample */

    /* Filter state (IIR coefficients) */
    int sample_history[4];  /* Previous input/output samples */

    /* Methods */
    void (*set_cutoff)(struct sid_filter *this, int freq);
    void (*set_resonance)(struct sid_filter *this, int q);
    void (*set_type)(struct sid_filter *this, sid_filter_type_t type);
    int (*apply)(struct sid_filter *this, int sample);
};

typedef struct sid_filter sid_filter_t;

/**
 * SID Voice - single polyphonic voice
 *
 * Combines oscillator, envelope, and filter for one note.
 */
struct sid_voice {
    int note;               /* MIDI note (0-127) */
    int velocity;           /* Note velocity (0-127) */
    int active;             /* 1 = voice is playing */

    /* Components */
    sid_oscillator_t osc;   /* Oscillator */
    sid_adsr_t envelope;    /* ADSR envelope */
    sid_filter_t filter;    /* Filter */

    /* Modulation */
    int portamento;         /* Pitch slide time (ms) */
    int target_frequency;   /* Target frequency for portamento */
    int vibrato_depth;      /* Vibrato depth (0-100 cents) */
    int vibrato_rate;       /* Vibrato rate (Hz) */

    /* Methods */
    void (*note_on)(struct sid_voice *this, int note, int velocity);
    void (*note_off)(struct sid_voice *this);
    void (*update)(struct sid_voice *this);
    int (*get_sample)(struct sid_voice *this);
    void (*set_vibrato)(struct sid_voice *this, int depth, int rate);
    void (*set_portamento)(struct sid_voice *this, int time);
};

typedef struct sid_voice sid_voice_t;

/**
 * SID Synthesizer - full polyphonic synthesizer
 *
 * Manages up to 3 voices (MEGA65 has 3 SID channels like C64).
 * Provides full synthesis control with effects.
 */
struct sid_synth {
    /* Polyphonic voices (up to 3) */
    sid_voice_t voice[3];   /* Three independent voices */
    int active_voices;      /* Number of currently playing voices */

    /* Master controls */
    int volume;             /* Master volume (0-255) */
    int master_tuning;      /* Master pitch offset (cents) */
    int glide_time;         /* Legato glide time (ms) */

    /* Global effects */
    int reverb_level;       /* Reverb amount (0-255) */
    int chorus_level;       /* Chorus amount (0-255) */
    int delay_time;         /* Delay time (0-1000 ms) */
    int delay_feedback;     /* Delay feedback (0-255) */

    /* State tracking */
    int sample_rate;        /* Sample rate in Hz */
    int frame_count;        /* Frames generated */

    /* Methods */
    void (*init)(struct sid_synth *this);
    void (*done)(struct sid_synth *this);

    /* Voices */
    void (*note_on)(struct sid_synth *this, int voice, int note, int velocity);
    void (*note_off)(struct sid_synth *this, int voice);
    void (*all_notes_off)(struct sid_synth *this);

    /* Synthesis */
    int (*get_sample)(struct sid_synth *this);
    void (*update)(struct sid_synth *this);

    /* Controls */
    void (*set_volume)(struct sid_synth *this, int volume);
    int (*get_volume)(struct sid_synth *this);
    void (*set_vibrato)(struct sid_synth *this, int voice, int depth, int rate);
    void (*set_portamento)(struct sid_synth *this, int time);

    /* Effects */
    void (*set_reverb)(struct sid_synth *this, int level);
    void (*set_chorus)(struct sid_synth *this, int level);
    void (*set_delay)(struct sid_synth *this, int time, int feedback);

    /* Filter */
    void (*set_cutoff)(struct sid_synth *this, int voice, int freq);
    void (*set_resonance)(struct sid_synth *this, int voice, int q);
};

typedef struct sid_synth sid_synth_t;

/* ============================================================================
 * SID SYNTHESIZER API FUNCTIONS
 * ============================================================================ */

/**
 * Create and initialize SID synthesizer
 *
 * Returns:
 *   Initialized sid_synth_t with all method pointers set
 *
 * Example:
 *   sid_synth_t synth = sid_synth_create();
 *   synth.init(&synth);
 *   synth.note_on(&synth, 0, 60, 127);  // Play middle C on voice 0
 */
sid_synth_t sid_synth_create(void);

/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

/* Volume levels */
#define SID_VOLUME_MIN          0       /* Silent */
#define SID_VOLUME_MAX          255     /* Maximum */
#define SID_VOLUME_DEFAULT      200     /* Default level */

/* Frequency range (Hz) */
#define SID_FREQ_MIN            20      /* Lowest frequency */
#define SID_FREQ_MAX            20000   /* Highest frequency */
#define SID_FREQ_A4             440     /* A4 tuning frequency */

/* Time ranges (milliseconds) */
#define SID_TIME_MIN            0       /* Immediate */
#define SID_TIME_MAX            5000    /* 5 seconds */

/* Note constants (MIDI) */
#define SID_NOTE_C0             0       /* Lowest MIDI note */
#define SID_NOTE_C4             60      /* Middle C */
#define SID_NOTE_A4             69      /* A4 (440 Hz) */
#define SID_NOTE_C8             108     /* Highest reasonable note */

/* Voice indices */
#define SID_VOICE_1             0       /* First voice */
#define SID_VOICE_2             1       /* Second voice */
#define SID_VOICE_3             2       /* Third voice */
#define SID_VOICES_MAX          3       /* Maximum polyphony */
