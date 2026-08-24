/* audio_digi.h — MEGA65 DIGI Audio Chip (Phase 9)
 *
 * Digital PCM audio playback system with struct method API.
 * Supports 8-bit PCM samples with volume, frequency, and mixing.
 */

#pragma once

/* ============================================================================
 * DIGI AUDIO DEFINITIONS
 * ============================================================================ */

/**
 * PCM sample format
 */
typedef enum {
    DIGI_FORMAT_UNSIGNED_8BIT,  /* Unsigned 8-bit (0-255, midpoint 0x80) */
    DIGI_FORMAT_SIGNED_8BIT     /* Signed 8-bit (-128 to 127, midpoint 0x00) */
} digi_format_t;

/**
 * DIGI sample state and methods
 *
 * Manages individual PCM samples with creation, destruction, and manipulation.
 */
typedef struct digi_sample {
    unsigned char *data;        /* Sample buffer pointer */
    int length;                 /* Sample length in bytes */
    int position;               /* Current playback position */
    int volume;                 /* Volume level (0-255) */
    int frequency;              /* Sample rate in Hz */
    digi_format_t format;       /* Sample format */
    int looping;                /* 1 = loop, 0 = play once */
    int playing;                /* 1 = currently playing, 0 = stopped */

    /* Methods */
    void (*destroy)(struct digi_sample *this);
    void (*silence)(struct digi_sample *this, digi_format_t format);
    void (*apply_volume)(struct digi_sample *this, int volume, digi_format_t format);
    void (*reverse)(struct digi_sample *this);

} digi_sample_t;

/**
 * DIGI playback system and methods
 *
 * Manages audio playback, volume, frequency, and sample lifecycle.
 */
typedef struct digi_system {
    /* State */
    int volume;                 /* Master volume (0-255) */
    int frequency;              /* Current playback frequency */
    int playing;                /* 1 = playback active */
    int paused;                 /* 1 = paused, 0 = running or stopped */

    unsigned char *current_sample;      /* Current sample data */
    int current_sample_length;          /* Current sample length */
    int current_position;               /* Current playback position */
    int current_frequency;              /* Current sample frequency */
    digi_format_t current_format;       /* Current sample format */
    int current_looping;                /* Current looping flag */

    /* Methods */
    void (*init)(struct digi_system *this);
    void (*done)(struct digi_system *this);

    int (*play_sample)(struct digi_system *this, unsigned char *sample, int length,
                       int frequency, digi_format_t format, int looping);
    void (*stop)(struct digi_system *this);
    void (*pause)(struct digi_system *this);
    void (*resume)(struct digi_system *this);

    int (*is_playing)(struct digi_system *this);
    int (*get_position)(struct digi_system *this);
    int (*seek)(struct digi_system *this, int position);

    void (*set_volume)(struct digi_system *this, int volume);
    int (*get_volume)(struct digi_system *this);
    void (*set_frequency)(struct digi_system *this, int frequency);
    int (*get_frequency)(struct digi_system *this);

    void (*update)(struct digi_system *this);

} digi_system_t;

/* ============================================================================
 * DIGI SYSTEM API FUNCTIONS
 * ============================================================================ */

/**
 * Create and initialize DIGI playback system
 *
 * Returns:
 *   Initialized digi_system_t with method pointers set
 *
 * Example:
 *   digi_system_t digi = digi_system_create();
 *   digi.init(&digi);
 */
digi_system_t digi_system_create(void);

/**
 * Create sample buffer
 *
 * Parameters:
 *   length — Sample buffer size in bytes
 *
 * Returns:
 *   Allocated and initialized digi_sample_t, or NULL on failure
 *
 * Example:
 *   digi_sample_t *sample = digi_sample_create(4410);  // 100ms at 44.1kHz
 */
digi_sample_t *digi_sample_create(int length);

/**
 * Mix two samples (static utility)
 *
 * Parameters:
 *   dest — Destination buffer
 *   src1 — First source sample
 *   src2 — Second source sample
 *   length — Number of bytes to mix
 *   mix_level — Blend factor (0-255): 0=src2 only, 255=src1 only
 */
void digi_sample_mix_static(unsigned char *dest,
                            unsigned char *src1, unsigned char *src2,
                            int length, int mix_level);


/* ============================================================================
 * CONSTANTS
 * ============================================================================ */

#define DIGI_VOLUME_MIN         0       /* Minimum volume (silent) */
#define DIGI_VOLUME_MAX         255     /* Maximum volume */
#define DIGI_VOLUME_DEFAULT     200     /* Default volume level */

#define DIGI_FREQ_8000          8000    /* 8 kHz (low quality, small) */
#define DIGI_FREQ_11025         11025   /* 11.025 kHz (telephone quality) */
#define DIGI_FREQ_22050         22050   /* 22.05 kHz (CD quality, halved) */
#define DIGI_FREQ_44100         44100   /* 44.1 kHz (CD quality) */
#define DIGI_FREQ_DEFAULT       22050   /* Default frequency */

#define DIGI_MAX_SAMPLE_SIZE    (256 * 1024)  /* 256KB max sample size */
