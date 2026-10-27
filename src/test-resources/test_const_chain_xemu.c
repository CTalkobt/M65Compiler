/* test_const_chain_xemu.c — constant add / subtract chains folded (#351,
 * #353): a global struct's array member at a constant index is one
 * absolute address; x - 25 + 23 is x - 2 at every width, wrapping as the
 * type does; and functions whose callers test their result (a char's by
 * the flags) next to void and int ones (ca45's return information).
 *
 * Results at $C0C0 (01 = pass):
 *   +0 pl.items[7] read, decremented, written; pl.items[0] and [19]
 *   +1 unsigned char chains, wrapping below 0 and above 255
 *   +2 int / signed int / long chains, wrapping too
 *   +3 pointer chains (p + 3 + 4, &a[2] + 5 - 1)
 *   +4 a char result tested directly, a void call between, an int result
 *   +5 a void function's loop whose X only the body reads: the knowledge
 *      kept at a merge label went stale once the ldx #0 before it went
 *   +6 $AA
 */
#define OUT ((volatile unsigned char *)0xC0C0)

struct player {
    unsigned char x, y;
    unsigned char hp, maxhp, mp, maxmp;
    unsigned char level, wdice, wsides, wplus, def;
    unsigned char items[20];
};
struct player pl;
volatile unsigned char vb;
volatile unsigned int vw;
volatile long vl;
unsigned char arr[16];
unsigned char hits;

static unsigned char get_item(void) { return pl.items[7]; }
static void dec_item(void) { pl.items[7]--; }
static unsigned char nonzero(unsigned char v) { return v != 0; }
static void touch(void) { hits++; }
static unsigned int twice(unsigned int v) { return v + v; }

/* (calc_graph's operator reduction) */
struct prog { unsigned char code[32]; };
static struct prog prog1, *out;
static unsigned char opstack[16];
static struct { unsigned char ncode, nops, ok; } cs;
static const char *err;
static const unsigned char prec[19] = { [1] = 1, [2] = 1, [3] = 2, [4] = 2, [6] = 3, [7] = 4 };
static unsigned char run(unsigned char op) {
    unsigned char pos = 0;
    void fail(const char *why) { if (cs.ok) err = why; pos = 1; cs.ok = 0; }
    void emit(unsigned char o) { if (cs.ncode < 31) out->code[cs.ncode++] = o; else fail("long"); }
    void reduce(unsigned char op) {
        while (cs.nops && opstack[cs.nops - 1] != 18) {
            unsigned char top = opstack[cs.nops - 1];
            unsigned char pt = top >= 9 ? 5 : prec[top];
            if (pt < prec[op] || (pt == prec[op] && op == 7)) break;
            emit(top);
            cs.nops--;
        }
    }
    reduce(op);
    return pos;
}

int main(void) {
    unsigned char i, ok, c;
    signed int s;
    unsigned char *p;

    for (i = 0; i < 20; i++) pl.items[i] = (unsigned char)(i * 3 + 1);
    dec_item();
    OUT[0] = get_item() == 7 * 3 && pl.items[0] == 1 && pl.items[19] == 58 && pl.def == 0;

    vb = 1;
    ok = (unsigned char)(vb - 25 + 23) == 255;
    vb = 250;
    ok = ok && (unsigned char)(vb + 10 - 3) == 1 && (unsigned char)(vb - 250 + 6) == 6;
    OUT[1] = ok;

    vw = 1;
    ok = (unsigned int)(vw - 25 + 23) == 0xFFFF;
    s = (signed int)vw;
    ok = ok && s - 25 + 23 == -1 && s + 30000 + 30000 - 59999 == 2;
    vl = 70000L;
    ok = ok && vl - 70001L + 3 == 2 && vl + 100000L - 1 == 169999L;
    OUT[2] = ok;

    for (i = 0; i < 16; i++) arr[i] = i;
    p = arr;
    p = p + 3;
    ok = *(p + 4) == 7 && *(&arr[2] + 5 - 1) == 6;
    OUT[3] = ok;

    ok = 1;
    vb = 0;
    if (nonzero(vb)) ok = 0;
    touch();
    vb = 9;
    if (!nonzero(vb)) ok = 0;
    c = nonzero(vb) ? 3 : 4;
    ok = ok && c == 3 && hits == 1 && twice(vw) == 2 && twice(0x8000) == 0;
    OUT[4] = ok;

    out = &prog1;
    opstack[0] = 18; opstack[1] = 1; opstack[2] = 3; opstack[3] = 6;
    cs.nops = 4; cs.ok = 1;
    ok = run(1) == 0;
    OUT[5] = ok && cs.nops == 1 && cs.ncode == 3 && prog1.code[0] == 6 && prog1.code[1] == 3 && prog1.code[2] == 1;

    OUT[6] = 0xAA;
    return 0;
}
