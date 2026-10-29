/* sprite_demo — "The Farmer and the Fish": a short, silly movie with the
 * VIC-IV's 8 hardware sprites (<sprites.h>) over a TGI picture (<tgi.h>)
 *
 * A lake ripples (its 16 water colours change every few frames: the picture
 * is drawn once), the sky follows the time of day (dawn, white light at
 * mid-morning and noon, sunset, dusk), the sun rises from behind the hills
 * and sets behind them with a silver glint on the water below it, a farmer
 * walks onto the dock, casts, a fish bites and is reeled in, the farmer
 * cheers and leaves, night falls: THE END. Then again, until RUN/STOP.
 *
 * The sprites take their colours from a palette bank of their own
 * (sprite_palette_bank), so the picture keeps its 256 colours:
 *   0 the rod and line  hi-res, SPRITE_TALL: the line's length is
 *                       sprite_height (cast out, reeled in)
 *   1 a cloud           SPRITE_WIDE, expanded, alpha-blended (on a MEGA65)
 *   2 the glint         16 colours, V400 rows (sprite_v400), twinkling by
 *                       its second colour set (sprite_altcolors)
 *   3 the farmer        16 colours, expanded: walking, casting, cheering
 *   4 the fish          16 colours: swimming, caught (sprite_overlap with
 *                       the line)
 *   5 high wisps        SPRITE_WIDE, repeated across the sky (sprite_tile:
 *                       a tiled sprite runs to the end of its raster line)
 *   6 the sun           16 colours, expanded; behind the hills by its image
 *                       clipped (clip_sun)
 *   7 a bird            hi-res, two frames
 * The images are ASCII pictures below, made into sprite data at the start.
 *
 * -DSPRITE_CHECK (make check): the movie once, then a report on the screen.
 */
#include <sprites.h>
#include <tgi.h>
#include <palette.h>
#include <input.h>
#include <stdio.h>
#include <string.h>

#define REG(a) (*(volatile unsigned char *)(a))

/* sprite numbers */
#define S_ROD    0
#define S_CLOUD  1
#define S_GLINT  2
#define S_FARMER 3
#define S_FISH   4
#define S_WISPS  5
#define S_BIRD   6
#define S_SUN    7      /* last: everything flies in front of it */

/* the picture's colours (the bank the screen shows) */
#define C_SKY    16     /* 16 entries, top to the horizon */
#define C_HILL   32     /* 4 */
#define C_GRASS  36
#define C_DIRT   37
#define C_PLANK  40
#define C_POST   41
#define C_WATER  48     /* 16 entries, cycled */
#define C_TEXT   250

#define HORIZON  96     /* the lake's top row */
#define MOVIE    2100   /* frames (42 seconds at 50 Hz) */
#define SUNSET   1750   /* the sun's day: rising behind the left hills, setting behind the right ones */

/* ------------------------------------------------------------------------ */
/* Sprite images: 256 bytes each, 64-byte aligned, in bank 0                 */

enum { I_ROD, I_CLOUD, I_GLINT_A, I_GLINT_B, I_WALK_A, I_WALK_B, I_CAST, I_CHEER,
       I_FISH_A, I_FISH_B, I_HOOKED_A, I_HOOKED_B, I_WISPS, I_SUN, I_BIRD_A, I_BIRD_B, NIMG };
static _Alignas(64) unsigned char img[NIMG][256];

/* 16-colour pictures: 16 pixels a row, a hex digit a pixel ('.' is 0) */
static void make16(unsigned char k, const char *const *rows, unsigned char n) {
    unsigned char r, x, c, ch;
    for (r = 0; r < n; r++)
        for (x = 0; x < 16; x++) {
            ch = (unsigned char)rows[r][x];
            c = ch >= 'a' ? ch - 'a' + 10 : ch >= '0' && ch <= '9' ? ch - '0' : 0;
            img[k][r * 8 + (x >> 1)] |= (x & 1) ? c : (unsigned char)(c << 4);
        }
}

/* hi-res pictures: '#' a set pixel, `bytes` bytes a row (3, or 8 wide) */
static void make_hi(unsigned char k, const char *const *rows, unsigned char n, unsigned char bytes) {
    unsigned char r, x;
    for (r = 0; r < n; r++)
        for (x = 0; x < bytes * 8 && rows[r][x]; x++)
            if (rows[r][x] == '#') img[k][r * bytes + (x >> 3)] |= (unsigned char)(0x80 >> (x & 7));
}

/* the farmer, facing left: 1 straw, 2 skin, 3 shirt, 4 overalls, 5 boots, 6 eye */
static const char *const walk_a[] = {
    "......1111......", ".....111111.....", "...1111111111...", ".....2222.......",
    "....62222.......", ".....2222.......", "......22........", ".....333333.....",
    "....33343333....", "...2.334433.2...", ".....344443.....", ".....444444.....",
    ".....444444.....", ".....444444.....", ".....44..44.....", "....44....44....",
    "...44......44...", "..555......555..", "..555......555..",
};
static const char *const walk_b[] = {
    "......1111......", ".....111111.....", "...1111111111...", ".....2222.......",
    "....62222.......", ".....2222.......", "......22........", ".....333333.....",
    "....33343333....", "....2334433.....", "....2344443.....", ".....444444.....",
    ".....444444.....", ".....444444.....", ".....44..44.....", ".....44..44.....",
    ".....44..44.....", "....555..555....", "....555..555....",
};
static const char *const cast[] = {
    "......1111......", ".....111111.....", "...1111111111...", ".....2222.......",
    "....62222.......", ".....2222.......", "......22........", ".....333333.....",
    "..22233343333...", ".....334433.....", ".....344443.....", ".....444444.....",
    ".....444444.....", ".....444444.....", ".....44..44.....", ".....44..44.....",
    ".....44..44.....", "....555..555....", "....555..555....",
};
static const char *const cheer[] = {
    "..2...1111......", "..2..111111.....", "..21111111111...", "..2..2222.......",
    "..3.62222.......", "..3..2222.......", "..33..22........", "...33333333.....",
    ".....333433332..", ".....334433.....", ".....344443.....", ".....444444.....",
    ".....444444.....", ".....444444.....", ".....44..44.....", "....44....44....",
    "...44......44...", "..555......555..", "..555......555..",
};
/* the fish: 1 body, 2 belly, 3 fins, 4 eye, 5 gill */
static const char *const fish_a[] = {
    "................", "................", "................", "................",
    "................", "................", ".......3........", "3...11111113....",
    "33.1111111111...", "33311122251141..", "3331222225111...", "33.1111111111...",
    "3...11111111....", ".......3........",
};
static const char *const fish_b[] = {
    "................", "................", "................", "................",
    "................", "................", ".......3........", "....11111113....",
    "3..1111111111...", "33311122251141..", "3331222225111...", "3..1111111111...",
    "....11111111....", ".......3........",
};
static const char *const hooked_a[] = {
    "......44........", ".....1111.......", "....111111......", "....141111......",
    "....155551......", "...31222213.....", "....122221......", "....122221......",
    "....111111......", ".....1111.......", "......11........", ".....3..3.......",
    "....33..33......", "...333..333.....",
};
static const char *const hooked_b[] = {
    "......44........", ".....1111.......", "....111111......", "....141111......",
    "....155551......", "...31222213.....", "....122221......", "....122221......",
    "....111111......", ".....1111.......", ".....11.........", "....3..3........",
    "...33..33.......", "..333..333......",
};
/* the glint (V400 rows: half as high): 1 silver, 2 grey, 3 white */
static const char *const glint_a[] = {
    "................", ".......3........", "....2213122.....", ".22111333111122.",
    "....2213122.....", ".......3........", "................", "..2211....1122..",
    "................", ".....21112......", "................", "...22.....22....",
};
static const char *const glint_b[] = {
    "................", "................", "......212.......", "..2211131112....",
    "......212.......", "................", ".22111....1112..", "................",
    "....2211122.....", "................", "....22....22....", "................",
};
/* the bird: two frames */
static const char *const bird_a[] = {
    "#......................#", "##....................##", ".##..................##.",
    "..##.......##.......##..", "...###...######...###...", ".....######..######.....",
};
static const char *const bird_b[] = {
    "........................", "........................", "........................",
    "...........##...........", ".....##################.", "..####.............####.",
};

/* the rod: up and to the left from the hand (row 11, column 22) to the tip
   (row 0, column 1), then the line straight down from the tip */
static void make_rod(void) {
    unsigned char r, x;
    for (r = 0; r < 12; r++) {
        x = (unsigned char)(1 + (r * 21) / 11);
        img[I_ROD][r * 3 + (x >> 3)] |= (unsigned char)(0x80 >> (x & 7));
        x++;
        img[I_ROD][r * 3 + (x >> 3)] |= (unsigned char)(0x80 >> (x & 7));
    }
    for (r = 12; r < 85; r++) img[I_ROD][r * 3] |= 0x40;      /* column 1 */
}

/* high wisps of cloud: thin streaks 64 wide, joining up when repeated */
static const unsigned char wisp_rows[][3] = {   /* row, first x, last x */
    {1, 6, 30}, {2, 0, 18}, {2, 40, 63}, {4, 22, 52}, {5, 30, 44}, {7, 48, 63}, {7, 0, 8}, {8, 52, 60},
};

static void make_wisps(void) {
    unsigned char i, x;
    for (i = 0; i < sizeof wisp_rows / sizeof wisp_rows[0]; i++)
        for (x = wisp_rows[i][1]; x <= wisp_rows[i][2]; x++)
            img[I_WISPS][wisp_rows[i][0] * 8 + (x >> 3)] |= (unsigned char)(0x80 >> (x & 7));
}

/* a cloud and the sun: discs */
static unsigned int d2(signed int dx, signed int dy) { return (unsigned int)(dx * dx + dy * dy); }

static void make_cloud(void) {
    unsigned char r, x;
    signed int sx, sy;
    for (r = 0; r < 21; r++)
        for (x = 0; x < 64; x++) {
            sx = (signed int)x; sy = (signed int)r;
            if (d2(sx - 20, sy - 11) < 70 || d2(sx - 34, sy - 8) < 90 || d2(sx - 46, sy - 12) < 60 ||
                (sy >= 12 && sy <= 17 && sx >= 12 && sx <= 54))
                img[I_CLOUD][r * 8 + (x >> 3)] |= (unsigned char)(0x80 >> (x & 7));
        }
}

/* the sun whole; img[I_SUN] is it less what the hills hide (clip_sun) */
static unsigned char sun_base[21 * 8];

static void make_sun(void) {
    unsigned char r, x, c;
    unsigned int d;
    for (r = 0; r < 21; r++)
        for (x = 0; x < 16; x++) {
            d = d2((signed int)(2 * x + 1) - 16, (signed int)(2 * r + 1) - 21);
            c = d < 60 ? 3 : d < 140 ? 1 : d < 190 ? 2 : 0;
            sun_base[r * 8 + (x >> 1)] |= (x & 1) ? c : (unsigned char)(c << 4);
        }
    memcpy(img[I_SUN], sun_base, sizeof sun_base);
}

static void make_images(void) {
    memset(img, 0, sizeof img);
    memset(sun_base, 0, sizeof sun_base);
    make_rod();
    make_cloud();
    make16(I_GLINT_A, glint_a, sizeof glint_a / sizeof glint_a[0]);
    make16(I_GLINT_B, glint_b, sizeof glint_b / sizeof glint_b[0]);
    make16(I_WALK_A, walk_a, sizeof walk_a / sizeof walk_a[0]);
    make16(I_WALK_B, walk_b, sizeof walk_b / sizeof walk_b[0]);
    make16(I_CAST, cast, sizeof cast / sizeof cast[0]);
    make16(I_CHEER, cheer, sizeof cheer / sizeof cheer[0]);
    make16(I_FISH_A, fish_a, sizeof fish_a / sizeof fish_a[0]);
    make16(I_FISH_B, fish_b, sizeof fish_b / sizeof fish_b[0]);
    make16(I_HOOKED_A, hooked_a, sizeof hooked_a / sizeof hooked_a[0]);
    make16(I_HOOKED_B, hooked_b, sizeof hooked_b / sizeof hooked_b[0]);
    make_wisps();
    make_sun();
    make_hi(I_BIRD_A, bird_a, sizeof bird_a / sizeof bird_a[0], 3);
    make_hi(I_BIRD_B, bird_b, sizeof bird_b / sizeof bird_b[0], 3);
}

/* the wisps one pixel to the right, around: each row's 64 bits rotated (a
   tiled sprite must start left of the screen to cover it, so the picture
   moves, not the sprite) */
static void drift_wisps(void) {
    unsigned char r, i, c, *row;
    for (r = 0; r < 9; r++) {
        row = img[I_WISPS] + r * 8;
        c = (unsigned char)(row[7] << 7);
        for (i = 0; i < 8; i++) {
            unsigned char b = row[i];
            row[i] = (unsigned char)((b >> 1) | c);
            c = (unsigned char)(b << 7);
        }
    }
}

static void show_image(unsigned char n, unsigned char k) {
    sprite_image(n, (unsigned long)(unsigned int)img[k]);
}

/* ------------------------------------------------------------------------ */
/* Colours                                                                   */

/* the sprites' bank: entries 0-15 the hi-res sprites' colours, n * 16 + c a
   16-colour sprite's, + 128 its second set */
static const unsigned char hi_rgb[16][3] = {
    {0, 0, 0}, {255, 255, 255}, {136, 0, 0}, {170, 255, 238}, {204, 68, 204}, {0, 204, 85},
    {0, 0, 170}, {238, 238, 119}, {221, 136, 85}, {102, 68, 0}, {255, 119, 119}, {51, 51, 51},
    {150, 150, 150}, {170, 255, 102}, {150, 190, 255}, {200, 200, 200},
};

static void set_rgb(unsigned char i, unsigned char r, unsigned char g, unsigned char b) {
    palette_set_color(i, r, g, b);
}

/* The picture's palette bank: the one the screen shows ($D070 bits 4-5; the
   KERNAL's is 3), mapped at $D100 (bits 6-7) for <palette.h>; the sprites
   take bank 1 (or 2, when the screen shows 1) */
static unsigned char d070, sprite_bank;

static void sprite_colours(void) {
    unsigned char i;
    d070 = REG(0xD070);
    sprite_bank = ((d070 >> 4) & 3) == 1 ? 2 : 1;
    REG(0xD070) = (d070 & 0x3F) | (unsigned char)(sprite_bank << 6);   /* the sprites' bank at $D100 */
    for (i = 0; i < 16; i++) set_rgb(i, hi_rgb[i][0], hi_rgb[i][1], hi_rgb[i][2]);
    /* the glint: silver, then bright */
    set_rgb(32 + 1, 192, 200, 210); set_rgb(32 + 2, 120, 130, 150); set_rgb(32 + 3, 240, 245, 255);
    set_rgb(160 + 1, 255, 255, 255); set_rgb(160 + 2, 200, 220, 255); set_rgb(160 + 3, 255, 255, 220);
    /* the farmer */
    set_rgb(48 + 1, 230, 200, 90); set_rgb(48 + 2, 240, 180, 140); set_rgb(48 + 3, 200, 40, 40);
    set_rgb(48 + 4, 50, 80, 180); set_rgb(48 + 5, 100, 60, 20); set_rgb(48 + 6, 20, 20, 20);
    /* the fish */
    set_rgb(64 + 1, 90, 160, 110); set_rgb(64 + 2, 220, 230, 200); set_rgb(64 + 3, 50, 110, 70);
    set_rgb(64 + 4, 10, 10, 10); set_rgb(64 + 5, 220, 60, 60);
    /* the sun */
    set_rgb(112 + 1, 255, 220, 40); set_rgb(112 + 2, 255, 150, 30); set_rgb(112 + 3, 255, 255, 190);
    /* the shown bank at $D100 again */
    REG(0xD070) = (d070 & 0x3F) | (unsigned char)((d070 << 2) & 0xC0);
}

/* The sky through the day: its top and horizon colours at frames of the
   movie (the sky between them graded); dawn violet and orange, pale and
   nearly white from mid-morning to noon, bluer in the afternoon, orange at
   sunset, then dusk */
static const struct { unsigned int f; unsigned char top[3], hor[3]; } sky_keys[] = {
    {0,    {50, 40, 100},   {255, 140, 80}},     /* dawn */
    {250,  {80, 100, 175},  {250, 195, 150}},    /* about 8 */
    {500,  {135, 175, 230}, {238, 242, 250}},    /* about 10: white light */
    {875,  {160, 195, 240}, {252, 252, 255}},    /* noon */
    {1250, {70, 120, 205},  {195, 222, 245}},    /* the afternoon */
    {1600, {70, 60, 135},   {255, 140, 80}},     /* sunset */
    {1800, {30, 22, 70},    {120, 50, 80}},      /* dusk */
    {2100, {18, 14, 45},    {60, 30, 55}},
};

static unsigned char mix(unsigned char a, unsigned char b, unsigned int w, unsigned int n) {
    return (unsigned char)((signed int)a + ((signed int)b - (signed int)a) * (signed int)w / (signed int)n);
}

/* the water (dark blues, one bright crest) */
static const unsigned char water_rgb[16][3] = {
    {20, 50, 120}, {22, 55, 126}, {25, 60, 132}, {28, 66, 140}, {32, 72, 148}, {36, 80, 156},
    {42, 90, 166}, {60, 110, 186}, {120, 170, 225}, {190, 220, 250}, {110, 160, 215}, {50, 100, 176},
    {38, 84, 160}, {32, 74, 150}, {27, 64, 138}, {23, 56, 128},
};

static unsigned char dark;      /* 0 (day) .. 16 (night) */

static unsigned char dim(unsigned char v, unsigned char tint) {
    return (unsigned char)(((unsigned int)v * (16 - dark) + (unsigned int)tint * dark) / 16);
}

/* the sky at frame f */
static void sky_colours(unsigned int f) {
    unsigned char k = 0, i, top[3], hor[3], c;
    unsigned int w, n;
    while (k < sizeof sky_keys / sizeof sky_keys[0] - 2 && f >= sky_keys[k + 1].f) k++;
    w = f - sky_keys[k].f;
    n = sky_keys[k + 1].f - sky_keys[k].f;
    if (w > n) w = n;
    for (c = 0; c < 3; c++) {
        top[c] = mix(sky_keys[k].top[c], sky_keys[k + 1].top[c], w, n);
        hor[c] = mix(sky_keys[k].hor[c], sky_keys[k + 1].hor[c], w, n);
    }
    for (i = 0; i < 16; i++)
        set_rgb(C_SKY + i, mix(top[0], hor[0], i, 15), mix(top[1], hor[1], i, 15), mix(top[2], hor[2], i, 15));
}

/* how dark the land and water are at frame f: dim at dawn, darker into the evening */
static unsigned char dark_at(unsigned int f) {
    unsigned int d;
    if (f < 450) return (unsigned char)((450 - f) * 8 / 450);
    if (f <= 1450) return 0;
    d = (f - 1450) * 12 / 400;
    return (unsigned char)(d > 12 ? 12 : d);
}

/* the water's colours, turned by `phase`: the crest moves through the rows */
static void water_colours(unsigned char phase) {
    unsigned char i, j;
    for (i = 0; i < 16; i++) {
        j = (unsigned char)((i + phase) & 15);
        set_rgb(C_WATER + i, dim(water_rgb[j][0], 10), dim(water_rgb[j][1], 15), dim(water_rgb[j][2], 40));
    }
}

/* ------------------------------------------------------------------------ */
/* The picture                                                               */

static void draw_sky(unsigned char from, unsigned char to) {
    unsigned char y;
    for (y = from; y < to; y++) {
        tgi_setcolor((unsigned char)(C_SKY + y / 6));
        tgi_line(0, y, 319, y);
    }
}

/* the hills' height at x: a few humps */
static signed int hill(signed int x) {
    signed int h = 0, xs;
    xs = x - 60;  if (xs > -70 && xs < 70) h = 18 - (xs * xs) / 280;
    xs = x - 170; if (xs > -60 && xs < 60 && 13 - (xs * xs) / 280 > h) h = 13 - (xs * xs) / 280;
    xs = x - 270; if (xs > -80 && xs < 80 && 22 - (xs * xs) / 300 > h) h = 22 - (xs * xs) / 300;
    return h;
}

/* the land's top row at each x (hills, the shore, else the horizon): what
   the sun goes behind */
static unsigned char land_y[320];

static void draw_scene(void) {
    signed int x, y, h, xs;
    unsigned char c;
    draw_sky(0, HORIZON);
    for (x = 0; x < 320; x += 2) {
        h = hill(x);
        land_y[x] = land_y[x + 1] = (unsigned char)(x >= 264 && h < 4 ? HORIZON - 4 : HORIZON - h);
        if (h > 0) {
            tgi_setcolor((unsigned char)(C_HILL + ((x / 2) & 1) + (h > 12 ? 2 : 0)));
            tgi_bar(x, HORIZON - h, x + 1, HORIZON - 1);
        }
    }
    /* the lake: streaks of the 16 water colours */
    for (y = HORIZON; y < 200; y++)
        for (x = 0; x < 320; x += 16) {
            c = (unsigned char)(((y - HORIZON) / 2 + ((x * 7 + y * 3) % 11 == 0 ? 1 : 0) + (x / 16 & 1)) & 15);
            tgi_setcolor((unsigned char)(C_WATER + c));
            tgi_bar(x, y, x + 15, y);
        }
    /* the shore on the right: grass, then earth */
    for (y = HORIZON - 4; y < 200; y++) {
        xs = 262 - (y - HORIZON) / 2;
        tgi_setcolor(y < HORIZON + 6 ? C_GRASS : C_DIRT);
        tgi_bar(xs, y, 319, y);
    }
    /* the dock */
    tgi_setcolor(C_POST);
    for (x = 182; x < 260; x += 24) tgi_bar(x, 121, x + 3, 146);
    tgi_setcolor(C_PLANK);
    tgi_bar(176, 117, 262, 121);
}

/* the cloud's and the wisps' colours (in the sprites' bank): greyer at
   dawn and dusk */
static void cloud_colours(void) {
    unsigned char now = REG(0xD070);    /* (the sprites' bank: sprite_palette_bank's) */
    REG(0xD070) = (now & 0x3F) | (unsigned char)(sprite_bank << 6);
    set_rgb(1, dim(255, 120), dim(255, 90), dim(255, 120));
    set_rgb(15, dim(200, 100), dim(200, 80), dim(200, 110));
    REG(0xD070) = now;
}

/* the hills, the shore and the dock (darker in the evening), the clouds */
static void picture_colours(void) {
    cloud_colours();
    set_rgb(C_HILL, dim(60, 15), dim(120, 20), dim(60, 30)); set_rgb(C_HILL + 1, dim(66, 15), dim(128, 20), dim(64, 30));
    set_rgb(C_HILL + 2, dim(44, 12), dim(100, 16), dim(50, 26)); set_rgb(C_HILL + 3, dim(50, 12), dim(108, 16), dim(54, 26));
    set_rgb(C_GRASS, dim(80, 20), dim(160, 30), dim(60, 30)); set_rgb(C_DIRT, dim(120, 30), dim(90, 22), dim(50, 25));
    set_rgb(C_PLANK, dim(150, 35), dim(100, 25), dim(50, 20)); set_rgb(C_POST, dim(90, 22), dim(60, 15), dim(30, 15));
    set_rgb(C_TEXT, 255, 240, 200);
}

/* ------------------------------------------------------------------------ */
/* The movie                                                                 */

/* The sun at (sx, sy) less what the land hides: a sprite behind the picture
   (sprite_behind) would be behind the whole sky too, every pixel of it
   drawn, so its pixels below the land's top are cleared (it is expanded: a
   sprite pixel is 2 x 2) */
static void clip_sun(signed int sx, signed int sy) {
    unsigned char r, c, b;
    signed int x, y, top[16];
    for (c = 0; c < 16; c++) {
        x = sx + 2 * c + 1;
        top[c] = x < 0 || x > 319 ? HORIZON : land_y[x];
    }
    for (r = 0; r < 21; r++) {
        y = sy + 2 * r + 1;
        for (c = 0; c < 16; c += 2) {
            b = sun_base[r * 8 + (c >> 1)];
            if (y >= top[c]) b &= 0x0F;
            if (y >= top[c + 1]) b &= 0xF0;
            img[I_SUN][r * 8 + (c >> 1)] = b;
        }
    }
}

/* a sprite at screen coordinates */
static void put(unsigned char n, signed int x, signed int y) {
    sprite_pos(n, (unsigned int)(x + SPRITE_X0), (unsigned int)(y + SPRITE_Y0));
}

static void wait_frame(void) {
    while (REG(0xD012) != 250) ;
    while (REG(0xD012) == 250) ;
}

static unsigned char caught, seen_bite;

/* the movie once: 1 when RUN/STOP was pressed */
static unsigned char movie(void) {
    unsigned int f;
    signed int fx = 340, sx, sy, gy, rod_x = 0, rod_y = 0, line = 12, fish_x = -40, fish_y = 160;
    signed int bird_x = -30, cloud_x = -60;
    unsigned char state = 0, frame_k = 0, t = 0, phase = 0, adj, d, low = 1;

    dark = dark_at(0);
    caught = 0;
    tgi_clear();
    picture_colours();
    sky_colours(0);
    water_colours(0);
    draw_scene();
    tgi_setcolor(C_TEXT);
    tgi_outtextxy(68, 6, "the farmer and the fish");

    sprite_init();
    sprite_palette_bank(sprite_bank);
    /* the rod and line */
    show_image(S_ROD, I_ROD);
    sprite_color(S_ROD, 12);
    sprite_size(S_ROD, SPRITE_TALL);
    sprite_height(12);
    /* the cloud: white, wide, twice as wide, see-through on a MEGA65 */
    show_image(S_CLOUD, I_CLOUD);
    sprite_color(S_CLOUD, 1);
    sprite_size(S_CLOUD, SPRITE_WIDE | SPRITE_EXPAND_X);
    sprite_alpha(S_CLOUD, 1);
    sprite_alpha_level(0xC0);
    /* the glint: 16 colours in V400 rows */
    show_image(S_GLINT, I_GLINT_A);
    sprite_mode(S_GLINT, SPRITE_16COLOR);
    sprite_size(S_GLINT, SPRITE_WIDE);
    sprite_v400(S_GLINT, 1);
    /* the farmer, the fish, the sun */
    show_image(S_FARMER, I_WALK_A);
    sprite_mode(S_FARMER, SPRITE_16COLOR);
    sprite_size(S_FARMER, SPRITE_WIDE | SPRITE_EXPAND_X | SPRITE_EXPAND_Y);
    show_image(S_FISH, I_FISH_A);
    sprite_mode(S_FISH, SPRITE_16COLOR);
    sprite_size(S_FISH, SPRITE_WIDE);
    show_image(S_SUN, I_SUN);
    sprite_mode(S_SUN, SPRITE_16COLOR);
    sprite_size(S_SUN, SPRITE_WIDE | SPRITE_EXPAND_X | SPRITE_EXPAND_Y);
    /* high wisps repeated across the sky (in front of the sun: a lower
       number), see-through on a MEGA65 */
    show_image(S_WISPS, I_WISPS);
    sprite_color(S_WISPS, 15);
    sprite_size(S_WISPS, SPRITE_WIDE);
    sprite_tile(S_WISPS, 1);
    sprite_alpha(S_WISPS, 1);
    /* the bird */
    show_image(S_BIRD, I_BIRD_A);
    sprite_color(S_BIRD, 11);

    put(S_FISH, fish_x, fish_y);
    put(S_FARMER, fx, 80);
    put(S_BIRD, bird_x, 30);
    put(S_CLOUD, cloud_x, 14);
    put(S_WISPS, -16, 40);                        /* (from the left border) */
    clip_sun(16, 100);
    sprite_show(S_SUN);
    sprite_show(S_CLOUD);
    sprite_show(S_WISPS);
    sprite_show(S_FARMER);
    sprite_show(S_FISH);
    adj = REG(0xD072);

    for (f = 0; f < MOVIE; f++) {
        wait_frame();
        if ((f & 3) == 0) water_colours(++phase);
        if (f == 150) draw_sky(0, 16);                      /* the title off */

        /* the time of day: the sky every 8 frames, the land when it darkens */
        if ((f & 7) == 0) {
            sky_colours(f);
            d = dark_at(f);
            if (d != dark) { dark = d; picture_colours(); }
        }

        /* the sun's arc, from behind the left hills to behind the right
           ones; its glint below it on the water while it is up */
        if (f <= SUNSET) {
            sx = 16 + (signed int)((unsigned long)f * 258 / SUNSET);
            sy = 100 - (signed int)((unsigned long)384 * f * (SUNSET - f) / ((unsigned long)SUNSET * SUNSET));
            put(S_SUN, sx, sy);
            if (sy + 42 > HORIZON - 23) {               /* near the land: clipped */
                if (f & 1) clip_sun(sx, sy);
                low = 1;
            } else if (low) {                           /* clear of it: whole */
                memcpy(img[I_SUN], sun_base, sizeof sun_base);
                low = 0;
            }
            if (sy + 42 < HORIZON - 6) sprite_show(S_GLINT);
            else sprite_hide(S_GLINT);
        }
        gy = HORIZON + 6 + (100 - sy) / 3;
        /* (V400 rows: y in physical lines) */
        sprite_pos(S_GLINT, (unsigned int)(sx + 8 + SPRITE_X0),
                   (unsigned int)(2 * (gy + SPRITE_Y0) - 2 * (adj - 2)));
        if ((f & 7) == 0) {
            t++;
            show_image(S_GLINT, (t & 2) ? I_GLINT_B : I_GLINT_A);
            sprite_altcolors(S_GLINT, t & 1);
        }

        /* the cloud and the wisps drift, a bird crosses */
        if ((f & 3) == 0) put(S_CLOUD, ++cloud_x, 14);
        if ((f & 7) == 0) drift_wisps();
        if (f >= 400 && f < 850) {
            if (f == 400) sprite_show(S_BIRD);
            bird_x += 1;
            put(S_BIRD, bird_x, 30 + ((f >> 4) & 3));
            if ((f & 7) == 0) show_image(S_BIRD, (f & 8) ? I_BIRD_B : I_BIRD_A);
            if (f == 849) sprite_hide(S_BIRD);
        }

        /* the fish swims to the right, slowly */
        if (!caught && (f & 3) == 0) {
            fish_x++;
            if ((f & 31) == 0) show_image(S_FISH, (f & 32) ? I_FISH_B : I_FISH_A);
            put(S_FISH, fish_x, fish_y);
        }

        switch (state) {
        case 0:     /* waiting */
            if (f == 150) state = 1;
            break;
        case 1:     /* the farmer walks onto the dock */
            if (f & 1) break;
            fx--;
            if ((f & 7) == 0) show_image(S_FARMER, (f & 8) ? I_WALK_B : I_WALK_A);
            put(S_FARMER, fx, 80);
            if (fx == 192) { show_image(S_FARMER, I_CAST); state = 2; }
            break;
        case 2:     /* the cast: the rod out, the line down */
            rod_x = fx - 16 + 6;
            rod_y = 80 + 16 - 11;
            put(S_ROD, rod_x, rod_y);
            sprite_show(S_ROD);
            if (line < 80 && f % 3 == 0) sprite_height((unsigned char)++line);
            /* a bite: the fish's box meets the line's */
            if (line >= 70 && sprite_overlap(S_FISH, S_ROD) && fish_x + 13 >= rod_x + 1) {
                seen_bite = 1;
                caught = 1;
                state = 3;
            }
            break;
        case 3:     /* reeled in: the line shorter, the fish on its end */
            if ((f & 7) == 0) show_image(S_FISH, (f & 8) ? I_HOOKED_B : I_HOOKED_A);
            put(S_ROD, rod_x + ((f >> 1) & 1), rod_y);
            if (line > 14 && (f & 1)) sprite_height((unsigned char)--line);
            fish_x = rod_x + 1 - 6;
            fish_y = rod_y + line - 1;
            put(S_FISH, fish_x, fish_y);
            if (line == 14) { show_image(S_FARMER, I_CHEER); state = 4; frame_k = 0; }
            break;
        case 4:     /* cheering */
            if (++frame_k == 100) { show_image(S_FARMER, I_CAST); state = 5; }
            break;
        case 5:     /* off to the right with the fish */
            if (f & 1) break;
            fx++;
            if ((f & 7) == 0) show_image(S_FARMER, (f & 8) ? I_WALK_B : I_WALK_A);
            put(S_FARMER, fx, 80);
            rod_x = fx - 10;
            put(S_ROD, rod_x, rod_y);
            put(S_FISH, rod_x + 1 - 6, fish_y);
            if (fx > 340) {
                sprite_hide(S_FARMER);
                sprite_hide(S_ROD);
                sprite_hide(S_FISH);
                state = 6;
            }
            break;
        }

        if (f == 1800) {                                /* night: the title */
            sprite_hide(S_WISPS);
            sprite_hide(S_CLOUD);
            tgi_setcolor(C_TEXT);
            tgi_settextscale(0x300, 0x300);
            tgi_outtextxy(76, 30, "the end");      /* 7 x 24 pixels, centred */
            tgi_settextscale(0x100, 0x100);
        }
#ifndef SPRITE_CHECK
        if (input_key() == CH_STOP) { sprite_done(); return 1; }
#endif
    }
    sprite_done();
    return 0;
}

int main(void) {
    unsigned char stop;
    make_images();
    tgi_install(tgi_static_stddrv);
    tgi_init();
    sprite_colours();
#ifdef SPRITE_CHECK
    stop = movie();
#else
    do stop = movie(); while (!stop);
#endif
    tgi_uninstall();
    REG(0xD070) = d070;
#ifdef SPRITE_CHECK
    if (!stop && caught && seen_bite && REG(0xD015) == 0)
        printf("sprite check ok, %u frames\n", (unsigned int)MOVIE);
    else
        printf("sprite check failed\n");
#endif
    return 0;
}
