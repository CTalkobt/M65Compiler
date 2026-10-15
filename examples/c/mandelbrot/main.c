/* mandelbrot — a Mandelbrot set viewer for the MEGA65, built with cc45
 *
 * Shows off:
 *   - VIC-IV full-colour character mode (FCM) as a 320 x 192 bitmap with a
 *     byte per pixel: 16-bit character numbers point every cell at its own
 *     64-byte pixel block in bank 4 ($40000), a text status line below
 *   - the 256-colour palette: a 240-entry gradient, cycled by rewriting the
 *     palette registers (no pixel is redrawn)
 *   - mixed C and assembly: the iteration (mandel_asm.s45) runs on the
 *     45GS02's 32-bit Q register with the hardware multiplier, in 4.28
 *     fixed point (zoom to about 2^20); points in the main cardioid and
 *     the period-2 bulb are recognised without iterating
 *   - DMA: each finished cell goes into far memory with dma_copy_far, the
 *     preview pass fills cells with dma_fill_far
 *   - scrolling without redrawing: every cell holds a pointer to its pixel
 *     block, so a move shifts the 40 x 24 pointers (the blocks that leave
 *     the screen come back on the exposed edge) and only that edge renders
 *
 * Keys: cursor keys, W A S D or a joystick (either port): move -
 *       Z / X, 1 / 2 (or fire): zoom in / out - + / -: colour cycling
 *       faster / slower - I / O: more / fewer iterations - P: palette -
 *       C: cycling on / off - R: reset - ? or H: help - RUN/STOP: quit
 */
#include <stdio.h>
#include <dma.h>
#include <input.h>
#include <mega65.h>

/* the iteration kernel (mandel_asm.s45) */
extern signed long mb_cr, mb_ci;
extern unsigned char mb_maxit;
unsigned char mandel_iter(void);

#define REG(a) (*(volatile unsigned char *)(a))

#define CW 40                      /* cells across */
#define CH 24                      /* cell rows of the image (row 24: status) */
#define W (CW * 8)
#define H (CH * 8)
#define PIX_LIN   0x0040000UL      /* pixel blocks, 64 bytes per cell */
#define COLOR_LIN 0xFF80000UL

#define ONE       0x10000000L      /* 1.0 in 4.28 fixed point */
#define BASE_STEP (ONE * 3 / W)    /* the whole set across the screen */
#define MIN_STEP  16L              /* zoom limit (precision) */

static unsigned int screen[CW * 25];     /* 16-bit character numbers */
static unsigned char cellbuf[64];

/* ------------------------------------------------------------------------ */
/* The view and the renderer                                                */

static signed long cx, cy, step;         /* centre and pixel size */
static unsigned char zoom;               /* step = BASE_STEP >> zoom */

/* per cell: 0 needs a preview, 1 needs its detail, 2 done; the renderer
 * does every preview first, then the details */
static unsigned char cstate[CW * CH];
static unsigned char level;              /* the state being worked on */
static unsigned int scan;
static unsigned char busy;

static unsigned char color_of(unsigned char n) {
    if (n >= mb_maxit) return 0;                  /* inside: black */
    return (unsigned char)(16 + (n * 4) % 240);
}

static signed long offs[8];              /* 0, step, ... 7 * step */

/* render everything again (a zoom, new iterations) */
static void restart(void) {
    unsigned int i;
    for (i = 0; i < 8; i++) offs[i] = (signed long)i * step;
    for (i = 0; i < CW * CH; i++) cstate[i] = 0;
    level = 0;
    scan = 0;
    busy = 1;
}

/* move the view by dc cells right and dr cells down: the cell pointers
 * (and their states) shift; the pixel blocks scrolled off the screen come
 * back on the exposed edge, which renders anew */
static unsigned int prev_screen[CW * CH];
static unsigned char prev_state[CW * CH];
static void scroll_cells(signed char dc, signed char dr) {
    signed char r, c;
    unsigned int i;
    for (i = 0; i < CW * CH; i++) { prev_screen[i] = screen[i]; prev_state[i] = cstate[i]; }
    for (r = 0; r < CH; r++)
        for (c = 0; c < CW; c++) {
            signed char sr = r + dr, sc = c + dc;
            unsigned char exposed = sr < 0 || sr >= CH || sc < 0 || sc >= CW;
            if (sr < 0) sr += CH; else if (sr >= CH) sr -= CH;
            if (sc < 0) sc += CW; else if (sc >= CW) sc -= CW;
            i = (unsigned int)(r * CW + c);
            screen[i] = prev_screen[sr * CW + sc];
            cstate[i] = exposed ? 0 : prev_state[sr * CW + sc];
        }
    level = 0;
    scan = 0;
    busy = 1;
}

static signed long cell_left, cell_top;
static unsigned char pixel(unsigned char x, unsigned char y) {
    mb_cr = cell_left + offs[x];
    mb_ci = cell_top + offs[y];
    return color_of(mandel_iter());
}

/* one cell: its preview (one sample) or its detail */
static void render_cell(unsigned int cell, unsigned char detail) {
    unsigned char col = (unsigned char)(cell % CW), row = (unsigned char)(cell / CW);
    signed long left = cx + ((signed long)col * 8 - W / 2) * step;
    signed long top = cy + ((signed long)row * 8 - H / 2) * step;
    unsigned long dest = (unsigned long)screen[cell] * 64;     /* its pixel block */
    if (!detail) {
        mb_cr = left + 4 * step;
        mb_ci = top + 4 * step;
        dma_fill_far(dest, 64, color_of(mandel_iter()));
    } else {
        /* the border first: all one colour fills the cell (the set is
         * connected, so nothing different lies inside: Mariani-Silver) */
        unsigned char x, y, c, same = 1;
        cell_left = left;
        cell_top = top;
        for (x = 0; x < 8; x++) {
            cellbuf[x] = pixel(x, 0);
            cellbuf[56 + x] = pixel(x, 7);
        }
        for (y = 1; y < 7; y++) {
            cellbuf[y * 8] = pixel(0, y);
            cellbuf[y * 8 + 7] = pixel(7, y);
        }
        c = cellbuf[0];
        for (x = 0; x < 8 && same; x++)
            if (cellbuf[x] != c || cellbuf[56 + x] != c || cellbuf[x * 8] != c || cellbuf[x * 8 + 7] != c) same = 0;
        if (same) {
            dma_fill_far(dest, 64, c);
        } else {
            /* the same one level down: the rows and columns between the
             * four 4 x 4 quadrants, then each quadrant's 2 x 2 inside is
             * filled (its border all one colour) or computed */
            unsigned char q, x0, y0, k, i;
            for (y = 1; y < 7; y++) {
                cellbuf[y * 8 + 3] = pixel(3, y);
                cellbuf[y * 8 + 4] = pixel(4, y);
            }
            for (x = 1; x < 7; x++) {
                if (x == 3) x = 5;
                cellbuf[24 + x] = pixel(x, 3);
                cellbuf[32 + x] = pixel(x, 4);
            }
            for (q = 0; q < 4; q++) {
                x0 = (q & 1) << 2;
                y0 = (q & 2) << 1;
                i = y0 * 8 + x0;
                c = cellbuf[i];
                same = 1;
                for (k = 0; k < 4 && same; k++)
                    if (cellbuf[i + k] != c || cellbuf[i + 24 + k] != c || cellbuf[i + k * 8] != c ||
                        cellbuf[i + k * 8 + 3] != c) same = 0;
                for (y = y0 + 1; y < y0 + 3; y++)
                    for (x = x0 + 1; x < x0 + 3; x++) cellbuf[y * 8 + x] = same ? c : pixel(x, y);
            }
            dma_copy_far(dest, (unsigned long)(unsigned int)cellbuf, 64);
        }
    }
}

/* the next cell that needs work, if any */
static void render_step(void) {
    unsigned int n;
    for (n = 0; n < CW * CH; n++) {
        unsigned int c = scan;
        if (++scan == CW * CH) scan = 0;
        if (cstate[c] == level) {
            render_cell(c, level);
            cstate[c] = (unsigned char)(level + 1);
            return;
        }
    }
    if (level == 0) { level = 1; scan = 0; }
    else busy = 0;
}

/* ------------------------------------------------------------------------ */
/* The palette: 240 entries (16-255) around a ring of key colours, cycled    */

#define NSCHEMES 3
#define NKEYS 6
static const unsigned char keys[NSCHEMES][NKEYS][3] = {
    {{0, 0, 0}, {160, 0, 0}, {255, 120, 0}, {255, 240, 80}, {255, 255, 255}, {120, 0, 80}},      /* fire */
    {{0, 0, 40}, {0, 60, 170}, {0, 200, 255}, {240, 255, 255}, {0, 150, 140}, {0, 30, 90}},      /* ocean */
    {{255, 0, 0}, {255, 255, 0}, {0, 255, 0}, {0, 255, 255}, {0, 0, 255}, {255, 0, 255}},        /* rainbow */
};
static unsigned char pr[240], pg[240], pb[240];   /* nibble-swapped, as the registers take them */
static unsigned char scheme, cycling = 1, pal_offset;

/* cycling speeds: palette steps per frame as (frames, steps) pairs; 0 stops */
#define NSPEEDS 6
static const unsigned char speed_frames[NSPEEDS] = {0, 4, 2, 1, 1, 1};
static const unsigned char speed_steps[NSPEEDS] = {0, 1, 1, 1, 2, 4};
static unsigned char speed = 3, speed_count;

/* once a frame: advance the palette at the current speed */
static void load_palette(void);
static void cycle_palette(void) {
    if (!cycling || !speed) return;
    if (++speed_count < speed_frames[speed]) return;
    speed_count = 0;
    pal_offset += speed_steps[speed];
    if (pal_offset >= 240) pal_offset -= 240;
    load_palette();
}

static unsigned char swap_nibbles(unsigned char v) {
    return (unsigned char)((v << 4) | (v >> 4));
}

static void make_palette(void) {
    unsigned char k, i;
    for (k = 0; k < NKEYS; k++) {
        const unsigned char *a = keys[scheme][k], *b = keys[scheme][(k + 1) % NKEYS];
        for (i = 0; i < 40; i++) {
            unsigned char e = (unsigned char)(k * 40 + i);
            pr[e] = swap_nibbles((unsigned char)(a[0] + ((signed int)b[0] - a[0]) * i / 40));
            pg[e] = swap_nibbles((unsigned char)(a[1] + ((signed int)b[1] - a[1]) * i / 40));
            pb[e] = swap_nibbles((unsigned char)(a[2] + ((signed int)b[2] - a[2]) * i / 40));
        }
    }
}

static void load_palette(void) {
    unsigned char i, j = pal_offset;
    for (i = 0; i < 240; i++) {
        REG(0xD100 + 16 + i) = pr[j];
        REG(0xD200 + 16 + i) = pg[j];
        REG(0xD300 + 16 + i) = pb[j];
        if (++j == 240) j = 0;
    }
}

/* ------------------------------------------------------------------------ */
/* The status line (row 24): position, zoom, iterations and "?=help"        */

static void put_at(unsigned char col, const char *s) {
    while (*s && col < CW) screen[CH * CW + col++] = (unsigned char)*s++;
}

/* v (4.28) as +d.ddddd */
static void put_fixed(unsigned char col, signed long v) {
    unsigned long f = (unsigned long)(v < 0 ? -v : v);
    unsigned char d;
    screen[CH * CW + col] = v < 0 ? 0x2D : 0x2B;              /* - + */
    screen[CH * CW + col + 1] = (unsigned char)(0x30 + (f >> 28));
    screen[CH * CW + col + 2] = 0x2E;                          /* . */
    f &= 0x0FFFFFFFUL;
    for (d = 0; d < 5; d++) {
        f *= 10;
        screen[CH * CW + col + 3 + d] = (unsigned char)(0x30 + (f >> 28));
        f &= 0x0FFFFFFFUL;
    }
}

static void put_num(unsigned char col, unsigned int v, unsigned char digits) {
    while (digits--) {
        screen[CH * CW + col + digits] = (unsigned char)(0x30 + v % 10);
        v /= 10;
    }
}

static void status(void) {
    unsigned char i;
    for (i = 0; i < CW; i++) screen[CH * CW + i] = 0x20;
    put_at(0, s"x");
    put_fixed(1, cx);
    put_at(10, s"y");
    put_fixed(11, -cy);
    put_at(20, s"z");
    put_num(21, zoom, 2);
    put_at(24, s"i");
    put_num(25, mb_maxit, 3);
    put_at(29, s"?=help");
    put_at(37, busy ? s"..." : s"   ");
}

/* ------------------------------------------------------------------------ */
/* The help screen: rows 4-17 show text drawn as FCM pixels (each help
 * character a 64-byte block in bank 5, made once from the ROM character
 * set); showing it swaps those rows' cell pointers, hiding it puts them
 * back - nothing re-renders */

static const char *const help_text[] = {
    s"       mandelbrot viewer - keys",
    s"",
    s" crsr  wasd  joystick    move",
    s" z  1  fire              zoom in",
    s" x  2                    zoom out",
    s" +  -                    colour speed",
    s" c                       cycling on/off",
    s" p                       next palette",
    s" i  o                    more/fewer iter",
    s" r                       reset the view",
    s" ?  h                    this help",
    s" run/stop                quit",
    s"",
    s"       any key: back to the picture",
};
#define HELP_ROW 4
#define HELP_LINES (sizeof help_text / sizeof help_text[0])
#define HELP_LIN   0x0050000UL           /* the help's pixel blocks */
#define CHARROM_LIN 0xFF7E000UL          /* the MEGA65 character set */

static unsigned char help_on;
static unsigned int help_saved[HELP_LINES * CW];

static void help_draw(void) {
    unsigned char r, c, y, x, glyph[8];
    for (r = 0; r < HELP_LINES; r++) {
        const char *t = help_text[r];
        unsigned char ink = r == 0 ? 7 : 1;                 /* yellow title, white text */
        for (c = 0; c < CW; c++) {
            unsigned char code = *t ? (unsigned char)*t++ : 0x20;
            dma_copy_far((unsigned long)(unsigned int)glyph, CHARROM_LIN + (unsigned int)code * 8, 8);
            for (y = 0; y < 8; y++)
                for (x = 0; x < 8; x++) cellbuf[y * 8 + x] = (glyph[y] & (0x80 >> x)) ? ink : 0;
            dma_copy_far(HELP_LIN + (unsigned long)(r * CW + c) * 64, (unsigned long)(unsigned int)cellbuf, 64);
        }
    }
}

static void help_show(void) {
    unsigned int i, base = HELP_ROW * CW;
    for (i = 0; i < HELP_LINES * CW; i++) {
        help_saved[i] = screen[base + i];
        screen[base + i] = (unsigned int)(HELP_LIN / 64 + i);
    }
    help_on = 1;
}

static void help_hide(void) {
    unsigned int i, base = HELP_ROW * CW;
    for (i = 0; i < HELP_LINES * CW; i++) screen[base + i] = help_saved[i];
    help_on = 0;
}

/* ------------------------------------------------------------------------ */
/* Input (<input.h>): key presses, one event per press with the hardware's
 * key repeat; the joysticks (either port) as held directions */


#define IN_LEFT  0x0001
#define IN_RIGHT 0x0002
#define IN_UP    0x0004
#define IN_DOWN  0x0008
#define IN_ZIN   0x0010
#define IN_ZOUT  0x0020
#define IN_MORE  0x0040
#define IN_FEWER 0x0080
#define IN_PAL   0x0100
#define IN_CYCLE 0x0200
#define IN_RESET 0x0400
#define IN_HELP  0x0800
#define IN_STOP  0x1000
#define IN_FAST  0x2000
#define IN_SLOW  0x4000
#define IN_MOVES (IN_LEFT | IN_RIGHT | IN_UP | IN_DOWN)

/* the key presses waiting in the queue */
static unsigned int read_keys(void) {
    unsigned int k = 0;
    unsigned char c;
    while ((c = input_key()) != 0) {
        switch (c) {
            case CH_CURSOR_RIGHT: case 'd': case 'D': k |= IN_RIGHT; break;     /* cursor right */
            case CH_CURSOR_LEFT: case 'a': case 'A': k |= IN_LEFT; break;      /* cursor left */
            case CH_CURSOR_DOWN: case 's': case 'S': k |= IN_DOWN; break;      /* cursor down */
            case CH_CURSOR_UP: case 'w': case 'W': k |= IN_UP; break;        /* cursor up */
            case 'z': case 'Z': case '1': k |= IN_ZIN; break;
            case 'x': case 'X': case '2': k |= IN_ZOUT; break;
            case '+': k |= IN_FAST; break;
            case '-': k |= IN_SLOW; break;
            case 'i': case 'I': k |= IN_MORE; break;
            case 'o': case 'O': k |= IN_FEWER; break;
            case 'p': case 'P': k |= IN_PAL; break;
            case 'c': case 'C': k |= IN_CYCLE; break;
            case 'r': case 'R': k |= IN_RESET; break;
            case '?': case 'h': case 'H': k |= IN_HELP; break;
            case CH_STOP: k |= IN_STOP; break;                          /* RUN/STOP */
            default: k |= 0x8000; break;                             /* another key */
        }
    }
    return k;
}

/* the joysticks, as held directions (fire: zoom in) */
static unsigned int read_joysticks(void) {
    unsigned int k = 0;
    unsigned char j = input_joy(1) | input_joy(2);
    if (j & JOY_LEFT) k |= IN_LEFT;
    if (j & JOY_RIGHT) k |= IN_RIGHT;
    if (j & JOY_UP) k |= IN_UP;
    if (j & JOY_DOWN) k |= IN_DOWN;
    if (j & JOY_FIRE) k |= IN_ZIN;
    return k;
}

static void reset_view(void) {
    cx = -ONE / 2;                        /* -0.5 */
    cy = 0;
    zoom = 0;
    step = BASE_STEP;
    mb_maxit = 48;
}

/* act on this frame's input; returns 1 to quit */
static unsigned int prev_joy;
static unsigned char move_wait;
static unsigned char handle_input(void) {
    unsigned int joy = read_joysticks(), keys = read_keys();
    unsigned int pressed = (joy & ~prev_joy) | keys;   /* new joystick directions, key presses */
    unsigned int moves = 0;
    unsigned char changed = 0;
    prev_joy = joy;
    if (keys & IN_STOP) return 1;
    if (help_on) {                        /* any key goes back to the picture */
        if (pressed) { help_hide(); status(); }
        return 0;
    }
    if (pressed & IN_HELP) { help_show(); return 0; }
    /* moves: 4 cells (32 pixels) a key press (the keyboard repeats them) or,
     * on a joystick, every 8 frames while held; the rendered cells move
     * along and only the exposed edge renders */
    moves = keys & IN_MOVES;
    if (joy & IN_MOVES) {
        if (pressed & joy & IN_MOVES) move_wait = 0;
        if (move_wait-- == 0) { moves |= joy & IN_MOVES; move_wait = 8; }
    }
    if (moves) {
        {
            signed char dc = 0, dr = 0;
            if (moves & IN_LEFT) dc = -4;
            if (moves & IN_RIGHT) dc = 4;
            if (moves & IN_UP) dr = -4;
            if (moves & IN_DOWN) dr = 4;
            cx += (signed long)dc * 8 * step;
            cy += (signed long)dr * 8 * step;
            scroll_cells(dc, dr);
            status();
        }
    }
    if ((pressed & IN_ZIN) && step > MIN_STEP) { step >>= 1; zoom++; changed = 1; }
    if ((pressed & IN_ZOUT) && zoom > 0) { step <<= 1; zoom--; changed = 1; }
    if ((pressed & IN_MORE) && mb_maxit < 240) { mb_maxit += 16; changed = 1; }
    if ((pressed & IN_FEWER) && mb_maxit > 16) { mb_maxit -= 16; changed = 1; }
    if (pressed & IN_RESET) { reset_view(); changed = 1; }
    if (pressed & IN_PAL) { scheme = (unsigned char)((scheme + 1) % NSCHEMES); make_palette(); load_palette(); }
    if (pressed & IN_CYCLE) cycling ^= 1;
    if ((pressed & IN_FAST) && speed < NSPEEDS - 1) { speed++; cycling = 1; }
    if ((pressed & IN_SLOW) && speed > 0) speed--;
    if (changed) { restart(); status(); }
    return 0;
}

/* ------------------------------------------------------------------------ */
/* Video mode                                                               */

static unsigned char saved[16];
static unsigned char saved_pal[3][240];

static void video_on(void) {
    unsigned int i;
    saved[0] = REG(0xD031); saved[1] = REG(0xD054); saved[2] = REG(0xD058); saved[3] = REG(0xD059);
    saved[4] = REG(0xD05D); saved[5] = REG(0xD05E); saved[6] = REG(0xD060); saved[7] = REG(0xD061);
    saved[8] = REG(0xD062); saved[9] = REG(0xD063); saved[10] = REG(0xD064); saved[11] = REG(0xD065);
    saved[12] = REG(0xD020); saved[13] = REG(0xD021);
    for (i = 0; i < 240; i++) {
        saved_pal[0][i] = REG(0xD100 + 16 + i);
        saved_pal[1][i] = REG(0xD200 + 16 + i);
        saved_pal[2][i] = REG(0xD300 + 16 + i);
    }

    /* the cells: image cells point at their pixel blocks, the last row is text */
    for (i = 0; i < CW * CH; i++) screen[i] = (unsigned int)(PIX_LIN / 64 + i);
    dma_fill_far(PIX_LIN, CW * CH * 64, 0);
    dma_fill_far(COLOR_LIN, CW * 25 * 2, 0);
    for (i = 0; i < CW; i++) dma_fill_far(COLOR_LIN + (CH * CW + i) * 2 + 1, 1, 1);   /* white text */

    REG(0xD020) = 0;
    REG(0xD021) = 0;
    REG(0xD031) = REG(0xD031) & 0x7F;                 /* 40 columns (the hot registers set it up) */
    REG(0xD05D) = REG(0xD05D) & 0x7F;                 /* then hot registers off */
    REG(0xD054) = REG(0xD054) | 0x05;                 /* CHR16 + FCLRHI: characters > $FF are FCM */
    REG(0xD058) = CW * 2;                             /* bytes per row */
    REG(0xD059) = 0;
    REG(0xD05E) = CW;
    REG(0xD060) = (unsigned char)(unsigned int)screen;
    REG(0xD061) = (unsigned char)((unsigned int)screen >> 8);
    REG(0xD062) = 0;
    REG(0xD063) = REG(0xD063) & 0xF0;
    REG(0xD064) = 0;                                  /* colour RAM from its start */
    REG(0xD065) = 0;
}

static void video_off(void) {
    unsigned int i;
    for (i = 0; i < 240; i++) {
        REG(0xD100 + 16 + i) = saved_pal[0][i];
        REG(0xD200 + 16 + i) = saved_pal[1][i];
        REG(0xD300 + 16 + i) = saved_pal[2][i];
    }
    REG(0xD054) = saved[1];
    REG(0xD058) = saved[2]; REG(0xD059) = saved[3];
    REG(0xD05E) = saved[5];
    REG(0xD060) = saved[6]; REG(0xD061) = saved[7]; REG(0xD062) = saved[8]; REG(0xD063) = saved[9];
    REG(0xD064) = saved[10]; REG(0xD065) = saved[11];
    REG(0xD05D) = saved[4];
    REG(0xD031) = saved[0];
    REG(0xD020) = saved[12]; REG(0xD021) = saved[13];
    putchar(0x93);                                    /* clear the (80-column) screen */
}

/* ------------------------------------------------------------------------ */

int main(void) {
    unsigned char last_raster = 0;
    reset_view();
    make_palette();
    help_draw();
    video_on();
    input_flush();        /* no stale key presses */
    __asm__("sei");                       /* no KERNAL interrupt meanwhile (its screen editor */
    load_palette();                       /* would interfere) */
    status();
    restart();
    for (;;) {
        unsigned char r = REG(0xD012);
        if (r < last_raster) {                        /* a new frame */
            cycle_palette();
            if (handle_input()) break;
        }
        last_raster = r;
        if (busy && !help_on) {
            render_step();
            if (!busy) status();
        }
    }
    __asm__("cli");
    video_off();
    return 0;
}
