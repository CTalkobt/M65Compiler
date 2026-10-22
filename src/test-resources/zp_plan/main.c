/* zp_plan/main.c — ln45's program-wide base-page plan (#304): built twice,
 * first with ln45 --zp-plan-out, then with cc45 -fzp-plan (test_xemu.sh)
 *
 * many() keeps ten pointers live across calls (20 bytes: more than a
 * function's block holds without a plan); chain() calls walk() calls step(),
 * each with pointers of its own, all active together.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] many(): the bytes read through ten pointers across calls
 *   [1] chain() -> walk() -> step(): sums through each level's pointers
 *   [2] the same again (nothing stale in the blocks)
 *   [3] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

unsigned char tick(unsigned char v);
unsigned int chain(const unsigned char *a, const unsigned char *b, unsigned char n);

static const unsigned char data[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

unsigned int many(void) {
    const unsigned char *p0 = data, *p1 = data + 1, *p2 = data + 2, *p3 = data + 3, *p4 = data + 4;
    const unsigned char *p5 = data + 5, *p6 = data + 6, *p7 = data + 7, *p8 = data + 8, *p9 = data + 9;
    unsigned int s = 0;
    s += tick(*p0); s += tick(*p1); s += tick(*p2); s += tick(*p3); s += tick(*p4);
    s += tick(*p5); s += tick(*p6); s += tick(*p7); s += tick(*p8); s += tick(*p9);
    s += *p0 + *p9 + *p5;                       /* still right after the calls */
    return s;                                   /* 55 + 1 + 10 + 6 = 72 */
}

int main(void) {
    static const unsigned char a[4] = { 1, 2, 3, 4 }, b[4] = { 10, 20, 30, 40 };
    OUT[0] = many() == 72;
    /* walk: 10 + 100 + 10 + 1; chain: + 100 - 1 */
    OUT[1] = chain(a, b, 4) == 220;
    OUT[2] = chain(a, b, 4) == 220 && many() == 72;
    OUT[3] = 0xAA;
    return 0;
}
