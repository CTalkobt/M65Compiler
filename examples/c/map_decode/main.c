/* map_decode — what the 45GS02 sees in its 64 KB, for a memory
 * configuration you set up
 *
 * The MEGA65 banks memory four ways, in order of priority (the MEGA65 Book,
 * "CPU Memory Banking"): the C64's $01 port (BASIC, KERNAL, the character
 * ROM or I/O at $A000 / $D000 / $E000), the MAP instruction (each 8 KB
 * slab of the lower and upper 32 KB moved by an offset, in a megabyte of
 * the 28-bit space), the VIC-III's $D030 ROM bits (C65 ROM at $8000,
 * $A000, $C000, $E000) and, above all of them, I/O at $D000.
 *
 * You edit the values; nothing is actually remapped. The program decodes
 * them for each 4 KB block and draws (TGI, 320x200):
 *   - the CPU's 64 KB as a bar of 16 blocks, coloured by what is there
 *     (RAM, moved RAM, a C64 ROM, a C65 ROM, the character ROM, I/O; chip
 *     RAM labelled with its bank),
 *   - the first 384 KB of chip RAM (banks 0-5, the ROMs in banks 2-3) with
 *     where each block reads from marked in its colour and a line to it,
 *   - the values, the A / X / Y / Z a MAP needs for them, and the selected
 *     block's 28-bit address.
 *
 * A MAP offset moves only the 8 KB slabs whose bit is set (SLABS): bank 1
 * at $0000-$7FFF is LO OFS $100 with SLABS %1111.
 *
 * Keys: CRSR up / down chooses a value, CRSR left / right or + / - changes
 * it (the offsets by 8 KB), 0-9 and A-F type it in hex, P steps through the
 * presets, Q quits. F1 or ? shows the keys and the registers, F3 or
 * CTRL+D each block in detail, F5 or CTRL+B a BASIC 65 program that sets
 * the configuration up (POKEs a routine to $1600 and SYSes it: dangerous,
 * BASIC needs its own map), F7, CTRL+M or M that routine as machine code;
 * any key goes back. (The pages take CTRL with B and D: plain, they are hex
 * digits. CTRL+M comes as 13, as RETURN does, which opens it too.)
 *
 * MAP_CHECK (make check): the decoder's cases and the drawing checked
 * without keys; each result byte at $C0C0 is 01, then $AA. MAP_KEYS (make
 * check too): the editor run by keys pressed through the virtual key
 * registers ($D615 / $D616: through the keyboard scanner and $D610, as
 * typed), then the values and the screen checked the same way.
 */
#include <tgi.h>
#include <input.h>

/* ---- the configuration ---- */

struct config {
    unsigned int lo_off, hi_off;    /* MAP offsets in 256-byte units (12 bits) */
    unsigned char lo_en, hi_en;     /* MAP: the 8 KB slabs moved (a bit each) */
    unsigned char lo_mb, hi_mb;     /* MAP: the megabyte (bits 20-27) */
    unsigned char port;             /* $01: LORAM, HIRAM, CHAREN (bits 0-2) */
    unsigned char d030;             /* $D030: ROM8, ROMA, ROMC, ROME (bits 3-5, 7) */
};

/* what a block holds */
enum { K_RAM, K_MAPPED, K_ROM64, K_CHAR, K_ROM65, K_IO };

static const unsigned char kind_colour[6] = {
    TGI_COLOR_GRAY2, TGI_COLOR_GREEN, TGI_COLOR_BLUE, TGI_COLOR_ORANGE, TGI_COLOR_PURPLE, TGI_COLOR_RED
};

struct block {
    unsigned long phys;   /* the 28-bit address it reads (I/O: $FFD3000 + the offset) */
    unsigned char kind;
};

/* Decode 4 KB block b ($b000-$bFFF) of the CPU's view */
static void decode(const struct config *c, unsigned char b, struct block *out) {
    unsigned int a = (unsigned int)b << 12;
    unsigned char slab = b >> 1;
    unsigned char lohi = c->port & 3;       /* LORAM | HIRAM */
    unsigned char io = 0;

    /* the C64's $01 */
    out->kind = K_RAM;
    out->phys = a;
    if (b >= 0xA && b <= 0xB && lohi == 3) {
        out->kind = K_ROM64;
        out->phys = 0x2A000UL + (a - 0xA000U);
    } else if (b >= 0xE && (c->port & 2)) {
        out->kind = K_ROM64;
        out->phys = 0x2E000UL + (a - 0xE000U);
    } else if (b == 0xD && lohi) {
        if (c->port & 4) io = 1;
        else {
            out->kind = K_CHAR;
            out->phys = 0x2D000UL;
        }
    }
    /* MAP: the lower 32 KB with A / X, the upper with Y / Z */
    if (slab < 4 ? (c->lo_en >> slab) & 1 : (c->hi_en >> (slab - 4)) & 1) {
        unsigned long off = slab < 4 ? c->lo_off : c->hi_off;
        unsigned char mb = slab < 4 ? c->lo_mb : c->hi_mb;
        out->kind = K_MAPPED;
        out->phys = ((unsigned long)mb << 20) | (((off << 8) + a) & 0xFFFFFUL);
    }
    /* the VIC-III's C65 ROM banking */
    if (b >= 0x8 && b <= 0x9 && (c->d030 & 0x08)) {
        out->kind = K_ROM65;
        out->phys = 0x38000UL + (a - 0x8000U);
    } else if (b >= 0xA && b <= 0xB && (c->d030 & 0x10)) {
        out->kind = K_ROM65;
        out->phys = 0x3A000UL + (a - 0xA000U);
    } else if (b == 0xC && (c->d030 & 0x20)) {
        out->kind = K_ROM65;
        out->phys = 0x2C000UL;
    } else if (b >= 0xE && (c->d030 & 0x80)) {
        out->kind = K_ROM65;
        out->phys = 0x3E000UL + (a - 0xE000U);
    }
    /* I/O is above all of them */
    if (io) {
        out->kind = K_IO;
        out->phys = 0xFFD3000UL;
    }
}

/* ---- names of the 28-bit address space ---- */

struct region {
    unsigned long start, end;
    const char *name;
};

static const struct region regions[] = {
    { 0x0000000UL, 0x001FFFFUL, "chip RAM" },
    { 0x0020000UL, 0x0023FFFUL, "C65 DOS" },
    { 0x0024000UL, 0x0029FFFUL, "ROM area" },
    { 0x002A000UL, 0x002BFFFUL, "C64 BASIC" },
    { 0x002C000UL, 0x002CFFFUL, "C65 interface" },
    { 0x002D000UL, 0x002DFFFUL, "C64 charset" },
    { 0x002E000UL, 0x002FFFFUL, "C64 KERNAL" },
    { 0x0030000UL, 0x0031FFFUL, "C65 monitor" },
    { 0x0032000UL, 0x0037FFFUL, "C65 BASIC" },
    { 0x0038000UL, 0x003BFFFUL, "C65 graphics" },
    { 0x003C000UL, 0x003DFFFUL, "ROM area" },
    { 0x003E000UL, 0x003FFFFUL, "C65 KERNAL" },
    { 0x0040000UL, 0x005FFFFUL, "chip RAM" },
    { 0x8000000UL, 0x87FFFFFUL, "attic RAM" },
    { 0xFF80000UL, 0xFF87FFFUL, "colour RAM" },
    { 0xFFD0000UL, 0xFFDFFFFUL, "I/O space" },
};

static const char *region_name(unsigned long phys) {
    unsigned char i;
    for (i = 0; i < sizeof regions / sizeof regions[0]; i++)
        if (phys >= regions[i].start && phys <= regions[i].end) return regions[i].name;
    return "nothing";
}

/* ---- presets ---- */

struct preset {
    const char *name;
    struct config c;
};

static const struct preset presets[] = {
    { "C64: BASIC, I/O, KERNAL ($37)", { 0, 0, 0, 0, 0, 0, 0x37, 0 } },
    { "C64: all RAM ($34)", { 0, 0, 0, 0, 0, 0, 0x34, 0 } },
    { "C64: the charset at $D000 ($33)", { 0, 0, 0, 0, 0, 0, 0x33, 0 } },
    { "cc45: RAM and I/O ($35)", { 0, 0, 0, 0, 0, 0, 0x35, 0 } },
    { "C65 ROMs by $D030 ($B8)", { 0, 0, 0, 0, 0, 0, 0x35, 0xB8 } },
    { "The Book's MAP: $10000, $48000", { 0x100, 0x480, 0x1, 0x3, 0, 0, 0x35, 0 } },
    { "Ethernet buffer at $6800", { 0xD80, 0, 0x8, 0, 0xFF, 0, 0x35, 0 } },
    { "Bank 4 at $8000-$BFFF", { 0, 0x380, 0, 0x3, 0, 0, 0x35, 0 } },
    { "Bank 1 at $0000-$7FFF", { 0x100, 0, 0xF, 0, 0, 0, 0x35, 0 } },
};
#define N_PRESETS (sizeof presets / sizeof presets[0])

#ifndef START_PRESET
#define START_PRESET 0      /* (-DSTART_PRESET=n starts with another) */
#endif

/* ---- text ---- */

static char line[42];
static unsigned char pos;

static void put(const char *s) {
    while (*s && pos < 40) line[pos++] = *s++;
    line[pos] = 0;
}

static void hex(unsigned long v, unsigned char digits) {
    while (digits-- && pos < 40) {
        unsigned char d = (unsigned char)(v >> (digits * 4)) & 15;
        line[pos++] = d < 10 ? '0' + d : 'A' + d - 10;
    }
    line[pos] = 0;
}

static void bin(unsigned char v, unsigned char digits) {
    while (digits-- && pos < 40) line[pos++] = (v >> digits) & 1 ? '1' : '0';
    line[pos] = 0;
}

/* ---- the editor's fields ---- */

enum { F_LO_OFF, F_LO_MB, F_LO_EN, F_HI_OFF, F_HI_MB, F_HI_EN, F_PORT, F_D030, F_ROW, N_FIELDS };

static struct config cfg;
static unsigned char field, row = 0xD, preset;

static unsigned int get_field(unsigned char f) {
    switch (f) {
        case F_LO_OFF: return cfg.lo_off;
        case F_LO_MB:  return cfg.lo_mb;
        case F_LO_EN:  return cfg.lo_en;
        case F_HI_OFF: return cfg.hi_off;
        case F_HI_MB:  return cfg.hi_mb;
        case F_HI_EN:  return cfg.hi_en;
        case F_PORT:   return cfg.port;
        case F_D030:   return cfg.d030;
        default:       return row;
    }
}

static const unsigned int field_mask[N_FIELDS] = { 0xFFF, 0xFF, 0xF, 0xFFF, 0xFF, 0xF, 0xFF, 0xFF, 0xF };

static void set_field(unsigned char f, unsigned int v) {
    v &= field_mask[f];
    switch (f) {
        case F_LO_OFF: cfg.lo_off = v; break;
        case F_LO_MB:  cfg.lo_mb = (unsigned char)v; break;
        case F_LO_EN:  cfg.lo_en = (unsigned char)v; break;
        case F_HI_OFF: cfg.hi_off = v; break;
        case F_HI_MB:  cfg.hi_mb = (unsigned char)v; break;
        case F_HI_EN:  cfg.hi_en = (unsigned char)v; break;
        case F_PORT:   cfg.port = (unsigned char)v; break;
        case F_D030:   cfg.d030 = (unsigned char)v; break;
        default:       row = (unsigned char)v; break;
    }
}

/* ---- drawing ---- */

#define TOP     4       /* the bar's and the ruler's top */
#define ROW_H   9       /* a 4 KB block's height in the bar */
#define BAR_X   36
#define BAR_W   64
#define RULER_X 196
#define RULER_W 14

/* a label for a block in the bar (at most 7 characters) */
static char ram_name[] = "RAM 0";
static const char *short_name(const struct block *k) {
    if (k->kind == K_IO) return "I/O";
    if (k->kind == K_CHAR) return "CHARSET";
    if (k->phys >= 0x2A000UL && k->phys < 0x2C000UL) return "BASIC";
    if (k->phys >= 0x2E000UL && k->phys < 0x30000UL) return "KERNAL";
    if (k->phys >= 0x3E000UL && k->phys < 0x40000UL) return "KERN65";
    if (k->phys >= 0x32000UL && k->phys < 0x38000UL) return "BASIC65";
    if (k->phys >= 0x38000UL && k->phys < 0x3C000UL) return "GRAPHIC";
    if (k->phys >= 0x2C000UL && k->phys < 0x2D000UL) return "INTERF";
    if (k->phys >= 0xFF80000UL && k->phys < 0xFF88000UL) return "COLOUR";
    if (k->phys >= 0x8000000UL && k->phys < 0x8800000UL) return "ATTIC";
    if (k->phys >= 0xFFD0000UL && k->phys < 0xFFE0000UL) return "IO SPC";
    if (k->phys < 0x60000UL) {
        ram_name[4] = '0' + (unsigned char)(k->phys >> 16);   /* (its bank) */
        return ram_name;
    }
    return "-";
}

static void text_at(unsigned char y, unsigned char colour) {
    tgi_setcolor(colour);
    tgi_outtextxy(0, y, line);
}

/* a field's value at x (in characters) of the text, highlighted when chosen */
static void field_at(unsigned char f, unsigned char x, unsigned char y, unsigned char digits, unsigned char binary) {
    pos = 0;
    if (binary) bin((unsigned char)get_field(f), digits);
    else hex(get_field(f), digits);
    if (f == field) {
        tgi_setcolor(TGI_COLOR_BLUE);
        tgi_bar(x * 8 - 1, y - 1, x * 8 + digits * 8, y + 7);
        tgi_setcolor(TGI_COLOR_WHITE);
    } else {
        tgi_setcolor(TGI_COLOR_YELLOW);
    }
    tgi_outtextxy(x * 8, y, line);
}

static void draw(void) {
    struct block k;
    unsigned char b, y;
    tgi_clear();

    /* the physical ruler: banks 0-5, 24 pixels a bank (1.5 a 4 KB block) */
    for (b = 0; b < 6; b++) {
        y = TOP + b * 24;
        tgi_setcolor(b == 2 || b == 3 ? TGI_COLOR_LIGHTBLUE : TGI_COLOR_GRAY1);
        tgi_bar(RULER_X, y, RULER_X + RULER_W, y + 23);
        tgi_setcolor(TGI_COLOR_BLACK);
        tgi_line(RULER_X, y, RULER_X + RULER_W, y);
        pos = 0;
        hex(b, 1);
        put(b == 2 || b == 3 ? " ROM" : " RAM");
        tgi_setcolor(TGI_COLOR_GRAY3);
        tgi_outtextxy(RULER_X + RULER_W + 4, y + 8, line);
    }
    tgi_setcolor(TGI_COLOR_GRAY3);
    tgi_outtextxy(RULER_X + RULER_W + 4, TOP + 6 * 24 - 8, "$5FFFF");

    /* the CPU's 64 KB, $0000 at the top */
    for (b = 0; b < 16; b++) {
        decode(&cfg, b, &k);
        y = TOP + b * ROW_H;
        pos = 0;
        hex(b, 1);
        put("000");
        tgi_setcolor(b == row ? TGI_COLOR_WHITE : TGI_COLOR_GRAY3);
        tgi_outtextxy(0, y + 1, line);
        tgi_setcolor(kind_colour[k.kind]);
        tgi_bar(BAR_X, y, BAR_X + BAR_W - 1, y + ROW_H - 2);
        tgi_setcolor(TGI_COLOR_WHITE);
        tgi_outtextxy(BAR_X + 2, y + 1, short_name(&k));
        /* where it reads from, when that is in the first 384 KB: marked on
         * the ruler in the block's colour, the line to the mark's middle */
        if (k.phys < 0x60000UL) {
            unsigned int unit = (unsigned int)(k.phys >> 12);
            unsigned char m = TOP + unit + (unit >> 1);
            tgi_setcolor(kind_colour[k.kind]);
            tgi_bar(RULER_X + 1, m, RULER_X + RULER_W - 1, m + 1);
            tgi_setcolor(b == row ? TGI_COLOR_WHITE : kind_colour[k.kind]);
            tgi_line(BAR_X + BAR_W, y + ROW_H / 2 - 1, RULER_X - 1, m + 1);
        }
    }
    /* the chosen block framed */
    y = TOP + row * ROW_H;
    tgi_setcolor(TGI_COLOR_WHITE);
    tgi_line(BAR_X - 1, y - 1, BAR_X + BAR_W, y - 1);
    tgi_line(BAR_X - 1, y + ROW_H - 1, BAR_X + BAR_W, y + ROW_H - 1);

    /* the values */
    pos = 0; put("LO OFS $    MB $   SLABS %");
    text_at(152, TGI_COLOR_GRAY3);
    field_at(F_LO_OFF, 8, 152, 3, 0);
    field_at(F_LO_MB, 16, 152, 2, 0);
    field_at(F_LO_EN, 26, 152, 4, 1);
    pos = 0; put("HI OFS $    MB $   SLABS %");
    text_at(160, TGI_COLOR_GRAY3);
    field_at(F_HI_OFF, 8, 160, 3, 0);
    field_at(F_HI_MB, 16, 160, 2, 0);
    field_at(F_HI_EN, 26, 160, 4, 1);
    pos = 0; put("$01 $     $D030 $     BLOCK $");
    text_at(168, TGI_COLOR_GRAY3);
    field_at(F_PORT, 5, 168, 2, 0);
    field_at(F_D030, 17, 168, 2, 0);
    field_at(F_ROW, 28, 168, 1, 0);

    /* the MAP for them */
    pos = 0;
    put("MAP A=$"); hex(cfg.lo_off & 0xFF, 2);
    put(" X=$"); hex((unsigned char)(cfg.lo_en << 4) | (cfg.lo_off >> 8), 2);
    put(" Y=$"); hex(cfg.hi_off & 0xFF, 2);
    put(" Z=$"); hex((unsigned char)(cfg.hi_en << 4) | (cfg.hi_off >> 8), 2);
    text_at(176, TGI_COLOR_LIGHTGREEN);

    /* the chosen block */
    decode(&cfg, row, &k);
    pos = 0;
    hex(row, 1); put("000-"); hex(row, 1); put("FFF > $"); hex(k.phys, 7);
    put(" "); put(k.kind == K_IO ? "I/O" : region_name(k.phys));
    text_at(184, TGI_COLOR_WHITE);
    /* a megabyte other than 0 is set first, by a MAP with X / Z = $0F */
    pos = 0;
    if (cfg.lo_mb || cfg.hi_mb) {
        put("FIRST MAP A=$"); hex(cfg.lo_mb, 2); put(" X=$0F Y=$"); hex(cfg.hi_mb, 2); put(" Z=$0F");
        text_at(192, TGI_COLOR_LIGHTGREEN);
    } else if ((cfg.lo_off && !cfg.lo_en) || (cfg.hi_off && !cfg.hi_en)) {
        /* (an offset moves nothing until a slab uses it) */
        put("An offset moves only the SLABS set");
        text_at(192, TGI_COLOR_ORANGE);
    } else {
        put(presets[preset].name);
        text_at(192, TGI_COLOR_GRAY2);
    }
    tgi_setcolor(TGI_COLOR_GRAY2);
    tgi_outtextxy(33 * 8, 168, "F1 HELP");
}

/* ---- the machine code that sets the configuration up ---- */

/* an instruction of the routine */
struct ins {
    unsigned char op, len;      /* the opcode, the instruction's bytes */
    unsigned int arg;           /* its operand */
    const char *m, *note;       /* the mnemonic, a comment */
};
static struct ins prog[18];
static unsigned char n_ins, n_bytes;

static void add(unsigned char op, unsigned char len, unsigned int arg, const char *m, const char *note) {
    struct ins *i = &prog[n_ins++];
    i->op = op;
    i->len = len;
    i->arg = arg;
    i->m = m;
    i->note = note;
    n_bytes += len;
}

/* The routine, run from $1600: $D030 first (while I/O is still there), then
 * $01, then MAP (a megabyte other than 0 first, by X / Z = $0F), EOM and
 * RTS. Interrupts stay off: the KERNAL's handler may be gone */
static void gen(void) {
    n_ins = n_bytes = 0;
    add(0x78, 1, 0, "SEI", "no interrupts");
    add(0xA9, 2, cfg.d030, "LDA", "");
    add(0x8D, 3, 0xD030, "STA", "the ROM bits");
    add(0xA9, 2, cfg.port, "LDA", "");
    add(0x8D, 3, 0x0001, "STA", "the CPU port");
    if (cfg.lo_mb || cfg.hi_mb) {
        add(0xA9, 2, cfg.lo_mb, "LDA", "lower megabyte");
        add(0xA2, 2, 0x0F, "LDX", "");
        add(0xA0, 2, cfg.hi_mb, "LDY", "upper megabyte");
        add(0xA3, 2, 0x0F, "LDZ", "");
        add(0x5C, 1, 0, "MAP", "the megabytes");
    }
    add(0xA9, 2, cfg.lo_off & 0xFF, "LDA", "lower offset");
    add(0xA2, 2, (unsigned char)(cfg.lo_en << 4) | (cfg.lo_off >> 8), "LDX", "slabs, offset");
    add(0xA0, 2, cfg.hi_off & 0xFF, "LDY", "upper offset");
    add(0xA3, 2, (unsigned char)(cfg.hi_en << 4) | (cfg.hi_off >> 8), "LDZ", "slabs, offset");
    add(0x5C, 1, 0, "MAP", "");
    add(0xEA, 1, 0, "EOM", "the MAP ends");
    add(0x60, 1, 0, "RTS", "");
}

/* byte k of the routine */
static unsigned char ml_byte(unsigned char k) {
    unsigned char i;
    for (i = 0; i < n_ins; i++) {
        if (k < prog[i].len) return k == 0 ? prog[i].op : k == 1 ? (unsigned char)prog[i].arg : prog[i].arg >> 8;
        k -= prog[i].len;
    }
    return 0;
}

#ifdef MAP_CHECK
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char is(unsigned char b, unsigned char kind, unsigned long phys) {
    struct block k;
    decode(&cfg, b, &k);
    return k.kind == kind && k.phys == phys;
}

static void use(unsigned char p) {
    cfg = presets[p].c;
}

int main(void) {
    unsigned char ok;
    /* the C64's $01 */
    use(0);
    OUT[0] = is(0x0, K_RAM, 0x0000) && is(0xA, K_ROM64, 0x2A000UL) && is(0xB, K_ROM64, 0x2B000UL) &&
             is(0xC, K_RAM, 0xC000) && is(0xD, K_IO, 0xFFD3000UL) && is(0xF, K_ROM64, 0x2F000UL);
    use(1);
    OUT[1] = is(0xA, K_RAM, 0xA000) && is(0xD, K_RAM, 0xD000) && is(0xE, K_RAM, 0xE000);
    use(2);
    ok = is(0xD, K_CHAR, 0x2D000UL) && is(0xA, K_ROM64, 0x2A000UL);
    cfg.port = 0x36;   /* HIRAM only: no BASIC, the KERNAL, I/O */
    OUT[2] = ok && is(0xA, K_RAM, 0xA000) && is(0xE, K_ROM64, 0x2E000UL) && is(0xD, K_IO, 0xFFD3000UL);
    /* MAP: the Book's example (A=$00 X=$11 Y=$80 Z=$34) */
    use(5);
    OUT[3] = is(0x0, K_MAPPED, 0x10000UL) && is(0x1, K_MAPPED, 0x11000UL) && is(0x2, K_RAM, 0x2000) &&
             is(0x8, K_MAPPED, 0x50000UL) && is(0xB, K_MAPPED, 0x53000UL) && is(0xC, K_RAM, 0xC000);
    /* a megabyte: the Ethernet buffer ($FFDE800) at $6800 */
    use(6);
    OUT[4] = is(0x6, K_MAPPED, 0xFFDE000UL) && is(0x7, K_MAPPED, 0xFFDF000UL) && is(0x5, K_RAM, 0x5000);
    /* $D030 above MAP, I/O above both */
    use(4);
    ok = is(0x8, K_ROM65, 0x38000UL) && is(0xA, K_ROM65, 0x3A000UL) && is(0xC, K_ROM65, 0x2C000UL) &&
         is(0xE, K_ROM65, 0x3E000UL) && is(0xD, K_IO, 0xFFD3000UL);
    cfg.hi_en = 0xF;
    cfg.hi_off = 0x380;
    cfg.d030 = 0x08;
    OUT[5] = ok && is(0x8, K_ROM65, 0x38000UL) && is(0xA, K_MAPPED, 0x42000UL) && is(0xD, K_IO, 0xFFD3000UL) &&
             is(0xF, K_MAPPED, 0x47000UL);
    /* the names */
    OUT[6] = region_name(0x2E000UL)[4] == 'K' && region_name(0xFF80000UL)[0] == 'c' &&
             region_name(0x1000000UL)[0] == 'n';
    /* the drawing: bank 4 at $8000-$BFFF */
    tgi_install(tgi_static_stddrv);
    tgi_init();
    preset = 7;
    use(7);
    draw();
    OUT[7] = tgi_getpixel(BAR_X + BAR_W - 2, TOP + 8 * ROW_H + 1) == TGI_COLOR_GREEN &&
             tgi_getpixel(BAR_X + BAR_W - 2, TOP + 0xE * ROW_H + 1) == TGI_COLOR_GRAY2 &&
             tgi_getpixel(BAR_X + BAR_W - 2, TOP + 0xD * ROW_H + 1) == TGI_COLOR_RED &&
             tgi_getpixel(RULER_X + 2, TOP + 0x40 + 0x20) == TGI_COLOR_GREEN &&    /* $40000 marked */
             tgi_getpixel(RULER_X + 2, TOP + 0x10 + 0x08 + 4) == TGI_COLOR_GRAY1;  /* bank 1 not */
    /* bank 1 at $0000-$7FFF: labelled with its bank and marked in bank 1 */
    use(8);
    OUT[8] = is(0x0, K_MAPPED, 0x10000UL) && is(0x7, K_MAPPED, 0x17000UL) && is(0x8, K_RAM, 0x8000) &&
             short_name(&(struct block){ 0x17000UL, K_MAPPED })[4] == '1';
    preset = 8;
    draw();
    OUT[8] = OUT[8] && tgi_getpixel(RULER_X + 2, TOP + 0x10 + 0x08) == TGI_COLOR_GREEN &&
             tgi_getpixel(RULER_X + 2, TOP + 0x17 + 0x0B) == TGI_COLOR_GREEN;
    tgi_unload();
    /* the routine for the Ethernet buffer: a megabyte, then the offsets */
    {
        static const unsigned char want[] = {
            0x78, 0xA9, 0x00, 0x8D, 0x30, 0xD0, 0xA9, 0x35, 0x8D, 0x01, 0x00,
            0xA9, 0xFF, 0xA2, 0x0F, 0xA0, 0x00, 0xA3, 0x0F, 0x5C,
            0xA9, 0x80, 0xA2, 0x8D, 0xA0, 0x00, 0xA3, 0x00, 0x5C, 0xEA, 0x60
        };
        unsigned char i;
        use(6);
        gen();
        ok = n_bytes == sizeof want;
        for (i = 0; i < sizeof want; i++) ok = ok && ml_byte(i) == want[i];
        /* without a megabyte: no first MAP */
        use(5);
        gen();
        OUT[9] = ok && n_bytes == 22 && ml_byte(11) == 0xA9 && ml_byte(12) == 0x00 && ml_byte(14) == 0x11 &&
                 ml_byte(16) == 0x80 && ml_byte(18) == 0x34 && ml_byte(19) == 0x5C;
    }
    OUT[10] = 0xAA;
    return 0;
}
#else
#ifdef MAP_KEYS
#include <mega65.h>
#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))
/* the virtual key register's code: row * 8 + column (KEY_ is column * 8 + row) */
#define VK(k) ((unsigned char)((((k) & 7) << 3) | ((k) >> 3)))
#define SH 0x100    /* with SHIFT: CRSR up / left, ? */
#define CT 0x200    /* with CTRL */

/* Preset 2, then the Book's MAP typed in: LO OFS 100, SLABS 1, HI OFS 480,
 * SLABS 3, $01 35; + and - on $D030, the block moved right, right, left
 * and typed (A), back up to $D030, each page by its F-key, then by ?,
 * CTRL+D, CTRL+B, CTRL+M, M and RETURN (and back each time), quit */
static const unsigned int script[] = {
    KEY_P, KEY_P,
    KEY_1, KEY_0, KEY_0, KEY_DOWN, KEY_DOWN, KEY_1,
    KEY_DOWN, KEY_4, KEY_8, KEY_0, KEY_DOWN, KEY_DOWN, KEY_3,
    KEY_DOWN, KEY_3, KEY_5,
    KEY_DOWN, KEY_PLUS, KEY_PLUS, KEY_MINUS,
    KEY_DOWN, KEY_RIGHT, KEY_RIGHT, SH | KEY_RIGHT, KEY_A,
    SH | KEY_DOWN,
    KEY_F1, KEY_SPACE, KEY_F3, KEY_SPACE, KEY_F5, KEY_SPACE, KEY_F7, KEY_SPACE,
    SH | KEY_SLASH, KEY_SPACE, CT | KEY_D, KEY_SPACE, CT | KEY_B, KEY_SPACE, CT | KEY_M, KEY_SPACE,
    KEY_M, KEY_SPACE, KEY_RETURN, KEY_SPACE,
    KEY_Q
};
static unsigned char next_key;

static void frames(unsigned char n) {
    while (n--) {
        while (REG(0xD012) != 0xF0) ;
        while (REG(0xD012) == 0xF0) ;
    }
}

/* the next key of the script pressed and let go, then read as typed */
static unsigned char get_key(void) {
    unsigned int k = next_key < sizeof script / sizeof script[0] ? script[next_key++] : KEY_Q;
    if (k & SH) REG(0xD616) = VK(KEY_LSHIFT);
    if (k & CT) REG(0xD616) = VK(KEY_CTRL);
    REG(0xD615) = VK(k & 0xFF);
    frames(3);
    REG(0xD615) = 0x7F;
    REG(0xD616) = 0x7F;
    frames(3);
    return input_wait();
}
#else
#define get_key input_wait
#endif

/* ---- the pages behind the function keys ---- */

static unsigned char py;            /* the next text line's y */
static unsigned char pages;         /* (MAP_KEYS: the pages shown, a bit each) */
static unsigned char shown;         /* (and how many times) */

static void page_start(const char *title) {
    tgi_clear();
    py = 0;
    pos = 0;
    put(title);
    text_at(py, TGI_COLOR_WHITE);
    py += 12;
}

static void say(const char *s, unsigned char colour) {
    pos = 0;
    put(s);
    text_at(py, colour);
    py += 8;
}

/* the line built in line[] */
static void say_line(unsigned char colour) {
    text_at(py, colour);
    py += 8;
}

static void page_end(void) {
    shown++;
    pos = 0;
    put("Any key: back");
    text_at(192, TGI_COLOR_GRAY2);
    get_key();
}

static void dec(unsigned char v) {
    if (v >= 100) line[pos++] = '0' + v / 100;
    if (v >= 10) line[pos++] = '0' + v / 10 % 10;
    line[pos++] = '0' + v % 10;
    line[pos] = 0;
}

/* F1: the keys and the registers */
static const char *const help[] = {
    "CRSR UP/DOWN  choose a value",
    "CRSR L/R + -  change it (OFS by 8 KB)",
    "0-9 A-F       type it in hex",
    "P preset  Q quit  F1 or ? this page",
    "F3/CTRL+D blocks  F5/CTRL+B BASIC POKEs",
    "F7/CTRL+M/M/RETURN the machine code",
    "",
    "MAP: A,X the lower 32 KB, Y,Z the upper",
    " A  offset bits 8-15 (OFS: 256s)",
    " X  bits 7-4: the slabs $6000 $4000",
    "    $2000 $0000; bits 3-0 offset 19-16",
    " Y, Z the same for $8000-$FFFF",
    " X=$0F: A is the megabyte (Z=$0F: Y)",
    " then EOM (NOP); no IRQs until then",
    "$01 bit 0 LORAM 1 HIRAM 2 CHAREN",
    " $A000 BASIC: LORAM and HIRAM",
    " $E000 KERNAL: HIRAM",
    " $D000 I/O: CHAREN, else the charset;",
    "       RAM when LORAM = HIRAM = 0",
    "$D030 bit 3 ROM8 4 ROMA 5 ROMC 7 ROME:",
    " C65 ROM at $8000 $A000 $C000 $E000",
    "Priority: $01, MAP, $D030, I/O on top",
};

static void page_help(void) {
    unsigned char i;
    page_start("MAP DECODE: KEYS AND REGISTERS");
    for (i = 0; i < sizeof help / sizeof help[0]; i++)
        say(help[i], i < 6 ? TGI_COLOR_YELLOW : TGI_COLOR_GRAY3);
    pages |= 1;
    page_end();
}

/* F3: each block in detail */
static const unsigned char kind_text[6] = {
    TGI_COLOR_GRAY3, TGI_COLOR_LIGHTGREEN, TGI_COLOR_LIGHTBLUE, TGI_COLOR_ORANGE, TGI_COLOR_PURPLE, TGI_COLOR_LIGHTRED
};
static const char *const kind_by[6] = { "-    ", "MAP  ", "$01  ", "$01  ", "$D030", "$01  " };

static void page_blocks(void) {
    struct block k;
    unsigned char b;
    page_start("EACH 4 KB BLOCK");
    say("CPU       28-BIT   BY    WHAT", TGI_COLOR_GRAY2);
    for (b = 0; b < 16; b++) {
        decode(&cfg, b, &k);
        pos = 0;
        hex(b, 1); put("000-"); hex(b, 1); put("FFF $"); hex(k.phys, 7);
        put(" "); put(kind_by[k.kind]); put(" ");
        if (k.kind == K_IO) put("I/O registers");
        else if (k.phys < 0x60000UL && !(k.phys >= 0x20000UL && k.phys < 0x40000UL)) {
            put("chip RAM bank "); hex(k.phys >> 16, 1);
        } else put(region_name(k.phys));
        say_line(b == row ? TGI_COLOR_WHITE : kind_text[k.kind]);
    }
    py += 4;
    say("BY: who put it there. $01 the C64 port,", TGI_COLOR_GRAY2);
    say("MAP the offsets, $D030 the C65 ROMs.", TGI_COLOR_GRAY2);
    pages |= 2;
    page_end();
}

/* F5: a BASIC 65 program that POKEs the routine to $1600 and SYSes it */
static void page_basic(void) {
    unsigned char i, n = 0, ln = 40;
    gen();
    page_start("BASIC 65: POKE THE ROUTINE, SYS IT");
    say("DANGEROUS: BASIC 65 needs its own map;", TGI_COLOR_ORANGE);
    say("the machine may hang or need a reset.", TGI_COLOR_ORANGE);
    say("MAP is an instruction, no POKE does it:", TGI_COLOR_GRAY3);
    say("the routine (F7) is put at $1600, run.", TGI_COLOR_GRAY3);
    py += 4;
    say("10 BANK 128", TGI_COLOR_LIGHTGREEN);
    pos = 0;
    put("20 FORI=0TO"); dec(n_bytes - 1); put(":READB:POKE$1600+I,B:NEXT");
    say_line(TGI_COLOR_LIGHTGREEN);
    say("30 SYS$1600", TGI_COLOR_LIGHTGREEN);
    while (n < n_bytes) {
        pos = 0;
        dec(ln); put(" DATA ");
        for (i = 0; i < 7 && n < n_bytes; i++, n++) {
            if (i) put(",");
            dec(ml_byte(n));
        }
        say_line(TGI_COLOR_LIGHTGREEN);
        ln += 10;
    }
    pages |= 4;
    page_end();
}

/* F7: the routine */
static void page_ml(void) {
    unsigned char i, j, a = 0;
    gen();
    page_start("THE ROUTINE (AT $1600)");
    for (i = 0; i < n_ins; i++) {
        struct ins *s = &prog[i];
        pos = 0;
        hex(0x1600 + a, 4); put(" ");
        for (j = 0; j < 3; j++) {
            if (j < s->len) hex(ml_byte(a + j), 2); else put("  ");
            put(" ");
        }
        put(s->m);
        if (s->len == 2) { put(" #$"); hex(s->arg, 2); put("  "); }
        else if (s->len == 3) { put(" $"); hex(s->arg, 4); put(" "); }
        else put("       ");
        put(s->note);
        say_line(s->op == 0x5C ? TGI_COLOR_LIGHTGREEN : TGI_COLOR_GRAY3);
        a += s->len;
    }
    py += 4;
    say("From BASIC 65: F5. From a program: as", TGI_COLOR_GRAY2);
    say("long as it is not in a block it moves.", TGI_COLOR_GRAY2);
    pages |= 8;
    page_end();
}

static unsigned char hexval(unsigned char k) {
    if (k >= '0' && k <= '9') return k - '0';
    if (k >= 'a' && k <= 'f') return k - 'a' + 10;
    if (k >= 'A' && k <= 'F') return k - 'A' + 10;
    return 255;
}

int main(void) {
    unsigned char k, d;
    unsigned int step;
    tgi_install(tgi_static_stddrv);
    if (tgi_geterror() != TGI_ERR_OK) return 1;
    tgi_init();
    preset = START_PRESET;
    cfg = presets[preset].c;
    while (1) {
        draw();
        k = get_key();
        step = field == F_LO_OFF || field == F_HI_OFF ? 0x20 : 1;   /* (the offsets by 8 KB) */
        if (k == 'q' || k == 'Q') break;
        if (k == 0x11) field = field + 1 < N_FIELDS ? field + 1 : 0;             /* CRSR down */
        else if (k == 0x91) field = field ? field - 1 : N_FIELDS - 1;            /* CRSR up */
        else if (k == 0x1D || k == '+') set_field(field, get_field(field) + step);   /* CRSR right */
        else if (k == 0x9D || k == '-') set_field(field, get_field(field) - step);   /* CRSR left */
        else if (k == 0xF1 || k == '?') page_help();
        else if (k == 0xF3 || k == 0x04) page_blocks();                       /* CTRL+D */
        else if (k == 0xF5 || k == 0x02) page_basic();                        /* CTRL+B */
        else if (k == 0xF7 || k == 0x0D || k == 'm' || k == 'M') page_ml();   /* CTRL+M, RETURN */
        else if (k == 'p' || k == 'P') {
            preset = preset + 1 < N_PRESETS ? preset + 1 : 0;
            cfg = presets[preset].c;
        } else if ((d = hexval(k)) != 255) {
            set_field(field, (get_field(field) << 4) | d);
        }
    }
#ifdef MAP_KEYS
    OUT[0] = next_key == sizeof script / sizeof script[0] && preset == 2;
    OUT[1] = cfg.lo_off == 0x100 && cfg.lo_en == 1 && cfg.lo_mb == 0 &&
             cfg.hi_off == 0x480 && cfg.hi_en == 3 && cfg.hi_mb == 0;
    OUT[2] = cfg.port == 0x35 && cfg.d030 == 1 && row == 0xA && field == F_D030;
    /* the screen drawn for them: $0000 and $8000 moved, $2000 RAM, $D000 I/O */
    OUT[3] = tgi_getpixel(BAR_X + BAR_W - 2, TOP + 0x0 * ROW_H + 1) == TGI_COLOR_GREEN &&
             tgi_getpixel(BAR_X + BAR_W - 2, TOP + 0x8 * ROW_H + 1) == TGI_COLOR_GREEN &&
             tgi_getpixel(BAR_X + BAR_W - 2, TOP + 0x2 * ROW_H + 1) == TGI_COLOR_GRAY2 &&
             tgi_getpixel(BAR_X + BAR_W - 2, TOP + 0xD * ROW_H + 1) == TGI_COLOR_RED;
#endif
    tgi_unload();
#ifdef MAP_KEYS
    OUT[4] = pages == 0x0F && shown == 10;
    /* (B and D still typed as hex digits) */
    OUT[4] = OUT[4] && hexval('b') == 11 && hexval('D') == 13;
    OUT[5] = 0xAA;
#endif
    return 0;
}
#endif
