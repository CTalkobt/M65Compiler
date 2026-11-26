/* test_divmod_sweep_xemu.c — / and % at 16 and 32 bits, signed and unsigned,
 * every pair of a table of values (edges: 0, 1, 255, 256, the sign bit, all
 * ones...), against a shift-and-subtract division written in C (no / or %)
 *
 * Results at $C0C0 (01 = pass; else the number of wrong results, at most $FE):
 *   [0] unsigned 16-bit
 *   [1] signed 16-bit (#794: 256 % 255 was $0101)
 *   [2] unsigned 32-bit (#795: $FFFFFFFF / 1 was $FFFFFFFE: the MEGA65's
 *       divider is one low for a divisor of 1)
 *   [3] signed 32-bit (-2^31 / 1 and % 1 too)
 *   [4] the values of the issues, written out
 *   [5] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define N __attribute__((noinline))

typedef unsigned int u16;
typedef signed int s16;
typedef unsigned long u32;
typedef signed long s32;

N static u16 du16(u16 a, u16 b) { return a / b; }
N static u16 mu16(u16 a, u16 b) { return a % b; }
N static s16 ds16(s16 a, s16 b) { return a / b; }
N static s16 ms16(s16 a, s16 b) { return a % b; }
N static u32 du32(u32 a, u32 b) { return a / b; }
N static u32 mu32(u32 a, u32 b) { return a % b; }
N static s32 ds32(s32 a, s32 b) { return a / b; }
N static s32 ms32(s32 a, s32 b) { return a % b; }

/* the quotient and remainder of a by b (b != 0), bit by bit */
static u32 rq, rr;
static void ref(u32 a, u32 b) {
    unsigned char i;
    rq = 0; rr = 0;
    for (i = 0; i < 32; i++) {
        rr = (rr << 1) | (a >> 31);
        a <<= 1;
        rq <<= 1;
        if (rr >= b) { rr -= b; rq |= 1; }
    }
}

static const u16 v16[] = {0, 1, 2, 3, 7, 10, 127, 128, 255, 256, 257, 1000, 1234, 4660,
                          0x7FFE, 0x7FFF, 0x8000, 0x8001, 0xFF00, 0xFF01, 0xFFFE, 0xFFFF};
static const u32 v32[] = {0, 1, 2, 3, 7, 255, 256, 300, 65535, 65536, 70000, 0x00FFFFFFUL,
                          0x01000000UL, 123456789UL, 0x7FFFFFFEUL, 0x7FFFFFFFUL, 0x80000000UL,
                          0x80000001UL, 0xBF743C43UL, 0xFFFF0000UL, 0xFFFFFFFEUL, 0xFFFFFFFFUL};
#define N16 (sizeof v16 / sizeof v16[0])
#define N32 (sizeof v32 / sizeof v32[0])

static unsigned char bad;
static void check(int ok) { if (!ok && bad < 0xFE) bad++; }
static void result(unsigned char k) { OUT[k] = bad ? bad + 1 : 1; bad = 0; }

int main(void) {
    unsigned char i, j;
    /* [0] */
    for (i = 0; i < N16; i++)
        for (j = 0; j < N16; j++) {
            u16 a = v16[i], b = v16[j];
            if (!b) continue;
            ref(a, b);
            check(du16(a, b) == (u16)rq);
            check(mu16(a, b) == (u16)rr);
        }
    result(0);
    /* [1] C: the quotient truncated toward 0, the remainder the dividend's sign */
    for (i = 0; i < N16; i++)
        for (j = 0; j < N16; j++) {
            s16 a = (s16)v16[i], b = (s16)v16[j];
            u16 ua, ub;
            s16 q, r;
            if (!b || (a == (s16)0x8000 && b == -1)) continue;
            ua = a < 0 ? (u16)-a : (u16)a;
            ub = b < 0 ? (u16)-b : (u16)b;
            ref(ua, ub);
            q = (s16)(u16)rq; r = (s16)(u16)rr;
            if ((a < 0) != (b < 0)) q = -q;
            if (a < 0) r = -r;
            check(ds16(a, b) == q);
            check(ms16(a, b) == r);
        }
    result(1);
    /* [2] */
    for (i = 0; i < N32; i++)
        for (j = 0; j < N32; j++) {
            u32 a = v32[i], b = v32[j];
            if (!b) continue;
            ref(a, b);
            check(du32(a, b) == rq);
            check(mu32(a, b) == rr);
        }
    result(2);
    /* [3] */
    for (i = 0; i < N32; i++)
        for (j = 0; j < N32; j++) {
            s32 a = (s32)v32[i], b = (s32)v32[j];
            u32 ua, ub;
            s32 q, r;
            if (!b || (a == (s32)0x80000000UL && b == -1)) continue;
            ua = a < 0 ? (u32)0 - (u32)a : (u32)a;
            ub = b < 0 ? (u32)0 - (u32)b : (u32)b;
            ref(ua, ub);
            q = (s32)rq; r = (s32)rr;
            if ((a < 0) != (b < 0)) q = -q;
            if (a < 0) r = -r;
            check(ds32(a, b) == q);
            check(ms32(a, b) == r);
        }
    result(3);
    /* [4] */
    check(ms16(256, 255) == 1);
    check(ms16(-32767 - 1, 255) == -128);
    check(ms16(1234, 255) == 214);
    check(ms16(4660, 255) == 70);
    check(ms16(-32767 - 1, 32767) == -1);
    check(du32(0xFFFFFFFFUL, 1) == 0xFFFFFFFFUL);
    check(mu32(0xFFFFFFFFUL, 1) == 0);
    check(du32(0xBF743C43UL, 1) == 0xBF743C43UL);
    check(ds32((s32)0x80000000UL, 1) == (s32)0x80000000UL);
    check(ms32((s32)0x80000000UL, 1) == 0);
    check(ds32((s32)0x80000001UL, 1) == (s32)0x80000001UL);
    result(4);
    OUT[5] = 0xAA;
    return 0;
}
