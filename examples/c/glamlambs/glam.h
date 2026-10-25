/* glam.h -- Glam Lambs: what main.c, title.c (the title page: a code
 * overlay) and salon.c (the salon: another) share */
#pragma once
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

/* (main.c's) */
#define BGK_LIN 0x5C000UL
#define BGC_LIN 0x5C800UL
#define V_BAA   0
#define V_BIRTH 1
#define V_CLICK 2
#define MUSIC_LIN 0x0050000UL         /* the tunes: bank 5 */
#define V_BAND 3                      /* the band's first voice: SID 2's first */
#define MUS_FARM 1
#define MUS_SALON 2
#define L_WOOL   0x03                 /* 0 natural, pink, blue, gold */
#define L_LIPS   0x0C                 /* 0 natural, red, purple, pink */
#define L_LASH   0x10
#define L_BOW    0x20
#define NTRENDS 8
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
#define SHEARER_FIRST   60
#define SHEARER_SOONER  15
#define SHEARER_SOONEST 10
#define SNIP_FRAMES 100
#define SHOO_TIME   10                /* seconds */
#define NFX 6
enum { M_TITLE, M_PLAY, M_SALON, M_OVER };
#define IDLE_T    60
#define ATTRACT_T 40
#ifndef SEASON
#define SEASON  180                   /* seconds (tests: -DSEASON=n, at least SHEARER_FIRST) */
#endif
#define TREND_T 30
#define BOX_X 21
#define BOX_Y 3
#define BOX_W 38
#define BOX_H 19
#define K_UP    0x01
#define K_DOWN  0x02
#define K_LEFT  0x04
#define K_RIGHT 0x08
#define K_FIRE  0x10
#define DEMO_PAUSE 12
#define PAL_R(i) REG(0xD100 + (i))
#define PAL_G(i) REG(0xD200 + (i))
#define PAL_B(i) REG(0xD300 + (i))

/* (main.c, salon.c, title.c) */
extern unsigned char clr[COLS * ROWS];
void put(unsigned char x, unsigned char y, unsigned char ch, unsigned char c);
void put_text(unsigned char x, unsigned char y, const char *s, unsigned char c);
void put_number(unsigned char x, unsigned char y, unsigned int v, unsigned char digits, unsigned char c);
void clear_span(unsigned char x, unsigned char y, unsigned char n);
void show(void);
void set_charset(unsigned long lin);
void put_ascii(unsigned char x, unsigned char y, const char *s, unsigned char c);
extern unsigned int rnd_state ;
unsigned int rnd(void);
unsigned char rnd_n(unsigned char n);
void snd_click(unsigned int f);
void music_restart(void);
void music_for(unsigned char salon);
extern const char *const wool_name[4];
extern const char *const lips_name[4];
extern const char *const yesno_name[2];
extern const char *const trend_name[NTRENDS];
extern unsigned char trend;
extern unsigned char s_x[MAXS], s_y[MAXS], s_flags[MAXS], s_look[MAXS];
extern unsigned char s_age[MAXS], s_life[MAXS], s_cool[MAXS], s_gen[MAXS], s_tmr[MAXS];
extern unsigned char s_wool[MAXS], s_woolt[MAXS], s_shear[MAXS];
extern unsigned char p_x, p_y, p_tmr, p_step;
extern unsigned char carry ;
extern unsigned char sh_on, sh_x, sh_y, sh_tmr, sh_step, sh_snip, sh_rest;
extern unsigned char mode;
extern unsigned char fps;
extern unsigned char season;
extern unsigned int score, hiscore;
extern unsigned int hiscore_was;
extern unsigned char over_t;
extern unsigned int lambs, glam_lambs;
extern unsigned int lamb_pts, wool_bags, income;
extern unsigned char sel;
extern unsigned char hud_dirty, panel_dirty;
extern unsigned char attract;
extern unsigned int idle_t, attract_t;
void draw_sheep_at(unsigned char x, unsigned char y, unsigned char i);
void fence_box(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1);
extern const char *const salon_rows[5];
void leave_salon(void);
void clear_flock(void);
extern const unsigned char pal_idx[12];
extern const unsigned char pal_rgb[36];
extern unsigned char saved_pal[48];
extern unsigned char saved_regs[13];
void title_frame(void);
void title(void);
void setup(void);
void restore(void);
void draw_over_box(void);
extern unsigned char salon_ok;
extern seam_screen_t sal;
void salon_load(void);
void salon_setup(void);
void salon_unsetup(void);
void draw_salon(void);
void salon_on(void);
void salon_off(void);
void salon_input(unsigned char pressed);
