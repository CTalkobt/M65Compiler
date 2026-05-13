/* text_scroller — MEGA65 horizontal text scroller
 *
 * Demonstrates mixed C and assembly:
 *   - C:   string handling, game loop, scroll logic
 *   - ASM: hardware screen ops (DMA-free byte shifting, VSYNC, colour RAM)
 *
 * The message scrolls continuously across row 12 (center of screen).
 * Border colour cycles through a palette on each full scroll.
 * Press RUN/STOP to exit.
 */

#include <cbm.h>
#include <string.h>

/* Assembly routines from scroll_hw.s */
void scroll_line_left(int row);
void set_text_color(int row, char color);
void wait_vsync(void);
void clear_screen(char color);

/* Screen constants */
#define SCREEN  0x0800
#define COLS    40
#define ROWS    25
#define SCROLL_ROW  12

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

const char *message = "    welcome to the mega65!    this scroller was built with cc45 -- a c compiler targeting the 45gs02 cpu.    mixed c and assembly makes hardware access easy.    press run/stop to exit...    ";

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

void draw_banner(char color) {
    char *scr;
    int i;

    /* top border: row 10 */
    scr = (char *)SCREEN + 10 * COLS;
    for (i = 0; i < COLS; i++)
        scr[i] = 0x43;  /* PETSCII horizontal bar */

    /* bottom border: row 14 */
    scr = (char *)SCREEN + 14 * COLS;
    for (i = 0; i < COLS; i++)
        scr[i] = 0x43;

    set_text_color(10, color);
    set_text_color(14, color);
}

int main() {
    int msg_pos;
    int msg_len;
    int pal_idx;
    int speed_count;
    char *row_ptr;
    char next_char;

    clear_screen(COL_WHITE);
    init_palette();

    BORDER = 0;   /* black border */
    BGCOLOR = 0;  /* black background */

    msg_len = strlen(message);
    msg_pos = 0;
    pal_idx = 0;
    speed_count = 0;

    /* Set scroll row colour */
    set_text_color(SCROLL_ROW, palette[pal_idx]);
    draw_banner(palette[pal_idx]);

    /* Title line */
    cbm_gotoxy(12, 2);
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
        scroll_line_left(SCROLL_ROW);

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
            set_text_color(SCROLL_ROW, palette[pal_idx]);
            draw_banner(palette[pal_idx]);
            BORDER = palette[pal_idx];
        }
    }

    /* Clean exit */
    BORDER = 14;   /* restore light blue border */
    BGCOLOR = 6;   /* restore blue background */
    clear_screen(COL_LBLUE);

    return 0;
}
