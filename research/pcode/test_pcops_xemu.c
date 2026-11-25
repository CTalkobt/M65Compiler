/* test_pcops_xemu.c — research/pcode: every operation of cc45's p-code
 * (PcodeEmit.cpp, pcode_rt.s45) against native code. Built twice, native
 * and with CC45_PCODE=all; both runs write their results from $A000 (a
 * group's number, then its results) and the two dumps must be the same
 * (run_pcops.sh compares them and names the first group that differs).
 * The values come from tables, through functions called with them, so
 * nothing is folded at compile time. $C0C0: $AA when the run ended. */

typedef unsigned char u8;
typedef unsigned int u16;
typedef unsigned long u32;
typedef signed char s8;
typedef signed int s16;
typedef signed long s32;

/* each result into a checksum (Fletcher's, 16 bits); after each call of a
 * test function its checksum is written (2 bytes), and started again */
static u8 *out = (u8 *)0xA000;
static u16 c1, c2;
static void group(u8 g) { *out++ = 0xEE; *out++ = g; }
static void w8(u8 v) { c1 += v; c2 += c1; }
static void w16(u16 v) { w8((u8)v); w8((u8)(v >> 8)); }
static void w32(u32 v) { w16((u16)v); w16((u16)(v >> 16)); }
static void sum(void) { *out++ = (u8)c2; *out++ = (u8)(c2 >> 8); c1 = c2 = 0; }

static const u8 v8[] = {0, 1, 2, 7, 0x7F, 0x80, 0x81, 0xFE, 0xFF, 100};
static const u16 v16[] = {0, 1, 2, 0xFF, 0x100, 0x7FFF, 0x8000, 0x8001, 0xFFFE, 0xFFFF, 1234, 4660};
static const u32 v32[] = {0, 1, 0xFF, 0x10000, 0x7FFFFFFFUL, 0x80000000UL, 0xFFFFFFFFUL, 123456789UL, 300};
#define N8 10
#define N16 12
#define N32 9

/* 8 bits */
static void ops8(u8 a, u8 b) {
    s8 sa = (s8)a, sb = (s8)b;
    w8(a + b); w8(a - b); w8(a & b); w8(a | b); w8(a ^ b); w8(a * b);
    if (b) { w8(a / b); w8(a % b); w8((u8)(sa / sb)); w8((u8)(sa % sb)); }
    w8(a << (b & 7)); w8(a >> (b & 7)); w8((u8)(sa >> (b & 7)));
    w8(a << 3); w8(a >> 3); w8((u8)(sa >> 3)); w8(a + 5); w8(a - 9); w8(a * 3);
    w8(a == b); w8(a != b); w8(a < b); w8(a <= b); w8(a > b); w8(a >= b);
    w8(sa < sb); w8(sa <= sb); w8(sa > sb); w8(sa >= sb);
    w8(a < 100); w8(sa < -3); w8(sa >= 5);
    w8((u8)-a); w8((u8)~a);
    w16(a); w16((u16)(s16)sa); w32(a); w32((u32)(s32)sa);
}
/* 16 bits */
static void ops16(u16 a, u16 b) {
    s16 sa = (s16)a, sb = (s16)b;
    w16(a + b); w16(a - b); w16(a & b); w16(a | b); w16(a ^ b); w16(a * b);
    if (b) { w16(a / b); w16(a % b); w16((u16)(sa / sb)); w16((u16)(sa % sb)); }
    w16(a << (b & 15)); w16(a >> (b & 15)); w16((u16)(sa >> (b & 15)));
    w16(a << 7); w16(a >> 9); w16((u16)(sa >> 9)); w16(a + 1000); w16(a - 1); w16(a * 10);
    w8(a == b); w8(a != b); w8(a < b); w8(a <= b); w8(a > b); w8(a >= b);
    w8(sa < sb); w8(sa <= sb); w8(sa > sb); w8(sa >= sb);
    w8(a < 1000); w8(sa < -300); w8(sa >= 5);
    w16((u16)-a); w16((u16)~a); w8((u8)a); w8((u8)(a >> 8));
    w32(a); w32((u32)(s32)sa);
}
/* 32 bits */
static void ops32(u32 a, u32 b) {
    s32 sa = (s32)a, sb = (s32)b;
    w32(a + b); w32(a - b); w32(a & b); w32(a | b); w32(a ^ b); w32(a * b);
    if (b) { w32(a / b); w32(a % b); w32((u32)(sa / sb)); w32((u32)(sa % sb)); }
    w32(a << (b & 31)); w32(a >> (b & 31)); w32((u32)(sa >> (b & 31)));
    w32(a << 8); w32(a >> 9); w32((u32)(sa >> 9)); w32(a + 70000UL); w32(a * 3);
    w8(a == b); w8(a != b); w8(a < b); w8(a <= b); w8(a > b); w8(a >= b);
    w8(sa < sb); w8(sa <= sb); w8(sa > sb); w8(sa >= sb);
    w8(a < 70000UL); w8(sa < -5);
    w32((u32)-a); w32(~a); w16((u16)a); w16((u16)(a >> 16));
}

/* elements of several sizes, through pointers and globals */
struct s3 { u8 a; u16 b; };
struct s5 { u16 a; u8 b; u16 c; };
static struct s3 t3[6];
static struct s5 t5[6];
static u16 t16[8];
static u8 t8[8];
static u32 t32[4];
static u16 g16;
static u8 g8;
static void elems(u8 i, u8 v) {
    struct s3 *p3 = t3;
    t3[i].a = v; t3[i].b = (u16)v * 300;
    t5[i].a = v + 1; t5[i].b = v ^ 0x55; t5[i].c = (u16)v << 4;
    t16[i] = (u16)v * 7; t8[i + 1] = v; t32[i & 3] = (u32)v * 100000UL;
    g16 += t16[i]; g8 ^= t8[i + 1];
    w8(p3[i].a); w16(p3[i].b); w16(t5[i].a); w8(t5[i].b); w16(t5[i].c);
    w16(t16[i]); w8(t8[i + 1]); w32(t32[i & 3]); w16(g16); w8(g8);
}

/* calls: arguments of each width, results of each width */
static u32 mix(u8 a, u16 b, u32 c, const u8 *p) { return c + b * a + p[1]; }
static u8 inc8(u8 a) { return a + 1; }
static s16 neg16(s16 a) { return -a; }
static void calls(u8 i) {
    w32(mix(v8[i], v16[i], v32[i % N32], v8));
    w8(inc8(v8[i])); w16((u16)neg16((s16)v16[i]));
}

/* branches on values of each width */
static u8 br(u8 a, u16 b, u32 c) {
    u8 r = 0;
    if (a) r |= 1;
    if (b) r |= 2;
    if (c) r |= 4;
    if (!a) r |= 8;
    if (b > 300) r |= 16;
    if ((s32)c < 0) r |= 32;
    while (a > 10) { a -= 10; r += 64; }
    return r;
}

int main(void) {
    u8 i, j;
    group(1);
    for (i = 0; i < N8; i++) for (j = 0; j < N8; j++) { ops8(v8[i], v8[j]); sum(); }
    group(2);
    for (i = 0; i < N16; i++) for (j = 0; j < N16; j++) { ops16(v16[i], v16[j]); sum(); }
    group(3);
    for (i = 0; i < N32; i++) for (j = 0; j < N32; j++) { ops32(v32[i], v32[j]); sum(); }
    group(4);
    for (i = 0; i < 6; i++) { elems(i, v8[i + 2]); sum(); }
    group(5);
    for (i = 0; i < N8; i++) { calls(i); sum(); }
    group(6);
    for (i = 0; i < N8; i++) { w8(br(v8[i], v16[i], v32[i % N32])); sum(); }
    group(7);
    *(volatile u8 *)0xC0C0 = 0xAA;
    return 0;
}
