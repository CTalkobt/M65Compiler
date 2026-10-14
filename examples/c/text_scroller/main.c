/* text_scroller — MEGA65 horizontal text scroller
 *
 * Demonstrates mixed C and assembly:
 *   - C:   string handling, game loop, scroll logic
 *   - ASM: hardware screen ops (DMA-free byte shifting, VSYNC, colour RAM)
 *
 * The message scrolls continuously across row 8 of the MEGA65's 80x25
 * text screen, between two banner rows.
 * Border colour cycles through a palette on each full scroll.
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
    while (1) {
        /* Wait for vertical blank — one scroll step per frame */
        wait_vsync();

        /* Check RUN/STOP key */
        if (cbm_stop())
            break;

        /* Shift the scroll row left by one character */
        scroll_line_left();

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
    BORDER = 14;   /* restore light blue border */
    BGCOLOR = 6;   /* restore blue background */
    cbm_chrout(0x9A);   /* light blue text */
    cbm_chrout(0x93);

    return 0;
}
