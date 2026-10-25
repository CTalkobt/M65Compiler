/* title.c -- Glam Lambs: the title page (a code overlay: its code is in
 * GLAMLAMBS.OV1, copied in to run) */
#pragma cc45 overlay(title)
#define GFX_DATA                      /* (gfx.h's glyphs: here, for setup) */
#include "glam.h"

static unsigned char pick;            /* the title's choice: 0 a season, 1 the studio */

static void set_palette(void);
static void make_background(void);
static unsigned char swap_nib(unsigned char v);
static void box_line(unsigned char y, const char *s, unsigned char c);
static void box_number(unsigned char y, const char *s, unsigned int v);

/* ------------------------------------------------------------------------ */
/* The title page: a block-letter logo, how to play, a parade of sheep      */

/* letters 5 high, 3 or 5 wide; a pixel is two cells (a solid block) */
static const char logo_text[] = "glam lambs";
static const char logo_letters[] = "glambs";
static const unsigned char logo_w[6] = {3, 3, 3, 5, 3, 3};
static const unsigned char logo_font[6][5] = {
    {7, 4, 5, 5, 7},                  /* g */
    {4, 4, 4, 4, 7},                  /* l */
    {2, 5, 7, 5, 5},                  /* a */
    {17, 27, 21, 17, 17},             /* m */
    {6, 5, 6, 5, 6},                  /* b */
    {7, 4, 7, 1, 7}};                 /* s */
static const unsigned char logo_color[4] = {C_PINK, C_YELLOW, C_WHITE, C_LBLUE};

#define LOGO_Y 0
#define LOGO_SPACE 4                  /* cells for a space */
#define PARADE_Y 21
#define NPARADE 4

/* the title's text: two columns of headings (a leading '#') and bullet
 * points (a leading '*'); plain ASCII, mixed case */
static const char *const title_left[] = {
    "#How to play",
    "",
    "*Catch a sheep: walk up, press fire",
    "*Carry it to the salon door",
    " in the right-hand fence",
    "*Style it: wool, lipstick, lashes,",
    " eyeshadow, hats, pearls and more",
    "*Let it go: its lambs inherit",
    " the look",
    "*Farmer Fergus shears full fleeces",
    " (the makeup goes too).",
    " Fire beside him: shoo!"};
static const char *const title_right[] = {
    "#Scoring",
    "",
    "*Glam lamb: 5 a trait x generation",
    "*On the trend: double points",
    "*All four traits: +50",
    "*Wool sold: 10 a bag, dyed 25",
    "#Styling studio",
    "*No clock, no score: style any",
    " sheep you like, as long as you like",
    "#Controls",
    "*Joystick, cursor keys or WASD",
    "*Fire, Space or Return"};

static void title_line(unsigned char x, unsigned char y, const char *s) {
    if (*s == '#') put_ascii(x, y, s + 1, C_YELLOW);
    else if (*s == '*') {
        put(x, y, G_BULLET, C_PINK);
        put_ascii((unsigned char)(x + 2), y, s + 1, C_WHITE);
    } else if (*s) put_ascii((unsigned char)(x + 2), y, s + 1, C_WHITE);
}

static unsigned char logo_index(char ch) {
    unsigned char f = 0;
    while (logo_letters[f] != ch) f++;
    return f;
}

static void draw_logo(void) {
    unsigned char k, r, b, w = 0, x;
    for (k = 0; logo_text[k]; k++)    /* the width, to centre it */
        w += logo_text[k] == ' ' ? LOGO_SPACE : logo_w[logo_index(logo_text[k])] * 2 + 1;
    x = (unsigned char)((COLS - w + 1) / 2);
    for (k = 0; logo_text[k]; k++) {
        unsigned char f, lw, c = logo_color[k & 3];
        if (logo_text[k] == ' ') { x += LOGO_SPACE; continue; }
        f = logo_index(logo_text[k]);
        lw = logo_w[f];
        for (r = 0; r < 5; r++)
            for (b = 0; b < lw; b++)
                if (logo_font[f][r] & (1 << (lw - 1 - b))) {
                    put((unsigned char)(x + b * 2), (unsigned char)(LOGO_Y + r), G_BLOCK, c);
                    put((unsigned char)(x + b * 2 + 1), (unsigned char)(LOGO_Y + r), G_BLOCK, c);
                }
        x += lw * 2 + 1;
    }
}

static void draw_title_page(void) {
    unsigned char i;
    memset(scr, CH_SPACE, COLS * ROWS);
    memset(clr, C_WHITE, sizeof clr);
    draw_logo();
    put_ascii(22, 6, "A sheep makeover game for the MEGA65", C_LBLUE);
    put_ascii(31, 7, "by Kimberly Taylor", C_PINK);
    for (i = 0; i < 12; i++) {
        title_line(3, (unsigned char)(9 + i), title_left[i]);
        title_line(43, (unsigned char)(9 + i), title_right[i]);
    }
    for (i = 0; i < COLS; i++) {
        put(i, 8, G_FENCE_H, C_BROWN);
        put(i, PARADE_Y + 2, G_FENCE_H, C_BROWN);
    }
}

/* once a frame: the parade walks right, the prompt blinks */
static unsigned char parade_t;

void title_frame(void) {
    unsigned char i;
    clear_span(0, PARADE_Y, COLS);
    clear_span(0, PARADE_Y + 1, COLS);
    if (++parade_t >= 10) {
        parade_t = 0;
        for (i = 0; i < NPARADE; i++) {
            s_x[i] = s_x[i] >= COLS - SW ? 0 : s_x[i] + 1;
            s_flags[i] ^= F_STEP;
        }
    }
    for (i = 0; i < NPARADE; i++) draw_sheep_at(s_x[i], PARADE_Y, i);
    /* the choice: a season, or the styling studio (the chosen one blinking) */
    clear_span(0, MSG_ROW, COLS);
    if (!pick || (FRAMECOUNT & 0x20)) put_ascii(4, MSG_ROW, "Play a season", pick ? C_LBLUE : C_YELLOW);
    if (pick || (FRAMECOUNT & 0x20)) put_ascii(22, MSG_ROW, "Styling studio", pick ? C_YELLOW : C_LBLUE);
    put(pick ? 20 : 2, MSG_ROW, '>', C_YELLOW);
    put_ascii(41, MSG_ROW, "<> pick  Fire: go  M: music  Stop: quit", C_WHITE);
}

/* the title's input: a direction (or S) picks, fire goes */
void title_input(unsigned char pressed) {
    if (pressed & (K_LEFT | K_RIGHT | K_UP | K_DOWN)) {
        pick ^= 1;
        snd_click(0x1800);
    }
    if (input_pressed(KEY_S)) pick = 1;
    if (pressed & K_FIRE) {
        if (pick) start_studio();
        else start_season();
    }
}

void title(void) {
    static const unsigned char parade_look[NPARADE] = {
        0x01 | L_BOW | L_LASH, 0x02 | 0x04, 0x03 | 0x08 | L_LASH, 0x01 | 0x0C | L_BOW};
    unsigned char i;
    salon_off();
    mode = M_TITLE;
    studio = 0;
#ifdef DEMO_STUDIO
    pick = 1;                         /* (tests: the demo plays the studio) */
#endif
    attract = 0;
    season = 0;
    idle_t = 0;
    carry = 0xFF;
    p_x = 0; p_y = 0;
    sh_on = 0;
    trend = rnd_n(NTRENDS);
    music_restart();
    music_for(0);
    clear_flock();
    set_charset(CHARSET2_LIN);
    for (i = 0; i < NPARADE; i++) {   /* the parade: sheep slots, never in the field */
        s_flags[i] = (unsigned char)(F_ALIVE | F_CARRIED | ((i & 1) ? F_RAM : 0));
        s_x[i] = (unsigned char)(i * 20);
        s_look[i] = parade_look[i];
        s_age[i] = ADULT_AGE;
        s_wool[i] = WOOL_FULL;
    }
    draw_title_page();
}



void setup(void) {
    unsigned char i;
    saved_regs[0] = REG(0xD068); saved_regs[1] = REG(0xD069); saved_regs[2] = REG(0xD06A);
    saved_regs[3] = REG(0xD020); saved_regs[4] = REG(0xD021);
    saved_regs[5] = REG(0xD022); saved_regs[6] = REG(0xD023); saved_regs[7] = REG(0xD016);
    saved_regs[8] = REG(0xD05D);
    for (i = 0; i < 4; i++) saved_regs[9 + i] = REG(0xD060 + i);
    /* the screen shown: bank 5 (built at $0800) */
    dma_fill_far(SHOWN_LIN, COLS * ROWS, CH_SPACE);
    REG(0xD060) = (unsigned char)SHOWN_LIN;
    REG(0xD061) = (unsigned char)(SHOWN_LIN >> 8);
    REG(0xD062) = (unsigned char)(SHOWN_LIN >> 16);
    REG(0xD063) = REG(0xD063) & 0xF0;
    /* the character sets: the ROM's (upper case and graphics; upper and
     * lower case for the title) copied to bank 4, our glyphs over CH_BASE.. */
    dma_copy_far(CHARSET_LIN, CHARROM_LIN, 2048);
    dma_copy_far(CHARSET_LIN + CH_BASE * 8, (unsigned long)(unsigned int)glyph_data, sizeof glyph_data);
    dma_copy_far(CHARSET2_LIN, CHARROM_LIN + 2048, 2048);
    dma_copy_far(CHARSET2_LIN + CH_BASE * 8, (unsigned long)(unsigned int)glyph_data, sizeof glyph_data);
    set_charset(CHARSET_LIN);
    set_palette();
    REG(0xD020) = C_DGREY;
    REG(0xD021) = C_MEADOW;
    REG(0xD022) = C_DGREY;
    REG(0xD023) = C_TUFT;
    fps = (REG(0xD06F) & 0x80) ? 60 : 50;
    memset(scr, CH_SPACE, COLS * ROWS);
    memset(clr, C_WHITE, sizeof clr);
    show();
    /* multicolour characters; with the VIC-IV's hot registers off, or
     * writing $D016 would recompute (and lose) the 80-column layout */
    REG(0xD05D) = REG(0xD05D) & 0x7F;
    REG(0xD016) = REG(0xD016) | 0x10;
    rnd_state ^= (unsigned int)FRAMECOUNT << 8 | REG(0xD012);
    if (!rnd_state) rnd_state = 0xACE1;
    make_background();
    sid_init();
}


static void make_background(void) {
    unsigned char x, y;
    for (y = 0; y < BGH; y++)
        for (x = 0; x < BGW; x++) {
            unsigned int o = (unsigned int)(y + FENCE_T) * COLS + x;   /* (made in the screen's buffers) */
            unsigned char ch = CH_SPACE, c = C_WHITE, row = (unsigned char)(y + FENCE_T);
            if (row == FENCE_T || row == FENCE_B) {
                ch = (x == FENCE_L || x == FENCE_R) ? G_POST : G_FENCE_H;
                c = C_BROWN;
            } else if (x == FENCE_L || x == FENCE_R) {
                if (x == FENCE_R && row >= GATE_Y0 && row <= GATE_Y1) { ch = G_GATE; c = C_YELLOW; }
                else if (x == FENCE_R && (row == GATE_Y0 - 1 || row == GATE_Y1 + 1)) { ch = G_POST; c = C_BROWN; }
                else { ch = G_FENCE_V; c = C_BROWN; }
            } else {
                unsigned char r = rnd_n(32);
                if (r < 2) { ch = G_GRASS1; c = MCOL; }
                else if (r < 4) { ch = G_GRASS2; c = MCOL; }
                else if (r == 4) { ch = G_FLOWER; c = (rnd() & 1) ? C_YELLOW : C_WHITE; }
            }
            scr[o] = ch;
            clr[o] = c;
        }
    for (y = 0; y < BGH; y++) {
        unsigned int o = (unsigned int)(y + FENCE_T) * COLS;
        dma_copy_far(BGK_LIN + (unsigned int)y * BGW, (unsigned long)(unsigned int)(scr + o), BGW);
        dma_copy_far(BGC_LIN + (unsigned int)y * BGW, (unsigned long)(unsigned int)(clr + o), BGW);
    }
    memset(scr, CH_SPACE, COLS * ROWS);
}


static void set_palette(void) {
    unsigned char i, k = 0;
    for (i = 0; i < 16; i++) {
        saved_pal[i] = PAL_R(i); saved_pal[16 + i] = PAL_G(i); saved_pal[32 + i] = PAL_B(i);
    }
    for (i = 0; i < 12; i++, k += 3) {
        unsigned char n = pal_idx[i];
        PAL_R(n) = swap_nib(pal_rgb[k]);
        PAL_G(n) = swap_nib(pal_rgb[k + 1]);
        PAL_B(n) = swap_nib(pal_rgb[k + 2]);
    }
}


static void restore_palette(void) {
    unsigned char i;
    for (i = 0; i < 16; i++) {
        PAL_R(i) = saved_pal[i]; PAL_G(i) = saved_pal[16 + i]; PAL_B(i) = saved_pal[32 + i];
    }
}


static unsigned char swap_nib(unsigned char v) {
    return (unsigned char)(v << 4 | v >> 4);
}


void restore(void) {
    unsigned char i;
    sid_init();
    REG(0xD016) = saved_regs[7];
    REG(0xD05D) = saved_regs[8];
    REG(0xD068) = saved_regs[0]; REG(0xD069) = saved_regs[1]; REG(0xD06A) = saved_regs[2];
    REG(0xD020) = saved_regs[3]; REG(0xD021) = saved_regs[4];
    REG(0xD022) = saved_regs[5]; REG(0xD023) = saved_regs[6];
    for (i = 0; i < 4; i++) REG(0xD060 + i) = saved_regs[9 + i];
    restore_palette();
    dma_fill_far(SCREEN_LIN, COLS * ROWS, CH_SPACE);
    dma_fill_far(COLOR_LIN, COLS * ROWS, C_WHITE);
}


void draw_over_box(void) {
    fence_box(BOX_X, BOX_Y, BOX_X + BOX_W - 1, BOX_Y + BOX_H - 1);
    put_text(BOX_X + 10, BOX_Y + 2, s"season", C_YELLOW);
    put_number(BOX_X + 17, BOX_Y + 2, season, season < 10 ? 1 : 2, C_YELLOW);
    put_text(season < 10 ? BOX_X + 19 : BOX_X + 20, BOX_Y + 2, s"is over!", C_YELLOW);
    box_number(BOX_Y + 4, s"lambs born", lambs);
    box_number(BOX_Y + 5, s"glam lambs", glam_lambs);
    box_number(BOX_Y + 6, s"lamb points", lamb_pts);
    box_number(BOX_Y + 7, s"wool bags", wool_bags);
    box_number(BOX_Y + 8, s"wool income", income);
    box_number(BOX_Y + 9, s"score", score);
    box_number(BOX_Y + 10, s"high score", hiscore);
    if (score > hiscore_was) box_line(BOX_Y + 11, s"a new high score!", C_PINK);
    box_line(BOX_Y + 13, s"your flock goes to market.", C_LBLUE);
    box_line(BOX_Y + 14, s"next season starts afresh", C_LBLUE);
    box_line(BOX_Y + 15, s"with a plain, unstyled flock.", C_LBLUE);
    if (!over_t) box_line(BOX_Y + 17, s"return or fire to go on", C_WHITE);
}


static void box_line(unsigned char y, const char *s, unsigned char c) {
    put_text((unsigned char)(BOX_X + (BOX_W - strlen(s)) / 2), y, s, c);
}


static void box_number(unsigned char y, const char *s, unsigned int v) {
    put_text(BOX_X + 8, y, s, C_WHITE);
    put_number(BOX_X + BOX_W - 13, y, v, 5, C_YELLOW);
}
