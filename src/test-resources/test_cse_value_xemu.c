/* test_cse_value_xemu.c — an element address reused across blocks when its
 * index is the same value (#361): &mons[m - 1] in two blocks (each with its
 * own zext and subtract of m) is computed once; not when m changes between
 * the two, nor for a variable whose address is taken; an address hoisted
 * out of a loop
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the issue's case: !mons[m - 1].awake && habits[mons[m - 1].kind]
 *   [1] the parameter assigned between the two uses: the new element
 *   [2] a local whose address is taken, changed through a pointer between
 *   [3] mons[m - 1] read in a loop that calls a function changing mons
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define N __attribute__((noinline))
struct mon { unsigned char x, y, kind, hp, awake, stun, carry; };
struct mon mons[24];
unsigned char habits[16];
unsigned char out;

N void f(unsigned char m) {
    out = 0;
    if (!mons[m - 1].awake && (habits[mons[m - 1].kind] & 16)) out = 1;
}

N unsigned char g(unsigned char m) {
    unsigned char a = mons[m - 1].kind;
    if (a == 3) m += 2;                 /* m changes on this path */
    return a + mons[m - 1].kind;
}

N void bump(unsigned char *p) { *p += 1; }

N unsigned char h(unsigned char k) {
    unsigned char m = k, a;
    a = mons[m - 1].hp;
    bump(&m);                           /* m changes through its address */
    if (a) a += mons[m - 1].hp;
    return a;
}

N unsigned char heal(unsigned char m) { mons[m - 1].hp += 10; return mons[m - 1].hp; }

N unsigned char loop(unsigned char m) {
    unsigned char i, s = 0;
    for (i = 0; i < 4; i++) s += mons[m - 1].hp + heal(m);
    return s;
}

int main(void) {
    unsigned char i;
    for (i = 0; i < 24; i++) {
        mons[i].kind = i % 16;
        mons[i].awake = i & 1;
        mons[i].hp = i;
    }
    for (i = 0; i < 16; i++) habits[i] = (i == 4) ? 16 : 0;
    f(5);                               /* mons[4]: asleep, kind 4: habits 16 */
    OUT[0] = out == 1;
    f(6);                               /* mons[5]: awake */
    OUT[0] = OUT[0] && out == 0;
    OUT[1] = g(4) == 3 + 5 && g(2) == 1 + 1;   /* mons[3].kind 3, then mons[5]; mons[1] twice */
    OUT[2] = h(7) == 6 + 7 && h(1) == 0;       /* mons[6].hp + mons[7].hp; mons[0].hp 0 */
    /* mons[2].hp 2: each pass hp, then heal (+10): 2+12, 12+22, 22+32, 32+42 */
    OUT[3] = loop(3) == (unsigned char)(14 + 34 + 54 + 74) && mons[2].hp == 42;
    OUT[4] = 0xAA;
    return 0;
}
