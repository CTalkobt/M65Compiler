/*
 * forth — a tiny Forth for the MEGA65 (a calculator you can program)
 *
 * Type at the "ok" prompt; RETURN runs the line, DEL edits it, RUN/STOP or
 * BYE goes back to BASIC.
 *
 *   2 3 + 4 * .             -> 20
 *   : sq dup * ;  7 sq .    -> 49
 *   : count 5 0 do i . loop ;  count   -> 0 1 2 3 4
 *   variable x  42 x !  x @ .          -> 42
 *   hex ff . decimal  100 sqrt .       -> FF 10
 *
 * Words: + - * / mod (star-slash) 1+ 1- 2* 2/ negate abs min max sqrt
 *        dup drop swap over rot ?dup  = < > 0= and or xor invert
 *        . u. .s cr emit space page  @ ! +!  hex decimal  words bye
 *        : ; if else then do loop i begin until again
 *        variable constant
 * Cells are 16-bit signed (`signed int`: plain int is unsigned in cc45);
 * / and mod truncate toward zero; star-slash (a b c -> a*b/c) keeps a
 * 32-bit intermediate.
 *
 * How it works: a line is read and split into words. Outside a definition
 * each word runs at once; inside `: name ... ;` it is compiled into the code
 * array as threaded code: a list of opcodes, each primitive one opcode, a
 * call to another definition OP_CALL <address>, a number OP_LIT <value>,
 * IF/LOOP/UNTIL branches with their target address. The inner interpreter
 * (run()) walks that list with GCC's computed goto: a table of label
 * addresses, `goto *ops[code[ip++]]` at the end of every primitive (no
 * switch, no loop back to a dispatcher; "direct threading" of a table).
 *
 * The GCC extensions it uses (cc45 supports them):
 *   - labels as values and computed goto (&&op_add, goto *ops[...])
 *   - statement expressions: POP is ({ check; stack[--sp]; }) and can
 *     `goto underflow` out of the middle of an expression
 *   - case ranges (case '0' ... '9':) in the number parser
 *   - a nested function (word(), reading the next word of the line, sees
 *     the line pointer of the function it is in)
 *
 * Input is the KERNAL's (getchar = GETIN, PETSCII), so the screen editor's
 * keys and the KERNAL's interrupts work as usual. -DFORTH_TEST builds a test
 * that runs a script and compares the output (results at $C0C0).
 */
#include <stdio.h>

typedef signed int cell;
typedef unsigned int ucell;
typedef unsigned char uchar;     /* (a nested function's return type is one word) */

/* ------------------------------------------------------------------------ */
/* Opcodes: the order of ops[] in run()                                     */

enum {
    OP_HALT, OP_LIT, OP_CALL, OP_EXIT, OP_BRANCH, OP_0BRANCH, OP_DO, OP_LOOP, OP_I,
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD, OP_MULDIV, OP_INC, OP_DEC, OP_TWOMUL, OP_TWODIV,
    OP_NEGATE, OP_ABS, OP_MIN, OP_MAX, OP_SQRT,
    OP_DUP, OP_DROP, OP_SWAP, OP_OVER, OP_ROT, OP_QDUP,
    OP_EQ, OP_LT, OP_GT, OP_ZEQ, OP_AND, OP_OR, OP_XOR, OP_INVERT,
    OP_DOT, OP_UDOT, OP_DOTS, OP_CR, OP_EMIT, OP_SPACE, OP_PAGE,
    OP_FETCH, OP_STORE, OP_PLUSSTORE, OP_HEX, OP_DECIMAL, OP_WORDS, OP_BYE,
    N_OPS
};

/* The words (immediate ones, handled by the compiler, have no opcode) */
enum { K_PRIM, K_COLON, K_VAR, K_CONST, K_IMM };
enum { I_COLON, I_SEMI, I_IF, I_ELSE, I_THEN, I_DO, I_LOOP, I_BEGIN, I_UNTIL, I_AGAIN,
       I_VARIABLE, I_CONSTANT };

struct builtin {
    const char *name;
    unsigned char kind, op;
};
static const struct builtin builtins[] = {
    { "+", K_PRIM, OP_ADD }, { "-", K_PRIM, OP_SUB }, { "*", K_PRIM, OP_MUL },
    { "/", K_PRIM, OP_DIV }, { "mod", K_PRIM, OP_MOD }, { "*/", K_PRIM, OP_MULDIV },
    { "1+", K_PRIM, OP_INC }, { "1-", K_PRIM, OP_DEC }, { "2*", K_PRIM, OP_TWOMUL },
    { "2/", K_PRIM, OP_TWODIV }, { "negate", K_PRIM, OP_NEGATE }, { "abs", K_PRIM, OP_ABS },
    { "min", K_PRIM, OP_MIN }, { "max", K_PRIM, OP_MAX }, { "sqrt", K_PRIM, OP_SQRT },
    { "dup", K_PRIM, OP_DUP }, { "drop", K_PRIM, OP_DROP }, { "swap", K_PRIM, OP_SWAP },
    { "over", K_PRIM, OP_OVER }, { "rot", K_PRIM, OP_ROT }, { "?dup", K_PRIM, OP_QDUP },
    { "=", K_PRIM, OP_EQ }, { "<", K_PRIM, OP_LT }, { ">", K_PRIM, OP_GT },
    { "0=", K_PRIM, OP_ZEQ }, { "and", K_PRIM, OP_AND }, { "or", K_PRIM, OP_OR },
    { "xor", K_PRIM, OP_XOR }, { "invert", K_PRIM, OP_INVERT },
    { ".", K_PRIM, OP_DOT }, { "u.", K_PRIM, OP_UDOT }, { ".s", K_PRIM, OP_DOTS },
    { "cr", K_PRIM, OP_CR }, { "emit", K_PRIM, OP_EMIT }, { "space", K_PRIM, OP_SPACE },
    { "page", K_PRIM, OP_PAGE }, { "@", K_PRIM, OP_FETCH }, { "!", K_PRIM, OP_STORE },
    { "+!", K_PRIM, OP_PLUSSTORE }, { "hex", K_PRIM, OP_HEX }, { "decimal", K_PRIM, OP_DECIMAL },
    { "words", K_PRIM, OP_WORDS }, { "bye", K_PRIM, OP_BYE }, { "i", K_PRIM, OP_I },
    { ":", K_IMM, I_COLON }, { ";", K_IMM, I_SEMI }, { "if", K_IMM, I_IF },
    { "else", K_IMM, I_ELSE }, { "then", K_IMM, I_THEN }, { "do", K_IMM, I_DO },
    { "loop", K_IMM, I_LOOP }, { "begin", K_IMM, I_BEGIN }, { "until", K_IMM, I_UNTIL },
    { "again", K_IMM, I_AGAIN }, { "variable", K_IMM, I_VARIABLE },
    { "constant", K_IMM, I_CONSTANT },
};
#define N_BUILTINS (sizeof builtins / sizeof builtins[0])

/* The user's words: newest last, looked up newest first */
#define NAME_LEN 12
#define MAX_WORDS 64
struct word {
    char name[NAME_LEN];
    unsigned char kind;
    ucell val;              /* code address, variable address or constant */
};
static struct word dict[MAX_WORDS];
static unsigned char nwords;

/* ------------------------------------------------------------------------ */
/* Machine state                                                            */

#define CODE_SIZE 1024
#define SCRATCH (CODE_SIZE - 4)      /* where a word typed outside a definition runs */
static ucell code[CODE_SIZE];
static ucell here;                   /* next free code cell */

#define STACK 32
static cell stack[STACK];
static unsigned char sp;
static ucell rstack[STACK];
static unsigned char rp;

#define MEM 64
static cell mem[MEM];                /* variables: an address is an index here */
static unsigned char nvars;

static unsigned char base = 10;
static unsigned char quit;
static unsigned char error;

/* ------------------------------------------------------------------------ */
/* Output (to the screen, or to a buffer for the test)                       */

#ifdef FORTH_TEST
static char outbuf[128];
static unsigned char outlen;
static void out(char c) { if (outlen < sizeof outbuf - 1) outbuf[outlen++] = c; }
#else
static void out(char c) { putchar(c); }
#endif

static void outs(const char *s) { while (*s) out(*s++); }

/* a number in the current base, then a space */
static void outnum(cell v, unsigned char sgn) {
    char buf[18];
    unsigned char n = 0;
    ucell u = (ucell)v;
    if (sgn && v < 0) { out('-'); u = (ucell)-v; }
    do {
        unsigned char d = (unsigned char)(u % base);
        buf[n++] = d < 10 ? '0' + d : 'a' + d - 10;
        u /= base;
    } while (u);
    while (n) out(buf[--n]);
    out(' ');
}

static void fail(const char *what, const char *msg) {
    if (what) { outs(what); out(' '); }
    outs(msg);
    out('\n');
    error = 1;
}

/* integer square root (of the cell taken as unsigned) */
static ucell isqrt(ucell n) {
    ucell r = 0, bit = 0x4000;
    while (bit > n) bit >>= 2;
    while (bit) {
        if (n >= r + bit) { n -= r + bit; r = (r >> 1) + bit; }
        else r >>= 1;
        bit >>= 2;
    }
    return r;
}

static void list_words(void) {
    signed char i;
    for (i = nwords - 1; i >= 0; i--) { outs(dict[i].name); out(' '); }
    for (i = 0; i < (signed char)N_BUILTINS; i++) { outs(builtins[i].name); out(' '); }
    out('\n');
}

/* ------------------------------------------------------------------------ */
/* The inner interpreter                                                    */

static void run(ucell start) {
    /* One label per opcode, in enum order: the "threaded" dispatch table */
    static void *const ops[N_OPS] = {
        &&op_halt, &&op_lit, &&op_call, &&op_exit, &&op_branch, &&op_0branch,
        &&op_do, &&op_loop, &&op_i,
        &&op_add, &&op_sub, &&op_mul, &&op_div, &&op_mod, &&op_muldiv, &&op_inc, &&op_dec,
        &&op_twomul, &&op_twodiv, &&op_negate, &&op_abs, &&op_min, &&op_max, &&op_sqrt,
        &&op_dup, &&op_drop, &&op_swap, &&op_over, &&op_rot, &&op_qdup,
        &&op_eq, &&op_lt, &&op_gt, &&op_zeq, &&op_and, &&op_or, &&op_xor, &&op_invert,
        &&op_dot, &&op_udot, &&op_dots, &&op_cr, &&op_emit, &&op_space, &&op_page,
        &&op_fetch, &&op_store, &&op_plusstore, &&op_hex, &&op_decimal, &&op_words, &&op_bye,
    };
    ucell ip = start;
    cell a, b;

/* the next opcode: jump straight to its label */
#define NEXT goto *ops[code[ip++]]
/* pop / push with the stack checks: a statement expression may leave the
 * expression (and the function's code path) with a goto */
#define POP ({ if (sp == 0) goto underflow; stack[--sp]; })
#define PUSH(v) do { if (sp == STACK) goto overflow; stack[sp++] = (v); } while (0)
#define RPUSH(v) do { if (rp == STACK) goto overflow; rstack[rp++] = (v); } while (0)

    NEXT;

op_halt:    return;
op_lit:     PUSH((cell)code[ip]); ip++; NEXT;
op_call:    RPUSH(ip + 1); ip = code[ip]; NEXT;
op_exit:    ip = rstack[--rp]; NEXT;
op_branch:  ip = code[ip]; NEXT;
op_0branch: a = POP; ip = a ? ip + 1 : code[ip]; NEXT;
op_do:      a = POP; b = POP; RPUSH((ucell)b); RPUSH((ucell)a); NEXT;   /* limit start DO */
op_loop:    a = (cell)rstack[rp - 1] + 1;
            if (a < (cell)rstack[rp - 2]) { rstack[rp - 1] = (ucell)a; ip = code[ip]; }
            else { rp -= 2; ip++; }
            NEXT;
op_i:       if (rp < 2) goto underflow; PUSH((cell)rstack[rp - 1]); NEXT;

op_add:     b = POP; a = POP; PUSH(a + b); NEXT;
op_sub:     b = POP; a = POP; PUSH(a - b); NEXT;
op_mul:     b = POP; a = POP; PUSH(a * b); NEXT;
op_div:     b = POP; a = POP; if (!b) goto divzero; PUSH(a / b); NEXT;
op_mod:     b = POP; a = POP; if (!b) goto divzero; PUSH(a % b); NEXT;
op_muldiv:  { cell c = POP; b = POP; a = POP;          /* a*b/c, 32 bits between */
              if (!c) goto divzero;
              PUSH((cell)((signed long)a * b / c)); }
            NEXT;
op_inc:     a = POP; PUSH(a + 1); NEXT;
op_dec:     a = POP; PUSH(a - 1); NEXT;
op_twomul:  a = POP; PUSH(a << 1); NEXT;
op_twodiv:  a = POP; PUSH(a >> 1); NEXT;
op_negate:  a = POP; PUSH(-a); NEXT;
op_abs:     a = POP; PUSH(a < 0 ? -a : a); NEXT;
op_min:     b = POP; a = POP; PUSH(a < b ? a : b); NEXT;
op_max:     b = POP; a = POP; PUSH(a > b ? a : b); NEXT;
op_sqrt:    a = POP; PUSH((cell)isqrt((ucell)a)); NEXT;

op_dup:     a = POP; PUSH(a); PUSH(a); NEXT;
op_drop:    (void)POP; NEXT;
op_swap:    b = POP; a = POP; PUSH(b); PUSH(a); NEXT;
op_over:    b = POP; a = POP; PUSH(a); PUSH(b); PUSH(a); NEXT;
op_rot:     { cell c = POP; b = POP; a = POP; PUSH(b); PUSH(c); PUSH(a); } NEXT;
op_qdup:    a = POP; PUSH(a); if (a) PUSH(a); NEXT;

/* Forth's true is -1 (all bits set) */
op_eq:      b = POP; a = POP; PUSH(a == b ? -1 : 0); NEXT;
op_lt:      b = POP; a = POP; PUSH(a < b ? -1 : 0); NEXT;
op_gt:      b = POP; a = POP; PUSH(a > b ? -1 : 0); NEXT;
op_zeq:     a = POP; PUSH(a == 0 ? -1 : 0); NEXT;
op_and:     b = POP; a = POP; PUSH(a & b); NEXT;
op_or:      b = POP; a = POP; PUSH(a | b); NEXT;
op_xor:     b = POP; a = POP; PUSH(a ^ b); NEXT;
op_invert:  a = POP; PUSH(~a); NEXT;

op_dot:     a = POP; outnum(a, 1); NEXT;
op_udot:    a = POP; outnum(a, 0); NEXT;
op_dots:    { unsigned char k;
              out('<'); outnum(sp, 0); out('>'); out(' ');
              for (k = 0; k < sp; k++) outnum(stack[k], 1); }
            NEXT;
op_cr:      out('\n'); NEXT;
op_emit:    a = POP; out((char)a); NEXT;
op_space:   out(' '); NEXT;
op_page:    out(147); NEXT;                     /* PETSCII: clear the screen */

op_fetch:   a = POP; if ((ucell)a >= MEM) goto badaddr; PUSH(mem[a]); NEXT;
op_store:   a = POP; b = POP; if ((ucell)a >= MEM) goto badaddr; mem[a] = b; NEXT;
op_plusstore: a = POP; b = POP; if ((ucell)a >= MEM) goto badaddr; mem[a] += b; NEXT;
op_hex:     base = 16; NEXT;
op_decimal: base = 10; NEXT;
op_words:   list_words(); NEXT;
op_bye:     quit = 1; return;

underflow:  fail(0, "stack empty"); return;
overflow:   fail(0, "stack full"); return;
divzero:    fail(0, "division by zero"); return;
badaddr:    fail(0, "bad address"); return;
#undef POP
#undef PUSH
#undef RPUSH
#undef NEXT
}

/* ------------------------------------------------------------------------ */
/* The outer interpreter and the compiler                                    */

static unsigned char compiling;
static ucell def_start;          /* the definition being compiled: its code */
static unsigned char def_words;  /* nwords before it (dropped on an error) */
#define CSTACK 16
static ucell cstack[CSTACK];     /* IF / DO / BEGIN addresses waiting for their end */
static unsigned char csp;

static void comma(ucell v) {
    if (here < SCRATCH) code[here++] = v;
    else fail(0, "code space full");
}

static unsigned char streq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

/* a number in the current base ($ for hex, - for negative) */
static unsigned char number(const char *s, cell *v) {
    ucell n = 0;
    unsigned char b = base, neg = 0, any = 0;
    if (*s == '-' && s[1]) { neg = 1; s++; }
    if (*s == '$') { b = 16; s++; }
    for (; *s; s++) {
        unsigned char d;
        switch ((unsigned char)*s) {
            case '0' ... '9': d = *s - '0'; break;
            case 'a' ... 'f': d = *s - 'a' + 10; break;    /* PETSCII: $41-$46 */
            default: return 0;
        }
        if (d >= b) return 0;
        n = n * b + d;
        any = 1;
    }
    *v = neg ? -(cell)n : (cell)n;
    return any;
}

static void reset_after_error(void) {
    sp = 0;
    rp = 0;
    csp = 0;
    if (compiling) {             /* drop the half-made definition */
        compiling = 0;
        here = def_start;
        nwords = def_words;
    }
}

/* run code[] from a scratch area: one opcode (and its operand) then HALT */
static void exec(ucell op, ucell operand, unsigned char has_operand) {
    code[SCRATCH] = op;
    code[SCRATCH + 1] = operand;
    code[SCRATCH + 1 + has_operand] = OP_HALT;
    run(SCRATCH);
}

static struct word *new_word(const char *name, unsigned char kind, ucell val) {
    struct word *w;
    unsigned char i;
    if (nwords == MAX_WORDS) { fail(name, "dictionary full"); return 0; }
    w = &dict[nwords++];
    for (i = 0; i < NAME_LEN - 1 && name[i]; i++) w->name[i] = name[i];
    w->name[i] = 0;
    w->kind = kind;
    w->val = val;
    return w;
}

/* Interpret (or compile) one line */
static void interpret(const char *line) {
    char tok[NAME_LEN + 8];

    /* the next word of the line into t (a nested function: it reads and
     * moves `line`, the enclosing function's variable). (cc45: it works on
     * a copy of `line` and is handed tok, not reaching into the enclosing
     * function's array.) */
    uchar word(char *t) {
        const char *p = line;
        uchar n = 0;
        while (*p == ' ') p++;
        while (*p && *p != ' ') {
            char c = *p++;
            if (n < NAME_LEN + 7) t[n++] = c;
        }
        t[n] = 0;
        line = p;
        return n;
    }

    error = 0;
    while (!error && !quit && word(tok)) {
        signed char i;
        cell v;
        const struct builtin *bi = 0;
        struct word *w = 0;

        for (i = nwords - 1; i >= 0; i--)
            if (streq(dict[i].name, tok)) { w = &dict[i]; break; }
        if (!w)
            for (i = 0; i < (signed char)N_BUILTINS; i++)
                if (streq(builtins[i].name, tok)) { bi = &builtins[i]; break; }

        if (w) {
            ucell op = w->kind == K_COLON ? OP_CALL : OP_LIT;
            if (compiling) { comma(op); comma(w->val); }
            else exec(op, w->val, 1);
        } else if (bi && bi->kind == K_PRIM) {
            if (compiling) comma(bi->op);
            else exec(bi->op, 0, 0);
        } else if (bi) {
            /* the compiler's own words */
            unsigned char imm = bi->op;
            if (imm != I_COLON && imm != I_VARIABLE && imm != I_CONSTANT && !compiling) {
                fail(tok, "only in a definition");
                break;
            }
            switch (imm) {
                case I_COLON:
                    if (!word(tok)) { fail(":", "needs a name"); break; }
                    def_start = here;
                    def_words = nwords;
                    if (new_word(tok, K_COLON, here)) { compiling = 1; csp = 0; }
                    break;
                case I_SEMI:
                    if (csp) { fail(";", "unfinished if/do/begin"); break; }
                    comma(OP_EXIT);
                    compiling = 0;
                    break;
                case I_IF:                      /* 0BRANCH <then> */
                    comma(OP_0BRANCH); cstack[csp++] = here; comma(0);
                    break;
                case I_ELSE:                    /* BRANCH <then>, IF jumps here */
                    if (!csp) { fail("else", "without if"); break; }
                    comma(OP_BRANCH); comma(0);
                    code[cstack[csp - 1]] = here;
                    cstack[csp - 1] = here - 1;
                    break;
                case I_THEN:
                    if (!csp) { fail("then", "without if"); break; }
                    code[cstack[--csp]] = here;
                    break;
                case I_DO:
                    comma(OP_DO); cstack[csp++] = here;
                    break;
                case I_LOOP:
                    if (!csp) { fail("loop", "without do"); break; }
                    comma(OP_LOOP); comma(cstack[--csp]);
                    break;
                case I_BEGIN:
                    cstack[csp++] = here;
                    break;
                case I_UNTIL:
                case I_AGAIN:
                    if (!csp) { fail(tok, "without begin"); break; }
                    comma(imm == I_UNTIL ? OP_0BRANCH : OP_BRANCH); comma(cstack[--csp]);
                    break;
                case I_VARIABLE:
                    if (!word(tok)) { fail("variable", "needs a name"); break; }
                    if (nvars == MEM) { fail(tok, "no room"); break; }
                    mem[nvars] = 0;
                    new_word(tok, K_VAR, nvars++);
                    break;
                case I_CONSTANT:
                    v = ({ if (sp == 0) { fail("constant", "needs a value"); break; } stack[--sp]; });
                    if (!word(tok)) { fail("constant", "needs a name"); break; }
                    new_word(tok, K_CONST, (ucell)v);
                    break;
            }
            if (csp >= CSTACK) fail(tok, "nested too deep");
        } else if (number(tok, &v)) {
            if (compiling) { comma(OP_LIT); comma((ucell)v); }
            else exec(OP_LIT, (ucell)v, 1);
        } else {
            fail(tok, "?");
        }
    }
    if (error) reset_after_error();
}

/* ------------------------------------------------------------------------ */

#ifdef FORTH_TEST
#define OUT ((volatile unsigned char *)0xC0C0)

/* each line's output (after the earlier lines ran) */
struct check { const char *in, *want; };
static const struct check checks[] = {
    { "2 3 + 4 * .", "20 " },
    { "-7 2 / . -7 2 mod . 7 negate abs .", "-3 -1 7 " },
    { "1000 30 7 */ . 3 9 min . 3 9 max .", "4285 3 9 " },
    { ": sq dup * ;  7 sq . 12 sq .", "49 144 " },
    { ": count 5 0 do i . loop ;  count", "0 1 2 3 4 " },
    { ": sign dup 0< if drop -1 else 0= if 0 else 1 then then ;", "0< ?\n" },
    { ": sign dup 0 < if drop -1 else 0= if 0 else 1 then then ; -5 sign . 0 sign . 9 sign .", "-1 0 1 " },
    { ": down 3 begin dup . 1- dup 0= until drop ; down", "3 2 1 " },
    { "variable x  42 x !  x @ .  8 x +!  x @ .", "42 50 " },
    { "10 constant ten  ten ten * .", "100 " },
    { "hex ff . 10 . decimal 255 .", "ff 10 255 " },
    { "100 sqrt . 99 sqrt . 30000 sqrt .", "10 9 173 " },
    { "1 2 3 .s rot .s drop drop drop", "<3 > 1 2 3 <3 > 2 3 1 " },
    { "+", "stack empty\n" },
    { "frob", "frob ?\n" },
    { "1 0 /", "division by zero\n" },
    { "if", "if only in a definition\n" },
    { "-1 u. 5 2 = . 5 5 = . 6 3 and . 6 3 or . 6 3 xor .", "65535 0 -1 2 7 5 " },
    { ": fact dup 1 > if dup 1- fact * then ; 7 fact .", "5040 " },
    { "bye", "" },
};
#define N_CHECKS (sizeof checks / sizeof checks[0])

int main(void) {
    unsigned char k, i;
    for (k = 0; k < N_CHECKS; k++) {
        outlen = 0;
        interpret(checks[k].in);
        outbuf[outlen] = 0;
        OUT[k] = streq(outbuf, checks[k].want);
    }
    OUT[N_CHECKS] = quit == 1 && sp == 0;
    OUT[N_CHECKS + 1] = 0xAA;
    (void)i;
    return 0;
}

#else

#define LINE 80
static char line[LINE + 1];

/* a line from the keyboard (the KERNAL's GETIN): DEL edits; RUN/STOP quits */
static unsigned char read_line(void) {
    unsigned char n = 0;
    for (;;) {
        unsigned char c = (unsigned char)getchar();
        switch (c) {
            case 0: continue;                      /* no key yet */
            case 3: quit = 1; line[0] = 0; return 0;   /* RUN/STOP */
            case 13: line[n] = 0; putchar(' '); return 1;
            case 20:                               /* DEL */
                if (n) { n--; putchar(20); }
                continue;
            case 0xC1 ... 0xDA: c -= 0x80; break;  /* shifted letters as plain ones */
            case 0x20 ... 0x5F: break;
            default: continue;
        }
        if (n < LINE) { line[n++] = c; putchar(c); }
    }
}

static const char *const demo[] = {
    "2 3 + 4 * .",
    ": sq dup * ;  7 sq .",
    ": count 5 0 do i . loop ;  count",
    "1000 30 7 */ .  hex 2a . ff 1+ . decimal",
};

int main(void) {
    unsigned char k;
    putchar(147);
    puts("tiny forth - a programmable calculator");
    puts("words lists the words, bye or run/stop quits");
    putchar('\n');
    for (k = 0; k < sizeof demo / sizeof demo[0]; k++) {
        outs(demo[k]);
        putchar(' ');
        interpret(demo[k]);
        outs(" ok\n");
    }
    while (!quit) {
        if (!read_line()) break;
        interpret(line);
        if (!quit) outs(compiling ? " compiled\n" : " ok\n");
    }
    putchar('\n');
    return 0;
}
#endif
