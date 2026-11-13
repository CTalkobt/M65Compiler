/* test_hoist_addr_xemu.c — an element address every arm of a switch (or
 * both arms of an if) computes is computed once before the branch (#566),
 * and is not when the index may change on the way to an arm
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the issue's key dispatch: each case works on units[u]
 *   [1] both arms of an if / else, and an else-if chain
 *   [2] an arm that changes the index before it uses it; an index assigned
 *       in a later condition of the chain
 *   [3] an arm with a loop of its own that moves the index; the dispatch in
 *       a loop that moves it between rounds
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

typedef struct { unsigned char where, kind, ref, job, jt, jg, x, y; unsigned int a, b, c, d, e, f, g, h; } unit_t;
unit_t units[12];
unsigned char list[8], sel, joined, nkeys;
const unsigned char *keys;

__attribute__((noinline)) void join(unsigned char u) { joined = u; units[u].where = 2; }
__attribute__((noinline)) unsigned char key(void) { return keys[nkeys++]; }

__attribute__((noinline)) void dispatch(void) {
    unsigned char u, k;
    for (;;) {
        k = key();
        u = list[sel];
        switch (k) {
        case 't':
            if (units[u].where == 1) join(u);
            if (units[u].where != 2) break;
            units[u].jt = 7;
            break;
        case 'o':
            if (units[u].where != 2) break;
            units[u].where = 1;
            break;
        case 'a':
            if (units[u].where == 3) units[u].where = 1;
            break;
        case 'n':
            sel++;
            break;
        case 27:
            return;
        }
    }
}

__attribute__((noinline)) void both(unsigned char u, unsigned char c) {
    if (c == 1) units[u].a = 0x1111;
    else if (c == 2) units[u].a += 0x0101;
    else units[u].b = units[u].a + c;
}

__attribute__((noinline)) void moved(unsigned char u, unsigned char k, unsigned char v) {
    switch (k) {
    case 1:
        u++;
        units[u].x = 0x51;        /* the next unit */
        break;
    case 2:
        units[u].x = 0x52;
        break;
    default:
        if (((u = v), k == 9)) units[u].x = 0x59;
        else units[u].y = 0x5A;
        break;
    }
}

__attribute__((noinline)) unsigned char walk(unsigned char u, unsigned char k) {
    unsigned char n = 0;
    switch (k) {
    case 1:
        while (units[u].kind) { units[u].kind = 0; u++; n++; }   /* the arm moves the index */
        break;
    case 2:
        units[u].kind = 9;
        n = 1;
        break;
    }
    return n;
}

__attribute__((noinline)) unsigned char rounds(unsigned char k) {
    unsigned char u, n = 0;
    for (u = 4; u < 8; u++) {
        if (k & 1) units[u].ref = u; else units[u].job = u + 1;
        k >>= 1;
        n += units[u].ref + units[u].job;
    }
    return n;
}

int main(void) {
    unsigned char ok, i;

    list[0] = 3; list[1] = 5;
    units[3].where = 1; units[5].where = 3;
    keys = (const unsigned char *)"taonotat\033";
    dispatch();
    /* t: unit 3 joined (where 2), jt 7; a: nothing; o: where 1; n: unit 5; o: nothing (3);
     * t: where 3: nothing; a: where 1; t: joined (2), jt 7 */
    OUT[0] = units[3].where == 1 && units[3].jt == 7 && units[5].where == 2 && units[5].jt == 7 &&
             joined == 5 && nkeys == 9 && units[4].where == 0 && units[4].jt == 0;

    both(2, 1);
    ok = units[2].a == 0x1111;
    both(2, 2);
    ok = ok && units[2].a == 0x1212;
    both(2, 7);
    OUT[1] = ok && units[2].b == 0x1219 && units[1].a == 0 && units[3].a == 0;

    moved(6, 1, 0);
    ok = units[7].x == 0x51 && units[6].x == 0;
    moved(6, 2, 0);
    ok = ok && units[6].x == 0x52;
    moved(6, 9, 8);
    ok = ok && units[8].x == 0x59 && units[6].x == 0x52;
    moved(6, 3, 9);
    OUT[2] = ok && units[9].y == 0x5A && units[6].y == 0 && units[9].x == 0;

    units[9].kind = 1; units[10].kind = 1;
    ok = walk(9, 1) == 2 && units[9].kind == 0 && units[10].kind == 0;
    ok = ok && walk(11, 2) == 1 && units[11].kind == 9 && walk(0, 3) == 0;
    for (i = 4; i < 8; i++) { units[i].ref = 0; units[i].job = 0; }
    /* k = 0101b: units 4 and 6 get ref, 5 and 7 get job */
    OUT[3] = ok && rounds(5) == 4 + 6 + 6 + 8 && units[4].ref == 4 && units[5].job == 6 &&
             units[6].ref == 6 && units[7].job == 8 && units[5].ref == 0;

    OUT[4] = 0xAA;
    return 0;
}
