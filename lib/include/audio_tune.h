/* audio_tune.h — a tune (song) player: pre-composed music as data, played
 * once a frame on SID voices (lib/stdlib_asm/tune.s45)
 *
 * A tune is the data tune45 makes of a text file (tune45 convert music.txt
 * -o music.c: a const array TUNE_<name> per tune; or --bin, one blob to keep
 * in far memory). Each voice is a byte stream of notes, rests and
 * instruments; a tick is the tune's frames per tick (8.8 fixed point).
 *
 *   tune_play(TUNE_march);          // or tune_play_far(addr)
 *   for (;;) { wait_frame(); tune_update(); }
 *
 * tune_update reads the streams, starts notes (an instrument's envelope,
 * waveform and pulse width; the note's frequency at the PAL or NTSC clock),
 * releases them early by the instrument's staccato gap and runs the
 * per-frame effects: vibrato (after its delay), pulse-width sweep, pitch
 * slide, arpeggio, the instrument's wave table (a waveform and pitch a
 * frame), its hard restart (the gate off and the envelope at its fastest
 * release a frame or few before the next note) and the filter (a filtered
 * instrument's note sets its chip's cutoff, sweep, resonance and mode and
 * routes the voice; the volume bits of $D418 are kept). It is in assembly with no static allocation: it may be
 * called from an __interrupt handler (a raster interrupt, as most games
 * play their music). The registers go through <audio_sid.h>'s shadow copy
 * and stereo mirror (sid_set_mirror), so sound effects on other voices mix
 * in; the voices a tune plays are its own while it plays.
 *
 * Tune voice i plays on SID voice first + i (tune_voices; 0 at first, 12
 * voices: SID 1 voices 0-2, SID 2 3-5, SID 3 6-8, SID 4 9-11). Digi voice i
 * (samples) plays on audio DMA channel first + i (tune_digi_channels).
 *
 * Stream bytes (tune45 writes them; docs: doc/bin/tune45.md):
 *   $00-$7F n d   note n (MIDI, 60 = middle C) for d ticks (d 0 = 256)
 *   $80 d         rest          $81 i   instrument i
 *   $82 d         the note held d ticks more (a tie)
 *   $83 a b       arpeggio: the note, +a, +b semitones a frame each (0, 0 off)
 *   $84 s k       the next note glides: it starts s semitones below its pitch
 *                 (signed) and from its second frame moves (distance +
 *                 2^k - 1) >> k a frame towards it (k 1-7; 0: no glide)
 *   $85 lo hi     the tune's frames per tick from now (8.8; tune45 puts the
 *                 tune's own at voice 0's start, so a loop starts as it did)
 *   $86 lo hi n   a pattern: the stream at offset lo hi, n times (to its
 *                 $87 each time), then on after the call (no calls in one)
 *   $87           a pattern's end
 *   $88 n d       a legato note: the pitch n for d ticks, the note going on
 *   $89 lo hi s   the pulse width and its sweep a frame (signed)
 *   $8A lo hi s   the voice's chip's cutoff (11 bits) and its sweep a frame
 * ($83 a $80: a two-note arpeggio, the note and +a. An instrument's hr byte's
 * bit 7: its effects go on through the release; bit 6: its pulse-width
 * sweep free (no bounce; stopped at 0 and $FFF, as pw:'s). Its vibrato
 * byte's high nibble: frames a step of the triangle, less 1.)
 *   $FE           back to the voice's start (a tune that loops)
 *   $FF           the voice's end
 * A digi voice's: $00-$7F h d  hit h (the tune's sample table) for d
 * ticks; $84 h d  hit h, 0-255; $80 d / $82 d  the sample rings on; $81 d
 * the channel stopped; $83 v  the voice's volume (0-255: times
 * tune_digi_volume's / 256; no time); $86 / $87 as above; $FE, $FF. A sample table entry: its
 * offset and bytes, the rate, and (flags bit 4) its loop's start from it
 * ($FFFF: played once).
 */
#pragma once

/* Start a tune (in bank 0 / anywhere in memory, a 28-bit address); a tune
 * playing stops */
__fastcall__ void tune_play(const unsigned char *tune);
__fastcall__ void tune_play_far(unsigned long addr);

/* Silence the tune's voices (their gates off) */
__fastcall__ void tune_stop(void);

/* Once a frame (50 Hz PAL, 60 Hz NTSC: the tempo is the same in time) */
__fastcall__ void tune_update(void);

/* 1 while playing; 0 once a tune that does not loop is over */
__fastcall__ unsigned char tune_playing(void);

/* Run-time controls (they stay set across tune_play):
 *   tune_voices   the SID voice of tune voice 0 (before tune_play)
 *   tune_tempo    a tempo scale, 8.8: 0x100 as written, 0x140 1.25 times as fast
 *   tune_vibrato  vibrato depth added to every note's (a wobble)
 *   tune_transpose / tune_detune   per tune voice: semitones; frequency +
 *                 amount/64 of itself (a chorus, out of tune) */
__fastcall__ void tune_voices(unsigned char first);
__fastcall__ void tune_tempo(unsigned int scale);
__fastcall__ void tune_vibrato(unsigned char extra);
__fastcall__ void tune_transpose(unsigned char voice, signed char semitones);
__fastcall__ void tune_detune(unsigned char voice, signed char amount);

/* The tune's position: ticks played since tune_play */
__fastcall__ unsigned int tune_ticks(void);
/* On ticks ticks at once, silently (after tune_play: back where a tune was,
 * from tune_ticks; tune45 gives a tune's TUNE_<NAME>_TICKS, once through, to
 * keep a looping tune's position short). The voices sound again from their
 * next events. Interrupts are held off while it runs (about 0.2 ms a
 * thousand ticks of a voice). */
__fastcall__ void tune_seek(unsigned int ticks);

/* The note tune voice voice sounds (MIDI, before its transpose), 0xFF when
 * it is silent (a rest, released, over) */
__fastcall__ unsigned char tune_note(unsigned char voice);

/* Digi voices (tune45's d1: .. d4:): samples on the audio DMA. A hit sets
 * its channel up and starts it (8-bit unsigned, once, at the volume on both
 * sides); the next hit on the channel cuts it short (tune45 mixes samples
 * hit together into one, and warns of tails cut short). The audio DMA
 * reads chip RAM only:
 *   tune_samples        where the tunes' samples are (tune45's
 *                       tune_sample_bank, or its --samples file loaded):
 *                       chip RAM, not across a 64 KB boundary
 *   tune_digi_channels  the audio DMA channel (0-3) of digi voice 0 (before
 *                       tune_play; voices past channel 3 are left out): the
 *                       others stay the program's, for its sound effects
 *   tune_digi_volume    0-255 (255 at first)
 * They stay set across tune_play. */
__fastcall__ void tune_samples(unsigned long addr);
__fastcall__ void tune_digi_channels(unsigned char first);
__fastcall__ void tune_digi_volume(unsigned char volume);

/* For a game: the music's volume, voices lent to the program's sound
 * effects, the next tune at a bar line.
 *   tune_volume   the music's volume 0-15 (15 at first): the SIDs' (their
 *                 $D418 volume, the rest of it kept) and the digi voices'
 *                 (17 a step, times tune_digi_volume's); at once
 *   tune_fade     to a volume over so many frames (tune_update steps it);
 *                 tune_fading: 1 until it gets there. A fade out and the
 *                 next tune: tune_fade(50, 0) ... tune_play ... tune_fade(50, 15)
 *   tune_borrow   a tune voice (0-11) lent to the program for so many
 *                 frames (0: until tune_return): the music goes on but
 *                 writes none of its registers (nor its filter routing), so
 *                 a sound effect can play on it (audio_sid.h); back, it
 *                 sounds again from its next note
 *   tune_queue    the tune started at the playing one's next bar line (its
 *                 ticks a multiple of bar: tune45 gives TUNE_<NAME>_BAR for
 *                 a tune with ticks a bar), or at once if on one or nothing
 *                 plays; tune_queued: 1 while it waits
 * The volume stays set across tune_play. */
__fastcall__ void tune_volume(unsigned char volume);
__fastcall__ void tune_fade(unsigned int frames, unsigned char volume);
__fastcall__ unsigned char tune_fading(void);
__fastcall__ void tune_borrow(unsigned char voice, unsigned int frames);
__fastcall__ void tune_return(unsigned char voice);
__fastcall__ void tune_queue(const unsigned char *tune, unsigned int bar);
__fastcall__ void tune_queue_far(unsigned long addr, unsigned int bar);
__fastcall__ unsigned char tune_queued(void);
