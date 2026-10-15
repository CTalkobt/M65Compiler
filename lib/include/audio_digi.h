/* audio_digi.h — sample playback through the MEGA65's audio DMA
 * (lib/stdlib/audio_digi.c)
 *
 * Four channels ($D720, $D730, $D740, $D750): 0 and 1 play on the left,
 * 2 and 3 on the right (each also has a volume for the other side). A
 * channel reads its sample from chip RAM by DMA while the program runs (the
 * CPU must be at full speed, 40 MHz); a sample is at most 64 KB.
 *
 *   digi_init();
 *   digi_play(0, sample, sizeof sample, 8000, 255, 0);   // 8 kHz, once
 *   while (digi_playing(0)) ;
 *
 * Samples are 8-bit unsigned unless the flags say otherwise.
 */
#pragma once

#define DIGI_CHANNELS   4

/* digi_play flags */
#define DIGI_LOOP       0x40    /* repeat from the start */
#define DIGI_SIGNED     0x01    /* signed samples (default unsigned) */
#define DIGI_16BIT      0x02    /* 16-bit samples (little endian) */
#define DIGI_NYBBLE_LO  0x04    /* 4-bit samples in the low nybbles */
#define DIGI_NYBBLE_HI  0x08    /* 4-bit samples in the high nybbles */

/* Audio DMA on (it then feeds the audio mixer), all channels stopped */
void digi_init(void);
/* Audio DMA off */
void digi_done(void);

/* The rate register value of a sample rate in Hz (rate * 2^24 / 40.5 MHz) */
unsigned long digi_rate_to_reg(unsigned int hz);

/* Play len bytes from sample (in bank 0) / from a 24-bit chip RAM address on
 * channel ch at rate Hz, volume 0-255 (the other side silent) */
void digi_play(unsigned char ch, const void *sample, unsigned int len, unsigned int rate,
               unsigned char volume, unsigned char flags);
void digi_play_far(unsigned char ch, unsigned long addr, unsigned int len, unsigned int rate,
                   unsigned char volume, unsigned char flags);

/* Stop the channel */
void digi_stop(unsigned char ch);
/* 1 while the channel plays (enabled and not at the end of its sample) */
unsigned char digi_playing(unsigned char ch);

/* Volume 0-255 on the channel's side and on the other side (panning) */
void digi_set_volume(unsigned char ch, unsigned char volume, unsigned char other_side);
/* A new rate (Hz) while it plays */
void digi_set_rate(unsigned char ch, unsigned int rate);
/* The address the channel reads next (24 bits) */
unsigned long digi_position(unsigned char ch);
