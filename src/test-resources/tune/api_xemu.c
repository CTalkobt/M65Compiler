/* api_xemu.c — the tune player's controls for a game, in xemu
 * (test_xemu.sh): a byte a check from $C0C0 (01 good), $AA after them.
 *   tune_volume: the four SIDs' volume, the digi voices' (17 a step)
 *   tune_fade: a step a frame to its end, tune_fading
 *   tune_borrow: the voice's registers left alone, then the music's again
 *   tune_queue: the next tune at the bar line, not before
 * The registers go to RAM (__sid_regs, __digi_regs). */
#include <audio_tune.h>
#include <audio_sid.h>
#include "music.h"

unsigned char __sid_regs[128];
unsigned char __digi_regs[0x50];

static unsigned char *res = (unsigned char *)0xC0C0;
static void check(unsigned char ok) { *res++ = ok ? 1 : 0; }
static void frames(unsigned char n) { while (n--) tune_update(); }

int main(void) {
    unsigned char i, held;
    sid_init();
    tune_samples(0x40000UL);
    tune_digi_channels(2);
    tune_play(tunes[TUNE_A]);
    frames(1);
    /* the volume: SIDs 1 and 2 at 5; the digi voice's 255 * 85 / 256 */
    tune_volume(5);
    check((__sid_regs[0x18] & 15) == 5 && (__sid_regs[0x38] & 15) == 5 && __digi_regs[0x30 + 9] == 84);
    /* a fade to 15 over 10 frames: 10 after 5 (a step a frame), 15 at the end */
    tune_fade(10, 15);
    frames(5);
    check((__sid_regs[0x18] & 15) == 10 && tune_fading());
    frames(5);
    check((__sid_regs[0x18] & 15) == 15 && !tune_fading() && __digi_regs[0x30 + 9] == 255);
    /* voice 0 lent for 3 frames: its frequency left as the program set it
     * (the music changes its note meanwhile), then the music's again */
    tune_borrow(0, 3);
    __sid_regs[1] = 0x77;
    held = 1;
    for (i = 0; i < 3; i++) { tune_update(); if (__sid_regs[1] != 0x77) held = 0; }
    check(held);
    frames(4);
    check(__sid_regs[1] != 0x77);
    /* queued at 2 ticks past a bar line: the bar's last 2 ticks, then b */
    tune_play(tunes[TUNE_A]);
    frames(6);   /* (a tick an update, the first on the first) */
    check(tune_ticks() == 6);
    tune_queue(tunes[TUNE_B], TUNE_A_BAR);
    frames(2);
    check(tune_queued() && tune_ticks() == 8);
    frames(1);
    check(!tune_queued() && tune_ticks() == 0);
    frames(1);
    check(tune_note(0) == 69);
    *res = 0xAA;
    return 0;
}
