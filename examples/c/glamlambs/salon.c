/* salon.c -- Glam Lambs: the salon's room (a code overlay: its code is
 * in GLAMLAMBS.OV1, copied in to run) */
#pragma cc45 overlay(salon)
#include "glam.h"

/* ------------------------------------------------------------------------ */
/* The salon's room: GLAMSALON (characters) into bank 4, GLAMSALON2 (more,  */
/* its screen, the looks, the palette) into bank 5; without them, the panel */

#define SAL_SCREEN_LIN 0x51800UL      /* its screen: two bytes a cell (80 x 25: to $5279F) */
#define SAL_COLOUR 0x1000             /* its colour RAM (an offset) */
#define PALSAVE_LIN 0x5D000UL         /* palette entries 16-255 as they were */
#define SCRATCH (scr + COLS)          /* (rows 1-23 of the buffers: the room has its own screen) */

unsigned char salon_ok;        /* the files loaded */
seam_screen_t sal;
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
 * they were kept in bank 5); and back at the end */
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

void draw_salon(void) {
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
void salon_on(void) {
    dma_copy_far(seam_cell(&sal, 0, 1), SAL2_LIN + SAL_SCREEN, COLS * (ROWS - 2) * 2);
    dma_fill_far(seam_colour(&sal, 0, 1), COLS * (ROWS - 2) * 2, 0);
    sal_key = 0;
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

void salon_input(unsigned char pressed) {
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

