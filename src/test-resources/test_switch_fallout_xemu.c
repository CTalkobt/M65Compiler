/* test_switch_fallout_xemu.c — a switch whose last case runs off its end
 * (no break) inside a loop: the switch's end is the loop's next iteration.
 * IRBuilder's branch to the switch's end had its label in the wrong operand,
 * so the code generator dropped it; while the end's block was there the
 * code fell into it, but branch folding removes that block (a lone branch)
 * and the last case then fell into whatever came next (the loop's exit)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] default last, no break: every character copied (a while loop)
 *   [1] a case last, no break (a for loop, the default first)
 *   [2] a do-while loop, the default last after a fall-through case
 *   [3] nested: the inner switch's last case falls out into the outer one's
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char buf[16];

/* the ROM's command-string builder's shape (basic45's sendp): escapes, the
 * rest copied by the default */
static unsigned char build(const unsigned char *t, unsigned char count) {
    unsigned char n = 0, c;
    while (count--) {
        c = *t++;
        switch (c) {
        case 1: buf[n++] = 'a'; break;
        case 2: if (n) n--; break;
        default: buf[n++] = c;
        }
    }
    return n;
}

static unsigned char last_case(const unsigned char *t, unsigned char count) {
    unsigned char n = 0, i;
    for (i = 0; i < count; i++) {
        switch (t[i]) {
        default: n += 1; break;
        case 7: n += 10; break;
        case 9: n += 100;
        }
    }
    return n;
}

static unsigned int do_loop(unsigned char k) {
    unsigned int s = 0;
    do {
        switch (k & 3) {
        case 0: s += 1;
        /* fall through */
        case 1: s += 2; break;
        default: s += 4;
        }
    } while (--k);
    return s;
}

static unsigned char nested(unsigned char a, unsigned char b) {
    unsigned char r = 0, i;
    for (i = 0; i < 4; i++) {
        switch (a) {
        case 1: r += 1; break;
        default:
            switch (b) {
            case 2: r += 2; break;
            default: r += 3;
            }
        }
    }
    return r;
}

int main(void) {
    static const unsigned char s1[] = {'x', 1, 'y', 2, 'z', 'w'};
    static const unsigned char s2[] = {7, 9, 3, 9, 7};
    unsigned char n, i;

    n = build(s1, sizeof s1);
    /* x a y (backspace) z w -> "xazw"... 'y' removed: "xa" + "zw" */
    OUT[0] = n == 4 && buf[0] == 'x' && buf[1] == 'a' && buf[2] == 'z' && buf[3] == 'w';

    OUT[1] = last_case(s2, sizeof s2) == 10 + 100 + 1 + 100 + 10;

    /* k = 5..1: 5&3=1:+2, 4&3=0:+3, 3:+4, 2:+4, 1:+2 */
    OUT[2] = do_loop(5) == 2 + 3 + 4 + 4 + 2;

    i = nested(0, 2);
    n = nested(0, 5);
    OUT[3] = i == 8 && n == 12 && nested(1, 0) == 4;

    OUT[4] = 0xAA;
    return 0;
}
