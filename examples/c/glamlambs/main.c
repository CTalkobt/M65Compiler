/* Glam Lambs -- a sheep makeover game for the MEGA65, built with cc45
 *
 * Kidnap sheep from the field, give them a makeover in the salon (wool
 * dye, lipstick, eyelashes, a bow) and let them go again. The score comes
 * only from how the made-up sheep breed: a lamb inherits each of its
 * parents' looks, and every glam trait a lamb is born with scores, more
 * for later generations and double for the season's trend.
 *
 * Keys: cursor keys / WASD or a joystick in either port move, SPACE /
 *       RETURN / FIRE grab, put down and confirm (and shoo the shearer),
 *       M music on / off, RUN/STOP quits.
 *
 * A minute into the first season (sooner each season after) Farmer Fergus
 * comes out to shear the made-up sheep back to natural. Music: Old MacDonald, Baa, Baa, Black Sheep and
 * Mary Had a Little Lamb in turn in the field, a slow bump and grind in
 * the salon: music.txt, played by the library's tune player (<audio_tune.h>)
 * from bank 5, where the game loads it (GLAMMUSIC on the disk).
 *
 * The screen is 80 x 25 text in multicolour character mode, with our
 * glyphs in a copy of the character set in bank 4. A cell coloured 8-15
 * is multicolour (its own colour is then 0-7), so text and the hi-res
 * glyphs keep to colours 0-7, and the game sets palette entries 0-7 to
 * its own colours (restored at the end). Every frame the whole screen is
 * built in two buffers (characters: the ROM's screen RAM at $0800, which
 * the VIC-IV does not show meanwhile; colours) and copied out by DMA to
 * the screen shown (bank 5) and colour RAM.
 *
 * The salon is a full-screen room (when GLAMSALON and GLAMSALON2 are on
 * the disk; else a panel beside the field): the sheep in a salon chair
 * and its face in the vanity's mirror, drawn by gen_salon.py in the
 * VIC-IV's full-colour characters (a byte a pixel, any of 256 colours) on
 * a screen of two-byte cells (<seam.h>) with the score and message lines
 * as text. The wool's dye and the lipstick are palette entries, the looks
 * (lashes, a bow, a ram's horns, a blink) cells swapped in.
 *
 * Build with -DDEMO to play by itself (for screenshots and soak tests).
 *
 * cc45's plain char/int/long are unsigned: signed values say so.
 */
#include <string.h>
#include <dma.h>
#include <mega65.h>
#include <audio_sid.h>
#include <audio_tune.h>
#include <cbm.h>
#include <input.h>
#include <palette.h>
#include <seam.h>

#define CH_BASE 0x60                  /* our glyphs replace screen codes $60.. */
#include "gfx.h"
#include "music.h"                    /* tune45 convert --bin music.txt */
#include "salon.h"                    /* gen_salon.py: the salon's picture */

#ifndef REG
#define REG(a) (*(volatile unsigned char *)(a))
#endif

#define COLS 80
#define ROWS 25
#define SCREEN_LIN  0x0000800UL       /* the ROM's screen: our back buffer (scr) */
#define SHOWN_LIN   0x005E000UL       /* the screen shown (bank 5) */
#define COLOR_LIN   0xFF80000UL
#define CHARROM_LIN 0xFF7E000UL       /* the MEGA65 character set */
#define CHARSET_LIN 0x004F800UL       /* our copy: bank 4 */
#define CHARSET2_LIN 0x004F000UL      /* the title's: the ROM's upper/lower case set */
#define FRAMECOUNT  REG(0xD7FA)
#define CRSR_LEFTUP REG(0xD60F)       /* bit 0 cursor left, bit 1 cursor up */

/* colours 0-7: the game's palette (see setup) */
#define C_BLACK 0
#define C_WHITE 1
#define C_RED 2
#define C_LBLUE 3
#define C_PURPLE 4
#define C_BROWN 5
#define C_PINK 6
#define C_YELLOW 7
#define MCOL 8                        /* + a colour: a multicolour cell */
/* the shared colours (any of 0-15) */
#define C_DGREY 11                    /* $D022: faces, legs */
#define C_MEADOW 12                   /* $D021: the background */
#define C_TUFT 13                     /* $D023: grass */

#define CH_SPACE 0x20

/* layout: the field (fenced) on the left, the salon panel on the right */
#define HUD_ROW  0
#define MSG_ROW  24
#define FENCE_T  1
#define FENCE_B  23
#define FENCE_L  0
#define FENCE_R  59
#define FX0 1                         /* the field's inside */
#define FX1 58
#define FY0 2
#define FY1 22
#define BGW 60                        /* the background: rows 1-23, columns 0-59 */
#define BGH 23
#define GATE_Y0 11                    /* the salon door, in the right fence */
#define GATE_Y1 13
#define PANEL_X 61                    /* the panel's frame: columns 61-79 */
#define PANEL_IN 63                   /* its text */
#define BIG_X 67                      /* the big sheep: 6 x 4 cells (bow row on top) */
#define BIG_Y 3

#define SW 4                          /* a sheep: 4 x 2 cells */
#define SH 2
#define PW 2                          /* the stylist: 2 x 2 */
#define PH 2

/* ------------------------------------------------------------------------ */
/* The screen, built here and copied by DMA once a frame                    */

#define scr ((unsigned char *)0x0800)
static unsigned char clr[COLS * ROWS];
/* the field's background (characters, colours: rows 1-23, columns 0-59),
 * made once, kept in bank 5 */
#define BGK_LIN 0x5C000UL
#define BGC_LIN 0x5C800UL

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

static void put_number(unsigned char x, unsigned char y, unsigned int v, unsigned char digits, unsigned char c) {
    unsigned int o = (unsigned int)y * COLS + x + digits;
    while (digits--) {
        --o;
        scr[o] = (unsigned char)(0x30 + v % 10);
        clr[o] = c;
        v /= 10;
    }
}

static void clear_span(unsigned char x, unsigned char y, unsigned char n) {
    unsigned int o = (unsigned int)y * COLS + x;
    while (n--) scr[o++] = CH_SPACE;
}

static void show(void) {
    dma_copy_far(SHOWN_LIN, SCREEN_LIN, COLS * ROWS);
    dma_copy_far(COLOR_LIN, (unsigned long)(unsigned int)clr, COLS * ROWS);
}

static void set_charset(unsigned long lin) {
    REG(0xD068) = (unsigned char)lin;
    REG(0xD069) = (unsigned char)(lin >> 8);
    REG(0xD06A) = (unsigned char)(lin >> 16);
}

/* text in plain ASCII, for the upper/lower case set: a-z are screen codes
 * 1-26, A-Z $41-$5A */
static void put_ascii(unsigned char x, unsigned char y, const char *s, unsigned char c) {
    unsigned int o = (unsigned int)y * COLS + x;
    while (*s) {
        unsigned char ch = (unsigned char)*s++;
        if (ch >= 'a' && ch <= 'z') ch = (unsigned char)(ch - 'a' + 1);
        else if (ch >= 'A' && ch <= 'Z') ch = (unsigned char)(ch - 'A' + 0x41);
        scr[o] = ch;
        clr[o++] = c;
    }
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
static unsigned char rnd_n(unsigned char n) {    /* 0 .. n - 1 */
    return (unsigned char)((rnd() & 0xFF) * n >> 8);
}

/* ------------------------------------------------------------------------ */
/* Sound (<audio_sid.h>): voice 0 the sheep, 1 births, 2 clicks             */

#define V_BAA   0
#define V_BIRTH 1
#define V_CLICK 2

static const sid_instrument_t snd_baa_i   = { SID_SAWTOOTH, 0x16, 0x84, 0 };
static const sid_instrument_t snd_birth_i = { SID_TRIANGLE, 0x08, 0x00, 0 };
static const sid_instrument_t snd_click_i = { SID_PULSE,    0x02, 0x00, 0x0400 };

static unsigned char baa_t, birth_t, click_t;
static unsigned int baa_f;
static const unsigned int birth_notes[4] = {0x1125, 0x159A, 0x19B1, 0x224B};   /* C E G C */

static void snd_start(unsigned char v, const sid_instrument_t *inst, unsigned int freq) {
    sid_gate(v, 0);
    sid_set_instrument(v, inst);
    sid_set_freq_reg(v, freq);
    sid_gate(v, 1);
}

static void snd_baa(void) {
    baa_f = 0x0C00 + (rnd() & 0x3FF);
    snd_start(V_BAA, &snd_baa_i, baa_f);
    baa_t = 24;
}

static void snd_birth(void) {
    snd_start(V_BIRTH, &snd_birth_i, birth_notes[0]);
    birth_t = 24;
}

static void snd_click(unsigned int f) {
    snd_start(V_CLICK, &snd_click_i, f);
    click_t = 3;
}

/* once a frame: the bleat's warble, the arpeggio, releases */
static void snd_update(void) {
    if (baa_t) {
        baa_t--;
        sid_set_freq_reg(V_BAA, baa_f + ((baa_t & 2) ? 0x80 : 0) - (unsigned int)(24 - baa_t) * 16);
        if (baa_t == 0) sid_gate(V_BAA, 0);
    }
    if (birth_t) {
        birth_t--;
        if ((birth_t & 7) == 0 && birth_t) snd_start(V_BIRTH, &snd_birth_i, birth_notes[3 - (birth_t >> 3)]);
        if (birth_t == 0) sid_gate(V_BIRTH, 0);
    }
    if (click_t && --click_t == 0) sid_gate(V_CLICK, 0);
}

/* ------------------------------------------------------------------------ */
/* Music: a nine-voice band on SIDs 2-4 (SID 1 is the sound effects')       */
/*                                                                          */
/*   SID 2 (left)   3 lead, 4 bass, 5 kick                                  */
/*   SID 3 (right)  6-8 the chords                                          */
/*   SID 4 (right)  9 the lead an octave down, 10 snare, 11 hi-hat          */
/*                                                                          */
/* The tunes are music.txt: tune45 convert --bin makes GLAMMUSIC (on the    */
/* disk) and music.h. music_load puts them in bank 5 and the library's tune */
/* player plays them from there: farm (Old MacDonald, Baa, Baa, Black Sheep */
/* and Mary Had a Little Lamb in turn) in the field, salon in the salon.    */
/* Without the file the game has no music.                                  */

#define MUSIC_LIN 0x0050000UL         /* the tunes: bank 5 */
#define V_BAND 3                      /* the band's first voice: SID 2's first */
#define MUS_FARM 1
#define MUS_SALON 2

static unsigned char music_ok, music_on = 1;
static unsigned char music_at;        /* the tune playing: 0, MUS_FARM, MUS_SALON */
/* where the farm tunes were, to carry on from there after the salon */
static unsigned int farm_pos;
static unsigned char farm_saved;

/* GLAMMUSIC into bank 5 from the drive the game was loaded from */
static void music_load(void) {
    static char name[] = p"glammusic";
    unsigned char dev = *(volatile unsigned char *)0x00BA, n;
    if (dev < 8) dev = 8;
    if (cbm_load_far(name, sizeof name - 1, dev, MUSIC_LIN)) return;
    dma_copy_far((unsigned long)(unsigned int)&n, MUSIC_LIN, 1);   /* its tunes */
    music_ok = n == NTUNES;
    tune_voices(V_BAND);
}

static void music_stop(void) {
    if (music_at == MUS_FARM) {
        farm_pos = tune_ticks() % TUNE_FARM_TICKS;
        farm_saved = 1;
    }
    tune_stop();
    music_at = 0;
}

static void music_play(unsigned char which) {
    if (music_at == which) return;
    music_stop();
    if (!music_ok) return;
    tune_play_far(MUSIC_LIN + (which == MUS_FARM ? TUNE_FARM_AT : TUNE_SALON_AT));
    if (which == MUS_FARM && farm_saved) tune_seek(farm_pos);
    music_at = which;
}

/* from the top (a new season, the title) */
static void music_restart(void) {
    music_stop();
    farm_saved = 0;
}

static void music_toggle(void) {
    music_on ^= 1;
    if (!music_on) music_stop();
    else if (!music_at) music_play(MUS_FARM);
}

/* the tunes for where we are */
static void music_for(unsigned char salon) {
    if (music_on) music_play(salon ? MUS_SALON : MUS_FARM);
}

/* ------------------------------------------------------------------------ */
/* Looks: the traits a sheep shows, the ones the salon changes              */

#define L_WOOL   0x03                 /* 0 natural, pink, blue, gold */
#define L_LIPS   0x0C                 /* 0 natural, red, purple, pink */
#define L_LASH   0x10
#define L_BOW    0x20

static const unsigned char wool_color[4] = {C_WHITE, C_PINK, C_LBLUE, C_YELLOW};
static const unsigned char lips_color[4] = {C_BLACK, C_RED, C_PURPLE, C_PINK};

static const char *const wool_name[4] = {s"natural", s"pink", s"blue", s"gold"};
static const char *const lips_name[4] = {s"natural", s"red", s"purple", s"pink"};
static const char *const yesno_name[2] = {s"no", s"yes"};

/* the glam traits of a look (0 - 4) */
static unsigned char glam(unsigned char look) {
    unsigned char n = 0;
    if (look & L_WOOL) n++;
    if (look & L_LIPS) n++;
    if (look & L_LASH) n++;
    if (look & L_BOW) n++;
    return n;
}

/* the season's trend: a trait value worth double */
#define NTRENDS 8
static const unsigned char trend_mask[NTRENDS] = {L_WOOL, L_WOOL, L_WOOL, L_LIPS, L_LIPS, L_LIPS, L_LASH, L_BOW};
static const unsigned char trend_val[NTRENDS] = {0x01, 0x02, 0x03, 0x04, 0x08, 0x0C, L_LASH, L_BOW};
static const char *const trend_name[NTRENDS] = {
    s"pink wool", s"blue wool", s"gold wool", s"red lips", s"purple lips", s"pink lips",
    s"long lashes", s"bows"};
static unsigned char trend;

static unsigned char on_trend(unsigned char look) {
    return (look & trend_mask[trend]) == trend_val[trend];
}

/* ------------------------------------------------------------------------ */
/* The flock                                                                */

#define MAXS 32
#define F_ALIVE   0x01
#define F_RAM     0x02
#define F_LEFT    0x04                /* facing left (head in the left column) */
#define F_CARRIED 0x08
#define F_STEP    0x10                /* which legs are forward */

#define ADULT_AGE 12                  /* seconds */
#define EWE_REST  10                  /* seconds between lambs */
#define RAM_REST  5
#define MAX_GEN   5
#define WOOL_FULL 3                   /* a fleece: shorn 0, short 1-2, full 3 */
#define WOOL_GROW 12                  /* seconds a step of regrowth */
#define MAX_SHEARS 6                  /* shorn this often, a sheep passes on */
#define BAG_PRICE 10                  /* a bag of wool; dyed: */
#define DYED_PRICE 25

/* a sheep takes SW x SH cells from (x, y) */
static unsigned char s_x[MAXS], s_y[MAXS], s_flags[MAXS], s_look[MAXS];
static unsigned char s_age[MAXS], s_life[MAXS], s_cool[MAXS], s_gen[MAXS], s_tmr[MAXS];
static unsigned char s_wool[MAXS], s_woolt[MAXS], s_shear[MAXS];   /* fleece 0-3, regrowth, times shorn */
static unsigned char flock;

/* the player: PW x PH cells from (p_x, p_y) */
static unsigned char p_x, p_y, p_tmr, p_step;
static unsigned char carry = 0xFF;    /* the sheep carried, or none */

/* Farmer Fergus, the shearer: PW x PH cells from (sh_x, sh_y). He comes
 * out SHEARER_FIRST seconds into the first season, SHEARER_SOONER seconds
 * sooner each season after (but not before SHEARER_SOONEST), hunts sheep in
 * full fleece and shears them back to natural (SNIP_FRAMES at it); FIRE
 * beside him shoos him off for a while */
#define SHEARER_FIRST   60
#define SHEARER_SOONER  15
#define SHEARER_SOONEST 10
#define SNIP_FRAMES 100
#define SHOO_TIME   10                /* seconds */
static unsigned char sh_on, sh_x, sh_y, sh_tmr, sh_step, sh_snip, sh_rest;
static unsigned char sh_target = 0xFF;

static unsigned char shearer_in(unsigned char x, unsigned char y, unsigned char w, unsigned char h) {
    return sh_on && sh_x < x + w && x < sh_x + PW && sh_y < y + h && y < sh_y + PH;
}

/* hearts and sparkles over the field */
#define NFX 6
static unsigned char fx_x[NFX], fx_y[NFX], fx_t[NFX], fx_ch[NFX], fx_c[NFX];

static void add_fx(unsigned char x, unsigned char y, unsigned char ch, unsigned char c) {
    unsigned char i, best = 0;
    for (i = 0; i < NFX; i++) if (fx_t[i] < fx_t[best]) best = i;
    fx_x[best] = x; fx_y[best] = y; fx_ch[best] = ch; fx_c[best] = c; fx_t[best] = 60;
}

static unsigned char adult(unsigned char i) {
    return s_age[i] >= ADULT_AGE;
}

/* the first sheep in the field (other than skip) overlapping the w x h
 * cells at x, y, or 0xFF */
static unsigned char sheep_in(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                              unsigned char skip) {
    unsigned char i;
    for (i = 0; i < MAXS; i++) {
        if (i == skip || (s_flags[i] & (F_ALIVE | F_CARRIED)) != F_ALIVE) continue;
        if (s_x[i] < x + w && x < s_x[i] + SW && s_y[i] < y + h && y < s_y[i] + SH) return i;
    }
    return 0xFF;
}

/* can sheep i (or a new one, i = 0xFF) stand at x, y? */
static unsigned char spot_free(unsigned char x, unsigned char y, unsigned char i) {
    if (x < FX0 || x + SW - 1 > FX1 || y < FY0 || y + SH - 1 > FY1) return 0;
    if (p_x < x + SW && x < p_x + PW && p_y < y + SH && y < p_y + PH) return 0;
    if (shearer_in(x, y, SW, SH)) return 0;
    return sheep_in(x, y, SW, SH, i) == 0xFF;
}

/* a free spot near x, y (searching outwards), 1 when found: in fx_, fy_ */
static unsigned char fx_, fy_;
static unsigned char find_spot(unsigned char x, unsigned char y) {
    unsigned char r;
    for (r = 0; r < 8; r++) {
        signed char dx, dy;
        for (dy = -(signed char)r; dy <= (signed char)r; dy++)
            for (dx = -(signed char)r * 2; dx <= (signed char)r * 2; dx++) {
                signed int nx = (signed int)x + dx, ny = (signed int)y + dy;
                if (nx < 0 || ny < 0) continue;
                if (spot_free((unsigned char)nx, (unsigned char)ny, 0xFF)) {
                    fx_ = (unsigned char)nx; fy_ = (unsigned char)ny;
                    return 1;
                }
            }
    }
    return 0;
}

static unsigned char new_sheep(unsigned char x, unsigned char y, unsigned char look, unsigned char ram,
                               unsigned char age, unsigned char gen) {
    unsigned char i;
    for (i = 0; i < MAXS; i++) {
        if (s_flags[i] & F_ALIVE) continue;
        s_x[i] = x; s_y[i] = y;
        s_flags[i] = (unsigned char)(F_ALIVE | (ram ? F_RAM : 0) | (rnd() & 1 ? F_LEFT : 0));
        s_look[i] = look;
        s_age[i] = age;
        s_life[i] = (unsigned char)(90 + rnd_n(60));
        s_cool[i] = (unsigned char)(ram ? 0 : 3 + rnd_n(6));
        s_gen[i] = gen;
        s_tmr[i] = (unsigned char)(10 + rnd_n(40));
        s_wool[i] = age >= ADULT_AGE ? WOOL_FULL : 1;
        s_woolt[i] = s_shear[i] = 0;
        flock++;
        return i;
    }
    return 0xFF;
}

/* ------------------------------------------------------------------------ */
/* Game state                                                               */

enum { M_TITLE, M_PLAY, M_SALON, M_OVER };
static unsigned char mode;
static unsigned char fps;              /* 50 (PAL) or 60 (NTSC) */
static unsigned char sec_frames;       /* frames to the next second */
static unsigned int time_left;         /* seconds of the season */
static unsigned char season;           /* the season (1, 2, ...) since the title */
static unsigned int shearer_at;        /* time_left when Fergus comes out */
static unsigned char trend_t;          /* seconds to the next trend */
static unsigned int score, hiscore;
static unsigned int hiscore_was;       /* the high score when the season began */
static unsigned char over_t;           /* frames before the season-over box takes fire */
static unsigned int lambs, glam_lambs;
static unsigned int lamb_pts, wool_bags, income;   /* the score: lamb_pts + income */
static unsigned char msg_t;
static unsigned char sel;              /* the salon's row */
static unsigned char hud_dirty, panel_dirty;

/* the attract mode: after IDLE_T seconds on the title without a key or the
 * joystick touched (any key starts the wait again), the
 * computer plays for ATTRACT_T seconds (any input ends it; fire plays) */
#define IDLE_T    60
#define ATTRACT_T 40
static unsigned char attract;
static unsigned int idle_t, attract_t;

#ifndef SEASON
#define SEASON  180                   /* seconds (tests: -DSEASON=n, at least SHEARER_FIRST) */
#endif
#define TREND_T 30

static void message(const char *s, unsigned char c) {
    unsigned char n = (unsigned char)strlen(s);
    clear_span(0, MSG_ROW, COLS);
    put_text((unsigned char)((COLS - n) / 2), MSG_ROW, s, c);
    msg_t = 150;
}

/* a message with a number in it: text, the number, more text */
static void message_n(const char *a, unsigned int v, const char *b, unsigned char c) {
    unsigned char na = (unsigned char)strlen(a), nb = (unsigned char)strlen(b), nd = 1, x;
    unsigned int t = v;
    while (t >= 10) { t /= 10; nd++; }
    clear_span(0, MSG_ROW, COLS);
    x = (unsigned char)((COLS - na - nd - nb) / 2);
    put_text(x, MSG_ROW, a, c);
    put_number((unsigned char)(x + na), MSG_ROW, v, nd, c);
    put_text((unsigned char)(x + na + nd), MSG_ROW, b, c);
    msg_t = 150;
}

/* ------------------------------------------------------------------------ */
/* Breeding                                                                 */

/* a look made of the parents': each trait from one of them, now and then
 * lost (back to natural) */
static unsigned char inherit(unsigned char ma, unsigned char pa) {
    unsigned char look = 0;
    look |= (rnd() & 1 ? ma : pa) & L_WOOL;
    look |= (rnd() & 1 ? ma : pa) & L_LIPS;
    look |= (rnd() & 1 ? ma : pa) & L_LASH;
    look |= (rnd() & 1 ? ma : pa) & L_BOW;
    if (rnd_n(8) == 0) look &= (unsigned char)~L_WOOL;
    if (rnd_n(8) == 0) look &= (unsigned char)~L_LIPS;
    if (rnd_n(8) == 0) look &= (unsigned char)~L_LASH;
    if (rnd_n(8) == 0) look &= (unsigned char)~L_BOW;
    return look;
}

/* the points of a newborn: 5 a glam trait, times its generation (up to
 * MAX_GEN), double on trend, 50 more with all four */
static void score_lamb(unsigned char look, unsigned char gen) {
    unsigned char t = glam(look);
    unsigned int pts;
    lambs++;
    if (mode != M_PLAY && mode != M_SALON) return;
    hud_dirty = 1;
    if (t == 0) {
        message(s"a plain lamb is born. no points", C_LBLUE);
        return;
    }
    glam_lambs++;
    pts = (unsigned int)t * 5 * (gen > MAX_GEN ? MAX_GEN : gen);
    if (on_trend(look)) pts *= 2;
    if (t == 4) {
        pts += 50;
        message_n(s"a supermodel lamb! +", pts, s"", C_YELLOW);
    } else if (on_trend(look)) {
        message_n(s"a glam lamb, right on trend! +", pts, s"", C_WHITE);
    } else {
        message_n(s"a glam lamb is born! +", pts, s"", C_WHITE);
    }
    lamb_pts += pts;
    score += pts;
    if (!attract && score > hiscore) hiscore = score;
}

/* once a second: each ewe ready for a lamb looks for a ready ram beside
 * her; made-up sheep are the more attractive */
static void breed(void) {
    unsigned char e, r;
    for (e = 0; e < MAXS; e++) {
        if ((s_flags[e] & (F_ALIVE | F_RAM | F_CARRIED)) != F_ALIVE || !adult(e) || s_cool[e]) continue;
        for (r = 0; r < MAXS; r++) {
            signed char dx, dy;
            unsigned char lamb;
            if ((s_flags[r] & (F_ALIVE | F_RAM | F_CARRIED)) != (F_ALIVE | F_RAM) || !adult(r) || s_cool[r]) continue;
            dx = (signed char)(s_x[r] - s_x[e]);
            dy = (signed char)(s_y[r] - s_y[e]);
            if (dx < -6 || dx > 6 || dy < -2 || dy > 2) continue;
            if (rnd_n(16) >= 5 + glam(s_look[e]) + glam(s_look[r])) continue;
            add_fx((unsigned char)(s_x[e] + 2), s_y[e] > FY0 ? s_y[e] - 1 : s_y[e], G_HEART, C_PINK);
            s_cool[r] = RAM_REST;
            s_cool[e] = EWE_REST;
            if (flock >= MAXS || !find_spot(s_x[e], (unsigned char)(s_y[e] + SH))) {
                if (mode == M_PLAY || mode == M_SALON) message(s"the field is full: no room for a lamb", C_LBLUE);
                break;
            }
            lamb = new_sheep(fx_, fy_, inherit(s_look[e], s_look[r]), rnd() & 1, 0,
                             (unsigned char)((s_gen[e] > s_gen[r] ? s_gen[e] : s_gen[r]) + 1));
            if (lamb == 0xFF) break;
            snd_birth();
            score_lamb(s_look[lamb], s_gen[lamb]);
            break;
        }
    }
}

/* once a second: ages, rests, old sheep leaving the field */
static void tick_flock(void) {
    unsigned char i;
    for (i = 0; i < MAXS; i++) {
        if (!(s_flags[i] & F_ALIVE)) continue;
        if (s_age[i] < 255) s_age[i]++;
        if (s_cool[i]) s_cool[i]--;
        if (s_wool[i] < WOOL_FULL && ++s_woolt[i] >= WOOL_GROW) {
            s_woolt[i] = 0;
            s_wool[i]++;
        }
        if (s_age[i] >= s_life[i] && i != carry) {
            s_flags[i] = 0;
            flock--;
            hud_dirty = 1;
        }
    }
}

/* ------------------------------------------------------------------------ */
/* Sheep moving about                                                       */

static signed char sgn(signed int v) {
    return v < 0 ? -1 : v > 0 ? 1 : 0;
}

static unsigned char try_move(unsigned char i, signed char dx, signed char dy) {
    unsigned char nx = (unsigned char)(s_x[i] + dx), ny = (unsigned char)(s_y[i] + dy);
    if (!spot_free(nx, ny, i)) return 0;
    s_x[i] = nx;
    s_y[i] = ny;
    s_flags[i] ^= F_STEP;
    if (dx > 0) s_flags[i] &= (unsigned char)~F_LEFT;
    if (dx < 0) s_flags[i] |= F_LEFT;
    return 1;
}

/* the nearest ewe ready for a lamb, or 0xFF */
static unsigned char nearest_ewe(unsigned char r) {
    unsigned char i, best = 0xFF;
    unsigned int bd = 0xFFFF;
    for (i = 0; i < MAXS; i++) {
        unsigned int d;
        if ((s_flags[i] & (F_ALIVE | F_RAM | F_CARRIED)) != F_ALIVE || !adult(i) || s_cool[i]) continue;
        d = (unsigned int)(s_x[i] > s_x[r] ? s_x[i] - s_x[r] : s_x[r] - s_x[i]) +
            (unsigned int)(s_y[i] > s_y[r] ? s_y[i] - s_y[r] : s_y[r] - s_y[i]) * 2;
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

static void move_sheep(void) {
    unsigned char i;
    for (i = 0; i < MAXS; i++) {
        signed char dx = 0, dy = 0;
        signed int px, py;
        if ((s_flags[i] & (F_ALIVE | F_CARRIED)) != F_ALIVE) continue;
        if (sh_snip && i == sh_target) continue;   /* held for the shears */
        if (s_tmr[i]) { s_tmr[i]--; continue; }
        px = (signed int)s_x[i] + 1 - p_x;   /* the stylist's middle to the sheep's */
        py = (signed int)s_y[i] - p_y;
        if ((mode == M_PLAY) && px >= -6 && px <= 6 && py >= -3 && py <= 3) {
            /* the stylist is near: run */
            dx = px ? sgn(px) : (signed char)(rnd() & 1 ? 1 : -1);
            dy = sgn(py);
            if (rnd_n(4) == 0) dy = (signed char)(rnd_n(3) - 1);
            s_tmr[i] = (unsigned char)(8 + rnd_n(8));
            if (rnd_n(16) == 0) snd_baa();
        } else {
            unsigned char e = 0xFF;
            if ((s_flags[i] & F_RAM) && adult(i) && !s_cool[i] && rnd_n(2)) e = nearest_ewe(i);
            if (e != 0xFF) {
                signed int ex = (signed int)s_x[e] - s_x[i], ey = (signed int)s_y[e] - s_y[i];
                dx = sgn(ex);
                dy = sgn(ey);
                if (ex >= -(SW + 1) && ex <= SW + 1 && ey >= -1 && ey <= 1) dx = dy = 0;   /* beside her */
            } else if (rnd_n(5) >= 2) {
                dx = (signed char)(rnd_n(3) - 1);
                dy = (signed char)(rnd_n(3) - 1);
            }
            s_tmr[i] = (unsigned char)(adult(i) ? 20 + rnd_n(40) : 12 + rnd_n(20));
        }
        if (!dx && !dy) continue;
        if (!try_move(i, dx, dy) && !(dx && try_move(i, dx, 0)) && dy) try_move(i, 0, dy);
    }
}

/* ------------------------------------------------------------------------ */
/* The field: background, then sheep, effects, the player                   */

static void make_background(void) {
    unsigned char x, y;
    for (y = 0; y < BGH; y++)
        for (x = 0; x < BGW; x++) {
            unsigned int o = (unsigned int)(y + FENCE_T) * COLS + x;   /* (made in the screen's buffers) */
            unsigned char ch = CH_SPACE, c = C_WHITE, row = (unsigned char)(y + FENCE_T);
            if (row == FENCE_T || row == FENCE_B) {
                ch = (x == FENCE_L || x == FENCE_R) ? G_POST : G_FENCE_H;
                c = C_BROWN;
            } else if (x == FENCE_L || x == FENCE_R) {
                if (x == FENCE_R && row >= GATE_Y0 && row <= GATE_Y1) { ch = G_GATE; c = C_YELLOW; }
                else if (x == FENCE_R && (row == GATE_Y0 - 1 || row == GATE_Y1 + 1)) { ch = G_POST; c = C_BROWN; }
                else { ch = G_FENCE_V; c = C_BROWN; }
            } else {
                unsigned char r = rnd_n(32);
                if (r < 2) { ch = G_GRASS1; c = MCOL; }
                else if (r < 4) { ch = G_GRASS2; c = MCOL; }
                else if (r == 4) { ch = G_FLOWER; c = (rnd() & 1) ? C_YELLOW : C_WHITE; }
            }
            scr[o] = ch;
            clr[o] = c;
        }
    for (y = 0; y < BGH; y++) {
        unsigned int o = (unsigned int)(y + FENCE_T) * COLS;
        dma_copy_far(BGK_LIN + (unsigned int)y * BGW, (unsigned long)(unsigned int)(scr + o), BGW);
        dma_copy_far(BGC_LIN + (unsigned int)y * BGW, (unsigned long)(unsigned int)(clr + o), BGW);
    }
    memset(scr, CH_SPACE, COLS * ROWS);
}

/* a sheep's 4 x 2 cells: three of wool (and legs), the head. Each _L
 * glyph follows its _R; a wool row is cells 0 1 2, each _R _L */
static void draw_sheep_at(unsigned char x, unsigned char y, unsigned char i) {
    unsigned char f = s_flags[i], look = s_look[i], top, bot, head, muz, k;
    unsigned char wc = MCOL + wool_color[look & L_WOOL];
    unsigned char mc = MCOL + ((look & L_LIPS) ? lips_color[(look & L_LIPS) >> 2] : C_RED);
    if (!adult(i)) {
        top = G_LAMB_T0_R;
        bot = (f & F_STEP) ? G_LAMB_BB0_R : G_LAMB_BA0_R;
        head = (look & L_LASH) ? G_LAMBTOPLASH_R : G_LAMBTOP_R;
        muz = G_LAMBMUZ_R;
    } else {
        if (s_wool[i] >= WOOL_FULL) {
            top = G_WOOL_T0_R;
            bot = (f & F_STEP) ? G_WOOL_BB0_R : G_WOOL_BA0_R;
        } else if (s_wool[i]) {
            top = G_SHORT_T0_R;
            bot = (f & F_STEP) ? G_SHORT_BB0_R : G_SHORT_BA0_R;
        } else {                          /* bare: pink skin */
            top = G_SHORN_T0_R;
            bot = (f & F_STEP) ? G_SHORN_BB0_R : G_SHORN_BA0_R;
            wc = MCOL + C_PINK;
        }
        if (f & F_RAM) head = (look & L_LASH) ? G_RAMLASH_R : G_RAM_R;
        else head = (look & L_LASH) ? G_EWELASH_R : G_EWE_R;
        muz = G_MUZ_R;
    }
    /* the muzzles: plain, lips, bow, lips and bow */
    if (look & L_LIPS) muz += 2;
    if (look & L_BOW) muz += 4;
    if (f & F_LEFT) {
        put(x, y, (unsigned char)(head + 1), MCOL + C_BLACK);
        put(x, (unsigned char)(y + 1), (unsigned char)(muz + 1), mc);
        for (k = 0; k < 3; k++) {
            put((unsigned char)(x + 1 + k), y, (unsigned char)(top + 5 - 2 * k), wc);
            put((unsigned char)(x + 1 + k), (unsigned char)(y + 1), (unsigned char)(bot + 5 - 2 * k), wc);
        }
    } else {
        for (k = 0; k < 3; k++) {
            put((unsigned char)(x + k), y, (unsigned char)(top + 2 * k), wc);
            put((unsigned char)(x + k), (unsigned char)(y + 1), (unsigned char)(bot + 2 * k), wc);
        }
        put((unsigned char)(x + 3), y, head, MCOL + C_BLACK);
        put((unsigned char)(x + 3), (unsigned char)(y + 1), muz, mc);
    }
}

static void draw_player(void) {
    unsigned char g = p_step ? G_PLAYER_B0 : G_PLAYER_A0;
    put(p_x, p_y, g, MCOL + C_YELLOW);
    put((unsigned char)(p_x + 1), p_y, (unsigned char)(g + 1), MCOL + C_YELLOW);
    put(p_x, (unsigned char)(p_y + 1), (unsigned char)(g + 2), MCOL + C_PINK);
    put((unsigned char)(p_x + 1), (unsigned char)(p_y + 1), (unsigned char)(g + 3), MCOL + C_PINK);
}

/* the shearer: the stylist's figure, white-haired, in brown overalls */
static void draw_shearer(void) {
    unsigned char g = sh_step ? G_PLAYER_B0 : G_PLAYER_A0;
    put(sh_x, sh_y, g, MCOL + C_WHITE);
    put((unsigned char)(sh_x + 1), sh_y, (unsigned char)(g + 1), MCOL + C_WHITE);
    put(sh_x, (unsigned char)(sh_y + 1), (unsigned char)(g + 2), MCOL + C_BROWN);
    put((unsigned char)(sh_x + 1), (unsigned char)(sh_y + 1), (unsigned char)(g + 3), MCOL + C_BROWN);
}

static void draw_field(void) {
    unsigned char y, i;
    for (y = 0; y < BGH; y++) {
        unsigned int o = (unsigned int)(y + FENCE_T) * COLS;
        dma_copy_far((unsigned long)(unsigned int)(scr + o), BGK_LIN + (unsigned int)y * BGW, BGW);
        dma_copy_far((unsigned long)(unsigned int)(clr + o), BGC_LIN + (unsigned int)y * BGW, BGW);
    }
    for (i = 0; i < MAXS; i++)
        if ((s_flags[i] & (F_ALIVE | F_CARRIED)) == F_ALIVE) draw_sheep_at(s_x[i], s_y[i], i);
    for (i = 0; i < NFX; i++) {
        if (!fx_t[i]) continue;
        fx_t[i]--;
        if (fx_t[i] & 0x08 || fx_t[i] > 16) put(fx_x[i], fx_y[i], fx_ch[i], fx_c[i]);
    }
    if (sh_on) draw_shearer();
    if (mode == M_PLAY || mode == M_SALON) {
        if (carry != 0xFF) {              /* held up over the stylist's head */
            unsigned char cx = p_x > FX1 - SW + 1 ? FX1 - SW + 1 : p_x - 1;
            draw_sheep_at(cx < FX0 ? FX0 : cx, p_y - SH > FENCE_T ? p_y - SH : FENCE_T, carry);
        }
        draw_player();
    }
}

/* ------------------------------------------------------------------------ */
/* The HUD and the salon panel                                              */

static void draw_hud(void) {
    clear_span(0, HUD_ROW, COLS);
    put_text(0, HUD_ROW, s"score", C_WHITE);
    put_number(6, HUD_ROW, score, 5, C_YELLOW);
    put_text(13, HUD_ROW, s"time", C_WHITE);
    put_number(18, HUD_ROW, time_left / 60, 1, C_YELLOW);
    put_text(19, HUD_ROW, s":", C_YELLOW);
    put_number(20, HUD_ROW, time_left % 60, 2, C_YELLOW);
    put_text(24, HUD_ROW, s"flock", C_WHITE);
    put_number(30, HUD_ROW, flock, 2, C_YELLOW);
    put_text(34, HUD_ROW, s"lambs", C_WHITE);
    put_number(40, HUD_ROW, lambs, 3, C_YELLOW);
    put_text(45, HUD_ROW, s"wool", C_WHITE);
    put_number(50, HUD_ROW, wool_bags, 3, C_YELLOW);
    put_text(55, HUD_ROW, s"trend:", C_WHITE);
    put_text(62, HUD_ROW, trend_name[trend], C_PINK);
}

/* a fenced, empty box: columns x0-x1, rows y0-y1 */
static void fence_box(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1) {
    unsigned char y;
    for (y = y0; y <= y1; y++) {
        unsigned char edge = (y == y0 || y == y1);
        clear_span(x0, y, (unsigned char)(x1 - x0 + 1));
        put(x0, y, edge ? G_POST : G_FENCE_V, C_BROWN);
        put(x1, y, edge ? G_POST : G_FENCE_V, C_BROWN);
        if (edge) {
            unsigned char x;
            for (x = x0 + 1; x < x1; x++) put(x, y, G_FENCE_H, C_BROWN);
        }
    }
}

static void panel_clear(void) {
    fence_box(PANEL_X, FENCE_T, COLS - 1, FENCE_B);
}

/* the big sheep: 6 x 3 cells under a row for the bow */
static const unsigned char big_cell[18] = {
    G_BIG0, G_BIG1, G_BIG2, G_BIG3, G_BEAR_A, G_BEAR_B,
    G_BIG4, G_BIG5, G_BIG6, G_BIG7, G_BFACE_A, G_BFACE_B,
    G_BIG8, G_BIG9, G_BIG10, G_BIG11, G_BMUZ_A, G_BMUZ_B};

static void draw_big(unsigned char look, unsigned char ram) {
    unsigned char wc = MCOL + wool_color[look & L_WOOL], lc = MCOL + lips_color[(look & L_LIPS) >> 2];
    unsigned char r, c, k = 0;
    clear_span(BIG_X, BIG_Y, 6);
    if (look & L_BOW) {
        put(BIG_X + 4, BIG_Y, G_BBOW_A, MCOL + C_RED);
        put(BIG_X + 5, BIG_Y, G_BBOW_B, MCOL + C_RED);
    }
    for (r = 0; r < 3; r++)
        for (c = 0; c < 6; c++, k++) {
            unsigned char ch = big_cell[k], col = wc;
            if (c >= 4) col = r == 2 ? lc : MCOL + C_BLACK;
            if (r == 0 && c >= 4) {
                if (ram) ch = (unsigned char)(G_BHORN_A + c - 4);
                else if (look & L_LASH) ch = (unsigned char)(G_BLASH_A + c - 4);
            }
            put((unsigned char)(BIG_X + c), (unsigned char)(BIG_Y + 1 + r), ch, col);
        }
    /* a ram's lashes: on the face below the horns */
    if (ram && (look & L_LASH)) put(BIG_X + 5, BIG_Y + 1, G_BLASH_B, MCOL + C_BLACK);
}

static const char *const salon_rows[5] = {s"wool", s"lips", s"lashes", s"bow", s"done"};

static void draw_panel(void) {
    unsigned char y = 9;
    panel_clear();
    if (mode == M_SALON) {
        unsigned char look = s_look[carry], r;
        put_text(PANEL_IN + 1, 2, s"* glam salon *", C_YELLOW);
        draw_big(look, s_flags[carry] & F_RAM);
        put_text(PANEL_IN + 5, 8, (s_flags[carry] & F_RAM) ? s"a ram" : s"a ewe", C_LBLUE);
        for (r = 0; r < 5; r++) {
            unsigned char c = (r == sel) ? C_YELLOW : C_WHITE;
            const char *v = 0;
            if (r == sel) put_text(PANEL_IN, (unsigned char)(10 + r), s">", C_YELLOW);
            put_text(PANEL_IN + 1, (unsigned char)(10 + r), salon_rows[r], c);
            if (r == 0) v = wool_name[look & L_WOOL];
            else if (r == 1) v = lips_name[(look & L_LIPS) >> 2];
            else if (r == 2) v = yesno_name[(look & L_LASH) ? 1 : 0];
            else if (r == 3) v = yesno_name[(look & L_BOW) ? 1 : 0];
            if (v) put_text(PANEL_IN + 8, (unsigned char)(10 + r), v, c);
        }
        put_text(PANEL_IN, 17, s"up/down: choose", C_LBLUE);
        put_text(PANEL_IN, 18, s"left/right:", C_LBLUE);
        put_text(PANEL_IN + 2, 19, s"change", C_LBLUE);
        put_text(PANEL_IN, 20, s"fire: let it go", C_LBLUE);
        put_text(PANEL_IN, 21, s"trend:", C_LBLUE);
        put_text(PANEL_IN + 2, 22, trend_name[trend], C_PINK);
        return;
    }
    /* playing */
    put_text(PANEL_IN + 1, 2, s"* glam salon *", C_YELLOW);
    if (carry != 0xFF) draw_big(s_look[carry], s_flags[carry] & F_RAM);
    else {
        put_text(PANEL_IN + 1, 5, s"bring me a", C_LBLUE);
        put_text(PANEL_IN + 1, 6, s"sheep, darling", C_LBLUE);
    }
    put_text(PANEL_IN, y, s"fire beside a", C_WHITE);
    put_text(PANEL_IN, y + 1, s"sheep: grab it", C_WHITE);
    put_text(PANEL_IN, y + 3, s"carry it to the", C_WHITE);
    put_text(PANEL_IN, y + 4, s"door  < here", C_WHITE);
    put(PANEL_IN + 5, y + 4, G_GATE, C_YELLOW);
    put_text(PANEL_IN, y + 6, s"fire: put down", C_WHITE);
    put_text(PANEL_IN, y + 8, s"rams", C_LBLUE);
    put(PANEL_IN + 5, y + 8, G_RAM_R, MCOL + C_BLACK);
    put_text(PANEL_IN + 7, y + 8, s"ewes", C_LBLUE);
    put(PANEL_IN + 12, y + 8, G_EWE_R, MCOL + C_BLACK);
    put_text(PANEL_IN, y + 10, s"trend:", C_LBLUE);
    put_text(PANEL_IN + 2, y + 11, trend_name[trend], C_PINK);
    if (sh_on) put_text(PANEL_IN, y + 13, s"fire by fergus!", C_YELLOW);
    else put_text(PANEL_IN, y + 13, s"m: music on/off", C_LBLUE);
}

/* the season-over box, in the middle of the screen over the (frozen) field */
#define BOX_X 21
#define BOX_Y 3
#define BOX_W 38
#define BOX_H 19

static void box_line(unsigned char y, const char *s, unsigned char c) {
    put_text((unsigned char)(BOX_X + (BOX_W - strlen(s)) / 2), y, s, c);
}

static void box_number(unsigned char y, const char *s, unsigned int v) {
    put_text(BOX_X + 8, y, s, C_WHITE);
    put_number(BOX_X + BOX_W - 13, y, v, 5, C_YELLOW);
}

static void draw_over_box(void) {
    fence_box(BOX_X, BOX_Y, BOX_X + BOX_W - 1, BOX_Y + BOX_H - 1);
    put_text(BOX_X + 10, BOX_Y + 2, s"season", C_YELLOW);
    put_number(BOX_X + 17, BOX_Y + 2, season, season < 10 ? 1 : 2, C_YELLOW);
    put_text(season < 10 ? BOX_X + 19 : BOX_X + 20, BOX_Y + 2, s"is over!", C_YELLOW);
    box_number(BOX_Y + 4, s"lambs born", lambs);
    box_number(BOX_Y + 5, s"glam lambs", glam_lambs);
    box_number(BOX_Y + 6, s"lamb points", lamb_pts);
    box_number(BOX_Y + 7, s"wool bags", wool_bags);
    box_number(BOX_Y + 8, s"wool income", income);
    box_number(BOX_Y + 9, s"score", score);
    box_number(BOX_Y + 10, s"high score", hiscore);
    if (score > hiscore_was) box_line(BOX_Y + 11, s"a new high score!", C_PINK);
    box_line(BOX_Y + 13, s"your flock goes to market.", C_LBLUE);
    box_line(BOX_Y + 14, s"next season starts afresh", C_LBLUE);
    box_line(BOX_Y + 15, s"with a plain, unstyled flock.", C_LBLUE);
    if (!over_t) box_line(BOX_Y + 17, s"return or fire to go on", C_WHITE);
}

/* ------------------------------------------------------------------------ */
/* Input                                                                    */

#define K_UP    0x01
#define K_DOWN  0x02
#define K_LEFT  0x04
#define K_RIGHT 0x08
#define K_FIRE  0x10

static unsigned char held, prev_held;

/* the MEGA65's cursor left and up keys show in the matrix as right SHIFT
 * + cursor right / down; $D60F tells them apart */
static unsigned char controls(void) {
    unsigned char c = 0, joy = input_joy(1) | input_joy(2), lu = CRSR_LEFTUP & 3;
    if (input_pressed(KEY_W) || (lu & 2) || (joy & JOY_UP)) c |= K_UP;
    if (input_pressed(KEY_S) || (input_pressed(KEY_DOWN) && !(lu & 2)) || (joy & JOY_DOWN)) c |= K_DOWN;
    if (input_pressed(KEY_A) || (lu & 1) || (joy & JOY_LEFT)) c |= K_LEFT;
    if (input_pressed(KEY_D) || (input_pressed(KEY_RIGHT) && !(lu & 1)) || (joy & JOY_RIGHT)) c |= K_RIGHT;
    if (input_pressed(KEY_SPACE) || input_pressed(KEY_RETURN) || (joy & JOY_FIRE)) c |= K_FIRE;
    return c;
}

static unsigned char next_to_sheep(void) {
    return sheep_in((unsigned char)(p_x - 1), (unsigned char)(p_y - 1), PW + 2, PH + 2, 0xFF);
}

/* the computer player (the attract mode, -DDEMO): walk to the nearest adult
 * sheep, grab it, carry it to the door, dress it up for the trend and let it
 * go. It plays at a player's pace: directions are held (so the stylist
 * walks at the usual speed), a button is pressed for a frame and then it
 * waits DEMO_PAUSE frames, as if thinking */
#define DEMO_PAUSE 12
static unsigned char demo_t, demo_wait;

static unsigned char demo_press(unsigned char k) {
    demo_wait = DEMO_PAUSE;
    return k;
}

static unsigned char demo_controls(void) {
    unsigned char c = 0, i, best = 0xFF, tx, ty;
    unsigned int bd = 0xFFFF;
    demo_t++;
    if (demo_wait) { demo_wait--; return 0; }
    if (mode == M_TITLE || mode == M_OVER) return (demo_t & 63) == 0 ? demo_press(K_FIRE) : 0;
    if (mode == M_SALON) {
        if (sel < 4) {
            unsigned char look = s_look[carry];
            if (sel == 0 && (look & L_WOOL) != 1 + (demo_t >> 7 & 1)) return demo_press(K_RIGHT);
            if (sel == 1 && (look & L_LIPS) != 0x04) return demo_press(K_RIGHT);
            if (sel == 2 && !(look & L_LASH)) return demo_press(K_RIGHT);
            if (sel == 3 && !(look & L_BOW) && (demo_t & 2)) return demo_press(K_RIGHT);
            return demo_press(K_DOWN);
        }
        return demo_press(K_FIRE);
    }
    if (carry != 0xFF) { tx = FX1; ty = GATE_Y0; }
    else {
        i = next_to_sheep();
        if (i != 0xFF && adult(i) && s_wool[i] >= WOOL_FULL && glam(s_look[i]) < 3) return demo_press(K_FIRE);
        for (i = 0; i < MAXS; i++) {
            unsigned int d;
            if ((s_flags[i] & (F_ALIVE | F_CARRIED)) != F_ALIVE || !adult(i) || s_wool[i] < WOOL_FULL ||
                glam(s_look[i]) >= 3) continue;
            d = (unsigned int)(s_x[i] + 1 > p_x ? s_x[i] + 1 - p_x : p_x - s_x[i] - 1) +
                (unsigned int)(s_y[i] > p_y ? s_y[i] - p_y : p_y - s_y[i]);
            if (d < bd) { bd = d; best = i; }
        }
        if (best == 0xFF) return 0;
        tx = (unsigned char)(s_x[best] + 1); ty = s_y[best];
    }
    if (tx > p_x) c |= K_RIGHT;
    if (tx < p_x) c |= K_LEFT;
    if (ty > p_y) c |= K_DOWN;
    if (ty < p_y) c |= K_UP;
    return c;
}

/* ------------------------------------------------------------------------ */
/* The stylist                                                              */

/* can the stylist stand at x, y? In the field, or in the door */
static unsigned char player_fits(unsigned char x, unsigned char y) {
    if (y < FY0 || y + PH - 1 > FY1 || x < FX0) return 0;
    if (x + PW - 1 > FX1 && (x != FX1 || y < GATE_Y0 || y + PH - 1 > GATE_Y1)) return 0;
    if (shearer_in(x, y, PW, PH)) return 0;
    return sheep_in(x, y, PW, PH, 0xFF) == 0xFF;
}

/* ------------------------------------------------------------------------ */
/* The shearer                                                              */

static unsigned char shearer_fits(unsigned char x, unsigned char y) {
    if (x < FX0 || x + PW - 1 > FX1 || y < FY0 || y + PH - 1 > FY1) return 0;
    if (p_x < x + PW && x < p_x + PW && p_y < y + PH && y < p_y + PH) return 0;
    return sheep_in(x, y, PW, PH, 0xFF) == 0xFF;
}

static void shearer_arrives(void) {
    unsigned char y;
    for (y = FY0 + 8; y < FY1; y++)
        if (shearer_fits(FX0, y)) break;
    if (y >= FY1) return;              /* no room yet: next second */
    sh_on = 1;
    sh_x = FX0; sh_y = y;
    sh_snip = sh_rest = 0;
    sh_target = 0xFF;
    panel_dirty = 1;
    message(s"uh oh! farmer fergus is out to shear full fleeces", C_YELLOW);
}

static void shoo(void) {
    sh_snip = 0;
    sh_target = 0xFF;
    sh_rest = SHOO_TIME;
    snd_click(0x0800);
    message(s"shoo! farmer fergus stomps off", C_WHITE);
}

/* the nearest sheep in full fleece, or 0xFF */
static unsigned char nearest_glam(void) {
    unsigned char i, best = 0xFF;
    unsigned int bd = 0xFFFF;
    for (i = 0; i < MAXS; i++) {
        unsigned int d;
        if ((s_flags[i] & (F_ALIVE | F_CARRIED)) != F_ALIVE || !adult(i) || s_wool[i] < WOOL_FULL) continue;
        d = (unsigned int)(s_x[i] + 1 > sh_x ? s_x[i] + 1 - sh_x : sh_x - s_x[i] - 1) +
            (unsigned int)(s_y[i] > sh_y ? s_y[i] - sh_y : sh_y - s_y[i]);
        if (!s_look[i]) d += 16;          /* he likes the made-up ones best */
        if (d < bd) { bd = d; best = i; }
    }
    return best;
}

/* the fleece comes off (and the makeup with it) and is sold; a sheep shorn
 * once too often passes on */
static void shear(unsigned char t) {
    unsigned char bags = s_wool[t];
    unsigned int pay = (unsigned int)bags * ((s_look[t] & L_WOOL) ? DYED_PRICE : BAG_PRICE);
    s_look[t] = 0;
    s_wool[t] = 0;
    s_woolt[t] = 0;
    sh_target = 0xFF;
    sh_tmr = 50;
    wool_bags += bags;
    income += pay;
    score += pay;
    if (!attract && score > hiscore) hiscore = score;
    hud_dirty = 1;
    snd_baa();
    if (++s_shear[t] >= MAX_SHEARS) {
        s_flags[t] = 0;
        flock--;
        add_fx((unsigned char)(s_x[t] + 1), s_y[t], G_HEART, C_WHITE);
        message_n(s"shorn once too often... rest in fleece. wool sold: +", pay, s"", C_LBLUE);
        return;
    }
    message_n(s"snip snip! makeup gone, wool sold: +", pay, s"", C_PINK);
}

static unsigned char shearer_step(signed char dx, signed char dy) {
    unsigned char nx = (unsigned char)(sh_x + dx), ny = (unsigned char)(sh_y + dy);
    if (!shearer_fits(nx, ny)) return 0;
    sh_x = nx; sh_y = ny;
    sh_step ^= 1;
    return 1;
}

/* once a frame */
static void move_shearer(void) {
    signed char dx, dy;
    unsigned char t;
    if (!sh_on) return;
    if (sh_snip) {
        t = sh_target;
        if ((s_flags[t] & (F_ALIVE | F_CARRIED)) != F_ALIVE) { sh_snip = 0; sh_target = 0xFF; return; }
        if ((sh_snip & 15) == 0) {
            add_fx((unsigned char)(s_x[t] + rnd_n(3)), s_y[t] > FY0 ? s_y[t] - 1 : s_y[t], G_SPARKLE, C_WHITE);
            snd_click(0x4000);
        }
        if (--sh_snip == 0) shear(t);
        return;
    }
    if (sh_tmr) { sh_tmr--; return; }
    sh_tmr = 8;
    t = sh_rest ? 0xFF : nearest_glam();
    if (t != 0xFF && sheep_in((unsigned char)(sh_x - 1), (unsigned char)(sh_y - 1), PW + 2, PH + 2, 0xFF) == t) {
        sh_target = t;
        sh_snip = SNIP_FRAMES;
        message(s_look[t] ? s"fergus is shearing a glam sheep! fire by him: shoo!" : s"fergus is shearing a sheep: wool money!", C_YELLOW);
        return;
    }
    if (t != 0xFF) {
        signed int ex = (signed int)s_x[t] + 1 - sh_x, ey = (signed int)s_y[t] - sh_y;
        dx = sgn(ex); dy = sgn(ey);
    } else {                          /* nothing to shear (or shooed): amble about */
        if (rnd_n(3)) return;
        dx = (signed char)(rnd_n(3) - 1);
        dy = (signed char)(rnd_n(3) - 1);
        if (sh_rest && p_x > sh_x) dx = -1;
    }
    if (!dx && !dy) return;
    if (!shearer_step(dx, dy) && !(dx && shearer_step(dx, 0)) && !(dy && shearer_step(0, dy)))
        shearer_step((signed char)(rnd_n(3) - 1), (signed char)(rnd_n(3) - 1));
}

/* ------------------------------------------------------------------------ */
/* The salon's room: GLAMSALON (characters) into bank 4, GLAMSALON2 (more,  */
/* its screen, the looks, the palette) into bank 5; without them, the panel */

#define SAL_SCREEN_LIN 0x51800UL      /* its screen: two bytes a cell (80 x 25: to $5279F) */
#define SAL_COLOUR 0x1000             /* its colour RAM (an offset) */
#define PALSAVE_LIN 0x5D000UL         /* palette entries 16-255 as they were */
#define SCRATCH (scr + COLS)          /* (rows 1-23 of the buffers: the room has its own screen) */

static unsigned char salon_ok;        /* the files loaded */
static seam_screen_t sal;
static unsigned char sal_look;        /* the look the palette shows (+ 1; 0: none yet) */
static unsigned int sal_key;          /* what the text shows (+ 1) */
static unsigned char blink_t, bulb_t, bulb_on;
#define NSPARK 4
static unsigned char spark_t[NSPARK], spark_kind;
static char sal_line[SAL_DOOR_W];

/* sparkles (column, row, the large one's character, the small one's) by
 * the menu's row: wool, lips, lashes, bow */
static const unsigned int spark_tab[4][4 * NSPARK] = {
    {SAL_SPARK_WOOL}, {SAL_SPARK_LIPS}, {SAL_SPARK_EYE}, {SAL_SPARK_BOW}};
static const unsigned char eye_runs[SAL_EYES_N * 4] = {SAL_EYES_RECTS};
static const unsigned char top_runs[SAL_TOP_N * 4] = {SAL_TOP_RECTS};

/* GLAMSALON and GLAMSALON2 from the drive the game was loaded from (before
 * our screen: the KERNAL's disk code) */
static void salon_load(void) {
    static char n1[] = p"glamsalon", n2[] = p"glamsalon2";
    unsigned char dev = *(volatile unsigned char *)0x00BA;
    if (dev < 8) dev = 8;
    if (cbm_load_far(n1, sizeof n1 - 1, dev, SAL_LIN) || cbm_load_far(n2, sizeof n2 - 1, dev, SAL2_LIN)) return;
    dma_copy_far((unsigned long)(unsigned int)scr, SAL2_LIN + SAL2_SIZE - 4, 4);
    salon_ok = scr[0] == 0x47 && scr[1] == 0x4C && scr[2] == 0x41 && scr[3] == 0x4D;   /* "GLAM" */
}

/* palette entries from the file (as palette_restore takes them) */
static void sal_palette(unsigned char first, unsigned char n, unsigned int at) {
    dma_copy_far((unsigned long)(unsigned int)SCRATCH, SAL2_LIN + at, 3 * n);
    palette_restore(first, n, SCRATCH);
}

/* once (after setup): its screen, the picture's palette (entries 16-255 as
 * they were kept in bank 5); and back at the end */
static void salon_setup(void) {
    seam_init(&sal, SAL_SCREEN_LIN, SAL_COLOUR, COLS, ROWS);
    palette_save(16, 240, SCRATCH);
    dma_copy_far(PALSAVE_LIN, (unsigned long)(unsigned int)SCRATCH, 720);
    sal_palette(SAL_PAL_FIRST, SAL_PAL_COUNT, SAL_PALETTE);
}

static void salon_unsetup(void) {
    dma_copy_far((unsigned long)(unsigned int)SCRATCH, PALSAVE_LIN, 720);
    palette_restore(16, 240, SCRATCH);
}

/* a sparkle's cell: the sparkle, or the room's again */
static void spark_cell(unsigned char i, unsigned char on) {
    const unsigned int *sp = spark_tab[spark_kind] + 4 * i;
    unsigned char x = (unsigned char)sp[0], y = (unsigned char)sp[1];
    if (on) seam_put(&sal, x, y, (spark_t[i] & 4) ? sp[2] : sp[3], 0);
    else seam_cells(&sal, x, y, SAL2_LIN + SAL_SCREEN + (((unsigned int)(y - 1) * COLS + x) << 1), 1);
}

/* sparkles round what the menu's row k changed, one after another */
static void sparkle(unsigned char k) {
    unsigned char i;
    if (sal.shown)
        for (i = 0; i < NSPARK; i++) spark_cell(i, 0);
    spark_kind = k;
    for (i = 0; i < NSPARK; i++) spark_t[i] = (unsigned char)(20 + 8 * i);
}

/* a line of text (screen codes) on the room: s from off */
static void sal_fill(unsigned char off, const char *s) {
    while (*s && off < SAL_DOOR_W) sal_line[off++] = *s++;
}

static void sal_text(unsigned char x, unsigned char y, unsigned char w, unsigned char off, const char *s,
                     unsigned char c) {
    memset(sal_line, CH_SPACE, w);
    sal_fill(off, s);
    seam_text(&sal, x, y, sal_line, w, c);
}

/* one look's cells over the room: runs (column, row, width) of numbers */
static void sal_runs(const unsigned char *run, unsigned char n, unsigned long src) {
    while (n--) {
        seam_cells(&sal, run[0], run[1], src, run[2]);
        src += (unsigned int)run[2] * 2;
        run += 4;
    }
}

/* the sign (the trend) and the doors (the menu, the help) */
static void sal_words(unsigned char look) {
    unsigned char i;
    sal_text(SAL_SIGN_X, SAL_SIGN_Y, SAL_SIGN_W, 2, s"* glam salon *", C_YELLOW);
    sal_text(SAL_SIGN_X, SAL_SIGN_Y + 1, SAL_SIGN_W, 0, s"trend: ", C_PINK);
    sal_fill(7, trend_name[trend]);
    seam_text(&sal, SAL_SIGN_X, SAL_SIGN_Y + 1, sal_line, SAL_SIGN_W, C_PINK);
    sal_text(SAL_DOOR_X, SAL_DOOR_Y, SAL_DOOR_W, 2,
             (s_flags[carry] & F_RAM) ? s"a ram in the chair!" : s"a ewe in the chair!", C_LBLUE);
    for (i = 0; i < 5; i++) {
        const char *v = s"";
        if (i == 0) v = wool_name[look & L_WOOL];
        else if (i == 1) v = lips_name[(look & L_LIPS) >> 2];
        else if (i == 2) v = yesno_name[(look & L_LASH) ? 1 : 0];
        else if (i == 3) v = yesno_name[(look & L_BOW) ? 1 : 0];
        sal_text(SAL_DOOR_X, (unsigned char)(SAL_DOOR_Y + 1 + i), SAL_DOOR_W, 13, v, i == sel ? C_YELLOW : C_WHITE);
        sal_fill(4, salon_rows[i]);
        if (i == sel) sal_line[2] = '>';
        seam_text(&sal, SAL_DOOR_X, (unsigned char)(SAL_DOOR_Y + 1 + i), sal_line, SAL_DOOR_W,
                  i == sel ? C_YELLOW : C_WHITE);
    }
    sal_text(SAL_DOOR_X, SAL_DOOR_Y + 6, SAL_DOOR_W, 1, s"up/down choose, l/r change", C_LBLUE);
    sal_text(SAL_DOOR_X, SAL_DOOR_Y + 7, SAL_DOOR_W, 1, s"fire: let it go, darling", C_LBLUE);
}

static void draw_salon(void) {
    unsigned char look = s_look[carry], i, k;
    unsigned int key = (unsigned int)(look | (unsigned int)sel << 6 | (unsigned int)trend << 9) + 1;
    if (sal_look != look + 1) {          /* the dye's and the lipstick's shades */
        sal_palette(SAL_WOOL, 16, SAL_DYES + 48 * (look & L_WOOL));
        sal_palette(SAL_LIPS, 8, SAL_LIPSTICKS + 24 * ((look & L_LIPS) >> 2));
        sal_look = (unsigned char)(look + 1);
    }
    if (++bulb_t >= 20) {                /* the bulbs take turns */
        bulb_t = 0;
        bulb_on ^= 1;
        sal_palette(SAL_BULBA, 8, SAL_BULBS + 24 * bulb_on);
    }
    if (blink_t) blink_t--;
    else blink_t = (unsigned char)(120 + rnd_n(120));
    /* the eyes' look and the head top's */
    k = (unsigned char)(((look & L_LASH) ? 2 : 0) | (blink_t < 6 ? 1 : 0));
    sal_runs(eye_runs, SAL_EYES_N, SAL2_LIN + SAL_EYES + (unsigned int)k * SAL_EYES_LOOK);
    k = (unsigned char)(((s_flags[carry] & F_RAM) ? 2 : 0) | ((look & L_BOW) ? 1 : 0));
    sal_runs(top_runs, SAL_TOP_N, SAL2_LIN + SAL_TOP + (unsigned int)k * SAL_TOP_LOOK);
    for (i = 0; i < NSPARK; i++) {
        if (spark_t[i]) spark_t[i]--;
        spark_cell(i, spark_t[i] && spark_t[i] <= 20);
    }
    if (key != sal_key) { sal_words(look); sal_key = key; }
    /* the score line and the messages */
    seam_row(&sal, 0, HUD_ROW, COLS, scr, clr);
    seam_row(&sal, 0, MSG_ROW, COLS, scr + MSG_ROW * COLS, clr + MSG_ROW * COLS);
}

/* into the room (drawn first) and out of it */
static void salon_on(void) {
    dma_copy_far(seam_cell(&sal, 0, 1), SAL2_LIN + SAL_SCREEN, COLS * (ROWS - 2) * 2);
    dma_fill_far(seam_colour(&sal, 0, 1), COLS * (ROWS - 2) * 2, 0);
    sal_key = 0;
    blink_t = 100;
    sparkle(0);
    draw_salon();
    seam_show(&sal);
    REG(0xD021) = SAL_GLASS;
}

static void salon_off(void) {
    if (!sal.shown) return;
    seam_hide(&sal);
    REG(0xD021) = C_MEADOW;
}

static void enter_salon(void) {
    mode = M_SALON;
    music_for(1);
    sel = 0;
    panel_dirty = 1;
    snd_click(0x2000);
    message(s"welcome to the salon! make it fabulous", C_YELLOW);
    if (salon_ok) salon_on();
}

static void leave_salon(void) {
    unsigned char i = carry;
    salon_off();
    p_x = FX1 - 1;                     /* back inside the gate */
    s_flags[i] &= (unsigned char)~F_CARRIED;
    if (find_spot(FX1 - SW - 2, p_y)) {
        s_x[i] = fx_; s_y[i] = fy_;
    }
    s_flags[i] |= F_LEFT;
    s_tmr[i] = 30;
    add_fx(s_x[i], s_y[i] > FY0 ? s_y[i] - 1 : s_y[i], G_SPARKLE, C_YELLOW);
    add_fx((unsigned char)(s_x[i] + 3), s_y[i] > FY0 ? s_y[i] - 1 : s_y[i], G_SPARKLE, C_WHITE);
    carry = 0xFF;
    mode = M_PLAY;
    music_for(0);
    panel_dirty = 1;
    snd_baa();
    message(glam(s_look[i]) ? s"gorgeous! off you go" : s"au naturel? if you say so", C_WHITE);
}

static void play_input(unsigned char pressed) {
    signed char dx = 0, dy = 0;
    if (pressed & K_FIRE) {
        if (carry == 0xFF && shearer_in((unsigned char)(p_x - 1), (unsigned char)(p_y - 1), PW + 2, PH + 2)) {
            shoo();
            return;
        }
        if (carry == 0xFF) {
            unsigned char i = next_to_sheep();
            if (i == 0xFF) return;
            if (!adult(i)) { message(s"lambs are too young for makeup!", C_LBLUE); return; }
            if (s_wool[i] < WOOL_FULL) { message(s"not enough wool to style yet: let it grow back!", C_LBLUE); return; }
            carry = i;
            s_flags[i] |= F_CARRIED;
            snd_baa();
            panel_dirty = 1;
            message((s_flags[i] & F_RAM) ? s"got a ram! to the salon" : s"got a ewe! to the salon", C_WHITE);
        } else if (find_spot(p_x > SW ? p_x - SW : FX0, p_y)) {
            unsigned char i = carry;
            s_x[i] = fx_; s_y[i] = fy_;
            s_flags[i] &= (unsigned char)~F_CARRIED;
            s_tmr[i] = 20;
            carry = 0xFF;
            panel_dirty = 1;
            snd_baa();
        }
        return;
    }
    if (p_tmr) p_tmr--;
    if (!(held & (K_UP | K_DOWN | K_LEFT | K_RIGHT))) { p_tmr = 0; return; }
    if (p_tmr && !(pressed & (K_UP | K_DOWN | K_LEFT | K_RIGHT))) return;
    if (held & K_UP) dy = -1;
    if (held & K_DOWN) dy = 1;
    if (held & K_LEFT) dx = -1;
    if (held & K_RIGHT) dx = 1;
    p_tmr = carry != 0xFF ? 6 : 4;
    {
        unsigned char nx = (unsigned char)(p_x + dx), ny = (unsigned char)(p_y + dy);
        if (!player_fits(nx, ny)) {
            /* diagonal: slide along whatever is in the way */
            if (dx && dy && player_fits(p_x, ny)) nx = p_x;
            else if (dx && dy && player_fits(nx, p_y)) ny = p_y;
            else return;
        }
        p_x = nx;
        p_y = ny;
        p_step ^= 1;
    }
    if (p_x == FX1) {                  /* in the door */
        if (carry != 0xFF) enter_salon();
        else {
            message(s"the salon wants a sheep, darling", C_LBLUE);
            p_x = FX1 - 1;
        }
    }
}

static void salon_input(unsigned char pressed) {
    unsigned char *look = &s_look[carry];
    if (pressed & K_UP) { sel = sel ? sel - 1 : 4; snd_click(0x1800); panel_dirty = 1; }
    if (pressed & K_DOWN) { sel = sel < 4 ? sel + 1 : 0; snd_click(0x1800); panel_dirty = 1; }
    if (pressed & (K_LEFT | K_RIGHT)) {
        unsigned char step = (pressed & K_RIGHT) ? 1 : 3;
        if (sel == 0) *look = (unsigned char)((*look & ~L_WOOL) | ((*look + step) & L_WOOL));
        else if (sel == 1) *look = (unsigned char)((*look & ~L_LIPS) | ((*look + (step << 2)) & L_LIPS));
        else if (sel == 2) *look ^= L_LASH;
        else if (sel == 3) *look ^= L_BOW;
        if (sel < 4) { snd_click(0x3000); panel_dirty = 1; sparkle(sel); }
    }
    if (pressed & K_FIRE) leave_salon();
}

/* ------------------------------------------------------------------------ */

static void clear_flock(void) {
    unsigned char i;
    for (i = 0; i < MAXS; i++) s_flags[i] = 0;
    for (i = 0; i < NFX; i++) fx_t[i] = 0;
    flock = 0;
}

/* a new flock: natural sheep, half of them rams, of mixed ages */
static void new_flock(void) {
    unsigned char n;
    clear_flock();
    for (n = 0; n < 12; n++) {
        if (!find_spot((unsigned char)(FX0 + 2 + rnd_n(FX1 - FX0 - 6)), (unsigned char)(FY0 + rnd_n(FY1 - FY0))))
            continue;
        new_sheep(fx_, fy_, 0, n & 1, (unsigned char)(ADULT_AGE + rnd_n(30)), 0);
    }
}

static void start_season(void) {
    mode = M_PLAY;
    score = 0; lambs = 0; glam_lambs = 0;
    lamb_pts = wool_bags = income = 0;
    hiscore_was = hiscore;
    memset(scr, CH_SPACE, COLS * ROWS);   /* off with the title page */
    set_charset(CHARSET_LIN);
    time_left = SEASON;
    if (season < 99) season++;
    {   /* Fergus: sooner each season */
        unsigned int at = (unsigned int)(season - 1) * SHEARER_SOONER;
        at = at + SHEARER_SOONEST < SHEARER_FIRST ? SHEARER_FIRST - at : SHEARER_SOONEST;
        shearer_at = SEASON - at;
    }
    trend = rnd_n(NTRENDS);
    trend_t = TREND_T;
    carry = 0xFF;
    p_x = FX1 - 6; p_y = GATE_Y0;
    sh_on = 0;
    new_flock();
    music_restart();
    music_for(0);
    hud_dirty = panel_dirty = 1;
    message_n(s"season ", season, s" begins! catch a sheep", C_YELLOW);
}

/* ------------------------------------------------------------------------ */
/* The title page: a block-letter logo, how to play, a parade of sheep      */

/* letters 5 high, 3 or 5 wide; a pixel is two cells (a solid block) */
static const char logo_text[] = "glam lambs";
static const char logo_letters[] = "glambs";
static const unsigned char logo_w[6] = {3, 3, 3, 5, 3, 3};
static const unsigned char logo_font[6][5] = {
    {7, 4, 5, 5, 7},                  /* g */
    {4, 4, 4, 4, 7},                  /* l */
    {2, 5, 7, 5, 5},                  /* a */
    {17, 27, 21, 17, 17},             /* m */
    {6, 5, 6, 5, 6},                  /* b */
    {7, 4, 7, 1, 7}};                 /* s */
static const unsigned char logo_color[4] = {C_PINK, C_YELLOW, C_WHITE, C_LBLUE};

#define LOGO_Y 1
#define LOGO_SPACE 4                  /* cells for a space */
#define PARADE_Y 21
#define NPARADE 4

/* the title's text: two columns of headings (a leading '#') and bullet
 * points (a leading '*'); plain ASCII, mixed case */
static const char *const title_left[] = {
    "#How to play",
    "",
    "*Catch a sheep: walk up, press fire",
    "*Carry it to the salon door",
    " in the right-hand fence",
    "*Dye the wool, add lipstick,",
    " lashes and a bow",
    "*Let it go: its lambs inherit",
    " the look",
    "*Farmer Fergus shears full fleeces",
    " (the makeup goes too).",
    " Fire beside him: shoo!"};
static const char *const title_right[] = {
    "#Scoring",
    "",
    "*Glam lamb: 5 a trait x generation",
    "*On the trend: double points",
    "*All four traits: +50",
    "*Wool sold: 10 a bag, dyed 25",
    "*Wool regrows; shorn 6 times,",
    " a sheep passes on",
    "",
    "#Controls",
    "*Joystick, cursor keys or WASD",
    "*Fire, Space or Return"};

static void title_line(unsigned char x, unsigned char y, const char *s) {
    if (*s == '#') put_ascii(x, y, s + 1, C_YELLOW);
    else if (*s == '*') {
        put(x, y, G_BULLET, C_PINK);
        put_ascii((unsigned char)(x + 2), y, s + 1, C_WHITE);
    } else if (*s) put_ascii((unsigned char)(x + 2), y, s + 1, C_WHITE);
}

static unsigned char logo_index(char ch) {
    unsigned char f = 0;
    while (logo_letters[f] != ch) f++;
    return f;
}

static void draw_logo(void) {
    unsigned char k, r, b, w = 0, x;
    for (k = 0; logo_text[k]; k++)    /* the width, to centre it */
        w += logo_text[k] == ' ' ? LOGO_SPACE : logo_w[logo_index(logo_text[k])] * 2 + 1;
    x = (unsigned char)((COLS - w + 1) / 2);
    for (k = 0; logo_text[k]; k++) {
        unsigned char f, lw, c = logo_color[k & 3];
        if (logo_text[k] == ' ') { x += LOGO_SPACE; continue; }
        f = logo_index(logo_text[k]);
        lw = logo_w[f];
        for (r = 0; r < 5; r++)
            for (b = 0; b < lw; b++)
                if (logo_font[f][r] & (1 << (lw - 1 - b))) {
                    put((unsigned char)(x + b * 2), (unsigned char)(LOGO_Y + r), G_BLOCK, c);
                    put((unsigned char)(x + b * 2 + 1), (unsigned char)(LOGO_Y + r), G_BLOCK, c);
                }
        x += lw * 2 + 1;
    }
}

static void draw_title_page(void) {
    unsigned char i;
    memset(scr, CH_SPACE, COLS * ROWS);
    memset(clr, C_WHITE, sizeof clr);
    draw_logo();
    put_ascii(22, 7, "A sheep makeover game for the MEGA65", C_LBLUE);
    for (i = 0; i < 12; i++) {
        title_line(3, (unsigned char)(9 + i), title_left[i]);
        title_line(43, (unsigned char)(9 + i), title_right[i]);
    }
    for (i = 0; i < COLS; i++) {
        put(i, 8, G_FENCE_H, C_BROWN);
        put(i, PARADE_Y + 2, G_FENCE_H, C_BROWN);
    }
}

/* once a frame: the parade walks right, the prompt blinks */
static unsigned char parade_t;
static void title_frame(void) {
    unsigned char i;
    clear_span(0, PARADE_Y, COLS);
    clear_span(0, PARADE_Y + 1, COLS);
    if (++parade_t >= 10) {
        parade_t = 0;
        for (i = 0; i < NPARADE; i++) {
            s_x[i] = s_x[i] >= COLS - SW ? 0 : s_x[i] + 1;
            s_flags[i] ^= F_STEP;
        }
    }
    for (i = 0; i < NPARADE; i++) draw_sheep_at(s_x[i], PARADE_Y, i);
    clear_span(0, MSG_ROW, COLS);
    if (FRAMECOUNT & 0x20)
        put_ascii(15, MSG_ROW, "Fire: start   -   M: music on/off   -   Run/Stop: quit", C_YELLOW);
}

static void title(void) {
    static const unsigned char parade_look[NPARADE] = {
        0x01 | L_BOW | L_LASH, 0x02 | 0x04, 0x03 | 0x08 | L_LASH, 0x01 | 0x0C | L_BOW};
    unsigned char i;
    salon_off();
    mode = M_TITLE;
    attract = 0;
    season = 0;
    idle_t = 0;
    carry = 0xFF;
    p_x = 0; p_y = 0;
    sh_on = 0;
    trend = rnd_n(NTRENDS);
    music_restart();
    music_for(0);
    clear_flock();
    set_charset(CHARSET2_LIN);
    for (i = 0; i < NPARADE; i++) {   /* the parade: sheep slots, never in the field */
        s_flags[i] = (unsigned char)(F_ALIVE | F_CARRIED | ((i & 1) ? F_RAM : 0));
        s_x[i] = (unsigned char)(i * 20);
        s_look[i] = parade_look[i];
        s_age[i] = ADULT_AGE;
        s_wool[i] = WOOL_FULL;
    }
    draw_title_page();
}

static void end_season(void) {
    unsigned char i;
    salon_off();
    if (carry != 0xFF) {              /* the sheep in hand goes home */
        i = carry;
        s_flags[i] &= (unsigned char)~F_CARRIED;
        if (find_spot(FX1 - SW - 2, p_y)) { s_x[i] = fx_; s_y[i] = fy_; }
        else { s_flags[i] = 0; flock--; }
        carry = 0xFF;
    }
    mode = M_OVER;
    over_t = fps;                     /* a second, so fire held for a grab doesn't skip it */
    sh_on = 0;
    music_for(0);
    hud_dirty = panel_dirty = 1;
    message_n(s"season over! you scored ", score, s"", C_YELLOW);
}

/* once a second */
static void second(void) {
    tick_flock();
    breed();
    hud_dirty = 1;
    if (mode != M_PLAY && mode != M_SALON) return;
    if (sh_rest) sh_rest--;
    if (!sh_on && time_left <= shearer_at) shearer_arrives();
#ifdef DEMO
    REG(0xC0F4) = season;             /* (tests: the shearer's schedule) */
    REG(0xC0F5) = sh_on;
    REG(0xC0F6) = (unsigned char)time_left;
#endif
    if (--trend_t == 0) {
        unsigned char t = trend;
        while (t == trend) t = rnd_n(NTRENDS);
        trend = t;
        trend_t = TREND_T;
        panel_dirty = 1;
        message(s"fashion news: the trend has changed!", C_PINK);
    }
    if (time_left && --time_left == 0) end_season();
}

/* ------------------------------------------------------------------------ */
/* The palette: entries 0-7 (the cells' own colours) and the shared ones,   */
/* as 0-255 intensities (the registers take them nibble-swapped)            */

#define PAL_R(i) REG(0xD100 + (i))
#define PAL_G(i) REG(0xD200 + (i))
#define PAL_B(i) REG(0xD300 + (i))

static const unsigned char pal_idx[12] = {0, 1, 2, 3, 4, 5, 6, 7, C_DGREY, C_MEADOW, C_TUFT, SAL_GLASS};
static const unsigned char pal_rgb[36] = {
    0x00, 0x00, 0x00,    /* black */
    0xFF, 0xFF, 0xFF,    /* white */
    0xE0, 0x20, 0x30,    /* red */
    0x70, 0xA8, 0xFF,    /* light blue */
    0xA8, 0x48, 0xD0,    /* purple */
    0x80, 0x50, 0x20,    /* brown */
    0xFF, 0x88, 0xC8,    /* pink */
    0xF8, 0xD8, 0x30,    /* yellow */
    0x38, 0x38, 0x38,    /* dark grey */
    0x6A, 0x80, 0x58,    /* the meadow: a soft sage green */
    0x80, 0x98, 0x68,    /* grass tufts */
    0x2A, 0x16, 0x34,    /* the salon's sign and doors: its text's background */
};
static unsigned char saved_pal[48];
static unsigned char saved_regs[13];

static unsigned char swap_nib(unsigned char v) {
    return (unsigned char)(v << 4 | v >> 4);
}

static void set_palette(void) {
    unsigned char i, k = 0;
    for (i = 0; i < 16; i++) {
        saved_pal[i] = PAL_R(i); saved_pal[16 + i] = PAL_G(i); saved_pal[32 + i] = PAL_B(i);
    }
    for (i = 0; i < 12; i++, k += 3) {
        unsigned char n = pal_idx[i];
        PAL_R(n) = swap_nib(pal_rgb[k]);
        PAL_G(n) = swap_nib(pal_rgb[k + 1]);
        PAL_B(n) = swap_nib(pal_rgb[k + 2]);
    }
}

static void restore_palette(void) {
    unsigned char i;
    for (i = 0; i < 16; i++) {
        PAL_R(i) = saved_pal[i]; PAL_G(i) = saved_pal[16 + i]; PAL_B(i) = saved_pal[32 + i];
    }
}

/* ------------------------------------------------------------------------ */

static void setup(void) {
    unsigned char i;
    saved_regs[0] = REG(0xD068); saved_regs[1] = REG(0xD069); saved_regs[2] = REG(0xD06A);
    saved_regs[3] = REG(0xD020); saved_regs[4] = REG(0xD021);
    saved_regs[5] = REG(0xD022); saved_regs[6] = REG(0xD023); saved_regs[7] = REG(0xD016);
    saved_regs[8] = REG(0xD05D);
    for (i = 0; i < 4; i++) saved_regs[9 + i] = REG(0xD060 + i);
    /* the screen shown: bank 5 (built at $0800) */
    dma_fill_far(SHOWN_LIN, COLS * ROWS, CH_SPACE);
    REG(0xD060) = (unsigned char)SHOWN_LIN;
    REG(0xD061) = (unsigned char)(SHOWN_LIN >> 8);
    REG(0xD062) = (unsigned char)(SHOWN_LIN >> 16);
    REG(0xD063) = REG(0xD063) & 0xF0;
    /* the character sets: the ROM's (upper case and graphics; upper and
     * lower case for the title) copied to bank 4, our glyphs over CH_BASE.. */
    dma_copy_far(CHARSET_LIN, CHARROM_LIN, 2048);
    dma_copy_far(CHARSET_LIN + CH_BASE * 8, (unsigned long)(unsigned int)glyph_data, sizeof glyph_data);
    dma_copy_far(CHARSET2_LIN, CHARROM_LIN + 2048, 2048);
    dma_copy_far(CHARSET2_LIN + CH_BASE * 8, (unsigned long)(unsigned int)glyph_data, sizeof glyph_data);
    set_charset(CHARSET_LIN);
    set_palette();
    REG(0xD020) = C_DGREY;
    REG(0xD021) = C_MEADOW;
    REG(0xD022) = C_DGREY;
    REG(0xD023) = C_TUFT;
    fps = (REG(0xD06F) & 0x80) ? 60 : 50;
    memset(scr, CH_SPACE, COLS * ROWS);
    memset(clr, C_WHITE, sizeof clr);
    show();
    /* multicolour characters; with the VIC-IV's hot registers off, or
     * writing $D016 would recompute (and lose) the 80-column layout */
    REG(0xD05D) = REG(0xD05D) & 0x7F;
    REG(0xD016) = REG(0xD016) | 0x10;
    rnd_state ^= (unsigned int)FRAMECOUNT << 8 | REG(0xD012);
    if (!rnd_state) rnd_state = 0xACE1;
    make_background();
    sid_init();
}

static void restore(void) {
    unsigned char i;
    sid_init();
    REG(0xD016) = saved_regs[7];
    REG(0xD05D) = saved_regs[8];
    REG(0xD068) = saved_regs[0]; REG(0xD069) = saved_regs[1]; REG(0xD06A) = saved_regs[2];
    REG(0xD020) = saved_regs[3]; REG(0xD021) = saved_regs[4];
    REG(0xD022) = saved_regs[5]; REG(0xD023) = saved_regs[6];
    for (i = 0; i < 4; i++) REG(0xD060 + i) = saved_regs[9 + i];
    restore_palette();
    dma_fill_far(SCREEN_LIN, COLS * ROWS, CH_SPACE);
    dma_fill_far(COLOR_LIN, COLS * ROWS, C_WHITE);
}

static unsigned char m_held;
static unsigned char frame_seen;      /* FRAMECOUNT the sound has caught up to */

int main(void) {
    music_load();                     /* (first: the KERNAL's disk code, before our screen) */
    salon_load();
    setup();
    if (salon_ok) salon_setup();
    title();
    input_flush();
    held = 0xFF;                      /* a key held at the start (RUN's RETURN) is no press */
    sec_frames = fps;
    frame_seen = FRAMECOUNT;
    for (;;) {
        unsigned char pressed;
        wait_vsync();
        while (frame_seen != FRAMECOUNT) {
            frame_seen++;
            snd_update();
            tune_update();
        }
#ifdef DEMO
        REG(0xC0F0) = music_ok;           /* (tests: the music's state) */
        REG(0xC0F1) = music_at;
        REG(0xC0F2) = (unsigned char)tune_ticks();
        REG(0xC0F3) = (unsigned char)(tune_ticks() >> 8);
#endif
        if (input_pressed(KEY_STOP)) break;
        if (input_pressed(KEY_M)) {
            if (!m_held) music_toggle();
            m_held = 1;
        } else m_held = 0;
        prev_held = held;
#ifdef DEMO
        held = demo_controls();
#else
        held = controls();
        if (attract) {
            if (held || input_waiting()) {   /* a touch: back to the title, or (fire) a game */
                unsigned char fire = held & K_FIRE;
                title();
                if (fire) start_season();
                held = prev_held = 0xFF;  /* (released first) */
            } else if (--attract_t == 0 || mode == M_OVER) {
                title();
                held = prev_held = 0xFF;
            } else {
                held = demo_controls();
                if (!msg_t) message(s"demo  -  fire to play, any key for the title", C_YELLOW);
            }
        } else if (mode == M_TITLE) {
            if (held || input_waiting()) idle_t = 0;   /* any key at all (the buffer is emptied each frame) */
            else if (++idle_t >= IDLE_T * fps) {
                start_season();
                attract = 1;
                attract_t = ATTRACT_T * fps;
                message(s"demo  -  fire to play, any key for the title", C_YELLOW);
                held = prev_held = 0;
            }
        }
#endif
        pressed = held & (unsigned char)~prev_held;
        if (mode == M_TITLE || mode == M_OVER) {
            if ((pressed & K_FIRE) && !over_t) start_season();
        } else if (mode == M_PLAY) {
            play_input(pressed);
        } else if (mode == M_SALON) {
            salon_input(pressed);
        }
        if (mode == M_TITLE) {
            title_frame();
            show();
            input_flush();
            continue;
        }
        if (mode == M_OVER) {
            if (over_t) over_t--;     /* the field stands still behind the box */
        } else {
            move_sheep();
            if (mode == M_PLAY || mode == M_SALON) move_shearer();
            if (--sec_frames == 0) { sec_frames = fps; second(); }
        }
        if (msg_t && --msg_t == 0) clear_span(0, MSG_ROW, COLS);
        if (hud_dirty) { draw_hud(); hud_dirty = 0; }
        if (panel_dirty) { draw_panel(); panel_dirty = 0; }
        if (sal.shown) draw_salon();  /* (the room's own screen) */
        else {
            draw_field();
            if (mode == M_OVER) draw_over_box();
            show();
        }
        input_flush();                /* the KERNAL's key buffer: unused */
#ifdef DEMO
        REG(0xC0FA) = salon_ok;       /* (tests: the salon's files loaded; $D054 in the room) */
        if (sal.shown) REG(0xC0FB) = REG(0xD054);
        REG(0xC0F7) = p_x;            /* (tests: the stylist's pace) */
        REG(0xC0F8) = p_y;
        REG(0xC0F9) = mode;
#endif
    }
    salon_off();
    if (salon_ok) salon_unsetup();
    restore();
    return 0;
}
