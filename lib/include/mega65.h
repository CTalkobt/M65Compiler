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
    unsigned char bitplanes_x_start; /* $D03E: bitplanes X start screen offset */
    unsigned char bitplanes_y_start; /* $D03F: bitplanes Y start screen offset */

    /* --- VIC-IV extended registers ($D040-$D07F) — per MEGA65 VHDL (viciv.vhdl) --- */
    unsigned char c65_dac[8];      /* $D040-$D047: Unimplemented C65 DACs (read $FF) */
    unsigned char border_y_top_lo; /* $D048: top border position LSB */
    unsigned char border_y_top_hi; /* $D049: top border MSB & sprite bitplane enables */
    unsigned char border_y_bot_lo; /* $D04A: bottom border position LSB */
    unsigned char border_y_bot_hi; /* $D04B: bottom border MSB & sprite bitplane enables */
    unsigned char x_chargen_lo;    /* $D04C: char generator X offset LSB */
    unsigned char x_chargen_hi;    /* $D04D: char generator X offset MSB & sprite tile enables */
    unsigned char y_chargen_lo;    /* $D04E: char generator Y offset LSB */
    unsigned char y_chargen_hi;    /* $D04F: char generator Y offset MSB & sprite tile enables */
    unsigned char xpos_lo;         /* $D050: physical scanline X position LSB (read) */
    unsigned char xpos_hi;         /* $D051: physical scanline X MSB & double raster flags */
    unsigned char ypos_lo;         /* $D052: physical scanline Y position LSB (read) */
    unsigned char ypos_hi;         /* $D053: physical scanline Y MSB & upscaler/bold flags */
    unsigned char ctrl_c;          /* $D054: VIC-IV mode control C */
    unsigned char sprite_extheight_en; /* $D055: sprite extended height enables */
    unsigned char sprite_extheight_sz; /* $D056: sprite extended height size */
    unsigned char sprite_extwidth_en;  /* $D057: sprite extended width enables */
    unsigned char virtual_row_width_lo; /* $D058: memory row width/step LSB */
    unsigned char virtual_row_width_hi; /* $D059: memory row width/step MSB */
    unsigned char chargen_x_scale; /* $D05A: character generator X scale factor */
    unsigned char chargen_y_scale; /* $D05B: character generator Y scale factor */
    unsigned char side_border_lo;  /* $D05C: left side border position LSB */
    unsigned char side_border_hi;  /* $D05D: left side border MSB & hot regs enable */
    unsigned char display_row_width_lo; /* $D05E: active display row width LSB */
    unsigned char sprite_h640_msbs; /* $D05F: sprite H640 position MSBs */
    unsigned char screen_ram_base_lo;  /* $D060: screen RAM base bits 0-7 */
    unsigned char screen_ram_base_mid; /* $D061: screen RAM base bits 8-15 */
    unsigned char screen_ram_base_hi;  /* $D062: screen RAM base bits 16-23 */
    unsigned char screen_ram_base_bank; /* $D063: screen RAM bank, row width MSB & FCM_MCM */
    unsigned char colour_ram_base_lo;  /* $D064: colour RAM base bits 0-7 */
    unsigned char colour_ram_base_hi;  /* $D065: colour RAM base bits 8-15 */
    unsigned char xcounter_delay;  /* $D066: xcounter pipeline delay / test pattern */
    unsigned char sprite_first_x;  /* $D067: sprite start X coordinate LSB */
    unsigned char charset_base_lo; /* $D068: character generator base bits 0-7 */
    unsigned char charset_base_mid; /* $D069: character generator base bits 8-15 */
    unsigned char charset_base_hi; /* $D06A: character generator base bits 16-23 */
    unsigned char sprite_16col_en; /* $D06B: sprite 16-colour mode enables */
    unsigned char sprite_ptr_lo;   /* $D06C: sprite pointer base bits 0-7 */
    unsigned char sprite_ptr_mid;  /* $D06D: sprite pointer base bits 8-15 */
    unsigned char sprite_ptr_hi;   /* $D06E: sprite pointer base bits 16-23 */
    unsigned char first_raster_vga60; /* $D06F: first raster line & VGA60/NTSC select */
    unsigned char palette_sel;     /* $D070: palette bank select routing */
    unsigned char bitplane_16col_en; /* $D071: bitplane 16-colour mode flags */
    unsigned char sprite_y_adjust; /* $D072: sprite Y adjust offset */
    unsigned char alpha_delay_scale; /* $D073: alpha delay (0-3) & ycounter scale (4-7) */
    unsigned char sprite_alpha_blend; /* $D074: sprite alpha blend enables */
    unsigned char sprite_alpha_val;  /* $D075: sprite alpha blend global value */
    unsigned char sprite_v400_en;  /* $D076: sprite V400 enables */
    unsigned char sprite_v400_y_msb; /* $D077: sprite V400 Y position MSBs */
    unsigned char sprite_v400_y_smsb; /* $D078: sprite V400 Y position super MSBs */
    unsigned char raster_compare_lo; /* $D079: raster compare LSB */
    unsigned char raster_compare_hi; /* $D07A: raster compare MSB & source select */
    unsigned char display_row_count; /* $D07B: number of text rows to display */
    unsigned char bitplane_bank;   /* $D07C: bitplane bank select & HSYNC/VSYNC polarity */
    unsigned char debug_x_lo;      /* $D07D: debug X position LSB */
    unsigned char debug_y_lo;      /* $D07E: debug Y position LSB */
    unsigned char debug_hi;        /* $D07F: debug X/Y MSB & out-of-frame */
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

/* ===== Keyboard Matrix Scan ===== */
/* Key codes: (column << 3) | row for the C64/MEGA65 8x8 keyboard matrix.
 * Use with key_pressed() for direct hardware scanning (multi-key capable).
 * For buffered single-key input, use cbm_getin() from <cbm.h> instead.
 *
 *      Col0    Col1    Col2    Col3    Col4    Col5    Col6    Col7
 * R0   DEL     RETURN  C-RT    F7      F1      F3      F5      C-DN
 * R1   3       W       A       4       Z       S       E       LSHIFT
 * R2   5       R       D       6       C       F       T       X
 * R3   7       Y       G       8       B       H       U       V
 * R4   9       I       J       0       M       K       O       N
 * R5   +       P       L       -       .       :       @       ,
 * R6   POUND   *       ;       HOME    RSHIFT  =       UP-ARW  /
 * R7   1       <-      CTRL    2       SPACE   C=      Q       STOP
 */

/* Row 0 */
#define KEY_DELETE   0x00  /* col 0, row 0 */
#define KEY_RETURN   0x08  /* col 1, row 0 */
#define KEY_RIGHT    0x10  /* col 2, row 0 */
#define KEY_F7       0x18  /* col 3, row 0 */
#define KEY_F1       0x20  /* col 4, row 0 */
#define KEY_F3       0x28  /* col 5, row 0 */
#define KEY_F5       0x30  /* col 6, row 0 */
#define KEY_DOWN     0x38  /* col 7, row 0 */

/* Row 1 */
#define KEY_3        0x01  /* col 0, row 1 */
#define KEY_W        0x09  /* col 1, row 1 */
#define KEY_A        0x11  /* col 2, row 1 */
#define KEY_4        0x19  /* col 3, row 1 */
#define KEY_Z        0x21  /* col 4, row 1 */
#define KEY_S        0x29  /* col 5, row 1 */
#define KEY_E        0x31  /* col 6, row 1 */
#define KEY_LSHIFT   0x39  /* col 7, row 1 */

/* Row 2 */
#define KEY_5        0x02  /* col 0, row 2 */
#define KEY_R        0x0A  /* col 1, row 2 */
#define KEY_D        0x12  /* col 2, row 2 */
#define KEY_6        0x1A  /* col 3, row 2 */
#define KEY_C        0x22  /* col 4, row 2 */
#define KEY_F        0x2A  /* col 5, row 2 */
#define KEY_T        0x32  /* col 6, row 2 */
#define KEY_X        0x3A  /* col 7, row 2 */

/* Row 3 */
#define KEY_7        0x03  /* col 0, row 3 */
#define KEY_Y        0x0B  /* col 1, row 3 */
#define KEY_G        0x13  /* col 2, row 3 */
#define KEY_8        0x1B  /* col 3, row 3 */
#define KEY_B        0x23  /* col 4, row 3 */
#define KEY_H        0x2B  /* col 5, row 3 */
#define KEY_U        0x33  /* col 6, row 3 */
#define KEY_V        0x3B  /* col 7, row 3 */

/* Row 4 */
#define KEY_9        0x04  /* col 0, row 4 */
#define KEY_I        0x0C  /* col 1, row 4 */
#define KEY_J        0x14  /* col 2, row 4 */
#define KEY_0        0x1C  /* col 3, row 4 */
#define KEY_M        0x24  /* col 4, row 4 */
#define KEY_K        0x2C  /* col 5, row 4 */
#define KEY_O        0x34  /* col 6, row 4 */
#define KEY_N        0x3C  /* col 7, row 4 */

/* Row 5 */
#define KEY_PLUS     0x05  /* col 0, row 5 */
#define KEY_P        0x0D  /* col 1, row 5 */
#define KEY_L        0x15  /* col 2, row 5 */
#define KEY_MINUS    0x1D  /* col 3, row 5 */
#define KEY_PERIOD   0x25  /* col 4, row 5 */
#define KEY_COLON    0x2D  /* col 5, row 5 */
#define KEY_AT       0x35  /* col 6, row 5 */
#define KEY_COMMA    0x3D  /* col 7, row 5 */

/* Row 6 */
#define KEY_POUND    0x06  /* col 0, row 6 */
#define KEY_STAR     0x0E  /* col 1, row 6 */
#define KEY_SEMI     0x16  /* col 2, row 6 */
#define KEY_HOME     0x1E  /* col 3, row 6 */
#define KEY_RSHIFT   0x26  /* col 4, row 6 */
#define KEY_EQUALS   0x2E  /* col 5, row 6 */
#define KEY_UPARROW  0x36  /* col 6, row 6 */
#define KEY_SLASH    0x3E  /* col 7, row 6 */

/* Row 7 */
#define KEY_1        0x07  /* col 0, row 7 */
#define KEY_LARROW   0x0F  /* col 1, row 7 */
#define KEY_CTRL     0x17  /* col 2, row 7 */
#define KEY_2        0x1F  /* col 3, row 7 */
#define KEY_SPACE    0x27  /* col 4, row 7 */
#define KEY_CBM      0x2F  /* col 5, row 7 */
#define KEY_Q        0x37  /* col 6, row 7 */
#define KEY_STOP     0x3F  /* col 7, row 7 */

/* Direct keyboard matrix scan — returns 1 if key pressed, 0 if not.
 * Scans CIA1 hardware directly; supports simultaneous multi-key detection.
 * Requires linking with c45.lib (or c45_zp.lib). */
__regparm unsigned char key_pressed(unsigned char keycode);

/* ===== Keyboard Event Queue ($D610-$D619) — per MEGA65 VHDL ===== */
/* Buffered keyboard input via hardware typing event queue.
 * Reading returns oldest key; writing any value advances the queue. */

#define ASCII_KEY_QUEUE    (*(volatile unsigned char *)0xD610)  /* ASCII key (0=empty) */
#define PETSCII_KEY_QUEUE  (*(volatile unsigned char *)0xD619)  /* PETSCII key (0xFF=empty) */
#define KEY_MODIFIERS_REG  (*(volatile unsigned char *)0xD611)  /* Modifier key state */

/* Modifier key bits ($D611) */
#define KEY_MOD_LSHIFT     0x01  /* Left Shift */
#define KEY_MOD_RSHIFT     0x02  /* Right Shift */
#define KEY_MOD_CTRL       0x04  /* Control */
#define KEY_MOD_MEGA       0x08  /* Mega / Commodore */
#define KEY_MOD_ALT        0x10  /* Alternate */
#define KEY_MOD_NOSCRL     0x20  /* No Scroll */
#define KEY_MOD_CAPS       0x40  /* Caps Lock */
#define KEY_MOD_DISABLE    0x80  /* Disable modifier mapping (writeable) */

/* Read next ASCII key from queue (returns 0 if empty) */
static char getkey_ascii(void) {
    char k = ASCII_KEY_QUEUE;
    if (k) ASCII_KEY_QUEUE = 0xFF;  /* advance queue */
    return k;
}

/* Read next PETSCII key from queue (returns 0xFF if empty) */
static char getkey_petscii(void) {
    char k = PETSCII_KEY_QUEUE;
    if ((unsigned char)k != 0xFF) PETSCII_KEY_QUEUE = 0xFF;  /* advance queue */
    return k;
}

/* ===== Memory Pointers ===== */

#define SCREEN_RAM  ((volatile unsigned char *)0x0800)
#define COLOUR_RAM  ((volatile unsigned char *)0xD800)

/* ===== F018B DMA Controller ($D700-$D70E) — per MEGA65 VHDL ===== */
/* IMPORTANT: Writing $D700 TRIGGERS DMA execution — write it LAST.
 * Correct sequence: set bank ($D702), set MSB ($D701), then write LSB ($D700). */

struct dma_regs {
    unsigned char addr_lsb_trig;   /* $D700: DMA list addr bits 0-7 (write TRIGGERS DMA) */
    unsigned char addr_msb;        /* $D701: DMA list addr bits 8-15 */
    unsigned char addr_bank;       /* $D702: DMA list addr bank bits 16-22 */
    unsigned char mode;            /* $D703: DMA mode/control (bit 0 = F018B mode) */
    unsigned char addr_mb;         /* $D704: DMA list addr megabyte bits 20-27 */
    unsigned char etrig;           /* $D705: enhanced trigger (flat 28-bit addr LSB) */
};

#define dma ((volatile struct dma_regs *)0xD700)

/* DMA list address LSB without triggering (for setup before trigger) */
#define DMA_ADDR_LSB_NOTRIG (*(volatile unsigned char *)0xD70E)

/* DMA command bytes (for DMA job lists) */
#define DMA_CMD_COPY   0x00
#define DMA_CMD_FILL   0x03
#define DMA_CMD_SWAP   0x04
#define DMA_CMD_MIX    0x04  /* with sub-command */

/* ===== MEGA65 Math Accelerator ($D768-$D77F) — per VHDL ===== */
/* Multiplier and divider share input registers $D770/$D774.
 * Writing to $D770-$D777 triggers division (div_start_over signal).
 * Multiplier is combinational (result available immediately).
 * No hardware remainder register — compute as: dividend - quotient * divisor. */

struct math_accel {
    unsigned char frac[4];         /* $D768: fractional quotient (32-bit) */
    unsigned char quotient[4];     /* $D76C: integer quotient (32-bit) */
    unsigned char arg1[4];         /* $D770: dividend / multiplicand (32-bit) */
    unsigned char arg2[4];         /* $D774: divisor / multiplier (32-bit) */
    unsigned char product[8];      /* $D778: 64-bit product result */
};

#define math ((volatile struct math_accel *)0xD768)

/* Math accelerator status */
#define MATH_BUSY  (*(volatile unsigned char *)0xD70F)
#define MATH_BUSY_DIV  0x80  /* Bit 7: divider busy */
#define MATH_BUSY_MUL  0x40  /* Bit 6: multiplier busy */

/* Hardware RNG */
#define HW_RANDOM  (*(volatile unsigned char *)0xD7EF)
#define HW_RNG_READY (*(volatile unsigned char *)0xD7FE)

/* ===== Audio Mixer & Digital Audio ($D6F4-$D6FD) — per MEGA65 VHDL ===== */
/* The mixer has 16 input sources x 8 output channels = 128 coefficients.
 * Coefficient index = input_channel + (output_channel * 16).
 * $D600-$D63F is C65 serial UART space, NOT audio. */

#define AUDIO_MIX_SEL      (*(volatile unsigned char *)0xD6F4)  /* Mixer register select */
#define AUDIO_MIX_DATA     (*(volatile unsigned char *)0xD6F5)  /* Mixer register data */

#define AUDIO_DIGI_LEFT    (*(volatile unsigned short *)0xD6F8) /* 16-bit PCM left channel */
#define AUDIO_DIGI_RIGHT   (*(volatile unsigned short *)0xD6FA) /* 16-bit PCM right channel */
#define AUDIO_READBACK     (*(volatile unsigned short *)0xD6FC) /* Audio loopback read-back */

/* Set mixer coefficient N (0-127) to a 16-bit volume value */
static void set_mixer_coefficient(unsigned char index, unsigned short volume) {
    AUDIO_MIX_SEL = index << 1;          /* select low byte */
    AUDIO_MIX_DATA = volume & 0xFF;
    AUDIO_MIX_SEL = (index << 1) | 1;    /* select high byte */
    AUDIO_MIX_DATA = volume >> 8;
}

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

/* ===== Ethernet Controller ($D6E0-$D6EE) — per MEGA65 VHDL (ethernet.vhdl) ===== */
/* RX/TX packet data is at $FFDE800-$FFDEFFF (2KB memory-mapped window),
 * NOT through register ports. */

struct eth_regs {
    unsigned char ctrl;            /* $D6E0: control (bit 0=reset, 1=TX reset, 7=TX idle) */
    unsigned char irq_ctrl;        /* $D6E1: interrupt status/control & RX buffer count */
    unsigned char txsz_lo;         /* $D6E2: TX frame size LSB */
    unsigned char txsz_hi;         /* $D6E3: TX frame size MSB */
    unsigned char cmd;             /* $D6E4: command ($01 = trigger transmit) */
    unsigned char mode;            /* $D6E5: promiscuous, CRC check, phase adjust */
    unsigned char miim_reg;        /* $D6E6: MIIM register & PHY select */
    unsigned char miim_val_lo;     /* $D6E7: MIIM value LSB */
    unsigned char miim_val_hi;     /* $D6E8: MIIM value MSB */
    unsigned char mac[6];          /* $D6E9-$D6EE: MAC address (6 bytes) */
};

#define eth ((volatile struct eth_regs *)0xD6E0)

/* Ethernet control bits ($D6E0) */
#define ETH_RST       0x01  /* Reset ethernet controller */
#define ETH_TXRST     0x02  /* Reset TX path */
#define ETH_TXIDLE    0x80  /* TX idle (read-only) */

/* Ethernet commands ($D6E4) */
#define ETH_CMD_TX    0x01  /* Trigger frame transmission */

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