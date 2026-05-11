/* bitfield_registers — MEGA65 hardware register access via bitfield structs
 *
 * Maps VIC-IV control registers as bitfield structs for clean,
 * readable hardware access without manual bit masking.
 *
 * Demonstrates: bitfields, volatile pointers, struct overlays on
 * hardware I/O, inline assembly for MEGA65 I/O unlock.
 */

#include <stdio.h>

/* VIC-IV Control Register A ($D031) */
typedef struct {
    unsigned char h640   : 1;  /* bit 0: 640px horizontal mode */
    unsigned char fast   : 1;  /* bit 1: fast (3.5MHz+) mode enable */
    unsigned char attr   : 1;  /* bit 2: extended attributes enable */
    unsigned char bpm    : 1;  /* bit 3: bitmap mode */
    unsigned char v400   : 1;  /* bit 4: 400 vertical mode */
    unsigned char chr16  : 1;  /* bit 5: 16-bit char set pointer */
    unsigned char mono   : 1;  /* bit 6: monochrome mode */
    unsigned char pal    : 1;  /* bit 7: PAL/NTSC half-line */
} vic_ctrl_a;

/* VIC-IV Control Register B ($D054) — Key register */
typedef struct {
    unsigned char fclksel : 2; /* bits 0-1: fast clock select (00=1MHz, 01=2MHz, 10=3.5MHz, 11=40MHz) */
    unsigned char vfast   : 1; /* bit 2: C65 2MHz mode */
    unsigned char side_b  : 1; /* bit 3: side border display */
    unsigned char sprhv   : 1; /* bit 4: sprite H640/V400 */
    unsigned char sprx64  : 1; /* bit 5: sprite 64-pixel wide */
    unsigned char rst_emu : 1; /* bit 6: reset to C64 on restore */
    unsigned char alpha   : 1; /* bit 7: alpha blend enable */
} vic_ctrl_b;

/* VIC-IV Sprite Enable ($D015) */
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

/* Screen RAM character + attribute */
typedef struct {
    unsigned char ch;        /* character code */
    unsigned char color : 4; /* bits 0-3: foreground color */
    unsigned char blink : 1; /* bit 4: blink */
    unsigned char under : 1; /* bit 5: underline */
    unsigned char bold  : 1; /* bit 6: bold */
    unsigned char rvs   : 1; /* bit 7: reverse */
} screen_char;

/* Map hardware registers to typed volatile pointers */
vic_ctrl_a    *VICIV_CTRL_A = 0xD031;
vic_ctrl_b    *VICIV_CTRL_B = 0xD054;
sprite_enable *SPR_ENABLE   = 0xD015;

/* Screen RAM (default location) */
screen_char *SCREEN = 0x0800;

static void unlock_mega65_io() {
    asm("lda #$47");
    asm("sta $d02f");
    asm("lda #$53");
    asm("sta $d02f");
}

static void print_ctrl_a() {
    vic_ctrl_a reg;
    /* Read the hardware register */
    reg = *VICIV_CTRL_A;

    printf("vic ctrl a ($d031):\n");
    printf("  h640=%d fast=%d attr=%d\n",
           reg.h640, reg.fast, reg.attr);
    printf("  bpm=%d v400=%d chr16=%d\n",
           reg.bpm, reg.v400, reg.chr16);
}

static void print_ctrl_b() {
    vic_ctrl_b reg;
    reg = *VICIV_CTRL_B;

    printf("vic ctrl b ($d054):\n");
    printf("  fclksel=%d vfast=%d\n",
           reg.fclksel, reg.vfast);
    printf("  alpha=%d rst_emu=%d\n",
           reg.alpha, reg.rst_emu);
}

int main() {
    unlock_mega65_io();

    puts("bitfield register demo");
    puts("----------------------");

    /* Read and display current register state */
    print_ctrl_a();
    print_ctrl_b();

    /* Set 40MHz mode via bitfield write */
    printf("\nsetting 40mhz...\n");
    VICIV_CTRL_B->fclksel = 3;  /* 11 = 40MHz */

    print_ctrl_b();

    /* Enable sprites 0 and 2 via bitfield */
    printf("\nenabling sprites 0,2...\n");
    SPR_ENABLE->spr0 = 1;
    SPR_ENABLE->spr2 = 1;
    printf("spr enable: $%x\n", *(unsigned char *)0xD015);

    /* Write styled character to screen using attribute bitfields */
    printf("\nwriting styled chars...\n");
    SCREEN[0].ch = 1;        /* 'a' in PETSCII */
    SCREEN[0].color = 2;     /* red */
    SCREEN[0].bold = 1;
    SCREEN[0].rvs = 1;

    SCREEN[1].ch = 2;        /* 'b' */
    SCREEN[1].color = 5;     /* green */
    SCREEN[1].under = 1;

    puts("\ndone.");
    return 0;
}
