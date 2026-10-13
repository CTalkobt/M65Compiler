/* test_switch_continue_xemu.c — continue <value> / continue default in a switch
 *
 * continue <value> jumps to the case of the innermost switch that <value>
 * selects (as if the switch ran again with it); continue default jumps to
 * default, or past the switch when there is none.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] a state machine of constant continues
 *   [1] a run-time value, a case range, continue default
 *   [2] continue default without a default case leaves the switch
 *   [3] char selector; plain continue inside the switch still means the loop
 *   [4] a nested switch: continue <value> belongs to the inner one
 *   [5] a signed selector (-fsigned-default not needed: declared signed)
 *   [6] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static int trace(int start) {
    int t = 0;
    switch (start) {
    case 1: t = t * 10 + 1; continue 3;
    case 2: t = t * 10 + 2; break;
    case 3: t = t * 10 + 3; continue 2;
    default: t = t * 10 + 9;
    }
    return t;
}

static int ranged(int v, signed int next) {
    int t = 0;
    switch (v) {
    case 0 ... 9: t += 1; if (next >= 0) { v = next; next = -1; continue v; } break;
    case 10: t += 10; continue default;
    case 20: t += 20; break;
    default: t += 100;
    }
    return t;
}

static int nodefault(int v) {
    int t = 0;
    switch (v) {
    case 1: t = 1; continue default;
    case 2: t = 2; break;
    }
    return t + 50;
}

static int loopy(void) {
    unsigned char c;
    int i, t = 0;
    for (i = 0; i < 4; i++) {
        c = (unsigned char)i;
        switch (c) {
        case 0: t += 1; continue;          /* the loop's continue */
        case 1: t += 10; continue 3;
        case 2: t += 100; break;
        case 3: t += 1000; break;
        }
        t += 5;
    }
    return t;   /* i=0: 1; i=1: 10+1000+5; i=2: 100+5; i=3: 1000+5 */
}

static int nested(int a, int b) {
    int t = 0;
    switch (a) {
    case 1:
        switch (b) {
        case 1: t += 1; continue 2;
        case 2: t += 2; break;
        }
        t += 10;
        break;
    case 2: t += 100; break;
    }
    return t;
}

static int signedsel(signed int v) {
    switch (v) {
    case -5: return 1;
    case -1: continue -5;
    default: return 0;
    }
}

int main(void) {
    OUT[0] = trace(1) == 132 && trace(3) == 32 && trace(2) == 2 && trace(7) == 9;
    OUT[1] = ranged(5, 20) == 21 && ranged(5, 7) == 2 && ranged(10, -1) == 110 && ranged(3, 99) == 101;
    OUT[2] = nodefault(1) == 51 && nodefault(2) == 52 && nodefault(3) == 50;
    OUT[3] = loopy() == 1 + 1015 + 105 + 1005;
    OUT[4] = nested(1, 1) == 13 && nested(1, 2) == 12 && nested(2, 0) == 100;
    OUT[5] = signedsel(-1) == 1 && signedsel(-5) == 1 && signedsel(3) == 0;
    OUT[6] = 0xAA;
    return 0;
}
