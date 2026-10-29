/* drums — a SID band and a sampled drum kit, played by the library's tune
 * player (<audio_tune.h>)
 *
 * music.txt is the music; tune45 convert makes music.c and music.h of it
 * (the Makefile), the samples too: tune_sample_bank, the kit's hits made
 * 8-bit. The SID voices play a filtered bass (hard restart, a cutoff sweep
 * each note), a lead with a wave table and arpeggio chords; two digi voices
 * play the drums on the audio DMA's channels 2 and 3 (tune_digi_channels),
 * leaving 0 and 1 to the program (its sound effects).
 *
 * The audio DMA reads chip RAM: the samples are in the program (bank 0);
 * tune_samples says where. Here the music is updated in the main loop,
 * once a frame (examples/c/tunes plays it from a raster interrupt).
 *
 * Keys: SPACE stop / play, + - tempo, RUN/STOP quit.
 *
 * Built with -DDRUMS_CHECK (make check): it plays 240 frames and reports
 * the ticks and how often each drum channel was set to another sample (its
 * start address, read back from the audio DMA: always within the samples).
 * (Not digi_playing: xemu leaves a channel's "stopped" bit clear when its
 * sample ends.)
 */
#include <conio.h>
#include <input.h>
#include <audio_sid.h>
#include <audio_digi.h>
#include <audio_tune.h>
#include "music.h"

#define REG(a) (*(volatile unsigned char *)(a))
#define DRUMS 2                         /* the drums' first channel */

static void wait_frame(void) {
    while (REG(0xD012) != 0xF8) ;
    while (REG(0xD012) == 0xF8) ;
}

static const char *const pcs[12] = {"c-", "c#", "d-", "d#", "e-", "f-", "f#", "g-", "g#", "a-", "a#", "b-"};
static const char *const parts[3] = {"bass (filter, hard restart)", "lead (wave table)", "chords (arpeggio)"};

/* channel ch's sample: its start address ($D721 + ch * 16), from the
 * samples' start */
static unsigned int sample_at(unsigned char ch) {
    unsigned char *r = (unsigned char *)(0xD720 + ch * 16);
    return (unsigned int)(r[1] | (r[2] << 8)) - (unsigned int)tune_sample_bank;
}

static void draw(unsigned int f) {
    unsigned char v, n;
    textcolor(1);
    for (v = 0; v < 3; v++) {
        n = tune_note(v);
        if (n == 0xFF) cprintfxy(2, 4 + v, "v%u  ...  %s", v + 1, parts[v]);
        else cprintfxy(2, 4 + v, "v%u  %s%u  %s", v + 1, pcs[n % 12], n / 12 - 1, parts[v]);
    }
    for (v = 0; v < 2; v++)
        cprintfxy(2, 8 + v, "d%u  channel %u: the sample at +%04x", v + 1, DRUMS + v, sample_at((unsigned char)(DRUMS + v)));
    textcolor(12);
    cprintfxy(2, 11, "tick %5u  frame %5u", tune_ticks(), f);
}

int main(void) {
    unsigned int f = 0, tempo = 0x100;
    unsigned char playing = 1;
#ifdef DRUMS_CHECK
    unsigned int was[2] = {0, 0}, at;
    unsigned char changes[2] = {0, 0}, inside = 1, v;
#endif
    sid_init();
    sid_set_mirror(1);                  /* SID 1 on SID 3 too: both sides */
    digi_init();
    tune_samples((unsigned int)tune_sample_bank);
    tune_digi_channels(DRUMS);
    tune_play(tunes[TUNE_GROOVE]);
    textcolor(1);
    clrscr();
    cputsxy(2, 1, "drums: a sid band and a sampled kit (cc0: samples/source.txt)");
    textcolor(12);
    cputsxy(2, 14, "space stop/play  + - tempo  run/stop quit");
#ifdef DRUMS_CHECK
    for (f = 0; f < 240; f++) {
        wait_frame();
        tune_update();
        for (v = 0; v < 2; v++) {
            at = sample_at((unsigned char)(DRUMS + v));
            if (at >= TUNE_SAMPLES_SIZE) inside = 0;
            if (f && at != was[v]) changes[v]++;
            was[v] = at;
        }
    }
    draw(f);
    textcolor(1);
    /* 40 ticks: kick and snare by turns (9 changes), hat to open hat and
     * back at ticks 12 and 14, 28 and 30 (4) */
    if (tune_ticks() == 40 && tune_playing() && inside && changes[0] == 9 && changes[1] == 4)
        cprintfxy(2, 16, "drums check ok: 40 ticks, the samples changed %u and %u times", changes[0], changes[1]);
    else
        cprintfxy(2, 16, "drums check failed: %u ticks, changed %u %u, inside %u", tune_ticks(), changes[0], changes[1], inside);
    tune_stop();
    digi_done();
    return 0;
#else
    for (;;) {
        wait_frame();
        if (playing) {
            tune_update();
            f++;
        }
        draw(f);
        if (!kbhit()) continue;
        switch (getch()) {
        case CH_STOP:
            tune_stop();
            digi_done();
            sid_set_mirror(0);
            clrscr();
            return 0;
        case ' ':
            if (playing) tune_stop();
            else tune_play(tunes[TUNE_GROOVE]);
            playing = !playing;
            break;
        case '+':
            if (tempo < 0x200) tune_tempo(tempo += 0x10);
            break;
        case '-':
            if (tempo > 0x80) tune_tempo(tempo -= 0x10);
            break;
        }
    }
#endif
}
