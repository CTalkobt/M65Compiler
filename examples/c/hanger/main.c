/* Hanger -- hangman for the MEGA65, built with cc45
 *
 * Guess the word a letter at a time. Each wrong letter adds a part to the
 * figure on the gallows; when it is complete, it's hanged. Every level has
 * a different victim: a human, a cat, a duck, a giraffe, an elephant and
 * an octopus, with more parts (more misses allowed) as the words get
 * harder.
 *
 * One player: the computer picks the words (from words.txt, the 500 most
 * common English words of 3+ letters, sorted easiest first; level n takes
 * the n-th sixth). Solve a word to move on to the next animal. A miss costs
 * one of three lives and you try that animal again with another word.
 *
 * Two players: in each level, player 1 types a word for player 2 to guess,
 * then the other way round. Solving scores for the guesser, a hanging for
 * the player who chose the word.
 *
 * Keys: A-Z guess (or type the secret word: RETURN to accept, DEL to rub
 *       out), RETURN to go on, RUN/STOP back to the title / quit.
 *
 * The screen is 80 x 25 text. The picture's glyphs (gen_gfx.py) are
 * loaded per level into a copy of the upper case character set in bank 4,
 * from screen code GLYPH0; codes $40-$47 are the frame's lines. The whole
 * screen is built in two buffers and copied out by DMA each frame.
 *
 * Build with -DDEMO for a self-playing game (screenshots, soak tests);
 * -DDEMO_LEVEL=n starts it at level n (0-5).
 *
 * cc45's plain char/int/long are unsigned.
 */
#include <string.h>
#include <dma.h>
#include <mega65.h>
#include <audio_sid.h>
#include <input.h>

#include "gfx.h"
#include "words.h"

#ifndef REG
#define REG(a) (*(volatile unsigned char *)(a))
#endif

#define COLS 80
#define ROWS 25
#define SCREEN_LIN  0x0000800UL
#define COLOR_LIN   0xFF80000UL
#define CHARROM_LIN 0xFF7E000UL       /* the MEGA65 character set */
#define CHARSET_LIN 0x004F800UL       /* our copy: bank 4 */
#define FRAMECOUNT  REG(0xD7FA)

#define C_BLACK 0
#define C_WHITE 1
#define C_RED 2
#define C_CYAN 3
#define C_PURPLE 4
#define C_GREEN 5
#define C_BLUE 6
#define C_YELLOW 7
#define C_ORANGE 8
#define C_BROWN 9
#define C_PINK 10
#define C_DGREY 11
#define C_GREY 12
#define C_LGREEN 13
#define C_LBLUE 14
#define C_LGREY 15

#define CH_SPACE 0x20
#define CH_STAR 0x2A
#define G_HLINE (FIXED0 + 0)
#define G_VLINE (FIXED0 + 1)
#define G_TL (FIXED0 + 2)
#define G_TR (FIXED0 + 3)
#define G_BL (FIXED0 + 4)
#define G_BR (FIXED0 + 5)
#define G_SLOT (FIXED0 + 6)
#define G_SOLID (FIXED0 + 7)
#define G_BOX (FIXED0 + 8)
#define G_DOT (FIXED0 + 9)

/* layout: the picture framed on the left, the panel on the right */
#define PIC_X 1
#define PIC_Y 2
#define PANEL_L 33                    /* the panel's frame */
#define PANEL_R 79
#define PANEL_X 34                    /* its inside */
#define PANEL_W 45
#define ROW_LEVEL 3
#define ROW_WHO 5
#define ROW_WORD 9
#define ROW_MISSES 13
#define ROW_ALPHA 16
#define ROW_MSG 20
#define ROW_HELP 23

#define MAXLEN 14
#define LIVES 3

/* ------------------------------------------------------------------------ */
/* The screen, built here and copied by DMA once a frame                    */

static unsigned char scr[COLS * ROWS], clr[COLS * ROWS];

static void put(unsigned char x, unsigned char y, unsigned char ch, unsigned char c) {
    unsigned int o = (unsigned int)y * COLS + x;
    scr[o] = ch;
    clr[o] = c;
}

static void put_text(unsigned char x, unsigned char y, const char *s, unsigned char c) {
    unsigned int o = (unsigned int)y * COLS + x;
    while (*s) {
        scr[o] = (unsigned char)*s++;
        clr[o++] = c;
    }
}

/* v without leading zeros; returns the column after it */
static unsigned char put_number(unsigned char x, unsigned char y, unsigned int v, unsigned char c) {
    unsigned char digits[5], n = 0;
    do {
        digits[n++] = (unsigned char)(0x30 + v % 10);
        v /= 10;
    } while (v);
    while (n) put(x++, y, digits[--n], c);
    return x;
}

static void clear_span(unsigned char x, unsigned char y, unsigned char n) {
    unsigned int o = (unsigned int)y * COLS + x;
    while (n--) scr[o++] = CH_SPACE;
}

/* text centred in the panel */
static void panel_text(unsigned char y, const char *s, unsigned char c) {
    clear_span(PANEL_X, y, PANEL_W);
    put_text((unsigned char)(PANEL_X + (PANEL_W - strlen(s)) / 2), y, s, c);
}

static void frame(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char c) {
    unsigned char i;
    for (i = x0 + 1; i < x1; i++) { put(i, y0, G_HLINE, c); put(i, y1, G_HLINE, c); }
    for (i = y0 + 1; i < y1; i++) { put(x0, i, G_VLINE, c); put(x1, i, G_VLINE, c); }
    put(x0, y0, G_TL, c); put(x1, y0, G_TR, c);
    put(x0, y1, G_BL, c); put(x1, y1, G_BR, c);
}

static void show(void) {
    dma_copy_far(SCREEN_LIN, (unsigned long)(unsigned int)scr, COLS * ROWS);
    dma_copy_far(COLOR_LIN, (unsigned long)(unsigned int)clr, COLS * ROWS);
}

static void wait_vsync(void) {
    while (REG(0xD012) == 0) ;
    while (REG(0xD012) != 0 || (REG(0xD011) & 0x80)) ;
}

/* ------------------------------------------------------------------------ */
/* Random numbers (16-bit xorshift)                                         */

static unsigned int rnd_state = 0xACE1;
static unsigned int rnd(void) {
    rnd_state ^= rnd_state << 7;
    rnd_state ^= rnd_state >> 9;
    rnd_state ^= rnd_state << 8;
    return rnd_state;
}

/* ------------------------------------------------------------------------ */
/* Sound: one voice playing a short tune of (MIDI note, frames) pairs       */

static const sid_instrument_t snd_blip = { SID_TRIANGLE, 0x08, 0x00, 0 };
static const sid_instrument_t snd_buzz = { SID_SAWTOOTH, 0x09, 0x00, 0 };
static const sid_instrument_t snd_tune = { SID_PULSE, 0x0A, 0x00, 0x0600 };

static const unsigned char tune_right[] = { 84, 4, 0 };
static const unsigned char tune_wrong[] = { 40, 6, 37, 10, 0 };
static const unsigned char tune_key[] = { 96, 2, 0 };
static const unsigned char tune_win[] = { 72, 6, 76, 6, 79, 6, 84, 16, 0 };
static const unsigned char tune_lose[] = { 55, 12, 54, 12, 53, 12, 52, 30, 0 };

static const unsigned char *tune_p;
static unsigned char tune_t;

static void play(const sid_instrument_t *inst, const unsigned char *tune) {
    sid_gate(0, 0);
    sid_set_instrument(0, inst);
    tune_p = tune;
    tune_t = 0;
}

static void snd_update(void) {
    if (!tune_p) return;
    if (tune_t) {
        if (--tune_t == 1) sid_gate(0, 0);
        return;
    }
    if (!*tune_p) { tune_p = 0; return; }
    sid_gate(0, 0);
    sid_set_freq_reg(0, sid_note_to_reg(tune_p[0]));
    sid_gate(0, 1);
    tune_t = tune_p[1];
    tune_p += 2;
}

/* ------------------------------------------------------------------------ */
/* The picture                                                              */

static unsigned char level;

static void load_level(unsigned char l) {
    level = l;
    dma_copy_far(CHARSET_LIN + GLYPH0 * 8, (unsigned long)(unsigned int)lvl_glyphs[l],
                 (unsigned int)lvl_nglyphs[l] * 8);
}

/* stage 0 the gallows, n the n-th part, lvl_parts + 1 the eyes crossed out */
static void apply_stage(unsigned char n) {
    const unsigned int *st = lvl_stages[level];
    const unsigned char *d = lvl_diffs[level] + st[n] * 4;
    unsigned int i = st[n + 1] - st[n];
    while (i--) {
        unsigned int off = d[0] | (unsigned int)d[1] << 8;
        put((unsigned char)(PIC_X + off % PIC_W), (unsigned char)(PIC_Y + off / PIC_W), d[2], d[3]);
        d += 4;
    }
}

static void draw_picture(unsigned char parts) {
    unsigned char y, i;
    for (y = 0; y < PIC_H; y++) clear_span(PIC_X, PIC_Y + y, PIC_W);
    for (i = 0; i <= parts; i++) apply_stage(i);
}

/* ------------------------------------------------------------------------ */
/* The game                                                                 */

enum { M_TITLE, M_ENTER, M_GUESS, M_DONE, M_OVER };

static unsigned char mode;
static unsigned char players;         /* 1 or 2 */
static unsigned char lives;
static unsigned int score[2];
static unsigned char guesser;         /* two players: who is guessing (0/1) */
static unsigned char half;            /* two players: 0 player 1 guesses first... */
static unsigned char won;             /* the last round */

static unsigned char word[MAXLEN + 1], wlen;
static unsigned char tried[27];       /* letter 1-26: 0 not yet, 1 in the word, 2 a miss */
static unsigned char misses, left;    /* left: letters still hidden */
static unsigned char missed[10];      /* the wrong letters, in order */
static unsigned char used[(NWORDS + 7) / 8];

static unsigned int hiscore;

/* a line of text put together in sbuf */
static char sbuf[48];
static unsigned char slen;

static void sb_reset(void) { slen = 0; sbuf[0] = 0; }
static void sb_add(const char *s) {
    while (*s && slen < sizeof sbuf - 1) sbuf[slen++] = *s++;
    sbuf[slen] = 0;
}
static void sb_num(unsigned int v) {
    char d[6];
    unsigned char n = 0;
    do { d[n++] = (char)(0x30 + v % 10); v /= 10; } while (v);
    while (n && slen < sizeof sbuf - 1) sbuf[slen++] = d[--n];
    sbuf[slen] = 0;
}
static void sb_player(unsigned char p) { sb_add(s"player "); sb_num(p + 1); }

/* ------------------------------------------------------------------------ */
/* Drawing the panel                                                        */

static void draw_header(void) {
    clear_span(0, 0, COLS);
    put_text(1, 0, s"hanger", C_CYAN);
    if (mode == M_TITLE) {
        if (hiscore) {
            put_text(62, 0, s"best score", C_LGREY);
            put_number(73, 0, hiscore, C_WHITE);
        }
        return;
    }
    if (players == 1) {
        put_text(58, 0, s"score", C_LGREY);
        put_number(64, 0, score[0], C_WHITE);
        put_text(72, 0, s"lives", C_LGREY);
        put_number(78, 0, lives, lives > 1 ? C_WHITE : C_PINK);
    } else {
        put_text(52, 0, s"player 1", guesser == 0 ? C_YELLOW : C_LGREY);
        put_number(61, 0, score[0], C_WHITE);
        put_text(67, 0, s"player 2", guesser == 1 ? C_YELLOW : C_LGREY);
        put_number(76, 0, score[1], C_WHITE);
    }
}

static void draw_frames(void) {
    unsigned char y;
    for (y = 1; y < ROWS; y++) clear_span(0, y, COLS);
    frame(0, 1, PIC_X + PIC_W, ROWS - 1, C_DGREY);
    frame(PANEL_L, 1, PANEL_R, ROWS - 1, C_DGREY);
}

static void draw_level(void) {
    sb_reset();
    sb_add(s"level "); sb_num(level + 1); sb_add(s" of "); sb_num(NLEVELS);
    sb_add(s":  the "); sb_add(lvl_name[level]);
    panel_text(ROW_LEVEL, sbuf, C_YELLOW);
}

/* the word: letters found (or all, reveal: the missed ones red) */
static unsigned char word_x(unsigned char n) {
    return (unsigned char)(PANEL_X + (PANEL_W - (n * 3 - 2)) / 2);
}

static void draw_word(unsigned char reveal) {
    unsigned char i, x = word_x(wlen);
    clear_span(PANEL_X, ROW_WORD, PANEL_W);
    clear_span(PANEL_X, ROW_WORD + 1, PANEL_W);
    for (i = 0; i < wlen; i++, x += 3) {
        unsigned char l = word[i];
        if (tried[l] == 1) put(x, ROW_WORD, l, C_WHITE);
        else if (reveal) put(x, ROW_WORD, l, C_PINK);
        put(x, ROW_WORD + 1, G_SLOT, C_GREY);
    }
}

/* misses: "misses 3 of 9:  q x z", and a block for each part, solid once
 * used. The status of a letter never shows by colour alone */
static void draw_misses(void) {
    unsigned char i, parts = lvl_parts[level], x;
    sb_reset();
    sb_add(s"misses "); sb_num(misses); sb_add(s" of "); sb_num(parts);
    if (misses) sb_add(s": ");
    for (i = 0; i < misses; i++) { sbuf[slen++] = ' '; sbuf[slen++] = (char)missed[i]; }
    sbuf[slen] = 0;
    panel_text(ROW_MISSES, sbuf, misses + 1 >= parts ? C_YELLOW : C_WHITE);
    clear_span(PANEL_X, ROW_MISSES + 1, PANEL_W);
    x = (unsigned char)(PANEL_X + (PANEL_W - (parts * 2 - 1)) / 2);
    for (i = 0; i < parts; i++, x += 2)
        put(x, ROW_MISSES + 1, i < misses ? G_SOLID : G_BOX, i < misses ? C_PINK : C_LGREY);
}

/* the letters not yet tried; a tried one is a dot */
static void draw_alpha(void) {
    unsigned char l, x = 0, y = ROW_ALPHA;
    for (l = 1; l <= 26; l++) {
        if (l == 1 || l == 14) {
            x = word_x(13);
            if (l == 14) y += 2;
        }
        if (tried[l]) put(x, y, G_DOT, C_GREY);
        else put(x, y, l, C_WHITE);
        x += 3;
    }
}

static void clear_rows(unsigned char y0, unsigned char y1) {
    while (y0 <= y1) clear_span(PANEL_X, y0++, PANEL_W);
}

static void message(const char *a, unsigned char ca, const char *b, unsigned char cb) {
    panel_text(ROW_MSG, a, ca);
    panel_text(ROW_MSG + 1, b, cb);
}

static void help(const char *s) {
    panel_text(ROW_HELP, s, C_LGREY);
}

/* ------------------------------------------------------------------------ */
/* Rounds                                                                   */

static void pick_word(void) {
    unsigned int base = tier[level], n = tier[level + 1] - base, i = 0;
    const unsigned char *w;
    unsigned char tries;
    for (tries = 0; tries < 50; tries++) {
        i = base + rnd() % n;
        if (!(used[i >> 3] & (1 << (i & 7)))) break;
    }
    used[i >> 3] |= (unsigned char)(1 << (i & 7));
    w = word_text + word_off[i];
    for (wlen = 0; w[wlen]; wlen++) word[wlen] = w[wlen];
}

static void draw_entry(void) {
    unsigned char i, x;
    clear_rows(ROW_WHO, ROW_HELP);
    sb_reset();
    sb_player((unsigned char)(1 - guesser)); sb_add(s", type a word for "); sb_player(guesser);
    panel_text(ROW_WHO, sbuf, C_WHITE);
    sb_reset();
    sb_player(guesser); sb_add(s": no peeking!");
    panel_text(ROW_WHO + 1, sbuf, C_PINK);
    x = word_x(wlen + (wlen < MAXLEN));
    for (i = 0; i < wlen; i++, x += 3) {
        put(x, ROW_WORD, CH_STAR, C_WHITE);
        put(x, ROW_WORD + 1, G_SLOT, C_GREY);
    }
    if (wlen < MAXLEN) put(x, ROW_WORD + 1, G_SLOT, (FRAMECOUNT & 16) ? C_YELLOW : C_BLACK);
    panel_text(ROW_MISSES, s"3 to 14 letters", C_LGREY);
    help(s"return when done   del rubs out   run/stop quits");
}

static void begin_guessing(void) {
    unsigned char i;
    memset(tried, 0, sizeof tried);
    misses = 0;
    left = wlen;
    mode = M_GUESS;
    clear_rows(ROW_WHO, ROW_HELP);
    if (players == 2) {
        sb_reset();
        sb_player(guesser); sb_add(s" is guessing");
        panel_text(ROW_WHO, sbuf, C_WHITE);
    } else {
        sb_reset();
        sb_add(s"save the "); sb_add(lvl_name[level]); sb_add(s"!");
        panel_text(ROW_WHO, sbuf, C_WHITE);
    }
    draw_word(0);
    draw_misses();
    draw_alpha();
    help(s"type a letter to guess      run/stop quits");
    (void)i;
}

static void start_round(void) {
    load_level(level);
    draw_frames();
    draw_header();
    draw_picture(0);
    draw_level();
    if (players == 1) {
        pick_word();
        begin_guessing();
    } else {
        guesser = (unsigned char)(1 - half);   /* player 1 sets the first word */
        draw_header();
        wlen = 0;
        mode = M_ENTER;
        draw_entry();
    }
}

static void new_game(unsigned char p) {
    players = p;
    score[0] = score[1] = 0;
    lives = LIVES;
    level = 0;
    half = 0;
    memset(used, 0, sizeof used);
#if defined(DEMO) && defined(DEMO_LEVEL)
    level = DEMO_LEVEL;
#endif
    start_round();
}

static void round_over(void) {
    unsigned int pts;
    unsigned char parts = lvl_parts[level];
    mode = M_DONE;
    sb_reset();
    if (won) {
        pts = (10 + 2 * (parts - misses)) * (level + 1);
        score[players == 1 ? 0 : guesser] += pts;
        play(&snd_tune, tune_win);
        if (players == 1) { sb_add(s"you saved the "); sb_add(lvl_name[level]); sb_add(s"!"); }
        else { sb_player(guesser); sb_add(s" saved the "); sb_add(lvl_name[level]); sb_add(s"!"); }
        panel_text(ROW_MSG, sbuf, C_LGREEN);
        sb_reset();
        sb_add(s"+"); sb_num(pts); sb_add(s" points");
        panel_text(ROW_MSG + 1, sbuf, C_LGREEN);
    } else {
        apply_stage((unsigned char)(parts + 1));
        draw_word(1);
        play(&snd_buzz, tune_lose);
        sb_add(s"the poor "); sb_add(lvl_name[level]); sb_add(s" is hanged.");
        panel_text(ROW_MSG, sbuf, C_PINK);
        sb_reset();
        if (players == 1) {
            --lives;
            if (lives == 1) sb_add(s"one life left");
            else if (lives) { sb_num(lives); sb_add(s" lives left"); }
            else sb_add(s"no lives left");
        } else {
            pts = 10 * (level + 1);
            score[1 - guesser] += pts;
            sb_add(s"+"); sb_num(pts); sb_add(s" points for "); sb_player((unsigned char)(1 - guesser));
        }
        panel_text(ROW_MSG + 1, sbuf, C_PINK);
    }
    draw_header();
    help(s"press return");
}

static void game_over(void) {
    mode = M_OVER;
    clear_rows(ROW_MSG, ROW_HELP);
    sb_reset();
    if (players == 1) {
        if (score[0] > hiscore) hiscore = score[0];
        if (lives) {
            message(s"you saved them all!", C_LGREEN, s"", C_WHITE);
            play(&snd_tune, tune_win);
        } else {
            message(s"game over", C_PINK, s"", C_WHITE);
        }
        sb_add(s"final score "); sb_num(score[0]);
        panel_text(ROW_MSG + 1, sbuf, C_YELLOW);
    } else {
        if (score[0] == score[1]) sb_add(s"it's a draw!");
        else { sb_player(score[1] > score[0]); sb_add(s" wins!"); }
        panel_text(ROW_MSG, sbuf, C_LGREEN);
        sb_reset();
        sb_num(score[0]); sb_add(s" to "); sb_num(score[1]);
        panel_text(ROW_MSG + 1, sbuf, C_YELLOW);
        play(&snd_tune, tune_win);
    }
    help(s"press return");
}

static void next_round(void) {
    if (players == 1) {
        if (won) {
            if (++level == NLEVELS) { level = NLEVELS - 1; game_over(); return; }
        } else if (!lives) {
            game_over();
            return;
        }
    } else if (++half == 2) {
        half = 0;
        if (++level == NLEVELS) { level = NLEVELS - 1; game_over(); return; }
    }
    start_round();
}

static void guess(unsigned char l) {
    unsigned char i, n = 0;
    if (tried[l]) {
        sb_reset();
        sb_add(s"you already tried ");
        sbuf[slen++] = (char)l; sbuf[slen] = 0;
        message(sbuf, C_ORANGE, s"", C_WHITE);
        return;
    }
    message(s"", C_WHITE, s"", C_WHITE);
    for (i = 0; i < wlen; i++) if (word[i] == l) n++;
    if (n) {
        tried[l] = 1;
        left -= n;
        play(&snd_blip, tune_right);
        draw_word(0);
        draw_alpha();
        if (!left) { won = 1; round_over(); }
    } else {
        tried[l] = 2;
        missed[misses] = l;
        apply_stage(++misses);
        play(&snd_buzz, tune_wrong);
        draw_misses();
        draw_alpha();
        if (misses == lvl_parts[level]) { won = 0; round_over(); }
    }
}

static void enter_key(unsigned char k, unsigned char l) {
    if (l && wlen < MAXLEN) {
        word[wlen++] = l;
        play(&snd_blip, tune_key);
    } else if (k == CH_DELETE && wlen) {
        --wlen;
    } else if (k == CH_RETURN) {
        if (wlen >= 3) begin_guessing();
        else message(s"at least 3 letters, please", C_ORANGE, s"", C_WHITE);
        return;
    }
    if (mode == M_ENTER) draw_entry();
}

/* ------------------------------------------------------------------------ */
/* The title                                                                */

static const unsigned char banner[6][5] = {
    { 0x11, 0x11, 0x1F, 0x11, 0x11 },   /* H */
    { 0x0E, 0x11, 0x1F, 0x11, 0x11 },   /* A */
    { 0x11, 0x19, 0x15, 0x13, 0x11 },   /* N */
    { 0x0F, 0x10, 0x13, 0x11, 0x0E },   /* G */
    { 0x1F, 0x10, 0x1E, 0x10, 0x1F },   /* E */
    { 0x1E, 0x11, 0x1E, 0x14, 0x13 },   /* R */
};
static const unsigned char banner_colour[5] = { C_YELLOW, C_YELLOW, C_ORANGE, C_ORANGE, C_RED };

static void title(void) {
    unsigned char i, r, b;
    mode = M_TITLE;
    load_level(0);
    draw_frames();
    draw_header();
    draw_picture(lvl_parts[0]);
    for (i = 0; i < 6; i++)
        for (r = 0; r < 5; r++)
            for (b = 0; b < 5; b++)
                if (banner[i][r] & (0x10 >> b))
                    put((unsigned char)(PANEL_X + 5 + i * 6 + b), (unsigned char)(3 + r), G_SOLID, banner_colour[r]);
    panel_text(9, s"hangman for the mega65", C_CYAN);
    panel_text(11, s"each level has a new victim:", C_LGREY);
    panel_text(12, s"a human, a cat, a duck, a giraffe,", C_LGREY);
    panel_text(13, s"an elephant and an octopus", C_LGREY);
    put_text(PANEL_X + 5, 16, s"1", C_YELLOW);
    put_text(PANEL_X + 9, 16, s"one player against the computer", C_WHITE);
    put_text(PANEL_X + 5, 18, s"2", C_YELLOW);
    put_text(PANEL_X + 9, 18, s"two players", C_WHITE);
    help(s"run/stop quits");
}

/* ------------------------------------------------------------------------ */

#ifdef DEMO
static const unsigned char demo_order[26] = {
    5, 20, 1, 15, 9, 14, 19, 8, 18, 4, 12, 21, 3, 13, 6, 23, 25, 16, 22, 2, 7, 11, 10, 17, 24, 26
};
static unsigned char demo_t;

static void demo(void) {
    unsigned char i;
    if (++demo_t < (mode == M_GUESS ? 25 : 120)) return;
    demo_t = 0;
    if (mode == M_TITLE) new_game(1);
    else if (mode == M_GUESS) {
        for (i = 0; i < 26 && tried[demo_order[i]]; i++) ;
        guess(demo_order[(rnd() & 3) == 0 && i < 24 ? i + 2 : i]);
    } else if (mode == M_DONE) next_round();
    else if (mode == M_OVER) title();
}
#endif

static unsigned char saved_regs[5];

static void setup(void) {
    saved_regs[0] = REG(0xD068); saved_regs[1] = REG(0xD069); saved_regs[2] = REG(0xD06A);
    saved_regs[3] = REG(0xD020); saved_regs[4] = REG(0xD021);
    dma_copy_far(CHARSET_LIN, CHARROM_LIN, 2048);
    dma_copy_far(CHARSET_LIN + FIXED0 * 8, (unsigned long)(unsigned int)fixed_glyphs, sizeof fixed_glyphs);
    REG(0xD068) = (unsigned char)CHARSET_LIN;
    REG(0xD069) = (unsigned char)(CHARSET_LIN >> 8);
    REG(0xD06A) = (unsigned char)(CHARSET_LIN >> 16);
    REG(0xD020) = C_BLACK;
    REG(0xD021) = C_BLACK;
    memset(scr, CH_SPACE, sizeof scr);
    memset(clr, C_WHITE, sizeof clr);
    show();
    sid_init();
}

static void restore(void) {
    sid_init();
    REG(0xD068) = saved_regs[0]; REG(0xD069) = saved_regs[1]; REG(0xD06A) = saved_regs[2];
    REG(0xD020) = saved_regs[3]; REG(0xD021) = saved_regs[4];
    dma_fill_far(SCREEN_LIN, COLS * ROWS, CH_SPACE);
    dma_fill_far(COLOR_LIN, COLS * ROWS, C_WHITE);
}

static unsigned char letter(unsigned char k) {   /* a key's letter 1-26, or 0 */
    if (k >= 0x41 && k <= 0x5A) return (unsigned char)(k - 0x40);
    if (k >= 0xC1 && k <= 0xDA) return (unsigned char)(k - 0xC0);
    return 0;
}

int main(void) {
    unsigned char frame_seen;
    setup();
    title();
    input_flush();
    frame_seen = FRAMECOUNT;
    for (;;) {
        unsigned char k;
        wait_vsync();
        while (frame_seen != FRAMECOUNT) {
            frame_seen++;
            snd_update();
        }
        rnd();                        /* stirred every frame: picks depend on timing */
#ifdef DEMO
        demo();
        k = input_key();
        if (k == CH_STOP) break;
#else
        k = input_key();
        if (k == CH_STOP) {
            if (mode == M_TITLE) break;
            title();
        } else if (mode == M_TITLE) {
            if (k == '1') new_game(1);
            else if (k == '2') new_game(2);
        } else if (mode == M_ENTER) {
            enter_key(k, letter(k));
        } else if (mode == M_GUESS) {
            if (letter(k)) guess(letter(k));
        } else if (k == CH_RETURN || k == ' ') {
            if (mode == M_DONE) next_round();
            else title();
        }
#endif
        if (mode == M_ENTER) draw_entry();   /* the cursor blinks */
        show();
    }
    restore();
    return 0;
}
