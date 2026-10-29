/* trace_xemu.c — the tune player's registers frame by frame, for
 * test_xemu.sh to compare with tune45's model (tune45 trace): SID 1's
 * registers (voices 0-2 and the filter, 25 bytes) after each of FRAMES
 * frames, at $8000.
 * Built with -DTUNE=<its index> against music.h / music.c (tune45 convert);
 * -DSEEK=<ticks>: tune_seek first. -DDIGI: digi voices from channel 2, the
 * samples at $40000 (the audio DMA's registers into RAM too): channels 2
 * and 3's 32 bytes, $D711, $D71E-$D71F after the SID's, 60 bytes a frame. */
#include <audio_tune.h>
#include <audio_sid.h>
#include "music.h"

#ifndef FRAMES
#define FRAMES 300
#endif

unsigned char __sid_regs[128];   /* (the player writes here, not to $D400) */
#ifdef DIGI
unsigned char __digi_regs[0x50]; /* (and here, not to $D710) */
#endif

int main(void) {
    unsigned char *log = (unsigned char *)0x8000;
    unsigned int f;
    unsigned char r;
    sid_init();
#ifdef DIGI
    tune_samples(0x40000UL);
    tune_digi_channels(2);
#endif
    tune_play(tunes[TUNE]);
#ifdef SEEK
    tune_seek(SEEK);
#endif
    for (f = 0; f < FRAMES; f++) {
        tune_update();
        for (r = 0; r < 25; r++) *log++ = __sid_regs[r];
#ifdef DIGI
        for (r = 0x30; r < 0x50; r++) *log++ = __digi_regs[r];
        *log++ = __digi_regs[1];
        *log++ = __digi_regs[0x0E];
        *log++ = __digi_regs[0x0F];
#endif
    }
    *(volatile unsigned char *)0xC0C0 = 0xAA;
    return 0;
}
