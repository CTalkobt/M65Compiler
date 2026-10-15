/* text_scroller — MEGA65 horizontal text scroller
 *
 * Demonstrates mixed C and assembly:
 *   - C:   string handling, game loop, scroll logic
 *   - ASM: hardware screen ops (DMA-free byte shifting, VSYNC, colour RAM)
 *
 * The message scrolls continuously across row 8 of the MEGA65's 80x25
 * text screen, between two banner rows.
 * Border colour cycles through a palette on each full scroll.
 * A ball (hardware sprite 0) bounces around the screen.
 * Press RUN/STOP to exit.
 */

#include <cbm.h>
#include <string.h>

/* Assembly routines from scroll_hw.s45; their arguments go through the
 * variables it defines */
void scroll_line_left(void);    /* the row at scroll_ptr */
void set_text_color(void);      /* colour RAM row at color_ptr := fill_color */
void wait_vsync(void);
extern char *scroll_ptr;
extern char *color_ptr;
extern char fill_color;

/* Screen constants: the 80x25 text screen. Colour RAM rows 0-11 lie at
 * $D800-$DBBF; above $DC00 are the CIAs (unless CRAM2K is set), so the
 * colour rows used here stay below row 12. */
#define SCREEN  0x0800
#define COLRAM  0xD800
#define COLS    80
#define ROWS    25
#define SCROLL_ROW  8

/* Border colour register */
#define BORDER  (*(char *)0xD020)
#define BGCOLOR (*(char *)0xD021)

/* Sprite registers (VIC-II, kept by the VIC-IV) */
#define REG(a)     (*(volatile unsigned char *)(a))
#define SPR0_X     REG(0xD000)
#define SPR0_Y     REG(0xD001)
#define SPR_X_MSB  REG(0xD010)   /* bit 8 of each sprite's X */
#define SPR_ENABLE REG(0xD015)
#define SPR0_COLOR REG(0xD027)
/* VIC-IV sprite pointer table address; $D06E bit 7: 16-bit pointers, so a
 * sprite's data can be on any 64-byte boundary (pointer = address / 64) */
#define SPRPTR_LO  REG(0xD06C)
#define SPRPTR_HI  REG(0xD06D)
#define SPRPTR_BNK REG(0xD06E)

/* Visible area in sprite coordinates (24x21 sprite fully on screen) */
#define BALL_XMIN  24
#define BALL_XMAX  (344 - 24)
#define BALL_YMIN  50
#define BALL_YMAX  (250 - 21)

/* The ball: 24x21 pixels, 3 bytes a row, 64-byte aligned for the VIC */
static _Alignas(64) unsigned char ball[64] = {
    0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x01, 0xFF, 0x80, 0x03, 0xFF, 0xC0,
    0x07, 0xFF, 0xE0, 0x0F, 0xFF, 0xF0, 0x1F, 0xFF, 0xF8, 0x1F, 0xFF, 0xF8,
    0x1F, 0xFF, 0xF8, 0x1F, 0xFF, 0xF8, 0x3F, 0xFF, 0xFC, 0x1F, 0xFF, 0xF8,
    0x1F, 0xFF, 0xF8, 0x1F, 0xFF, 0xF8, 0x1F, 0xFF, 0xF8, 0x0F, 0xFF, 0xF0,
    0x07, 0xFF, 0xE0, 0x03, 0xFF, 0xC0, 0x01, 0xFF, 0x80, 0x00, 0x7E, 0x00,
    0x00, 0x00, 0x00, 0x00,
};
/* 16-bit sprite pointers; the VIC-IV ignores the low 4 bits of the table
 * address in 16-bit pointer mode, so the table is 16-byte aligned */
static _Alignas(16) unsigned int sprite_ptrs[8];
static unsigned char saved_ptr[3];    /* the pointer table address BASIC uses */

/* Ball position and velocity (signed: it moves both ways) */
static signed int ball_x = 100, ball_y = 60;
static signed int ball_dx = 2, ball_dy = 1;

static void ball_init(void) {
    saved_ptr[0] = SPRPTR_LO;
    saved_ptr[1] = SPRPTR_HI;
    saved_ptr[2] = SPRPTR_BNK;
    sprite_ptrs[0] = (unsigned int)ball / 64;
    SPRPTR_LO = (unsigned int)sprite_ptrs & 0xFF;
    SPRPTR_HI = (unsigned int)sprite_ptrs >> 8;
    SPRPTR_BNK = 0x80;            /* bank 0, 16-bit pointers */
    SPR0_COLOR = 7;               /* yellow */
    SPR_ENABLE = SPR_ENABLE | 0x01;
}

static void ball_move(void) {
    ball_x += ball_dx;
    ball_y += ball_dy;
    if (ball_x <= BALL_XMIN || ball_x >= BALL_XMAX) ball_dx = -ball_dx;
    if (ball_y <= BALL_YMIN || ball_y >= BALL_YMAX) ball_dy = -ball_dy;
    SPR0_X = ball_x & 0xFF;
    if (ball_x > 255) SPR_X_MSB = SPR_X_MSB | 0x01;
    else SPR_X_MSB = SPR_X_MSB & 0xFE;
    SPR0_Y = ball_y;
}

static void ball_done(void) {
    SPR_ENABLE = SPR_ENABLE & 0xFE;
    SPRPTR_LO = saved_ptr[0];
    SPRPTR_HI = saved_ptr[1];
    SPRPTR_BNK = saved_ptr[2];
}

/* PETSCII colour codes for colour RAM */
#define COL_WHITE   1
#define COL_CYAN    3
#define COL_GREEN   5
#define COL_BLUE    6
#define COL_YELLOW  7
#define COL_LGREEN  13
#define COL_LBLUE   14

/* s"...": screen codes, as screen RAM holds them (PETSCII would show
 * graphics characters) */
const char *message = s"    welcome to the mega65!    this scroller was built with cc45 -- a c compiler targeting the 45gs02 cpu.    mixed c and assembly makes hardware access easy.    press run/stop to exit...    ";

char palette[7];
int palette_len;

void init_palette() {
    palette[0] = COL_CYAN;
    palette[1] = COL_LBLUE;
    palette[2] = COL_BLUE;
    palette[3] = COL_GREEN;
    palette[4] = COL_LGREEN;
    palette[5] = COL_YELLOW;
    palette[6] = COL_WHITE;
    palette_len = 7;
}

void color_row(int row, char color) {
    color_ptr = (char *)COLRAM + row * COLS;
    fill_color = color;
    set_text_color();
}

void draw_banner(char color) {
    char *scr;
    int i;

    /* top border: row 6 */
    scr = (char *)SCREEN + 6 * COLS;
    for (i = 0; i < COLS; i++)
        scr[i] = 0x43;  /* screen code: horizontal bar */

    /* bottom border: row 10 */
    scr = (char *)SCREEN + 10 * COLS;
    for (i = 0; i < COLS; i++)
        scr[i] = 0x43;

    color_row(6, color);
    color_row(10, color);
}

int main() {
    int msg_pos;
    int msg_len;
    int pal_idx;
    char *row_ptr;
    char next_char;

    /* White text, then clear the screen: the KERNAL clears all 2000 cells
     * and fills their colour RAM with the text colour */
    cbm_chrout(0x05);
    cbm_chrout(0x93);
    init_palette();

    BORDER = 0;   /* black border */
    BGCOLOR = 0;  /* black background */

    msg_len = strlen(message);
    msg_pos = 0;
    pal_idx = 0;

    /* Set scroll row colour */
    scroll_ptr = (char *)SCREEN + SCROLL_ROW * COLS;
    color_row(SCROLL_ROW, palette[pal_idx]);
    draw_banner(palette[pal_idx]);

    /* Title line */
    cbm_gotoxy(33, 2);
    {
        const char *title = "text scroller";
        int t;
        for (t = 0; title[t]; t++)
            cbm_chrout(title[t]);
    }

    /* Main scroll loop */
    ball_init();
    while (1) {
        /* Wait for vertical blank — one scroll step per frame */
        wait_vsync();

        /* Check RUN/STOP key */
        if (cbm_stop())
            break;

        /* Shift the scroll row left by one character */
        scroll_line_left();
        ball_move();

        /* Place next character from message at rightmost column */
        next_char = message[msg_pos];
        row_ptr = (char *)SCREEN + SCROLL_ROW * COLS;
        row_ptr[COLS - 1] = next_char;

        /* Advance message position, wrap around */
        msg_pos = msg_pos + 1;
        if (msg_pos >= msg_len)
            msg_pos = 0;

        /* Cycle palette colour every full message scroll */
        if (msg_pos == 0) {
            pal_idx = pal_idx + 1;
            if (pal_idx >= palette_len)
                pal_idx = 0;
            color_row(SCROLL_ROW, palette[pal_idx]);
            draw_banner(palette[pal_idx]);
            BORDER = palette[pal_idx];
        }
    }

    /* Clean exit */
    ball_done();
    BORDER = 14;   /* restore light blue border */
    BGCOLOR = 6;   /* restore blue background */
    cbm_chrout(0x9A);   /* light blue text */
    cbm_chrout(0x93);

    return 0;
}
