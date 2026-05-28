/* mega65.h — MEGA65 hardware register mappings for cc45
 *
 * Provides struct overlays for memory-mapped I/O registers.
 * All fields are volatile to prevent dead-store elimination.
 *
 * Usage:
 *   #include <mega65.h>
 *   vic4->border = 0;          // black border
 *   vic4->bg0 = 6;             // blue background
 *   vic4->sprite_enable = 0x01; // enable sprite 0
 *   vic4->ctrl_b |= VIC4_FCM;  // enable full-colour mode
 */

#pragma once

/* ===== VIC-IV Registers ($D000-$D07F) ===== */

struct vic4_sprite_pos {
    unsigned char x;            /* X position (low 8 bits) */
    unsigned char y;            /* Y position */
};

struct vic4_regs {
    /* --- VIC-II compatible ($D000-$D02E) --- */
    struct vic4_sprite_pos sprite[8]; /* $D000: sprite positions */
    unsigned char sprite_x_msb;    /* $D010: sprite X position bit 8 */
    unsigned char ctrl1;           /* $D011: RST8|ECM|BMM|DEN|RSEL|YSCROLL */
    unsigned char raster;          /* $D012: raster counter (low 8 bits) */
    unsigned char lightpen_x;      /* $D013: light pen X */
    unsigned char lightpen_y;      /* $D014: light pen Y */
    unsigned char sprite_enable;   /* $D015: sprite enable bits */
    unsigned char ctrl2;           /* $D016: RES|MCM|CSEL|XSCROLL */
    unsigned char sprite_y_expand; /* $D017: sprite Y expansion */
    unsigned char memptr;          /* $D018: screen/char memory pointers */
    unsigned char irq_status;      /* $D019: interrupt status (write to ack) */
    unsigned char irq_enable;      /* $D01A: interrupt enable mask */
    unsigned char sprite_priority; /* $D01B: sprite-to-bg priority */
    unsigned char sprite_multicolor; /* $D01C: sprite multicolor enable */
    unsigned char sprite_x_expand; /* $D01D: sprite X expansion */
    unsigned char sprite_coll;     /* $D01E: sprite-sprite collision (read clears) */
    unsigned char sprite_bg_coll;  /* $D01F: sprite-bg collision (read clears) */
    unsigned char border;          /* $D020: border colour */
    unsigned char bg0;             /* $D021: background colour 0 */
    unsigned char bg1;             /* $D022: background colour 1 */
    unsigned char bg2;             /* $D023: background colour 2 */
    unsigned char bg3;             /* $D024: background colour 3 */
    unsigned char sprite_mcol0;    /* $D025: sprite multicolour 0 */
    unsigned char sprite_mcol1;    /* $D026: sprite multicolour 1 */
    unsigned char sprite_color[8]; /* $D027: sprite colours 0-7 */

    /* --- VIC-III registers ($D02F-$D03F) --- */
    unsigned char key;             /* $D02F: VIC-III key (write $A5 then $96) */
    unsigned char ctrl_a;          /* $D030: VIC-III control A */
    unsigned char ctrl_b;          /* $D031: VIC-III/IV control B */
    unsigned char bitplane_enable; /* $D032: bitplane enable */
    unsigned char bitplane_addr[8]; /* $D033: bitplane 0-7 addresses */
    unsigned char bitplane_comp;   /* $D03B: bitplane companion bits */
    unsigned char bitplane_x;      /* $D03C: bitplane X offset */
    unsigned char bitplane_y;      /* $D03D: bitplane Y offset */
    unsigned char _reserved1[2];   /* $D03E-$D03F */

    /* --- VIC-IV extended registers ($D040-$D07F) --- */
    unsigned char screen_x_lo;     /* $D040: screen X position low */
    unsigned char screen_x_hi;     /* $D041: screen X position high */
    unsigned char screen_y_lo;     /* $D042: screen Y position low */
    unsigned char screen_y_hi;     /* $D043: screen Y position high */
    unsigned char xpos_lo;         /* $D044: physical raster X (read) low */
    unsigned char xpos_hi;         /* $D045: physical raster X (read) high */
    unsigned char raster_hi;       /* $D046: raster Y high (extended) */
    unsigned char _reserved2;      /* $D047 */
    unsigned char border_left_lo;  /* $D048: left border position low */
    unsigned char border_left_hi;  /* $D049: left border position high */
    unsigned char border_right_lo; /* $D04A: right border position low */
    unsigned char border_right_hi; /* $D04B: right border position high */
    unsigned char border_top_lo;   /* $D04C: top border position low */
    unsigned char border_top_hi;   /* $D04D: top border position high */
    unsigned char border_bot_lo;   /* $D04E: bottom border position low */
    unsigned char border_bot_hi;   /* $D04F: bottom border position high */
    unsigned char charstep_lo;     /* $D050: character generator step low */
    unsigned char charstep_hi;     /* $D051: character generator step high */
    unsigned char chrwidth;        /* $D052: character width (pixels - 1) */
    unsigned char chrheight;       /* $D053: character height (pixels - 1) */
    unsigned char ctrl_c;          /* $D054: VIC-IV control C */
    unsigned char sprite_extheight_en; /* $D055: sprite extended height enable */
    unsigned char sprite_extheight;    /* $D056: sprite extended height size */
    unsigned char sprite_extwidth;     /* $D057: sprite extended width mode */
    unsigned char screen_addr_lo;  /* $D058: screen RAM base low */
    unsigned char screen_addr_mid; /* $D059: screen RAM base mid */
    unsigned char screen_addr_hi;  /* $D05A: screen RAM base high */
    unsigned char screen_addr_bank; /* $D05B: screen RAM bank/flags */
    unsigned char colour_addr_lo;  /* $D05C: colour RAM base low */
    unsigned char colour_addr_hi;  /* $D05D: colour RAM base high */
    unsigned char charset_addr_lo; /* $D05E: charset address low */
    unsigned char charset_addr_mid; /* $D05F: charset address mid */
    unsigned char charset_addr_hi; /* $D060: charset address high */
    unsigned char screen_row_lo;   /* $D061: screen row width (bytes) low */
    unsigned char screen_row_hi;   /* $D062: screen row width high */
    unsigned char colour_row_lo;   /* $D063: colour row step low */
    unsigned char colour_row_hi;   /* $D064: colour row step high */
    unsigned char _reserved3[3];   /* $D065-$D067 */
    unsigned char screen_rows;     /* $D068: number of text rows */
    unsigned char palette_ctrl;    /* $D069: palette control */
    unsigned char _reserved4[2];   /* $D06A-$D06B */
    unsigned char sprite_y_msb;    /* $D06C: sprite Y position MSBs */
    unsigned char _reserved5[3];   /* $D06D-$D06F */
    unsigned char palette_sel;     /* $D070: palette bank select */
    unsigned char _reserved6[3];   /* $D071-$D073 */
    unsigned char sprite_alpha;    /* $D074: sprite alpha/transparency */
    unsigned char _reserved7[3];   /* $D075-$D077 */
    unsigned char dat[4];          /* $D078: DAT (Direct Access Table) */
    unsigned char version;         /* $D07C: VIC-IV version */
    unsigned char _reserved8[3];   /* $D07D-$D07F */
};

/* Pointer to VIC-IV register block at $D000 */
#define vic4 ((volatile struct vic4_regs *)0xD000)

/* ===== $D011 (ctrl1) bits ===== */
#define VIC4_RST8     0x80  /* Raster bit 8 */
#define VIC4_ECM      0x40  /* Extended colour mode */
#define VIC4_BMM      0x20  /* Bitmap mode */
#define VIC4_DEN      0x10  /* Display enable */
#define VIC4_RSEL     0x08  /* Row select (25/24 rows) */

/* ===== $D016 (ctrl2) bits ===== */
#define VIC4_MCM      0x10  /* Multicolour mode */
#define VIC4_CSEL     0x08  /* Column select (40/38 cols) */

/* ===== $D019 / $D01A IRQ bits ===== */
#define VIC4_IRQ_RASTER  0x01
#define VIC4_IRQ_SPRITE  0x02  /* Sprite-background collision */
#define VIC4_IRQ_SCOLL   0x04  /* Sprite-sprite collision */
#define VIC4_IRQ_LP      0x08  /* Light pen */

/* ===== $D02F (key) unlock values ===== */
#define VIC3_KEY1     0xA5  /* Write first to unlock VIC-III */
#define VIC3_KEY2     0x96  /* Write second to unlock VIC-III */

/* ===== $D030 (ctrl_a) bits ===== */
#define VIC3_CRAM2K   0x01  /* Enable 2K colour RAM */
#define VIC3_EXTSYNC  0x02  /* External sync */
#define VIC3_PAL      0x04  /* PAL (vs NTSC) */
#define VIC3_ROM8     0x08  /* Map ROM at $8000 */
#define VIC3_ROMA     0x10  /* Map ROM at $A000 */
#define VIC3_ROMC     0x20  /* Map ROM at $C000 */
#define VIC3_ROME     0x40  /* Map ROM at $E000 */

/* ===== $D031 (ctrl_b) bits ===== */
#define VIC4_V400     0x08  /* 400-line (interlace) mode */
#define VIC4_H640     0x80  /* 640-pixel horizontal mode */
#define VIC4_FAST     0x40  /* 40 MHz CPU speed */
#define VIC4_ATTR     0x20  /* Enable VIC-III attributes */
#define VIC4_BPM      0x10  /* Bitplane mode */
#define VIC4_FCM      0x04  /* Full-colour mode */
#define VIC4_MCM2     0x02  /* VIC-III multicolour */
#define VIC4_INT      0x01  /* VIC-III interlace */

/* ===== $D054 (ctrl_c) bits ===== */
#define VIC4_CHR16    0x01  /* 16-bit character numbers */
#define VIC4_FCLRHI   0x02  /* Full-colour chars use high nybble */
#define VIC4_FCLRLO   0x04  /* Full-colour chars use low nybble */
#define VIC4_SMTH     0x08  /* Enable smooth scrolling */
#define VIC4_VIC400   0x10  /* VIC-IV 400 scanline mode */
#define VIC4_PALEMU   0x20  /* PAL CRT emulation */
#define VIC4_SPRH640  0x40  /* Sprite H640 mode */
#define VIC4_ALPHAEN  0x80  /* Alpha blending enable */

/* ===== Colour constants ===== */
#define COLOR_BLACK     0
#define COLOR_WHITE     1
#define COLOR_RED       2
#define COLOR_CYAN      3
#define COLOR_PURPLE    4
#define COLOR_GREEN     5
#define COLOR_BLUE      6
#define COLOR_YELLOW    7
#define COLOR_ORANGE    8
#define COLOR_BROWN     9
#define COLOR_PINK      10
#define COLOR_DGREY     11
#define COLOR_MGREY     12
#define COLOR_LGREEN    13
#define COLOR_LBLUE     14
#define COLOR_LGREY     15

/* ===== Convenience functions ===== */

/* Unlock VIC-III/IV registers (required before accessing $D030+) */
static void vic4_unlock(void) {
    vic4->key = VIC3_KEY1;
    vic4->key = VIC3_KEY2;
}

/* Unlock and enable 40 MHz */
static void vic4_fast(void) {
    vic4_unlock();
    vic4->ctrl_b |= VIC4_FAST;
}

/* Set sprite N position (X may be > 255) */
static void vic4_sprite_pos(unsigned char n, int xpos, unsigned char ypos) {
    vic4->sprite[n].x = (unsigned char)xpos;
    vic4->sprite[n].y = ypos;
    if (xpos > 255)
        vic4->sprite_x_msb |= (1 << n);
    else
        vic4->sprite_x_msb &= ~(1 << n);
}

/* ===== Direct register access (optimal codegen) ===== */
/* These emit direct sta/lda $D0xx via pointer constant propagation. */

#define VREG_BORDER      (*(volatile unsigned char *)0xD020)
#define VREG_BG0         (*(volatile unsigned char *)0xD021)
#define VREG_BG1         (*(volatile unsigned char *)0xD022)
#define VREG_BG2         (*(volatile unsigned char *)0xD023)
#define VREG_BG3         (*(volatile unsigned char *)0xD024)
#define VREG_CTRL1       (*(volatile unsigned char *)0xD011)
#define VREG_CTRL2       (*(volatile unsigned char *)0xD016)
#define VREG_RASTER      (*(volatile unsigned char *)0xD012)
#define VREG_SPR_ENABLE  (*(volatile unsigned char *)0xD015)
#define VREG_SPR_XMSB    (*(volatile unsigned char *)0xD010)
#define VREG_MEMPTR      (*(volatile unsigned char *)0xD018)
#define VREG_IRQ_STATUS  (*(volatile unsigned char *)0xD019)
#define VREG_IRQ_ENABLE  (*(volatile unsigned char *)0xD01A)
#define VREG_KEY         (*(volatile unsigned char *)0xD02F)
#define VREG_CTRL_A      (*(volatile unsigned char *)0xD030)
#define VREG_CTRL_B      (*(volatile unsigned char *)0xD031)
#define VREG_CTRL_C      (*(volatile unsigned char *)0xD054)