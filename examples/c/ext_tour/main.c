/* ext_tour — a tour of cc45's extensions to C
 *
 * Each section does a small job with one extension (or a family of them)
 * and prints what it got, then "ok" or "FAIL"; the last line counts them
 * ("21/21 EXTENSIONS WORK"). Read it top to bottom: every use is marked
 * with a comment "EXT:" naming the extension, and doc/bin/cc45.md has the
 * details. Extensions other examples show already are not repeated here:
 * computed goto, nested functions, statement expressions and case ranges
 * (calc_graph, forth), __interrupt (calc_graph, tunes), __far functions
 * and code overlays (far_functions, overlays), string prefixes (most of
 * them).
 *
 * make check builds it with both calling conventions and runs both in
 * xemu (__fastcall__ matters in the stack-convention build).
 */
#include <stdio.h>
#include <string.h>

static unsigned char passed, total;

static void report(const char *name, const char *what, int ok) {
    total++;
    if (ok) passed++;
    printf("%-21s %-46s %s\n", name, what, ok ? "ok" : "FAIL");
}

static char line[48];

/* --- struct methods and operator overloading --------------------------- */

/* EXT: struct methods. A function inside a struct is a method; `this` (or
 * __this) is the object it is called on. */
struct Money {
    signed long cents;
    void add(signed long c) { this->cents += c; }
    unsigned char whole() { return (unsigned char)(this->cents / 100); }
    /* EXT: operator overloading. a + b on two Money values calls this. */
    struct Money operator+(struct Money o) {
        struct Money r;
        r.cents = this->cents + o.cents;
        return r;
    }
    int operator<(struct Money o) { return this->cents < o.cents; }
};

static int tour_methods(void) {
    struct Money wallet, gift, sum;
    wallet.cents = 1250;
    wallet.add(399);                     /* a method call: Money__add(&wallet, 399) */
    gift.cents = 500;
    sum = wallet + gift;                 /* operator+ */
    sprintf(line, "12.50 + 3.99 + 5.00 = %u.%02u", sum.whole(), (unsigned)(sum.cents % 100));
    report("methods, operators", line, sum.cents == 2149 && gift < wallet && !(sum < gift));
    return 0;
}

/* --- break N ------------------------------------------------------------ */

static const unsigned char room[4][8] = {
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 9, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
};

static void tour_break_n(void) {
    unsigned char x, y;
    for (y = 0; y < 4; y++)
        for (x = 0; x < 8; x++)
            if (room[y][x] == 9)
                break 2;                 /* EXT: break N leaves both loops at once */
    sprintf(line, "the 9 is at x %u, y %u", x, y);
    report("break N", line, x == 5 && y == 2);
}

/* --- continue <value> in a switch ----------------------------------------- */

/* A state machine that sums "12+30-2": each state names the next one with
 * continue <value>, which re-enters the switch at that case. */
enum { NUMBER, OPERATOR, DONE };

static signed int sum_expression(const char *s) {
    signed int acc = 0, cur = 0;
    char op = '+';
    unsigned char state = NUMBER;
    switch (state) {
    case NUMBER:
        cur = 0;
        while (*s >= '0' && *s <= '9') cur = cur * 10 + (*s++ - '0');
        acc = op == '+' ? acc + cur : acc - cur;
        continue (*s ? OPERATOR : DONE); /* EXT: continue <value> */
    case OPERATOR:
        op = *s++;
        continue NUMBER;
    default:                             /* (DONE) */
        break;
    }
    return acc;
}

static void tour_continue_value(void) {
    signed int r = sum_expression("12+30-2");
    sprintf(line, "\"12+30-2\" by a state machine = %d", r);
    report("continue <value>", line, r == 40);
}

/* --- repeat -------------------------------------------------------------- */

static unsigned char reverse_bits(unsigned char b) {
    unsigned char r = 0;
    repeat(8) {                          /* EXT: repeat(N): 8 copies, no loop counter */
        r = (unsigned char)(r << 1 | (b & 1));
        b >>= 1;
    }
    return r;
}

static void tour_repeat(void) {
    unsigned char squares[4];
    repeat(unsigned char i, 4) squares[i - 1] = i * i;   /* EXT: repeat(type var, N) */
    sprintf(line, "reverse $%02x = $%02x, squares %u %u %u %u", 0x1d, reverse_bits(0x1d),
            squares[0], squares[1], squares[2], squares[3]);
    report("repeat", line, reverse_bits(0x1d) == 0xb8 && squares[3] == 16);
}

/* --- __likely / __unlikely, __builtin_expect -------------------------------- */

static unsigned int checksum(const unsigned char *p, unsigned char n, unsigned char *bad) {
    unsigned int sum = 0;
    unsigned char i;
    for (i = 0; i < n; i++) {
        /* EXT: __unlikely: the rare case's code is placed after the
         * function's return, the common one falls straight through */
        if (__unlikely(p[i] == 0xff)) {
            (*bad)++;
            continue;
        }
        sum += p[i];
    }
    return sum;
}

static void tour_likely(void) {
    static const unsigned char data[] = {10, 20, 0xff, 30, 40, 0xff, 50};
    unsigned char bad = 0;
    unsigned int s = checksum(data, sizeof data, &bad);
    if (__builtin_expect(s > 100, 1))    /* EXT: __builtin_expect, as __likely */
        sprintf(line, "sum %u, %u bad bytes skipped", s, bad);
    report("__likely, __unlikely", line, s == 150 && bad == 2);
}

/* --- __pure ------------------------------------------------------------- */

/* EXT: __pure: no side effects, so the optimizer may reuse a result
 * (vowels(w) + vowels(w) is one call) or hoist a call out of a loop */
__pure static unsigned char vowels(const char *s) {
    unsigned char n = 0;
    for (; *s; s++)
        if (strchr("aeiou", *s)) n++;
    return n;
}

static void tour_pure(void) {
    static const char word[] = "extensions";
    unsigned char twice = vowels(word) + vowels(word);
    sprintf(line, "vowels in \"%s\" counted twice = %u", word, twice);
    report("__pure", line, twice == 8);
}

/* --- __zp ----------------------------------------------------------------- */

/* EXT: __zp: in the program's base page, every access a 2-byte instruction */
__zp unsigned int ticks;
static __zp unsigned char history[4] = {7, 7, 7, 7};

static void tour_zp(void) {
    unsigned char i;
    for (i = 0; i < 200; i++) {
        ticks += 3;
        history[i & 3] = (unsigned char)ticks;
    }
    sprintf(line, "600 ticks counted, last bytes %u %u", history[2], history[3]);
    report("__zp variables", line, ticks == 600 && history[3] == (600 & 0xff));
}

/* --- __cpu and __flags ------------------------------------------------- */

#pragma cc45 recurse                    /* (a recursive function: a stack frame) */
static unsigned char depth(unsigned char n) { return n ? depth((unsigned char)(n - 1)) + 1 : 0; }

static void tour_cpu(void) {
    unsigned int before, after;
    unsigned char a, carry;
    before = __cpu.SP;                   /* EXT: __cpu: the CPU's registers */
    depth(5);
    after = __cpu.SP;                    /* the stack is balanced again */
    __flags.Carry = 1;                   /* EXT: __flags: SEC */
    asm("lda #$10\n adc #$05");          /* 16 + 5 + carry */
    a = __cpu.A;
    __flags.Carry = 0;                   /* CLC */
    carry = __flags.Carry;
    sprintf(line, "SP same after calls, $10+$05+C = $%02x", a);
    report("__cpu, __flags", line, before == after && a == 0x16 && !carry);
}

/* --- __fastcall__ -------------------------------------------------------- */

/* EXT: __fastcall__: this one function takes its arguments in the base
 * page's parameter block even in a file built with -fno-zpcall (make check
 * builds both); in the default ZP build it changes nothing */
__fastcall__ static unsigned int scaled(unsigned int x, unsigned char k) { return x * k; }

static void tour_fastcall(void) {
    unsigned int r = scaled(123, 4);
    sprintf(line, "scaled(123, 4) = %u, %s convention build", r,
#ifdef __CC45_STACK__  /* (cc45 -fno-zpcall; __CC45_ZPCALL__ otherwise) */
            "stack"
#else
            "ZP"
#endif
    );
    report("__fastcall__", line, r == 492);
}

/* --- #pragma encoding ---------------------------------------------------- */

#pragma encoding screencode             /* EXT: the literals that follow are screen codes */
static const char banner_sc[] = "TOUR";
#pragma encoding petscii
static const char banner_pet[] = "TOUR";

static void tour_encoding(void) {
    sprintf(line, "\"TOUR\" as screen codes %u %u %u %u", banner_sc[0], banner_sc[1], banner_sc[2],
            banner_sc[3]);
    report("#pragma encoding", line, banner_sc[0] == 20 && banner_sc[3] == 18 && banner_pet[0] == 0xd4);
}

/* --- #pragma cc45 optimize(off) ------------------------------------------- */

/* EXT: optimize(off) ... optimize(on): this function is compiled as
 * written, to read its code or step through it in a debugger */
#pragma cc45 optimize(off)
static unsigned int triangle(unsigned char n) {
    unsigned int t = 0;
    unsigned char i;
    for (i = 1; i <= n; i++) t = t + i;
    return t;
}
#pragma cc45 optimize(on)

static void tour_optimize(void) {
    unsigned int t = triangle(20);
    sprintf(line, "1 + 2 + ... + 20 = %u, compiled as written", t);
    report("optimize(off)", line, t == 210);
}

/* --- __noescape ------------------------------------------------------------ */

/* EXT: __noescape: the address never leaves this file's code, so in a code
 * overlay (#pragma cc45 overlay) or a __far function's chunk the string is
 * kept with the code instead of in resident memory; in a resident file
 * like this one it is an ordinary constant */
static const char __noescape motto[] = "kept with its code";

static void tour_noescape(void) {
    sprintf(line, "\"%s\", %u bytes", motto, (unsigned)sizeof motto);
    report("__noescape", line, strlen(motto) == 18);
}

/* --- GCC vectors ---------------------------------------------------------- */

/* EXT: GCC vector types: an operation works on every element */
typedef unsigned char rgba __attribute__((vector_size(4)));

static void tour_vectors(void) {
    rgba orange = {240, 120, 0, 255}, blue = {0, 64, 200, 255};
    rgba mix = (orange >> 1) + (blue >> 1);   /* each channel averaged */
    sprintf(line, "orange+blue averaged: %u %u %u %u", mix[0], mix[1], mix[2], mix[3]);
    report("vector_size", line, mix[0] == 120 && mix[1] == 92 && mix[2] == 100 && mix[3] == 254);
}

/* --- wide integers ---------------------------------------------------------- */

static void tour_wide(void) {
    unsigned long long f = 1;            /* EXT: long long: 64 bits (struct __int64) */
    unsigned char i;
    __int(32) small = -5;                /* EXT: __int(N): __int(32) is signed long */
    __int128 big = 1;                    /* EXT: __int128 */
    for (i = 2; i <= 20; i++) f *= i;
    big <<= 100;
    sprintf(line, "20! = $%08lx%08lx", (unsigned long)(f >> 32), (unsigned long)f);
    report("long long, __int(N)", line,
           f == 2432902008176640000ULL && (unsigned long)(big >> 98) == 4 && small < 0);
}

/* --- _Complex ------------------------------------------------------------- */

static void tour_complex(void) {
    _Complex float p = 3.0 + 4.0i;       /* EXT: _Complex and imaginary literals */
    _Complex float q = p * 1.0i;         /* turned a quarter: -4 + 3i */
    float re = __real__ q, im = __imag__ q;
    sprintf(line, "(3+4i) * i = %d%+di", (int)re, (int)im);
    report("_Complex", line, re == -4.0f && im == 3.0f);
}

/* --- GCC builtins ---------------------------------------------------------- */

static void tour_builtins(void) {
    unsigned int sum;
    unsigned char ovf = __builtin_add_overflow(60000u, 6000u, &sum);  /* EXT: overflow builtins */
    sprintf(line, "popcount 7, clz 4, bswap $3412, overflow %u", ovf);
    report("__builtin_*", line,
           __builtin_popcount(0xb5) == 5 && __builtin_clz(0x0800) == 4 &&   /* EXT: bit builtins */
           __builtin_ctz(0x40) == 6 && __builtin_bswap16(0x1234) == 0x3412 && ovf &&
           __builtin_constant_p(42) && !__builtin_constant_p(ticks));
}

/* --- #pragma weak --------------------------------------------------------- */

/* EXT: #pragma weak: a default another file may replace with its own
 * definition (the linker takes that one) */
#pragma weak
unsigned char tour_theme(void) { return 1; }

static void tour_weak(void) {
    sprintf(line, "default tour_theme() = %u (weak)", tour_theme());
    report("#pragma weak", line, tour_theme() == 1);
}

/* --- #pragma push_macro / pop_macro --------------------------------------- */

#define LIMIT 10
#pragma push_macro("LIMIT")             /* EXT: save the definition */
#undef LIMIT
#define LIMIT 99
static const unsigned char inner_limit = LIMIT;
#pragma pop_macro("LIMIT")              /* ... and get it back */
static const unsigned char outer_limit = LIMIT;

static void tour_push_macro(void) {
    sprintf(line, "LIMIT %u inside, %u restored", inner_limit, outer_limit);
    report("push/pop_macro", line, inner_limit == 99 && outer_limit == 10);
}

/* The MEGA65's ASCII font (BASIC 65's FONT A, at $29000 in the ROM: it has
 * the _ that every name here needs) and the upper/lower case set */
static void ascii_font(void) {
    volatile unsigned char *vic = (volatile unsigned char *)0xd000;
    vic[0x68] = 0x00;                   /* character generator base $029000 */
    vic[0x69] = 0x90;
    vic[0x6a] = 0x02;
    putchar(14);                        /* lower case (the set's second half) */
}

int main(void) {
    ascii_font();
    printf("ext_tour: cc45's extensions to C, one per line\n\n");
    tour_methods();
    tour_break_n();
    tour_continue_value();
    tour_repeat();
    tour_likely();
    tour_pure();
    tour_zp();
    tour_cpu();
    tour_fastcall();
    tour_encoding();
    tour_optimize();
    tour_noescape();
    tour_vectors();
    tour_wide();
    tour_complex();
    tour_builtins();
    tour_weak();
    tour_push_macro();
    printf("\n%u/%u extensions work\n", passed, total);
    return passed == total ? 0 : 1;
}
