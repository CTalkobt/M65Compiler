/* tuneplay.c — tune45 play --emulator / --send: the tunes of a music file on
 * the MEGA65, through the library's player (<audio_tune.h>)
 *
 * Built by lib/Makefile into tuneplay.o45; tune45 links it with the tunes
 * (tune45 convert's C with --player: tunes[], tune_count, tune_names[],
 * tune_file, tune_start) and the library. On the screen: the tunes, the one
 * playing, each voice's note, the ticks played and the controls. Keys:
 *   CRSR up/down, 1-9   choose a tune (it starts)
 *   SPACE   stop / play again      R   from the start
 *   + -     tempo                  V   vibrato (0-3 added)
 *   T G     transpose up / down    RUN/STOP   quit
 */
#include <conio.h>
#include <input.h>
#include <audio_sid.h>
#include <audio_tune.h>

extern const unsigned char *const tunes[];
extern const unsigned char tune_count;
extern const char *const tune_names[];
extern const char tune_file[];
extern const unsigned char tune_start;
extern const unsigned int tune_sample_size;      /* the digi voices' samples */
extern const unsigned char tune_sample_bank[];

#define REG(a) (*(volatile unsigned char *)(a))
#define LISTY 3

static unsigned char cur, playing, vib;
static signed char transp;
static unsigned int tempo = 0x100;

static void wait_frame(void) {
    while (REG(0xD012) != 0xF8) ;
    while (REG(0xD012) == 0xF8) ;
}

static const char *const names[12] = {"c-", "c#", "d-", "d#", "e-", "f-", "f#", "g-", "g#", "a-", "a#", "b-"};

static void start(void) {
    unsigned char v;
    for (v = 0; v < 12; v++) tune_transpose(v, transp);
    tune_play(tunes[cur]);
    playing = 1;
}

static void draw_list(void) {
    unsigned char k;
    for (k = 0; k < tune_count && k < 20; k++) {
        revers(k == cur);
        textcolor(k == cur ? 7 : 1);
        cprintfxy(1, LISTY + k, "%2u %-16s %s", k + 1, tune_names[k], (tunes[k][0] & 1) ? "loop" : "once");
        revers(0);
    }
}

static void draw_state(void) {
    unsigned char v, n, nv = tunes[cur][1];
    textcolor(15);
    cprintfxy(40, LISTY, "%-8s ticks %5u", playing && tune_playing() ? "playing" : "stopped", tune_ticks());
    cprintfxy(40, LISTY + 1, "tempo %3u%%  vibrato +%u  transpose %+3d ", (unsigned)(tempo * 100u / 256u), vib, transp);
    for (v = 0; v < 12; v++) {
        if (v >= nv) { cputsxy(40, LISTY + 3 + v, "              "); continue; }
        n = tune_note(v);
        textcolor(n == 0xFF ? 11 : 13);
        if (n == 0xFF) cprintfxy(40, LISTY + 3 + v, "voice %2u  ---  ", v + 1);
        else cprintfxy(40, LISTY + 3 + v, "voice %2u  %s%d ", v + 1, names[n % 12], n / 12 - 1);
    }
}

int main(void) {
    int key;
    sid_init();
    sid_set_mirror(1);
    if (tune_sample_size) tune_samples((unsigned long)(unsigned int)tune_sample_bank);
    textcolor(1);
    clrscr();
    cprintfxy(0, 0, "tune45 play  %s", tune_file);
    textcolor(12);
    cputsxy(0, 24, "crsr/1-9 tune  space stop/play  r restart  +/- tempo  v vibrato  t/g transpose");
    cur = tune_start < tune_count ? tune_start : 0;
    start();
    draw_list();
    for (;;) {
        wait_frame();
        if (playing) tune_update();
        draw_state();
        if (!kbhit()) continue;
        key = getch();
        if (key == CH_STOP) break;
        if (key == CH_CURSOR_DOWN || key == CH_CURSOR_UP || (key >= '1' && key <= '9')) {
            if (key == CH_CURSOR_DOWN) cur = cur + 1 < tune_count ? cur + 1 : 0;
            else if (key == CH_CURSOR_UP) cur = cur ? cur - 1 : tune_count - 1;
            else if (key - '1' < tune_count) cur = (unsigned char)(key - '1');
            start();
            draw_list();
        } else if (key == ' ') {
            if (playing && tune_playing()) { tune_stop(); playing = 0; }
            else start();
        } else if (key == 'r') {
            start();
        } else if (key == '+' && tempo < 0x400) {
            tempo += 0x20;
            tune_tempo(tempo);
        } else if (key == '-' && tempo > 0x40) {
            tempo -= 0x20;
            tune_tempo(tempo);
        } else if (key == 'v') {
            vib = (unsigned char)((vib + 1) & 3);
            tune_vibrato(vib);
        } else if (key == 't' && transp < 24) {
            transp++;
            start();
        } else if (key == 'g' && transp > -24) {
            transp--;
            start();
        }
    }
    tune_stop();
    sid_set_mirror(0);
    textcolor(1);
    clrscr();
    return 0;
}
