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

/* Sprite position helper (workaround for issue #84: nested struct array access) */
#define VIC4_SPRITE(n) ((volatile struct vic4_sprite_pos *)(0xD000 + (n) * 2))

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

/* ===== SID Registers ($D400 / $D420) ===== */

struct sid_voice {
    unsigned char freq_lo;         /* +$00: frequency low */
    unsigned char freq_hi;         /* +$01: frequency high */
    unsigned char pw_lo;           /* +$02: pulse width low */
    unsigned char pw_hi;           /* +$03: pulse width high (bits 3:0) */
    unsigned char ctrl;            /* +$04: waveform/gate control */
    unsigned char attack_decay;    /* +$05: attack (hi nybble) / decay (lo nybble) */
    unsigned char sustain_release; /* +$06: sustain (hi nybble) / release (lo nybble) */
};

struct sid_regs {
    struct sid_voice voice[3];     /* $00-$14: 3 voices (7 bytes each) */
    unsigned char filter_lo;       /* $15: filter cutoff low (bits 2:0) */
    unsigned char filter_hi;       /* $16: filter cutoff high */
    unsigned char filter_ctrl;     /* $17: filter routing / resonance */
    unsigned char volume;          /* $18: volume / filter mode */
    unsigned char pot_x;           /* $19: paddle X (read only) */
    unsigned char pot_y;           /* $1A: paddle Y (read only) */
    unsigned char osc3;            /* $1B: oscillator 3 output (read only) */
    unsigned char env3;            /* $1C: envelope 3 output (read only) */
};

/* Pointers to SID register blocks */
#define sid1 ((volatile struct sid_regs *)0xD400)
#define sid2 ((volatile struct sid_regs *)0xD420)
#define sid3 ((volatile struct sid_regs *)0xD440)
#define sid4 ((volatile struct sid_regs *)0xD460)

/* Voice access helpers (workaround for issue #84: nested struct array access) */
#define SID1_VOICE(n) ((volatile struct sid_voice *)(0xD400 + (n) * 7))
#define SID2_VOICE(n) ((volatile struct sid_voice *)(0xD420 + (n) * 7))
#define SID3_VOICE(n) ((volatile struct sid_voice *)(0xD440 + (n) * 7))
#define SID4_VOICE(n) ((volatile struct sid_voice *)(0xD460 + (n) * 7))

/* SID voice control bits ($D404/$D40B/$D412) */
#define SID_GATE      0x01  /* Gate (start/release) */
#define SID_SYNC      0x02  /* Sync with preceding voice */
#define SID_RINGMOD   0x04  /* Ring modulation */
#define SID_TEST      0x08  /* Test bit (resets oscillator) */
#define SID_TRIANGLE  0x10  /* Triangle waveform */
#define SID_SAWTOOTH  0x20  /* Sawtooth waveform */
#define SID_PULSE     0x40  /* Pulse/square waveform */
#define SID_NOISE     0x80  /* Noise waveform */

/* SID filter control bits ($D417) */
#define SID_FILT1     0x01  /* Route voice 1 through filter */
#define SID_FILT2     0x02  /* Route voice 2 through filter */
#define SID_FILT3     0x04  /* Route voice 3 through filter */
#define SID_FILT_EXT  0x08  /* Route external audio through filter */

/* SID volume/filter mode bits ($D418) */
#define SID_LP        0x10  /* Low-pass filter */
#define SID_BP        0x20  /* Band-pass filter */
#define SID_HP        0x40  /* High-pass filter */
#define SID_MUTE3     0x80  /* Mute voice 3 output */

/* ===== CIA 1 Registers ($DC00-$DC0F) ===== */

struct cia_regs {
    unsigned char pra;             /* $00: port A data (keyboard col / joy 2) */
    unsigned char prb;             /* $01: port B data (keyboard row / joy 1) */
    unsigned char ddra;            /* $02: port A data direction */
    unsigned char ddrb;            /* $03: port B data direction */
    unsigned char timer_a_lo;      /* $04: timer A low byte */
    unsigned char timer_a_hi;      /* $05: timer A high byte */
    unsigned char timer_b_lo;      /* $06: timer B low byte */
    unsigned char timer_b_hi;      /* $07: timer B high byte */
    unsigned char tod_10ths;       /* $08: TOD tenths of seconds */
    unsigned char tod_sec;         /* $09: TOD seconds */
    unsigned char tod_min;         /* $0A: TOD minutes */
    unsigned char tod_hr;          /* $0B: TOD hours */
    unsigned char sdr;             /* $0C: serial data register */
    unsigned char icr;             /* $0D: interrupt control (read: status, write: mask) */
    unsigned char cra;             /* $0E: control register A */
    unsigned char crb;             /* $0F: control register B */
};

/* Pointers to CIA register blocks */
#define cia1 ((volatile struct cia_regs *)0xDC00)
#define cia2 ((volatile struct cia_regs *)0xDD00)

/* CIA interrupt control bits ($DC0D/$DD0D) */
#define CIA_ICR_TA    0x01  /* Timer A underflow */
#define CIA_ICR_TB    0x02  /* Timer B underflow */
#define CIA_ICR_ALRM  0x04  /* TOD alarm */
#define CIA_ICR_SP    0x08  /* Serial port full/empty */
#define CIA_ICR_FLAG  0x10  /* FLAG pin edge */
#define CIA_ICR_SET   0x80  /* Write: 1=set bits, 0=clear bits */

/* CIA control register A bits ($DC0E/$DD0E) */
#define CIA_CRA_START  0x01  /* Start timer A */
#define CIA_CRA_PBON   0x02  /* Timer A output on PB6 */
#define CIA_CRA_TOGGLE 0x04  /* PB6 toggle (vs pulse) */
#define CIA_CRA_ONESHOT 0x08 /* One-shot (vs continuous) */
#define CIA_CRA_LOAD   0x10  /* Force-load timer A latch */
#define CIA_CRA_INMODE 0x20  /* Count CNT pin (vs phi2) */
#define CIA_CRA_SPOUT  0x40  /* Serial port output mode */
#define CIA_CRA_TODIN  0x80  /* TOD 50Hz input (vs 60Hz) */

/* CIA control register B bits ($DC0F/$DD0F) */
#define CIA_CRB_START  0x01  /* Start timer B */
#define CIA_CRB_PBON   0x02  /* Timer B output on PB7 */
#define CIA_CRB_TOGGLE 0x04  /* PB7 toggle (vs pulse) */
#define CIA_CRB_ONESHOT 0x08 /* One-shot (vs continuous) */
#define CIA_CRB_LOAD   0x10  /* Force-load timer B latch */
#define CIA_CRB_INMODE0 0x20 /* Timer B input: 00=phi2, 01=CNT */
#define CIA_CRB_INMODE1 0x40 /*   10=timer A underflow, 11=TA underflow+CNT */
#define CIA_CRB_ALARM  0x80  /* TOD write sets alarm (vs clock) */

/* Joystick direction bits (CIA1 PRA/PRB) */
#define JOY_UP        0x01
#define JOY_DOWN      0x02
#define JOY_LEFT      0x04
#define JOY_RIGHT     0x08
#define JOY_FIRE      0x10

/* Read joystick: bits are active-LOW, invert for natural reading */
static unsigned char joy1_read(void) {
    return cia1->prb ^ 0x1F;
}

static unsigned char joy2_read(void) {
    return cia1->pra ^ 0x1F;
}

/* ===== Memory Pointers ===== */

#define SCREEN_RAM  ((volatile unsigned char *)0x0800)
#define COLOUR_RAM  ((volatile unsigned char *)0xD800)

/* ===== F018B DMA Controller ($D700-$D705) ===== */

struct dma_regs {
    unsigned char control;         /* $D700: DMA trigger (write 0 to execute) */
    unsigned char addr_lo;         /* $D701: DMA list address low */
    unsigned char addr_mid;        /* $D702: DMA list address mid */
    unsigned char addr_hi;         /* $D703: DMA list address high */
    unsigned char addr_bank;       /* $D704: DMA list address bank */
    unsigned char etrig;           /* $D705: enhanced trigger */
};

#define dma ((volatile struct dma_regs *)0xD700)

/* DMA command bytes (for DMA job lists) */
#define DMA_CMD_COPY   0x00
#define DMA_CMD_FILL   0x03
#define DMA_CMD_SWAP   0x04
#define DMA_CMD_MIX    0x04  /* with sub-command */

/* ===== MEGA65 Math Accelerator ($D760-$D77F) ===== */

struct math_divider {
    unsigned char arg1[4];         /* $D760: dividend (32-bit) */
    unsigned char arg2[4];         /* $D764: divisor (32-bit) */
    unsigned char quotient[4];     /* $D768: quotient result (32-bit) */
    unsigned char _reserved[2];    /* $D76C-$D76D */
    unsigned char sign;            /* $D76E: sign scratch byte */
    unsigned char _reserved2;      /* $D76F */
};

struct math_multiplier {
    unsigned char arg1[4];         /* $D770: multiplicand (32-bit) */
    unsigned char arg2[4];         /* $D774: multiplier (32-bit) */
    unsigned char result[4];       /* $D778: product result (32-bit) */
    unsigned char _reserved[4];    /* $D77C-$D77F */
};

#define math_div ((volatile struct math_divider *)0xD760)
#define math_mul ((volatile struct math_multiplier *)0xD770)

/* Math accelerator status */
#define MATH_BUSY  (*(volatile unsigned char *)0xD70F)
#define MATH_BUSY_BIT  0x80  /* Bit 7 of $D70F: set while computing */

/* Hardware RNG */
#define HW_RANDOM  (*(volatile unsigned char *)0xD7EF)
#define HW_RNG_READY (*(volatile unsigned char *)0xD7FE)

/* ===== Audio Mixer ($D63C-$D63F) ===== */

struct audio_mixer_regs {
    unsigned char sid1_vol;        /* $D63C: SID 1 left/right volume */
    unsigned char sid2_vol;        /* $D63D: SID 2 left/right volume */
    unsigned char sid3_vol;        /* $D63E: SID 3 left/right volume */
    unsigned char sid4_vol;        /* $D63F: SID 4 left/right volume */
};

#define audio_mixer ((volatile struct audio_mixer_regs *)0xD63C)

/* Audio mixer volume nybbles: high = left channel, low = right channel */
#define AUDIO_LEFT(vol)   ((vol) << 4)
#define AUDIO_RIGHT(vol)  ((vol) & 0x0F)
#define AUDIO_BOTH(vol)   (((vol) << 4) | ((vol) & 0x0F))

/* ===== Floppy Disk Controller ($D080-$D09F) ===== */

struct fdc_regs {
    unsigned char control;         /* $D080: FDC control / status */
    unsigned char command;         /* $D081: FDC command register */
    unsigned char stat2;           /* $D082: FDC status 2 */
    unsigned char swap;            /* $D083: FDC swap (side/density) */
    unsigned char track;           /* $D084: current track */
    unsigned char sector;          /* $D085: current sector */
    unsigned char side;            /* $D086: current side */
    unsigned char data;            /* $D087: data register */
    unsigned char clock;           /* $D088: clock divider */
    unsigned char step;            /* $D089: step rate */
    unsigned char _reserved[6];    /* $D08A-$D08F */
};

#define fdc ((volatile struct fdc_regs *)0xD080)

/* FDC commands */
#define FDC_CMD_READ    0x40  /* Read sector */
#define FDC_CMD_WRITE   0xA0  /* Write sector */
#define FDC_CMD_STEP_IN  0x18 /* Step in (toward center) */
#define FDC_CMD_STEP_OUT 0x10 /* Step out (toward edge) */
#define FDC_CMD_RESTORE  0x08 /* Seek to track 0 */

/* FDC status bits ($D080 read) */
#define FDC_BUSY      0x80  /* Command in progress */
#define FDC_DRQ       0x40  /* Data request (byte ready) */
#define FDC_EQ        0x20  /* Equal (sector found) */
#define FDC_RNF       0x10  /* Record not found */
#define FDC_WRPROT    0x04  /* Write protect */
#define FDC_TK0       0x02  /* Track 0 */

/* ===== Hypervisor Traps ($D640-$D67F) ===== */

struct hyper_regs {
    unsigned char trap[64];        /* $D640-$D67F: hypervisor trap registers */
};

#define hyper ((volatile struct hyper_regs *)0xD640)

/* Common hypervisor trap calls (write to $D640+n to trigger) */
#define HTRAP_SETNAME  0x2E  /* Set current filename for next load/save */
#define HTRAP_LOADFILE 0x36  /* Load file to address */

/* ===== SD Card Controller ($D680-$D6A0) ===== */

struct sd_regs {
    unsigned char status;          /* $D680: SD status / bus select */
    unsigned char command;         /* $D681: SD command */
    unsigned char addr0;           /* $D682: sector address byte 0 (LSB) */
    unsigned char addr1;           /* $D683: sector address byte 1 */
    unsigned char addr2;           /* $D684: sector address byte 2 */
    unsigned char addr3;           /* $D685: sector address byte 3 (MSB) */
    unsigned char _reserved[2];    /* $D686-$D687 */
    unsigned char buf_addr_lo;     /* $D688: buffer address low (within SD sector buffer) */
    unsigned char buf_addr_hi;     /* $D689: buffer address high */
    unsigned char _reserved2[6];   /* $D68A-$D68F */
    unsigned char rdata;           /* $D690: read data (from sector buffer) */
};

#define sd ((volatile struct sd_regs *)0xD680)

/* SD status bits ($D680) */
#define SD_BUSY       0x80  /* Controller busy */
#define SD_RESET      0x40  /* Card reset state */
#define SD_ERROR      0x20  /* Last command had error */
#define SD_SDHC       0x10  /* SDHC card detected */

/* SD commands ($D681) */
#define SD_CMD_RESET  0x00  /* Reset SD card */
#define SD_CMD_READ   0x01  /* Read sector to buffer */
#define SD_CMD_WRITE  0x02  /* Write buffer to sector */
#define SD_CMD_FLUSH  0x03  /* Flush write cache */

/* ===== Ethernet Controller ($D6E0-$D6FF) ===== */

struct eth_regs {
    unsigned char ctrl;            /* $D6E0: Ethernet control */
    unsigned char txszlo;          /* $D6E1: TX frame size low */
    unsigned char txszhi;          /* $D6E2: TX frame size high */
    unsigned char _reserved1[5];   /* $D6E3-$D6E7 */
    unsigned char mac[6];          /* $D6E9-$D6ED: MAC address (note: starts at +8, gap at +3..+7) */
    unsigned char _reserved2;      /* $D6EE */
    unsigned char miimlo;          /* $D6EF: MIIM register low */
    unsigned char _reserved3[8];   /* $D6F0-$D6F7 */
    unsigned char rxbuf;           /* $D6E8: RX buffer read port */
    unsigned char txbuf;           /* $D6E9: TX buffer write port */
};

#define eth ((volatile struct eth_regs *)0xD6E0)

/* Ethernet control bits ($D6E0) */
#define ETH_RST       0x01  /* Reset ethernet controller */
#define ETH_TXEN      0x02  /* Enable TX */
#define ETH_RXEN      0x04  /* Enable RX */
#define ETH_RXIRQ     0x10  /* RX interrupt pending (read) */
#define ETH_TXDONE    0x20  /* TX complete (read) */

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