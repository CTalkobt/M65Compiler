/*
 * calc_graph — a scientific calculator that plots, on a raster-split screen
 *
 * Type an expression and press RETURN: without x it is calculated
 * ("sqrt(2)*pi = 4.44288"); with x it is plotted as y = f(x) (up to four
 * curves, in four colours). Numbers (1.5, 2e3), + - * / ^ (right
 * associative), parentheses, unary minus, implicit multiplication (2x,
 * 3sin(x), 2(x+1)), x, pi, e, ans (the last result) and the functions sin cos
 * tan atn sqrt log (natural) exp abs int. Commands: "clear" (the curves and
 * the history), "range N" (x from -N to N). DEL deletes, RUN/STOP quits.
 *
 * The screen (VIC-IV, CHR16 + FCLRHI: characters above $FF are 8x8 full
 * colour blocks): rows 0-8 are text, rows 9-24 a 320x128 graph whose pixel
 * blocks are in bank 4. A raster interrupt splits it: at the graph's first
 * line it turns the border and the background navy, at the bottom border
 * black again (where it also counts the frames that time the main loop).
 * (xemu's -screenshot renders the frame again at exit without running the
 * CPU, so such a split shows in its window but not in its screenshots.)
 *
 * The interrupt: the IRQ vector at $FFFE is in the KERNAL ROM. Its entry
 * pushes A X Y Z and the base page register, sets B to 0 and jumps through
 * ($0314). An __interrupt function cannot be put there directly (it pushes
 * and pulls its own registers and ends with RTI), and the program's code may
 * not even be visible: the float routines call the BASIC ROM through the
 * KERNAL's JSRFAR, which maps the ROM over $2000-$7FFF with interrupts
 * enabled. So $0314 points at a small stub copied to $BE00 (outside that
 * range; the C stack starts below it): it passes other interrupts to the
 * KERNAL's handler, saves the mapping in use (the KERNAL keeps a copy at
 * $011C-$011F), maps $0000-$7FFF to plain RAM, enters the __interrupt
 * handler as the CPU would (return address and flags pushed), and on its
 * RTI restores the mapping and returns as the KERNAL's handler would.
 *
 * GCC extensions, each where it fits (see the comments marked GCC:):
 *   computed goto (&&label, goto *p) for the bytecode evaluator, nested
 *   functions for the expression compiler's helpers, statement expressions
 *   and typeof in macros, case ranges in the tokenizer, designated
 *   initializers for the tables, the ?: operator without a middle operand,
 *   __attribute__ ((noinline, always_inline, aligned)), plus cc45's
 *   __interrupt.
 *
 * Build with -DCALC_TEST for the test: expressions evaluated against their
 * values, the interrupt checked while floats run, results at $C0C0
 * (make check). -DCALC_AUTOQUIT=N leaves after N frames (the exit path
 * without a key: headless xemu cannot type).
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <dma.h>

#pragma cc45 stack_top(0xBDFF)   /* $BE00-$BFFF: the interrupt stub */

#define REG(a) (*(volatile unsigned char *)(a))
#define ASCIIKEY REG(0xD610)

/* GCC: statement expressions and typeof: each argument evaluated once (the
 * names differ: cc45 does not yet scope a nested statement expression's
 * variables apart from an enclosing one's of the same name) */
#define MIN(a, b) ({ typeof(a) _mn_a = (a); typeof(b) _mn_b = (b); _mn_a < _mn_b ? _mn_a : _mn_b; })
#define MAX(a, b) ({ typeof(a) _mx_a = (a); typeof(b) _mx_b = (b); _mx_a > _mx_b ? _mx_a : _mx_b; })
#define CLAMP(v, lo, hi) MIN(MAX((v), (lo)), (hi))

/* ------------------------------------------------------------------------ */
/* The expression compiler: infix text to bytecode (shunting-yard)          */

#define OP_END   0
#define OP_NUM   1      /* followed by a constant's index */
#define OP_X     2
#define OP_ADD   3
#define OP_SUB   4
#define OP_MUL   5
#define OP_DIV   6
#define OP_POW   7
#define OP_NEG   8
#define OP_SIN   9      /* the functions: OP_SIN .. OP_INT */
#define OP_COS  10
#define OP_TAN  11
#define OP_ATN  12
#define OP_SQRT 13
#define OP_LOG  14
#define OP_EXP  15
#define OP_ABS  16
#define OP_INT  17
#define OP_LPAR 18      /* only on the compiler's operator stack */
#define N_OPS   19

#define MAXCODE 64
#define MAXCONST 12

struct prog {
    unsigned char code[MAXCODE];
    float k[MAXCONST];
    unsigned char nk;
    unsigned char uses_x;
};

/* GCC (C99): designated initializers */
struct fname { const char *name; unsigned char op; };
static const struct fname fnames[] = {
    { .name = "sin", .op = OP_SIN }, { .name = "cos", .op = OP_COS },
    { .name = "tan", .op = OP_TAN }, { .name = "atn", .op = OP_ATN },
    { .name = "sqrt", .op = OP_SQRT }, { .name = "log", .op = OP_LOG },
    { .name = "exp", .op = OP_EXP }, { .name = "abs", .op = OP_ABS },
    { .name = "int", .op = OP_INT },
};
#define N_FNAMES (sizeof fnames / sizeof fnames[0])

/* binding power of the binary operators (^ binds tighter than unary minus:
 * -2^2 = -4; it is right associative) */
static const unsigned char prec[N_OPS] = {
    [OP_ADD] = 1, [OP_SUB] = 1, [OP_MUL] = 2, [OP_DIV] = 2, [OP_NEG] = 3, [OP_POW] = 4,
};

static float ans;                 /* the last result ("ans") */
static const char *err_text;      /* the last error, or 0 */

/* The compiler's state is at file scope; its nested helpers read the
 * enclosing function's s and pos through the static chain. (cc45 does not
 * yet handle nested functions that change the enclosing function's locals
 * or use its arrays and struct pointers.) */
static struct prog *out;
static unsigned char opstack[16];
static struct {
    unsigned char ncode, nops, ok;
    unsigned char want_operand;     /* the next token starts an operand */
    unsigned char depth;            /* open parentheses */
} cs;
unsigned char err_pos;              /* where the error is in the text */

/* Compile text (cc45's lowercase letters) into p. Returns 1 if it is an
 * expression, 0 with err_text set if not. */
static unsigned char compile(const char *s, struct prog *p) {
    unsigned char pos = 0;

    /* GCC: nested functions, defined inside compile(): fail() reads the
     * enclosing function's pos through the static chain */
    void fail(const char *why) {
        if (cs.ok) { err_text = why; err_pos = pos; }
        cs.ok = 0;
    }
    void emit(unsigned char op) {
        if (cs.ncode < MAXCODE - 1) out->code[cs.ncode++] = op; else fail("too long");
    }
    void emit_const(float v) {
        if (out->nk >= MAXCONST) { fail("too many numbers"); return; }
        out->k[out->nk] = v;
        emit(OP_NUM);
        emit(out->nk++);
    }
    /* pop operators that bind at least as tightly as op (right associative
     * ^ only those that bind tighter) to the output */
    void reduce(unsigned char op) {
        while (cs.nops && opstack[cs.nops - 1] != OP_LPAR) {
            unsigned char top = opstack[cs.nops - 1];
            unsigned char pt = top >= OP_SIN ? 5 : prec[top];
            if (pt < prec[op] || (pt == prec[op] && op == OP_POW)) break;
            emit(top);
            cs.nops--;
        }
    }
    void push(unsigned char op) {
        if (cs.nops < sizeof opstack) opstack[cs.nops++] = op; else fail("too deep");
    }
    /* an operand where an operator was due: implicit multiplication */
    void operand(void) {
        if (!cs.want_operand) { reduce(OP_MUL); push(OP_MUL); }
        cs.want_operand = 0;
    }
    void binary(unsigned char op) {
        if (cs.want_operand) { fail("missing number"); return; }
        reduce(op);
        push(op);
        cs.want_operand = 1;
    }

    out = p;
    cs.ncode = cs.nops = cs.depth = 0;
    cs.ok = cs.want_operand = 1;
    p->nk = 0;
    p->uses_x = 0;
    err_text = 0;
    while (cs.ok && s[pos]) {
        unsigned char c = (unsigned char)s[pos];
        switch (c) {
        case ' ':
            pos++;
            break;
        case '0' ... '9':           /* GCC: case ranges */
        case '.': {
            float v = 0, scale = 1;
            unsigned char frac = 0, digits = 0;
            operand();
            for (;; pos++) {
                c = (unsigned char)s[pos];
                if (c == '.' && !frac) { frac = 1; continue; }
                if (c < '0' || c > '9') break;
                digits++;
                if (frac) { scale = scale / 10; v = v + (c - '0') * scale; }
                else v = v * 10 + (c - '0');
            }
            if (!digits) { fail("bad number"); break; }
            if (s[pos] == 'e' && (s[pos + 1] >= '0' && s[pos + 1] <= '9' || s[pos + 1] == '-')) {
                signed char e = 0, sign = 1;
                pos++;
                if (s[pos] == '-') { sign = -1; pos++; }
                while (s[pos] >= '0' && s[pos] <= '9') e = e * 10 + (s[pos++] - '0');
                while (e-- > 0) v = sign > 0 ? v * 10 : v / 10;
            }
            emit_const(v);
            break;
        }
        case 'a' ... 'z': {          /* GCC: case ranges */
            char word[8];
            unsigned char n = 0, i;
            while (s[pos] >= 'a' && s[pos] <= 'z') {
                if (n < sizeof word - 1) word[n++] = s[pos];
                pos++;
            }
            word[n] = 0;
            operand();
            if (!strcmp(word, "x")) { emit(OP_X); p->uses_x = 1; break; }
            if (!strcmp(word, "pi")) { emit_const(3.14159265f); break; }
            if (!strcmp(word, "e")) { emit_const(2.71828183f); break; }
            if (!strcmp(word, "ans")) { emit_const(ans); break; }
            for (i = 0; i < N_FNAMES; i++)
                if (!strcmp(word, fnames[i].name)) break;
            if (i == N_FNAMES) { fail("unknown name"); break; }
            while (s[pos] == ' ') pos++;
            if (s[pos] != '(') { fail("( after function"); break; }
            push(fnames[i].op);     /* applied when its ) closes */
            cs.want_operand = 1;
            break;
        }
        case '(':
            operand();
            push(OP_LPAR);
            cs.depth++;
            cs.want_operand = 1;
            pos++;
            break;
        case ')':
            if (cs.want_operand || !cs.depth) { fail(cs.depth ? "missing number" : "unbalanced )"); break; }
            reduce(OP_ADD);                 /* everything back to the ( */
            cs.nops--;                         /* the ( */
            cs.depth--;
            if (cs.nops && opstack[cs.nops - 1] >= OP_SIN && opstack[cs.nops - 1] <= OP_INT) emit(opstack[--cs.nops]);
            pos++;
            break;
        case '+':
            pos++;
            if (!cs.want_operand) binary(OP_ADD);      /* a leading + does nothing */
            break;
        case '-':
            pos++;
            if (cs.want_operand) push(OP_NEG); else binary(OP_SUB);
            break;
        case '*': pos++; binary(OP_MUL); break;
        case '/': pos++; binary(OP_DIV); break;
        case '^': pos++; binary(OP_POW); break;
        default:
            fail("unexpected character");
        }
    }
    if (cs.ok && cs.want_operand) fail("missing number");
    if (cs.ok && cs.depth) fail("missing )");
    if (cs.ok) {
        while (cs.nops) emit(opstack[--cs.nops]);
        emit(OP_END);
    }
    return cs.ok;
}

/* ------------------------------------------------------------------------ */
/* The evaluator: bytecode on a stack of floats                             */

static unsigned char eval_error;

/* x ^ y: whole exponents by multiplying (exact: 2^10 = 1024, (-2)^3 = -8),
 * others through powf (exp and log) */
static float power(float b, float y) {
    signed int n = (signed int)y;
    if ((float)n == y && n > -64 && n < 64) {
        float r = 1;
        unsigned char i, m = n < 0 ? -n : n;
        for (i = 0; i < m; i++) r = r * b;
        if (n < 0) {
            if (r == 0) { eval_error = 1; return 0; }
            r = 1 / r;
        }
        return r;
    }
    if (b < 0) { eval_error = 1; return 0; }
    return powf(b, y);
}

/* GCC: __attribute__((noinline)): one copy of the evaluator and its tables */
__attribute__((noinline))
static float eval(const struct prog *p, float x) {
    /* GCC: labels as values (&&label) in a table indexed by the opcode,
     * with designated initializers; each operation ends with goto *: one
     * indirect jump per operation instead of a switch's compare chain */
    static void *const ops[N_OPS] = {
        [OP_END] = &&op_end, [OP_NUM] = &&op_num, [OP_X] = &&op_x,
        [OP_ADD] = &&op_add, [OP_SUB] = &&op_sub, [OP_MUL] = &&op_mul,
        [OP_DIV] = &&op_div, [OP_POW] = &&op_pow, [OP_NEG] = &&op_neg,
        [OP_SIN] = &&op_sin, [OP_COS] = &&op_cos, [OP_TAN] = &&op_tan,
        [OP_ATN] = &&op_atn, [OP_SQRT] = &&op_sqrt, [OP_LOG] = &&op_log,
        [OP_EXP] = &&op_exp, [OP_ABS] = &&op_abs, [OP_INT] = &&op_int,
    };
    float st[16], *sp = st, a = 0;      /* a: the top of the stack */
    const unsigned char *pc = p->code;
    eval_error = 0;
    errno = 0;
#define NEXT goto *ops[*pc++]
    NEXT;
op_num:  *sp++ = a; a = p->k[*pc++]; NEXT;
op_x:    *sp++ = a; a = x; NEXT;
op_add:  a = *--sp + a; NEXT;
op_sub:  a = *--sp - a; NEXT;
op_mul:  a = *--sp * a; NEXT;
op_div:  --sp;
         if (a == 0) eval_error = 1; else a = *sp / a;
         NEXT;
op_pow:  a = power(*--sp, a); NEXT;
op_neg:  a = -a; NEXT;
op_sin:  a = sinf(a); NEXT;
op_cos:  a = cosf(a); NEXT;
op_tan:  a = tanf(a); NEXT;
op_atn:  a = atanf(a); NEXT;
op_sqrt: if (a < 0) eval_error = 1; else a = sqrtf(a); NEXT;
op_log:  if (a <= 0) eval_error = 1; else a = logf(a); NEXT;
op_exp:  if (a > 88) eval_error = 1; else a = expf(a); NEXT;
op_abs:  a = fabsf(a); NEXT;
op_int:  a = floorf(a); NEXT;
op_end:
#undef NEXT
    if (errno) eval_error = 1;
    return a;
}

#ifndef CALC_TEST
/* ------------------------------------------------------------------------ */
/* The screen                                                               */

#define CW 40
#define TEXT_ROWS 9                     /* rows 0-8: text */
#define GRAPH_ROW 9                     /* rows 9-24: the graph */
#define GRAPH_CROWS 16
#define GW 320
#define GH (GRAPH_CROWS * 8)
#define PIX_LIN   0x0040000UL           /* the graph's pixel blocks (bank 4) */
#define COLOR_LIN 0xFF80000UL

/* 16-bit character numbers: in BASIC's 80-column screen memory (unused
 * while the program runs; cleared when it ends) */
#define screen ((unsigned int *)0x0800)

/* the graph's colours: palette entries 16-23 (BASIC's text uses 0-15) */
#define GC_BACK  0                      /* transparent: the background colour */
#define GC_GRID  17
#define GC_AXIS  18
#define GC_CURVE 19                     /* 19-22: the four curves */
#define GC_C2    20
#define GC_C3    21
#define GC_C4    22
#define GC_MARK  23

/* (R, G, B, 0-15) for entries 16-23 */
static const unsigned char graph_pal[8][3] = {
    [0] = { 0, 1, 4 }, [1] = { 1, 3, 7 }, [2] = { 9, 10, 12 },
    [3] = { 15, 14, 3 }, [4] = { 3, 14, 15 }, [5] = { 15, 6, 12 }, [6] = { 5, 15, 5 },
    [7] = { 15, 15, 15 },
};
/* the curves' colours in the text area (BASIC's palette) */
static const unsigned char curve_text_colour[4] = { 7, 3, 10, 13 };

static unsigned char scode(unsigned char c) {
    if (c >= 0x40 && c < 0x60) return c - 0x40;
    if (c >= 0x60 && c < 0x80) return c - 0x20;
    return c;
}
static void put(unsigned char x, unsigned char y, const char *s, unsigned char colour) {
    unsigned int i = y * CW + x;
    while (*s && x++ < CW) {
        screen[i] = scode((unsigned char)*s++);
        dma_fill_far(COLOR_LIN + i * 2 + 1, 1, colour);
        i++;
    }
}
static void clear_row(unsigned char y) {
    unsigned char x;
    for (x = 0; x < CW; x++) screen[y * CW + x] = ' ';
}
#endif /* !CALC_TEST */

/* ------------------------------------------------------------------------ */
/* The raster interrupt                                                     */

#define SPLIT_LINE (51 + 9 * 8)         /* the graph's first line (text row 9) */
#define GC_NAVY  16                     /* the graph area's background colour */
#define BOTTOM_LINE 251                 /* the bottom border */
#define STUB ((unsigned char *)0xBE00)

volatile unsigned int frames;           /* counted by the interrupt */
static unsigned char at_split;          /* the next interrupt is the split */
static unsigned char kernal_line;       /* the raster line the KERNAL used */
static unsigned char measured;
static unsigned char old_vector[2], old_d01a;

/* cc45: __interrupt saves A X Y Z and B, runs on its own base page, RTI */
__interrupt void raster_isr(void) {
    if (!measured) {
        kernal_line = REG(0xD012);      /* where the KERNAL's interrupt came */
        measured = 1;
    }
    if (at_split) {
        REG(0xD020) = GC_NAVY;          /* the graph's border and background */
        REG(0xD021) = GC_NAVY;
        REG(0xD012) = BOTTOM_LINE;
        at_split = 0;
    } else {
        REG(0xD020) = 0;                /* black again from the bottom border */
        REG(0xD021) = 0;                /* to the split */
        REG(0xD012) = SPLIT_LINE;
        at_split = 1;
        frames++;                       /* once a frame, in the border */
    }
    REG(0xD019) = 0x01;                 /* acknowledge the raster interrupt */
}

static void put16(unsigned char *p, unsigned int v) { p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); }

/* The stub at $0314's target (see the top of the file), assembled by hand:
 * it is copied to $BE00 and patched with the handler's address. */
static void irq_install(void) {
    static const unsigned char stub[] = {
        0xAD, 0x19, 0xD0,       /* +0   lda $D019        VIC interrupt flags  */
        0x29, 0x01,             /* +3   and #$01         raster?              */
        0xD0, 0x03,             /* +5   bne +3                                */
        0x6C, 0x00, 0x00,       /* +7   jmp (old)        no: the KERNAL's     */
        0xA2, 0x03,             /* +10  ldx #3           the mapping in use   */
        0xBD, 0x1C, 0x01,       /* +12  lda $011C,x      (the KERNAL's copy)  */
        0x48,                   /* +15  pha              saved on the stack   */
        0xCA,                   /* +16  dex                                   */
        0x10, 0xF9,             /* +17  bpl +12                               */
        0xA9, 0x00,             /* +19  lda #0           $0000-$7FFF: RAM     */
        0xAA,                   /* +21  tax                                   */
        0xAC, 0x1E, 0x01,       /* +22  ldy $011E        $8000-$FFFF as is    */
        0xAB, 0x1F, 0x01,       /* +25  ldz $011F                             */
        0x5C,                   /* +28  map                                   */
        0xEA,                   /* +29  eom                                   */
        0xA9, 0x00,             /* +30  lda #>tail       as an interrupt      */
        0x48,                   /* +32  pha              enters: the return   */
        0xA9, 0x00,             /* +33  lda #<tail       address and flags    */
        0x48,                   /* +35  pha                                   */
        0x08,                   /* +36  php                                   */
        0x4C, 0x00, 0x00,       /* +37  jmp _raster_isr                       */
        0x68,                   /* +40  tail: pla        the saved mapping    */
        0xFA,                   /* +41  plx                                   */
        0x7A,                   /* +42  ply                                   */
        0xFB,                   /* +43  plz                                   */
        0x5C,                   /* +44  map                                   */
        0xEA,                   /* +45  eom                                   */
        0x68, 0x5B,             /* +46  pla, tab         the KERNAL's pushes: */
        0xFB, 0x7A, 0xFA, 0x68, /* +48  plz ply plx pla  B Z Y X A            */
        0x40,                   /* +52  rti                                   */
    };
    unsigned int tail = 0xBE00 + 40;
    memcpy(STUB, stub, sizeof stub);
    STUB[53] = REG(0x0314);             /* the KERNAL's handler, for jmp (old) */
    STUB[54] = REG(0x0315);
    put16(STUB + 8, 0xBE00 + 53);
    STUB[31] = (unsigned char)(tail >> 8);
    STUB[34] = (unsigned char)tail;
    put16(STUB + 38, (unsigned int)raster_isr);

    __asm__("sei");
    old_vector[0] = REG(0x0314);
    old_vector[1] = REG(0x0315);
    old_d01a = REG(0xD01A);
    measured = 0;
    at_split = 1;
    REG(0xD05D) = REG(0xD05D) & 0x7F;   /* hot registers off: $D011 writes */
    REG(0xD011) = REG(0xD011) & 0x7F;   /* leave the VIC-IV set-up alone   */
    REG(0x0314) = 0x00;
    REG(0x0315) = 0xBE;
    REG(0xD01A) = 0x01;                 /* raster interrupts only */
    REG(0xD019) = 0x0F;
    __asm__("cli");
}

static void irq_remove(void) {
    __asm__("sei");
    REG(0x0314) = old_vector[0];
    REG(0x0315) = old_vector[1];
    REG(0xD012) = kernal_line;
    REG(0xD01A) = old_d01a & 0x0F;
    REG(0xD019) = 0x0F;
    __asm__("cli");
}

static void wait_frames(unsigned int n) {
    unsigned int t = frames + n;
    while ((signed int)(frames - t) < 0) ;
}

#ifdef CALC_TEST
/* ------------------------------------------------------------------------ */
/* The test: expressions and their values; the interrupt during floats       */

#define OUT ((volatile unsigned char *)0xC0C0)

struct check { const char *text; float x, want; };
static const struct check checks[] = {
    { "1+2*3", 0, 7 }, { "(1+2)*3", 0, 9 }, { "2^10", 0, 1024 }, { "2^3^2", 0, 512 },
    { "-2^2", 0, -4 }, { "(-2)^3", 0, -8 }, { "10/4", 0, 2.5f }, { "3--2", 0, 5 },
    { "2x", 3, 6 }, { "2(3+4)", 0, 14 }, { "x^2-1", -3, 8 }, { "sqrt(16)+abs(-1)", 0, 5 },
    { "sin(0)+cos(0)", 0, 1 }, { "exp(0)+log(1)", 0, 1 }, { "1.5e2", 0, 150 },
    { "int(2.7)+int(-2.5)", 0, -1 }, { "3sin(x)", 0, 0 }, { "pi*2", 0, 6.2831853f },
    { "2^-2", 0, 0.25f }, { "atn(1)*4", 0, 3.1415927f },
};
static const char *const bad[] = { "1+", "(2", "2)", "foo(1)", "sin 1", "1 2 +", "*3", "" };

static unsigned char near(float a, float b) {
    float d = a - b;
    if (d < 0) d = -d;
    return d < 0.0001f + (b < 0 ? -b : b) * 0.00001f;
}

int main(void) {
    static struct prog p;
    unsigned char i, ok = 1, n = 0;
    float before[8], during;
    unsigned int f0;
    for (i = 0; i < sizeof checks / sizeof checks[0]; i++) {
        unsigned char r = compile(checks[i].text, &p);
        float v = r ? eval(&p, checks[i].x) : 0;
        OUT[i] = r && !eval_error && near(v, checks[i].want);
        if (!OUT[i]) ok = 0;
    }
    n = i;
    for (i = 0; i < sizeof bad / sizeof bad[0]; i++)
        OUT[n + i] = !compile(bad[i], &p) && err_text != 0;      /* each an error */
    n += i;
    compile("1/0", &p);
    eval(&p, 0);
    OUT[n++] = eval_error;                          /* division by zero */
    compile("sqrt(x)", &p);
    eval(&p, -1);
    OUT[n++] = eval_error;                          /* a negative root */
    /* the interrupt: running while the ROM does the floats (the program's
     * $2000-$7FFF mapped away), and the results the same */
    compile("sin(x)*x^2+sqrt(abs(x))/3", &p);
    for (i = 0; i < 8; i++) before[i] = eval(&p, i * 0.7f);
    irq_install();
    f0 = frames;
    for (i = 0; i < 8; i++) {
        unsigned char k;
        for (k = 0; k < 20; k++) {
            during = eval(&p, i * 0.7f);
            if (during != before[i]) ok = 0;
        }
    }
    OUT[n++] = ok && measured && frames - f0 > 5;   /* the split ran meanwhile */
    irq_remove();
    OUT[n++] = REG(0x0314) == old_vector[0] && REG(0x0315) == old_vector[1];
    OUT[n] = 0xAA;
    return 0;
}

#else
/* ------------------------------------------------------------------------ */
/* The graph                                                                */

#define MAXF 4
#define NOVALUE 255
static struct prog curves[MAXF];
/* each curve's pixel row + 1 by column: 0 above the graph, GH + 1 below */
static unsigned char ys[MAXF][GW];
static unsigned char ncurves, next_curve;
static float xrange = 8;                /* x from -xrange to xrange */
static signed int trace_x = -1;         /* the marker's column */
static unsigned char trace_curve;

/* GCC: __attribute__((aligned)): a cell's pixels on a 64-byte boundary */
static unsigned char colbuf[GRAPH_CROWS][64] __attribute__((aligned(64)));
/* the grid: which columns and rows have lines (set by grid()) */
static unsigned char gridx[GW / 8];     /* a bit per pixel column */
static unsigned char gridy[GH];         /* 0, GC_GRID or GC_AXIS */

static float x_at(signed int col) { return (col - GW / 2) * xrange / (GW / 2); }
static float scale(void) { return (GW / 2) / xrange; }      /* pixels per unit */

static void compute_curve(unsigned char f) {
    signed int c;
    float s = scale();
    for (c = 0; c < GW; c++) {
        float y = eval(&curves[f], x_at(c));
        if (eval_error || y > 1000 || y < -1000) ys[f][c] = NOVALUE;
        /* (signed bounds: a plain int constant is unsigned in cc45) */
        else ys[f][c] = (unsigned char)CLAMP((signed int)(GH / 2 - y * s) + 1, (signed int)0, (signed int)(GH + 1));
    }
}

/* grid lines every 1, 2 or 5 units, the axes through 0 */
static void grid(void) {
    signed int unit = (signed int)scale(), i, step;
    step = unit * (xrange > 20 ? 5 : (xrange > 8 ? 2 : 1));
    memset(gridx, 0, sizeof gridx);
    memset(gridy, 0, sizeof gridy);
    for (i = GW / 2 % step; i < GW; i += step) gridx[i >> 3] |= 0x80 >> (i & 7);
    for (i = GH / 2 % step; i < GH; i += step) gridy[i] = GC_GRID;
    gridy[GH / 2] = GC_AXIS;
}

/* GCC: __attribute__((always_inline)) */
__attribute__((always_inline))
static inline void pixel(unsigned char x, unsigned char y, unsigned char c) {
    colbuf[y >> 3][(y & 7) * 8 + x] = c;
}

/* one 8-pixel column of cells: background, grid, axes, curves, the marker */
static void draw_column(unsigned char cc) {
    unsigned char px, f, r, y;
    for (r = 0; r < GRAPH_CROWS; r++) memset(colbuf[r], GC_BACK, 64);
    for (y = 0; y < GH; y++)            /* the horizontal lines, dotted */
        if (gridy[y]) for (px = 0; px < 8; px += gridy[y] == GC_AXIS ? 1 : 2) pixel(px, y, gridy[y]);
    for (px = 0; px < 8; px++) {
        signed int col = cc * 8 + px;
        if (gridx[cc] & (0x80 >> px)) { /* a vertical line: the axis solid */
            unsigned char c = col == GW / 2 ? GC_AXIS : GC_GRID;
            for (y = 0; y < GH; y += c == GC_AXIS ? 1 : 2) pixel(px, y, c);
        }
        /* the curves: a vertical run from the last column's row to this one */
        for (f = 0; f < ncurves; f++) {
            unsigned char y1 = ys[f][col], y0 = col ? ys[f][col - 1] : y1;
            if (y1 == NOVALUE) continue;
            if (y0 == NOVALUE) y0 = y1;
            {
                unsigned char lo = MIN(y0, y1), hi = MAX(y0, y1);
                for (y = MAX(lo, 1); y <= MIN(hi, GH); y++) pixel(px, y - 1, GC_CURVE + f);
            }
        }
        if (col == trace_x && trace_curve < ncurves) {
            unsigned char ty = ys[trace_curve][col];
            if (ty != NOVALUE)
                for (y = MAX(ty, 3) - 2; y <= MIN(ty + 2, GH); y++) pixel(px, y - 1, GC_MARK);
        }
    }
    for (r = 0; r < GRAPH_CROWS; r++)
        dma_copy_far(PIX_LIN + ((unsigned long)r * CW + cc) * 64, (unsigned long)(unsigned int)colbuf[r], 64);
}

static void draw_graph(void) {
    unsigned char c;
    grid();
    for (c = 0; c < CW; c++) draw_column(c);
}

/* ------------------------------------------------------------------------ */
/* The text area: history, the marker's position, the input line            */

#define HIST 6
static char hist[HIST][CW + 1];
static unsigned char hist_colour[HIST];
static char input[CW - 2];
static unsigned char ilen;

static void show_history(void) {
    unsigned char i;
    for (i = 0; i < HIST; i++) {
        clear_row(1 + i);
        put(0, 1 + i, hist[i], hist_colour[i]);
    }
}
static void add_history(const char *s, unsigned char colour) {
    unsigned char i;
    for (i = 0; i < HIST - 1; i++) {
        strcpy(hist[i], hist[i + 1]);
        hist_colour[i] = hist_colour[i + 1];
    }
    strncpy(hist[HIST - 1], s, CW);
    hist[HIST - 1][CW] = 0;
    hist_colour[HIST - 1] = colour;
    show_history();
}

static void show_input(unsigned char cursor) {
    clear_row(8);
    put(0, 8, ">", 1);
    put(2, 8, input, 1);
    if (cursor) screen[8 * CW + 2 + ilen] = 0xA0;      /* a reverse space */
}

static void show_trace(void) {
    char buf[CW + 1];
    clear_row(7);
    if (trace_x < 0 || trace_curve >= ncurves) return;
    {
        float x = x_at(trace_x), y = eval(&curves[trace_curve], x);
        if (eval_error) sprintf(buf, "y%d: x=%-8.4g undefined", trace_curve + 1, x);
        else sprintf(buf, "y%d: x=%-8.4g y=%.5g", trace_curve + 1, x, y);
    }
    put(0, 7, buf, curve_text_colour[trace_curve]);
}

/* a line typed: a command, a calculation or a curve */
static void enter(const char *line) {
    char buf[CW + 8];
    static struct prog p;
    if (!strcmp(line, "clear")) {
        unsigned char i;
        ncurves = next_curve = 0;
        trace_x = -1;
        for (i = 0; i < HIST; i++) hist[i][0] = 0;
        show_history();
        draw_graph();
        return;
    }
    if (!strncmp(line, "range ", 6)) {
        if (compile(line + 6, &p) && !p.uses_x) {
            float r = eval(&p, 0);
            if (!eval_error && r >= 1 && r <= 100) {
                unsigned char f;
                xrange = r;
                for (f = 0; f < ncurves; f++) compute_curve(f);
                draw_graph();
                sprintf(buf, "x from %g to %g", -r, r);
                add_history(buf, 15);
                return;
            }
        }
        add_history("range: a number from 1 to 100", 2);
        return;
    }
    if (!compile(line, &p)) {
        /* GCC: ?: with the middle operand left out (err_text if not 0) */
        sprintf(buf, "%.20s: %s", line, err_text ?: "error");
        add_history(buf, 2);
        return;
    }
    if (p.uses_x) {
        unsigned char f = next_curve;
        curves[f] = p;
        if (ncurves < MAXF) ncurves++;
        next_curve = (unsigned char)((next_curve + 1) % MAXF);
        compute_curve(f);
        trace_curve = f;
        sprintf(buf, "y%d = %.34s", f + 1, line);
        add_history(buf, curve_text_colour[f]);
        draw_graph();
    } else {
        float v = eval(&p, 0);
        if (eval_error) sprintf(buf, "%.24s = undefined", line);
        else { ans = v; sprintf(buf, "%.22s = %.9g", line, v); }
        add_history(buf, 1);
    }
}

/* ------------------------------------------------------------------------ */
/* The display set-up (as examples/c/mandelbrot)                            */

/* the VIC registers changed (restored in the reverse order: $D031 last) */
static const unsigned int vregs[] = {
    0xD031, 0xD05D, 0xD054, 0xD058, 0xD059, 0xD05E, 0xD060, 0xD061, 0xD062, 0xD063,
    0xD064, 0xD065, 0xD020, 0xD021,
};
#define N_VREGS (sizeof vregs / sizeof vregs[0])
static unsigned char saved[N_VREGS], saved_pal[3][8];

/* entries 16-23 of the palette in use (a register holds its 8-bit value
 * with the nibbles swapped: a 4-bit component c is c * $11 either way) */
static void palette(unsigned char set) {
    unsigned char i, k;
    for (i = 0; i < 8; i++)
        for (k = 0; k < 3; k++) {
            volatile unsigned char *r = (volatile unsigned char *)(0xD110 + k * 0x100 + i);
            if (set) { saved_pal[k][i] = *r; *r = (unsigned char)(graph_pal[i][k] * 0x11); }
            else *r = saved_pal[k][i];
        }
}

static void video_on(void) {
    unsigned int i;
    for (i = 0; i < N_VREGS; i++) saved[i] = REG(vregs[i]);
    palette(1);                                       /* the graph's colours */

    for (i = 0; i < CW * 25; i++) screen[i] = ' ';
    for (i = 0; i < CW * GRAPH_CROWS; i++) screen[GRAPH_ROW * CW + i] = (unsigned int)(PIX_LIN / 64 + i);
    dma_fill_far(PIX_LIN, CW * GRAPH_CROWS * 64, GC_BACK);
    dma_fill_far(COLOR_LIN, CW * 25 * 2, 0);

    REG(0xD020) = 0;
    REG(0xD021) = 0;
    REG(0xD031) = REG(0xD031) & 0x7F;                 /* 40 columns */
    REG(0xD05D) = REG(0xD05D) & 0x7F;                 /* hot registers off */
    REG(0xD054) = REG(0xD054) | 0x05;                 /* CHR16 + FCLRHI */
    REG(0xD058) = CW * 2;
    REG(0xD059) = 0;
    REG(0xD05E) = CW;
    REG(0xD060) = 0x00;                               /* the screen at $0800 */
    REG(0xD061) = 0x08;
    REG(0xD062) = 0;
    REG(0xD063) = REG(0xD063) & 0xF0;
    REG(0xD064) = 0;
    REG(0xD065) = 0;
}

static void video_off(void) {
    unsigned char i = N_VREGS;
    palette(0);
    while (i--) REG(vregs[i]) = saved[i];
    putchar(0x93);                                    /* clear BASIC's screen */
}

/* ------------------------------------------------------------------------ */
/* Main: a demo to start with, then the keyboard                            */

static const char *const demo[] = {
    "sqrt(2)*pi", "2^16-1", "sin(x)*3", "x^2/4-3", "2cos(x/2)+exp(-x*x)", 0,
};

int main(void) {
    unsigned char i, blink = 0;
    unsigned int last;
    video_on();
    irq_install();
    put(0, 0, "calc graph - type an expression, return", 3);
    show_history();
    draw_graph();
    for (i = 0; demo[i]; i++) enter(demo[i]);
    trace_x = 0;
    show_input(1);
    while (ASCIIKEY) ASCIIKEY = 0;
    last = frames;
    for (;;) {
        unsigned char c;
        /* the interrupt counts the frames: 50 (or 60) a second */
        while (frames == last) ;
        last = frames;
        if ((last & 15) == 0) show_input(blink ^= 1);
        /* the marker walks along the newest curve */
        if (ncurves && (last & 1) == 0) {
            signed int old = trace_x;
            trace_x = trace_x + 2 >= GW ? 0 : trace_x + 2;
            draw_column((unsigned char)(old >> 3));
            if ((trace_x >> 3) != (old >> 3)) draw_column((unsigned char)(trace_x >> 3));
            if ((last & 7) == 0) show_trace();
        }
#ifdef CALC_AUTOQUIT
        if (last > CALC_AUTOQUIT) break;             /* the exit path's test */
#endif
        if ((c = ASCIIKEY) == 0) continue;
        ASCIIKEY = 0;
        if (c == 0x03) break;                         /* RUN/STOP */
        switch (c) {
        case 0x0D:                                    /* RETURN */
            if (ilen) {
                enter(input);
                ilen = 0;
                input[0] = 0;
            }
            break;
        case 0x08: case 0x14: case 0x7F:              /* DEL */
            if (ilen) input[--ilen] = 0;
            break;
        /* the ASCII queue's letters (either case) as cc45's lowercase */
        case 0x41 ... 0x5A:                           /* GCC: case ranges */
        case 0x61 ... 0x7A:
            c = (unsigned char)((c & 0x1F) | 0x40);
            /* fall through */
        case 0x20 ... 0x3F:
        case 0x5E:                                    /* ^ */
            if (ilen < sizeof input - 2) { input[ilen++] = c; input[ilen] = 0; }
            break;
        }
        show_input(1);
    }
    irq_remove();
    video_off();
    while (ASCIIKEY) ASCIIKEY = 0;
    return 0;
}
#endif
