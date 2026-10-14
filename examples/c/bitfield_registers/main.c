/* bitfield_registers — MEGA65 hardware register access via bitfield structs
 *
 * Maps VIC-III/VIC-IV control registers as bitfield structs for readable
 * hardware access without manual bit masking. Bit layouts per the MEGA65
 * Book (VIC-IV registers) and mega65-core's iomap.txt.
 *
 * Demonstrates: bitfields, volatile pointers, struct overlays on hardware
 * I/O, VIC-III extended attributes in colour RAM.
 */

#include <stdio.h>
#include <mega65.h>

/* VIC-III Control Register B ($D031) */
typedef struct {
    unsigned char intl  : 1;  /* bit 0: INT   interlaced mode */
    unsigned char mono  : 1;  /* bit 1: MONO  monochrome composite output */
    unsigned char h1280 : 1;  /* bit 2: H1280 1280 horizontal pixels */
    unsigned char v400  : 1;  /* bit 3: V400  400 vertical pixels */
    unsigned char bpm   : 1;  /* bit 4: BPM   bit-plane mode */
    unsigned char attr  : 1;  /* bit 5: ATTR  extended attributes, 8-bit colours */
    unsigned char fast  : 1;  /* bit 6: FAST  C65 fast mode (with VFAST: 40.5 MHz) */
    unsigned char h640  : 1;  /* bit 7: H640  640 pixels / 80 columns */
} vic3_ctrl_b;

/* VIC-IV Control Register C ($D054) */
typedef struct {
    unsigned char chr16  : 1; /* bit 0: CHR16  16-bit character numbers (2 screen bytes per char) */
    unsigned char fclrlo : 1; /* bit 1: FCLRLO full-colour mode for chars <= $FF */
    unsigned char fclrhi : 1; /* bit 2: FCLRHI full-colour mode for chars > $FF */
    unsigned char smth   : 1; /* bit 3: SMTH   horizontal smoothing */
    unsigned char sprh640: 1; /* bit 4: SPR_H640 sprite H640 */
    unsigned char palemu : 1; /* bit 5: PALEMU PAL CRT scan-line emulation */
    unsigned char vfast  : 1; /* bit 6: VFAST  MEGA65 fast mode (40.5 MHz) */
    unsigned char alphen : 1; /* bit 7: ALPHEN alpha compositor */
} vic4_ctrl_c;

/* VIC-II Sprite Enable ($D015) */
typedef struct {
    unsigned char spr0 : 1;
    unsigned char spr1 : 1;
    unsigned char spr2 : 1;
    unsigned char spr3 : 1;
    unsigned char spr4 : 1;
    unsigned char spr5 : 1;
    unsigned char spr6 : 1;
    unsigned char spr7 : 1;
} sprite_enable;

/* A colour RAM byte with VIC-III extended attributes ($D031 ATTR set):
 * the character's colour and its attributes. Screen RAM holds only the
 * character code (one byte per cell in the 80x25 text mode). */
typedef struct {
    unsigned char color : 4;  /* bits 0-3: foreground colour */
    unsigned char blink : 1;  /* bit 4: blink */
    unsigned char rvs   : 1;  /* bit 5: reverse */
    unsigned char bold  : 1;  /* bit 6: bold (with reverse: alternate palette) */
    unsigned char under : 1;  /* bit 7: underline */
} color_attr;

/* Map hardware registers to typed volatile pointers */
volatile vic3_ctrl_b   *VIC3_CTRL_B = (vic3_ctrl_b *)0xD031;
volatile vic4_ctrl_c   *VIC4_CTRL_C = (vic4_ctrl_c *)0xD054;
volatile sprite_enable *SPR_ENABLE  = (sprite_enable *)0xD015;

/* The 80x25 text screen: characters, and their colour/attribute bytes
 * (the first 2 KB of colour RAM appear at $D800 with CRAM2K, $D030 bit 0;
 * rows 0-12 are below $DC00 either way) */
volatile unsigned char *SCREEN = SCREEN_RAM;
volatile color_attr    *COLORS = (color_attr *)0xD800;

static void print_ctrl_b(void) {
    vic3_ctrl_b reg = *VIC3_CTRL_B;   /* one read of the hardware register */
    printf("vic-iii ctrl b ($d031):\n");
    printf("  h640=%d fast=%d attr=%d bpm=%d v400=%d\n",
           reg.h640, reg.fast, reg.attr, reg.bpm, reg.v400);
}

static void print_ctrl_c(void) {
    vic4_ctrl_c reg = *VIC4_CTRL_C;
    printf("vic-iv ctrl c ($d054):\n");
    printf("  vfast=%d chr16=%d fclrlo=%d palemu=%d\n",
           reg.vfast, reg.chr16, reg.fclrlo, reg.palemu);
}

int main(void) {
    /* crt0 has already enabled the VIC-IV registers ($47 $53 to $D02F) */
    putchar(0x93);   /* clear the screen */
    puts("bitfield register demo");
    puts("----------------------");

    /* Read and display current register state */
    print_ctrl_b();
    print_ctrl_c();

    /* 40.5 MHz: FAST ($D031 bit 6) and VFAST ($D054 bit 6) together
     * (BASIC 65 starts at 40 MHz; this shows the bitfield writes) */
    printf("\nsetting 40mhz...\n");
    VIC3_CTRL_B->fast = 1;
    VIC4_CTRL_C->vfast = 1;
    print_ctrl_c();

    /* Enable sprites 0 and 2 via bitfield */
    printf("\nenabling sprites 0,2...\n");
    SPR_ENABLE->spr0 = 1;
    SPR_ENABLE->spr2 = 1;
    printf("spr enable: $%x\n", *(volatile unsigned char *)0xD015);

    /* Styled characters: extended attributes on, then the character in
     * screen RAM and its colour + attributes in colour RAM (row 2, column 60) */
    printf("\nwriting styled chars...\n");
    VIC3_CTRL_B->attr = 1;
    SCREEN[2 * 80 + 60 + 0] = 1;          /* 'a' (screen code) */
    COLORS[2 * 80 + 60 + 0].color = 2;    /* red, bold (bold + reverse */
    COLORS[2 * 80 + 60 + 0].bold = 1;     /* would select the alternate palette) */
    COLORS[2 * 80 + 60 + 0].rvs = 0;
    COLORS[2 * 80 + 60 + 0].under = 0;

    SCREEN[2 * 80 + 60 + 1] = 2;          /* 'b' */
    COLORS[2 * 80 + 60 + 1].color = 5;    /* green */
    COLORS[2 * 80 + 60 + 1].rvs = 0;
    COLORS[2 * 80 + 60 + 1].bold = 0;
    COLORS[2 * 80 + 60 + 1].under = 1;

    SCREEN[2 * 80 + 60 + 2] = 3;          /* 'c' */
    COLORS[2 * 80 + 60 + 2].color = 7;    /* yellow */
    COLORS[2 * 80 + 60 + 2].rvs = 1;
    COLORS[2 * 80 + 60 + 2].bold = 0;
    COLORS[2 * 80 + 60 + 2].under = 0;

    puts("\ndone.");
    return 0;
}
