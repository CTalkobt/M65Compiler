/* far_scroller — a landscape map in far memory, scrolled with DMA
 *
 * The map (400 x 25 screen codes, 10000 bytes) is allocated with
 * far_malloc in bank 4 ($40000), outside the program's 64 KB. Each frame
 * the visible 80-column window is copied into screen RAM with one
 * dma_copy_far per row: the sky at half speed, the mountains and ground
 * at full speed (parallax). The colours go straight into colour RAM at
 * $FF80000 with dma_fill_far (all 2000 cells, beyond the $D800 window).
 * RUN/STOP exits.
 *
 * Demonstrates: far_ptr.h / far_ops.h, dma.h's 28-bit DMA, vsync timing.
 */
#include <stdio.h>
#include <cbm.h>
#include <dma.h>
#include <far_ptr.h>
#include <far_ops.h>

#define COLS    80
#define ROWS    25
#define MAPW    320              /* the scroll period (columns) */
#define STORE   (MAPW + COLS)    /* stored width: the first 80 columns repeat */
#define SKY_END 12               /* rows 1-11: sky (half speed) */
#define GROUND  22               /* rows 22-24: ground */

#define SCREEN_LIN 0x0000800UL   /* screen RAM */
#define COLOR_LIN  0xFF80000UL   /* colour RAM */

#define REG(a)  (*(volatile unsigned char *)(a))
#define BORDER  REG(0xD020)
#define BGCOLOR REG(0xD021)

static unsigned char row[STORE];
static unsigned long map_lin;    /* linear address of the map */

/* A triangle wave 0 .. p/2 with period p */
static unsigned char tri(unsigned int x, unsigned char p) {
    signed int d = (signed int)(x % p) - p / 2;
    return (unsigned char)(d < 0 ? -d : d);
}

/* Mountain height (rows) at column x: periodic in MAPW (80 and 32 divide
 * it); peaks up to 11 rows reach the sky rows (white: snow caps) */
static unsigned char height(unsigned int x) {
    return 1 + tri(x, 80) / 5 + tri(x, 32) / 8;
}

/* A repeatable pseudo-random byte for (x, r) */
static unsigned char hash(unsigned int x, unsigned char r) {
    unsigned int h = x * 31 + r * 97;
    h ^= h >> 5;
    h *= 73;
    return (unsigned char)(h ^ (h >> 8));
}

static void put_text(unsigned int x, const char *s) {
    while (*s) row[x++] = (unsigned char)*s++;
}

/* Build one map row in row[], then store it in far memory */
static void build_row(unsigned char r) {
    unsigned int x;
    for (x = 0; x < MAPW; x++) {
        unsigned char c = 0x20;                          /* space */
        if (r >= GROUND) {
            c = (r > GROUND && hash(x, r) < 40) ? 0xAE : 0xA0;   /* grass; soil with pebbles */
        } else if (r >= GROUND - height(x)) {
            c = 0xA0;                                    /* mountain (reverse space) */
        } else if (r < SKY_END) {
            unsigned char h = hash(x, r);
            c = h < 6 ? 0x2A : h < 14 ? 0x2E : 0x20;     /* stars */
        }
        row[x] = c;
    }
    if (r == 4) {
        put_text(12, s"this landscape lives in bank 4 ($40000)");
        put_text(172, s"24 dma copies into screen ram per frame");
    }
    if (r == 8) {
        put_text(90, s"far malloc and dma copy far");
        put_text(250, s"cc45 for the mega65");
    }
    for (x = MAPW; x < STORE; x++) row[x] = row[x - MAPW];   /* seamless wrap */
    far_memcpy_from_local(far_make((bank_t)(map_lin >> 16), (uint16_t)map_lin + (uint16_t)r * STORE), row, STORE);
}

/* The colour of each row: white sky, layered mountain greys/greens, ground */
static unsigned char row_color(unsigned char r) {
    static const unsigned char layers[] = {15, 15, 15, 12, 12, 12, 11, 11, 11, 11};
    if (r == 0) return 7;                 /* yellow title */
    if (r < SKY_END) return 1;            /* white stars and text */
    if (r < GROUND) return layers[r - SKY_END];
    return r == GROUND ? 5 : 9;           /* green grass, brown soil */
}

/* Wait for the start of the next frame (raster line 0) */
static void wait_vsync(void) {
    while (REG(0xD012) == 0) ;
    while (REG(0xD012) != 0 || (REG(0xD011) & 0x80)) ;
}

int main(void) {
    far_ptr_t map;
    unsigned char r, saved_border = BORDER, saved_bg = BGCOLOR;
    unsigned int x = 0, sky_x = 0;
    unsigned char frame = 0;

    map = far_malloc((size_t)STORE * ROWS, BANK4);
    if (far_is_null(map)) {
        puts("no far memory");
        return 1;
    }
    map_lin = ((unsigned long)far_bank(map) << 16) | far_offset(map);

    BORDER = 0;
    BGCOLOR = 0;
    putchar(0x93);                        /* clear the screen */
    for (r = 0; r < ROWS; r++) dma_fill_far(COLOR_LIN + (unsigned int)r * COLS, COLS, row_color(r));
    for (r = 1; r < ROWS; r++) build_row(r);

    /* the fixed title row */
    {
        static const char title[] = s"far scroller - map in bank 4, dma to screen ram - run/stop exits";
        dma_fill_far(SCREEN_LIN, COLS, 0x20);
        dma_copy_far(SCREEN_LIN + 8, (unsigned long)(unsigned int)title, sizeof title - 1);
    }

    while (!cbm_stop()) {
        wait_vsync();
        for (r = 1; r < ROWS; r++) {
            unsigned int from = r < SKY_END ? sky_x : x;
            dma_copy_far(SCREEN_LIN + (unsigned int)r * COLS, map_lin + (unsigned int)r * STORE + from, COLS);
        }
        if (++x == MAPW) x = 0;
        if (++frame & 1) { if (++sky_x == MAPW) sky_x = 0; }
    }

    far_free(map);
    BORDER = saved_border;
    BGCOLOR = saved_bg;
    putchar(0x05);                        /* white text */
    putchar(0x93);
    return 0;
}
