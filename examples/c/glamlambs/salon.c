/* salon.c -- Glam Lambs: the salon (a code overlay: its code is in
 * GLAMLAMBS.OV1, copied in to run): the styles and their menu, the room */
#pragma cc45 overlay(salon)
#include "glam.h"

/* ------------------------------------------------------------------------ */
/* The styles: the menu's rows, each a list of choices                      */

/* wool, lips, lashes, eyeshadow, blush, headwear, necklace, hooves; done */
static const char *const row_name[NROWS] = {
    s"wool", s"lips", s"lash", s"eyes", s"cheek", s"hat", s"neck", s"hoof", s"done"};
static const char *const shadow_name[4] = {s"none", s"blue", s"purple", s"gold"};
static const char *const blush_name[2] = {s"no", s"rosy"};
static const char *const hat_name[6] = {s"none", s"bow", s"dotty", s"flower", s"tiara", s"tophat"};
static const char *const neck_name[3] = {s"none", s"pearls", s"ribbon"};
static const char *const hoof_name[4] = {s"plain", s"red", s"pink", s"gold"};
static const unsigned char row_count[NROWS - 1] = {4, 4, 2, 4, 2, 6, 3, 4};

/* row r's choice for sheep i (the hat: 0 none, 1.. the kinds) */
static unsigned char style_get(unsigned char i, unsigned char r) {
    unsigned char l = s_look[i], st = s_style[i];
    switch (r) {
    case 0: return l & L_WOOL;
    case 1: return (l & L_LIPS) >> 2;
    case 2: return (l & L_LASH) ? 1 : 0;
    case 3: return (st & S_SHADOW) >> 3;
    case 4: return (st & S_BLUSH) ? 1 : 0;
    case 5: return (l & L_BOW) ? (unsigned char)((st & S_HAT) + 1) : 0;
    case 6: return st >> 6;
    case 7: return l >> 6;
    }
    return 0;
}

static void style_set(unsigned char i, unsigned char r, unsigned char v) {
    unsigned char *l = &s_look[i], *st = &s_style[i];
    switch (r) {
    case 0: *l = (unsigned char)((*l & ~L_WOOL) | v); break;
    case 1: *l = (unsigned char)((*l & ~L_LIPS) | v << 2); break;
    case 2: *l = (unsigned char)(v ? *l | L_LASH : *l & ~L_LASH); break;
    case 3: *st = (unsigned char)((*st & ~S_SHADOW) | v << 3); break;
    case 4: *st = (unsigned char)(v ? *st | S_BLUSH : *st & ~S_BLUSH); break;
    case 5:
        *l = (unsigned char)(v ? *l | L_BOW : *l & ~L_BOW);
        if (v) *st = (unsigned char)((*st & ~S_HAT) | (v - 1));
        break;
    case 6: *st = (unsigned char)((*st & ~S_NECK) | v << 6); break;
    case 7: *l = (unsigned char)((*l & ~L_HOOF) | v << 6); break;
    }
}

/* row r's choice's name */
static const char *style_name(unsigned char i, unsigned char r) {
    unsigned char v = style_get(i, r);
    switch (r) {
    case 0: return wool_name[v];
    case 1: return lips_name[v];
    case 2: return yesno_name[v];
    case 3: return shadow_name[v];
    case 4: return blush_name[v];
    case 5: return hat_name[v];
    case 6: return neck_name[v];
    case 7: return hoof_name[v];
    }
    return s"";
}

/* ------------------------------------------------------------------------ */
/* The salon's room: GLAMSALON (characters) into bank 4, GLAMSALON2 (more,  */
/* its screen, the looks, the palettes) into bank 5; without them, the panel */

#define SAL_SCREEN_LIN 0x51800UL      /* its screen: two bytes a cell (80 x 25: to $5279F) */
#define SAL_COLOUR 0x1000             /* its colour RAM (an offset) */
#define PALSAVE_LIN 0xFF83000UL       /* palette entries 16-255 as they were (in colour RAM) */
#define SCRATCH (scr + COLS)          /* (rows 1-23 of the buffers: the room has its own screen) */

unsigned char salon_ok;        /* the files loaded */
seam_screen_t sal;
static unsigned int sal_pal;          /* the look the palette shows (+ 1; 0: none yet) */
static unsigned int sal_key;          /* what the text shows */
static unsigned char blink_t, bulb_t, bulb_on;
#define NSPARK 4
static unsigned char spark_t[NSPARK], spark_kind;
static char sal_line[SAL_DOOR_W];

/* sparkles (column, row, the large one's character, the small one's) round
 * the wool, the lips and cheeks, the eyes, the head's top, the neck, the
 * hooves; and which a menu row's change celebrates */
static const unsigned int spark_tab[6][4 * NSPARK] = {
    {SAL_SPARK_WOOL}, {SAL_SPARK_LIPS}, {SAL_SPARK_EYE}, {SAL_SPARK_BOW}, {SAL_SPARK_NECK}, {SAL_SPARK_HOOF}};
static const unsigned char row_spark[NROWS - 1] = {0, 1, 2, 2, 1, 3, 4, 5};
/* the looks' runs of cells: column, row, width */
static const unsigned char eyes_runs[SAL_EYES_N * 3] = {SAL_EYES_RUNS};
static const unsigned char horns_runs[SAL_HORNS_N * 3] = {SAL_HORNS_RUNS};
static const unsigned char hat_runs[SAL_HAT_N * 3] = {SAL_HAT_RUNS};
static const unsigned char neck_runs[SAL_NECK_N * 3] = {SAL_NECK_RUNS};

/* GLAMSALON and GLAMSALON2 from the drive the game was loaded from (before
 * our screen: the KERNAL's disk code) */
void salon_load(void) {
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
 * they were kept); and back at the end */
void salon_setup(void) {
    seam_init(&sal, SAL_SCREEN_LIN, SAL_COLOUR, COLS, ROWS);
    palette_save(16, 240, SCRATCH);
    dma_copy_far(PALSAVE_LIN, (unsigned long)(unsigned int)SCRATCH, 720);
    sal_palette(SAL_PAL_FIRST, SAL_PAL_COUNT, SAL_PALETTE);
}

void salon_unsetup(void) {
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

/* sparkles round what the menu's row r changed, one after another */
static void sparkle(unsigned char r) {
    unsigned char i;
    if (sal.shown)
        for (i = 0; i < NSPARK; i++) spark_cell(i, 0);
    spark_kind = row_spark[r];
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

/* look k of a group of runs: their cells over the room */
static void sal_runs(const unsigned char *run, unsigned char n, unsigned int at, unsigned int size, unsigned char k) {
    unsigned long src = SAL2_LIN + at + (unsigned long)k * size;
    while (n--) {
        seam_cells(&sal, run[0], run[1], src, run[2]);
        src += (unsigned int)run[2] * 2;
        run += 3;
    }
}

/* the sign (the trend; the studio) and the doors: the menu in two columns
 * (rows 0-4, 5-8), the help */
static void sal_words(void) {
    unsigned char r;
    sal_text(SAL_SIGN_X, SAL_SIGN_Y, SAL_SIGN_W, 2, s"* glam salon *", C_YELLOW);
    if (studio) sal_text(SAL_SIGN_X, SAL_SIGN_Y + 1, SAL_SIGN_W, 2, s"styling studio", C_PINK);
    else {
        sal_text(SAL_SIGN_X, SAL_SIGN_Y + 1, SAL_SIGN_W, 0, s"trend: ", C_PINK);
        sal_fill(7, trend_name[trend]);
        seam_text(&sal, SAL_SIGN_X, SAL_SIGN_Y + 1, sal_line, SAL_SIGN_W, C_PINK);
    }
    sal_text(SAL_DOOR_X, SAL_DOOR_Y, SAL_DOOR_W, 2,
             (s_flags[carry] & F_RAM) ? s"a ram in the chair!" : s"a ewe in the chair!", C_LBLUE);
    for (r = 0; r < 5; r++) {
        unsigned char y = (unsigned char)(SAL_DOOR_Y + 1 + r), c;
        memset(sal_line, CH_SPACE, SAL_DOOR_W);
        sal_fill(1, row_name[r]);
        sal_fill(7, style_name(carry, r));
        if (r + 5 < NROWS) {
            sal_fill(15, row_name[r + 5]);
            if (r + 5 < NROWS - 1) sal_fill(21, style_name(carry, (unsigned char)(r + 5)));
        }
        if (sel == r) sal_line[0] = '>';
        if (sel == r + 5) sal_line[14] = '>';
        c = sel == r ? C_YELLOW : C_WHITE;               /* (each column its colour) */
        seam_text(&sal, SAL_DOOR_X, y, sal_line, 14, c);
        c = sel == r + 5 ? C_YELLOW : C_WHITE;
        seam_text(&sal, SAL_DOOR_X + 14, y, sal_line + 14, SAL_DOOR_W - 14, c);
    }
    sal_text(SAL_DOOR_X, SAL_DOOR_Y + 6, SAL_DOOR_W, 1, s"up/down choose, l/r change", C_LBLUE);
    sal_text(SAL_DOOR_X, SAL_DOOR_Y + 7, SAL_DOOR_W, 1, s"fire: let it go, darling", C_LBLUE);
}

void draw_salon(void) {
    unsigned char look = s_look[carry], style = s_style[carry], i;
    unsigned int pal = ((unsigned int)style << 8 | look) + 1;
    if (sal_pal != pal) {                /* the dye's, the lipstick's, the tones' shades */
        sal_palette(SAL_WOOL, 16, SAL_DYES + 48 * (look & L_WOOL));
        sal_palette(SAL_LIPS, 8, SAL_LIPSTICKS + 24 * ((look & L_LIPS) >> 2));
        sal_palette(SAL_SHADOW, SAL_TONE, SAL_SHADOWS + 3 * SAL_TONE * ((style & S_SHADOW) >> 3));
        sal_palette(SAL_BLUSH, SAL_TONE, SAL_BLUSHES + ((style & S_BLUSH) ? 3 * SAL_TONE : 0));
        sal_palette(SAL_HOOF, SAL_TONE, SAL_HOOVES + 3 * SAL_TONE * (look >> 6));
        sal_pal = pal;
    }
    if (++bulb_t >= 20) {                /* the bulbs take turns */
        bulb_t = 0;
        bulb_on ^= 1;
        sal_palette(SAL_BULBA, 8, SAL_BULBS + 24 * bulb_on);
    }
    if (blink_t) blink_t--;
    else blink_t = (unsigned char)(120 + rnd_n(120));
    /* the looks: the eyes, the horns, the headwear, the necklace */
    sal_runs(eyes_runs, SAL_EYES_N, SAL_EYES, SAL_EYES_LOOK,
             (unsigned char)(((look & L_LASH) ? 2 : 0) | (blink_t < 6 ? 1 : 0)));
    sal_runs(horns_runs, SAL_HORNS_N, SAL_HORNS, SAL_HORNS_LOOK, (s_flags[carry] & F_RAM) ? 1 : 0);
    sal_runs(hat_runs, SAL_HAT_N, SAL_HAT, SAL_HAT_LOOK, style_get(carry, 5));
    sal_runs(neck_runs, SAL_NECK_N, SAL_NECK, SAL_NECK_LOOK, (unsigned char)(style >> 6));
    for (i = 0; i < NSPARK; i++) {
        if (spark_t[i]) spark_t[i]--;
        spark_cell(i, spark_t[i] && spark_t[i] <= 20);
    }
    {
        unsigned int key = pal ^ ((unsigned int)sel << 3 | (unsigned int)trend << 7) * 0x0101u;
        if (key != sal_key) { sal_words(); sal_key = key; }
    }
    /* the score line and the messages */
    seam_row(&sal, 0, HUD_ROW, COLS, scr, clr);
    seam_row(&sal, 0, MSG_ROW, COLS, scr + MSG_ROW * COLS, clr + MSG_ROW * COLS);
}

/* into the room (drawn first) and out of it */
void salon_on(void) {
    dma_copy_far(seam_cell(&sal, 0, 1), SAL2_LIN + SAL_SCREEN, COLS * (ROWS - 2) * 2);
    dma_fill_far(seam_colour(&sal, 0, 1), COLS * (ROWS - 2) * 2, 0);
    sal_key = 0xFFFF;
    blink_t = 100;
    sparkle(0);
    draw_salon();
    seam_show(&sal);
    REG(0xD021) = SAL_GLASS;
}

void salon_off(void) {
    if (!sal.shown) return;
    seam_hide(&sal);
    REG(0xD021) = C_MEADOW;
}

/* the salon's menu: up / down a row, left / right a choice, fire: done */
void salon_input(unsigned char pressed) {
    if (pressed & K_UP) { sel = sel ? sel - 1 : NROWS - 1; snd_click(0x1800); panel_dirty = 1; }
    if (pressed & K_DOWN) { sel = sel < NROWS - 1 ? sel + 1 : 0; snd_click(0x1800); panel_dirty = 1; }
    if ((pressed & (K_LEFT | K_RIGHT)) && sel < NROWS - 1) {
        unsigned char n = row_count[sel], v = style_get(carry, sel);
        v = (unsigned char)((pressed & K_RIGHT) ? (v + 1 == n ? 0 : v + 1) : (v ? v - 1 : n - 1));
        style_set(carry, sel, v);
        snd_click(0x3000);
        panel_dirty = 1;
        if (sal.shown) sparkle(sel);
    }
    if ((pressed & K_FIRE) || ((pressed & (K_LEFT | K_RIGHT)) && sel == NROWS - 1)) leave_salon();
}

/* without the room's files: the menu on the panel beside the field */
void salon_panel(void) {
    unsigned char r;
    put_text(PANEL_IN + 1, 2, s"* glam salon *", C_YELLOW);
    draw_big(s_look[carry], s_flags[carry] & F_RAM);
    put_text(PANEL_IN + 5, 8, (s_flags[carry] & F_RAM) ? s"a ram" : s"a ewe", C_LBLUE);
    for (r = 0; r < NROWS; r++) {
        unsigned char c = (r == sel) ? C_YELLOW : C_WHITE, y = (unsigned char)(10 + r);
        if (r == sel) put_text(PANEL_IN, y, s">", C_YELLOW);
        put_text(PANEL_IN + 1, y, row_name[r], c);
        if (r < NROWS - 1) put_text(PANEL_IN + 7, y, style_name(carry, r), c);
    }
    put_text(PANEL_IN, 20, s"up/down, l/r", C_LBLUE);
    put_text(PANEL_IN, 21, s"fire: let it go", C_LBLUE);
    if (!studio) put_text(PANEL_IN, 22, trend_name[trend], C_PINK);
}
