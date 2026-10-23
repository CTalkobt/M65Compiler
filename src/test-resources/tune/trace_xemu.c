/* trace_xemu.c — the tune player's registers frame by frame, for
 * test_xemu.sh to compare with tune45's model (tune45 trace): voices 0-2's
 * registers (SID 1, 21 bytes) after each of FRAMES frames, at $8000.
 * Built with -DTUNE=<its index> against music.h / music.c (tune45 convert). */
#include <audio_tune.h>
#include <audio_sid.h>
#include "music.h"

#ifndef FRAMES
#define FRAMES 300
#endif

unsigned char __sid_regs[128];   /* (the player writes here, not to $D400) */

int main(void) {
    unsigned char *log = (unsigned char *)0x8000;
    unsigned int f;
    unsigned char r;
    sid_init();
    tune_play(tunes[TUNE]);
    for (f = 0; f < FRAMES; f++) {
        tune_update();
        for (r = 0; r < 21; r++) *log++ = __sid_regs[r];
    }
    *(volatile unsigned char *)0xC0C0 = 0xAA;
    return 0;
}
