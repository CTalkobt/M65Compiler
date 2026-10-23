/* tunes — the pfest game's music, played by the library's tune player
 * (<audio_tune.h>) from a raster interrupt, as a game plays its music
 *
 * music.txt is the game's music text; tune45 convert makes music.c and
 * music.h of it (the Makefile). The player runs once a frame in an
 * __interrupt handler: tune_update has no static allocation (ln45 refuses
 * a program whose interrupt handler can reach a function with it), so the
 * main loop only draws and reads keys while the music goes on by itself.
 *
 * The interrupt: the KERNAL's IRQ entry pushes A X Y Z and the base page
 * register and jumps through ($0314). An __interrupt function saves its own
 * registers and ends with RTI, so $0314 points at a small stub (at $BE00):
 * other interrupts go on to the KERNAL's handler; a raster interrupt enters
 * the __interrupt handler as the CPU would (a return address and the flags
 * pushed), and after its RTI the stub pulls what the KERNAL pushed and
 * returns from the interrupt.
 *
 * Keys: CRSR up/down or 1-9 a tune, SPACE stop / play, + - tempo,
 * V vibrato, T G transpose, S sick (the game's music when the player feels
 * sick: faster, a wobble, a voice out of tune), RUN/STOP quit.
 *
 * Built with -DTUNES_CHECK (make check): the music plays from the interrupt
 * for 150 frames and the program reports on the screen.
 */
#include <conio.h>
#include <input.h>
#include <string.h>
#include <audio_sid.h>
#include <audio_tune.h>
#include "music.h"

#define REG(a) (*(volatile unsigned char *)(a))
#define STUB ((unsigned char *)0xBE00)
#define LINE 0xF8                       /* the raster line of the interrupt (the bottom border) */

static const char *const names[NTUNES] = {
    "title", "march", "clear", "passout", "gameover", "story",
    "stroll", "polka", "teacups", "funhouse", "boat", "fireworks",
};

volatile unsigned int frames;           /* counted by the interrupt */
static unsigned char old_vector[2], old_d01a, old_d012;

/* once a frame: the music */
__interrupt void music_isr(void) {
    tune_update();
    frames++;
    REG(0xD019) = 0x01;                 /* acknowledge the raster interrupt */
}

static void irq_install(void) {
    static const unsigned char stub[] = {
        0xAD, 0x19, 0xD0,       /* +0   lda $D019        VIC interrupt flags  */
        0x29, 0x01,             /* +3   and #$01         raster?              */
        0xD0, 0x03,             /* +5   bne +3                                */
        0x6C, 0x00, 0x00,       /* +7   jmp (old)        no: the KERNAL's     */
        0xA9, 0x00,             /* +10  lda #>tail       as an interrupt      */
        0x48,                   /* +12  pha              enters: the return   */
        0xA9, 0x00,             /* +13  lda #<tail       address and flags    */
        0x48,                   /* +15  pha                                   */
        0x08,                   /* +16  php                                   */
        0x4C, 0x00, 0x00,       /* +17  jmp _music_isr                        */
        0x68, 0x5B,             /* +20  tail: pla, tab   the KERNAL's pushes: */
        0xFB, 0x7A, 0xFA, 0x68, /* +22  plz ply plx pla  B Z Y X A            */
        0x40,                   /* +26  rti                                   */
    };
    unsigned int tail = 0xBE00 + 20, isr = (unsigned int)music_isr;
    memcpy(STUB, stub, sizeof stub);
    STUB[27] = REG(0x0314);             /* the KERNAL's handler, for jmp (old) */
    STUB[28] = REG(0x0315);
    STUB[8] = 27;                       /* jmp ($BE1B) */
    STUB[9] = 0xBE;
    STUB[11] = (unsigned char)(tail >> 8);
    STUB[14] = (unsigned char)tail;
    STUB[18] = (unsigned char)isr;
    STUB[19] = (unsigned char)(isr >> 8);
    __asm__("sei");
    old_vector[0] = REG(0x0314);
    old_vector[1] = REG(0x0315);
    old_d01a = REG(0xD01A);
    old_d012 = REG(0xD012);
    REG(0xD05D) = REG(0xD05D) & 0x7F;   /* hot registers off: the $D011 write */
    REG(0xD011) = REG(0xD011) & 0x7F;   /* leaves the VIC-IV set-up alone     */
    REG(0xD012) = LINE;
    REG(0x0314) = 0x00;
    REG(0x0315) = 0xBE;
    REG(0xD01A) = 0x01;                 /* raster interrupts */
    REG(0xD019) = 0x0F;
    __asm__("cli");
}

static void irq_remove(void) {
    __asm__("sei");
    REG(0x0314) = old_vector[0];
    REG(0x0315) = old_vector[1];
    REG(0xD012) = old_d012;
    REG(0xD01A) = old_d01a & 0x0F;
    REG(0xD019) = 0x0F;
    __asm__("cli");
}

static unsigned char cur, vib, sick;
static signed char transp;
static unsigned int tempo = 0x100;

/* the controls, again (the sick music is the game's: faster, a wobble,
 * voice 2 out of tune) */
static void controls(void) {
    unsigned char v;
    tune_tempo(sick ? tempo + tempo / 4 : tempo);
    tune_vibrato((unsigned char)(vib + (sick ? 2 : 0)));
    for (v = 0; v < 3; v++) {
        tune_transpose(v, transp);
        tune_detune(v, (signed char)(sick && v == 1 ? 1 : 0));
    }
}

static void start(void) {
    controls();
    tune_play(tunes[cur]);
}

static void draw_list(void) {
    unsigned char k;
    for (k = 0; k < NTUNES; k++) {
        revers(k == cur);
        textcolor(k == cur ? 7 : 1);
        cprintfxy(2, 3 + k, "%2u %-10s %s", k + 1, names[k], (tunes[k][0] & 1) ? "loop" : "once");
        revers(0);
    }
}

static const char *const pcs[12] = {"c ", "c#", "d ", "d#", "e ", "f ", "f#", "g ", "g#", "a ", "a#", "b "};

/* (the lines end before column 80: a line that reached it would go on at
 * the next one's start) */
static void draw_voices(void) {
    static const unsigned char bar_color[3] = {5, 7, 10};   /* green, yellow, light red */
    unsigned char v, n, w, k;
    textcolor(15);
    cprintfxy(36, 3, "%-8s ticks %5u  frames %5u", tune_playing() ? "playing" : "stopped", tune_ticks(), frames);
    cprintfxy(36, 4, "tempo %3u%% vibrato +%u transp %+3d %s",
              (unsigned)((sick ? tempo + tempo / 4 : tempo) * 100u / 256u), vib, transp, sick ? "sick" : "    ");
    for (v = 0; v < 3; v++) {
        n = tune_note(v);
        textcolor(n == 0xFF ? 11 : 13);
        if (n == 0xFF) cprintfxy(36, 7 + 2 * v, "voice %u  ---  ", v + 1);
        else cprintfxy(36, 7 + 2 * v, "voice %u  %s%d  ", v + 1, pcs[n % 12], n / 12 - 1);
        /* a bar as long as the note is high (columns 51-78) */
        w = n == 0xFF ? 0 : n > 36 ? (unsigned char)((n - 36) / 2 + 1) : 1;
        if (w > 28) w = 28;
        gotoxy(51, 7 + 2 * v);
        textcolor(bar_color[v]);
        for (k = 0; k < 28; k++) {
            revers(k < w);
            putch(' ');
        }
        revers(0);
    }
}

int main(void) {
    int key;
    sid_init();
    sid_set_mirror(1);                  /* SID 1 on both sides (SID 3 too) */
    textcolor(1);
    clrscr();
    cputsxy(2, 0, "tunes: the pfest game's music, played from a raster interrupt");
    textcolor(12);
    cputsxy(2, 23, "crsr/1-9 tune  space stop/play  +/- tempo  v vibrato  t/g transpose");
    cputsxy(2, 24, "s sick  run/stop quit");
    cur = TUNE_MARCH;
    start();
    draw_list();
    irq_install();
#ifdef TUNES_CHECK
    {
        unsigned int t0;
        unsigned char ok;
        while (frames < 150) draw_voices();
        t0 = tune_ticks();
        /* march: 5 frames a tick, so 150 frames are 30 ticks (the first at once) */
        ok = t0 >= 29 && t0 <= 31 && tune_playing();
        sick = 1;
        start();
        frames = 0;
        while (frames < 100) draw_voices();
        /* sick: 1.25 times as fast: 100 frames are 25 ticks */
        ok = ok && tune_ticks() >= 24 && tune_ticks() <= 26;
        irq_remove();
        tune_stop();
        textcolor(1);
        gotoxy(2, 21);
        cprintf(ok ? "tunes check ok: %u ticks\n" : "tunes check failed: %u ticks\n", t0);
        return 0;
    }
#endif
    for (;;) {
        draw_voices();
        if (!kbhit()) continue;
        key = getch();
        if (key == CH_STOP) break;
        if (key == CH_CURSOR_DOWN || key == CH_CURSOR_UP || (key >= '1' && key <= '9')) {
            if (key == CH_CURSOR_DOWN) cur = cur + 1 < NTUNES ? cur + 1 : 0;
            else if (key == CH_CURSOR_UP) cur = cur ? cur - 1 : NTUNES - 1;
            else if (key - '1' < NTUNES) cur = (unsigned char)(key - '1');
            start();
            draw_list();
        } else if (key == ' ') {
            if (tune_playing()) tune_stop();
            else start();
        } else if (key == '+' && tempo < 0x300) {
            tempo += 0x20;
            controls();
        } else if (key == '-' && tempo > 0x60) {
            tempo -= 0x20;
            controls();
        } else if (key == 'v') {
            vib = (unsigned char)((vib + 1) & 3);
            controls();
        } else if (key == 't' && transp < 12) {
            transp++;
            start();
        } else if (key == 'g' && transp > -12) {
            transp--;
            start();
        } else if (key == 's') {
            sick = !sick;
            controls();
        }
    }
    irq_remove();
    tune_stop();
    sid_set_mirror(0);
    textcolor(1);
    clrscr();
    return 0;
}
