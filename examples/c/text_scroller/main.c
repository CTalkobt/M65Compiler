/* text_scroller — MEGA65 horizontal text scroller
 *
 * Demonstrates mixed C and assembly:
 *   - C:   string handling, game loop, scroll logic
 *   - ASM: hardware screen ops (DMA-free byte shifting, VSYNC, colour RAM)
 *
 * The message scrolls continuously across row 7 of the MEGA65's 80x25
 * text screen, between two banner rows (3 and 11).
 * Border colour cycles through a palette on each full scroll.
 * A ball (hardware sprite 0, through <sprites.h>) bounces between the banners, passing in
 * front of the text on one crossing and behind it on the next. The
 * message is in reverse video: a sprite behind text is hidden only by the
 * characters' set pixels, so a solid band shows the difference.
 * Press RUN/STOP to exit.
 */

#include <cbm.h>
#include <string.h>
#include <sprites.h>

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
#define SCROLL_ROW  7
#define TOP_BANNER  3
#define BOTTOM_BANNER 11

/* Border colour register */
#define BORDER  (*(char *)0xD020)
#define BGCOLOR (*(char *)0xD021)

/* Ball limits in sprite coordinates: X across the visible width; Y between
 * the banner lines (text row r starts at sprite Y 50 + 8r; the ball's
 * pixels are sprite rows 1-19) */
#define BALL_XMIN  24
#define BALL_XMAX  (344 - 24)
#define BALL_YMIN  (50 + 8 * (TOP_BANNER + 1) - 1)
#define BALL_YMAX  (50 + 8 * BOTTOM_BANNER - 20)

/* The ball: 24x21 pixels, 3 bytes a row, 64-byte aligned for the VIC */
static _Alignas(64) unsigned char ball[64] = {
    0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x01, 0xFF, 0x80, 0x03, 0xFF, 0xC0,
    0x07, 0xFF, 0xE0, 0x0F, 0xFF, 0xF0, 0x1F, 0xFF, 0xF8, 0x1F, 0xFF, 0xF8,
    0x1F, 0xFF, 0xF8, 0x1F, 0xFF, 0xF8, 0x3F, 0xFF, 0xFC, 0x1F, 0xFF, 0xF8,
    0x1F, 0xFF, 0xF8, 0x1F, 0xFF, 0xF8, 0x1F, 0xFF, 0xF8, 0x0F, 0xFF, 0xF0,
    0x07, 0xFF, 0xE0, 0x03, 0xFF, 0xC0, 0x01, 0xFF, 0x80, 0x00, 0x7E, 0x00,
    0x00, 0x00, 0x00, 0x00,
};

/* Ball position and velocity (signed: it moves both ways) */
static signed int ball_x = 100, ball_y = BALL_YMIN;
static signed int ball_dx = 3, ball_dy = 2;
static unsigned char ball_behind;

static void ball_init(void) {
    sprite_init();                /* (sprite_done puts BASIC's sprite registers back) */
    sprite_image(0, (unsigned long)(unsigned int)ball);
    sprite_color(0, 7);           /* yellow */
    sprite_pos(0, (unsigned int)ball_x, (unsigned int)ball_y);
    sprite_show(0);
}

static void ball_move(void) {
    ball_x += ball_dx;
    ball_y += ball_dy;
    /* each bounce off a side: in front of the text, then behind it */
    if (ball_x <= BALL_XMIN || ball_x >= BALL_XMAX) {
        ball_x = ball_x <= BALL_XMIN ? BALL_XMIN : BALL_XMAX;
        ball_dx = -ball_dx;
        ball_behind ^= 1;
        sprite_behind(0, ball_behind);
    }
    if (ball_y <= BALL_YMIN) { ball_y = BALL_YMIN; ball_dy = -ball_dy; }
    if (ball_y >= BALL_YMAX) { ball_y = BALL_YMAX; ball_dy = -ball_dy; }
    sprite_pos(0, (unsigned int)ball_x, (unsigned int)ball_y);
}

static void ball_done(void) {
    sprite_done();
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

    /* top border */
    scr = (char *)SCREEN + TOP_BANNER * COLS;
    for (i = 0; i < COLS; i++)
        scr[i] = 0x43;  /* screen code: horizontal bar */

    /* bottom border */
    scr = (char *)SCREEN + BOTTOM_BANNER * COLS;
    for (i = 0; i < COLS; i++)
        scr[i] = 0x43;

    color_row(TOP_BANNER, color);
    color_row(BOTTOM_BANNER, color);
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
    {   /* the message row starts as reverse spaces (a solid band) */
        int c;
        for (c = 0; c < COLS; c++) scroll_ptr[c] = 0xA0;
    }
    color_row(SCROLL_ROW, palette[pal_idx]);
    draw_banner(palette[pal_idx]);

    /* Title line */
    cbm_gotoxy(33, 1);
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
        next_char = message[msg_pos] | 0x80;   /* reverse video */
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
