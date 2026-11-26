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
    w8(a / 7); w8(a % 10); w8((u8)(sa / 3)); w8((u8)(sa % 5));
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
    /* word operations with byte immediates (width bits 11) */
    w16(a ^ 0x5A); w16(a * 13); w16(a / 7); w16(a % 13); w16((u16)(sa / 9)); w16((u16)(sa % 9));
    w16(a - 200); w16(a & 0xF0); w16(a | 0x81); w16(a + 255);
    if (a < 200) w8(1); if (a != 255) w8(2); if (sa > 100) w8(3); if (b == 0) w8(4);
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
    w32(a / 1000); w32(a % 70000UL); w32((u32)(sa / 7)); w32((u32)(sa % 300));
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

/* through a pointer at an offset (LDO, STO: up to 255; 300 stays ADDI + LD) */
struct big { u8 a; u32 l; u8 pad[240]; u16 w; u8 b; u32 m; u8 pad2[40]; u16 far_; };
static struct big bg[2];
static struct big *const bp[2] = {&bg[0], &bg[1]};   /* (main stays p-code: no 294-byte elements) */
__attribute__((noinline)) static void fields(struct big *p, u8 v) {
    p->a = v; p->l = (u32)v * 70001UL; p->w = (u16)v * 513; p->b = v ^ 0xA5;
    p->m = (u32)v << 20; p->far_ = (u16)v * 3;
    w8(p->a); w32(p->l); w16(p->w); w8(p->b); w32(p->m); w16(p->far_);
    w32(p->l + p->m); w8(bg[0].b);
}

/* calls: arguments of each width, results of each width */
static u32 mix(u8 a, u16 b, u32 c, const u8 *p) { return c + b * a + p[1]; }
static u8 inc8(u8 a) { return a + 1; }
static s16 neg16(s16 a) { return -a; }
/* (CALLC: up to 3 arguments, the widths packed; each result width) */
__attribute__((noinline)) static u32 mix3(u32 c, u8 a, u16 b) { return c ^ ((u32)b << 8) ^ a; }
__attribute__((noinline)) static u8 mix3b(u16 b, u8 a, u32 c) { return (u8)(c >> 3) + a - (u8)b; }
__attribute__((noinline)) static u16 mix2(u8 a, u32 c) { return (u16)(c >> 7) ^ a; }
__attribute__((noinline)) static u32 mix4(u8 a, u16 b, u32 c, u8 d) { return (c - b) ^ ((u32)a << 16) ^ ((u32)d << 24); }
__attribute__((noinline)) static u16 hi16(u32 c, u8 a, u16 b) { return (u16)(c >> 16) + (a & 0) + (b & 0); }
__attribute__((noinline)) static u32 mix5(u8 a, u16 b, u32 c, u8 d, u16 e) { return c + ((u32)b << 3) - a + ((u32)d << 20) + e * 5; }   /* (5 parameters: the entry copies them; CALL) */
static u16 acc4;
__attribute__((noinline)) static void put4(u16 a, u8 b, u16 c, u8 d) { acc4 = acc4 * 3 + a - c + (u16)(b ^ d); }
static u8 hits;
__attribute__((noinline)) static void tick(void) { hits++; }
static void calls(u8 i) {
    u32 r = mix3(v32[i % N32], v8[i], v16[i]);
    w32(r); w8(mix3b(v16[i], v8[i], v32[i % N32])); w16(mix2(v8[i], v32[i % N32]));
    tick(); w8(hits);
    /* constant arguments: CALLC's immediate bytes (and 0x1234, 70000: not) */
    w8(mix3b(200, 0xFF, v32[i % N32])); w8(mix3b(0x1234, 0, v32[i % N32]));
    w16(mix2(7, v32[i % N32])); w16(mix2(v8[i], 70000UL));
    w32(mix3(v32[i % N32], 0, 255)); w32(mix3(1, v8[i], 256));
    /* an address as a long, in a slot a long constant had (glamlambs' dma_copy_far: the upper bytes were left) */
    w16(hi16(0x0FF7E000UL, v8[i], v16[i])); w16(hi16((u32)(u16)t16, v8[i], v16[i]));   /* (its address differs: its upper half 0) */
    /* 4 arguments (CALLC4), constants among them */
    w32(mix4(v8[i], v16[i], v32[i % N32], 9)); w32(mix4(3, 300, v32[i % N32], v8[i]));
    put4(v16[i], 0xFE, 77, v8[i]); w16(acc4);
    w32(mix5(v8[i], v16[i], v32[i % N32], v8[(i + 3) % N8], v16[(i + 5) % N16]));
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

/* compare and branch (CBR, CBRI): every condition, each width */
static u16 cb8(u8 a, u8 b) {
    s8 sa = (s8)a, sb = (s8)b;
    u16 r = 0;
    if (a == b) r |= 1;
    if (a != b) r |= 2;
    if (a < b) r |= 4;
    if (a <= b) r |= 8;
    if (a > b) r |= 16;
    if (a >= b) r |= 32;
    if (sa < sb) r |= 64;
    if (sa <= sb) r |= 128;
    if (sa > sb) r |= 256;
    if (sa >= sb) r |= 512;
    if (a < 100) r |= 1024;
    if (a >= 100) r |= 2048;
    if (sa < -3) r |= 4096;
    if (sa > -3) r |= 8192;
    while (a != b && a > 100) { a >>= 1; r += 16384; }
    return r;
}
static u16 cb16(u16 a, u16 b) {
    s16 sa = (s16)a, sb = (s16)b;
    u16 r = 0;
    if (a == b) r |= 1;
    if (a != b) r |= 2;
    if (a < b) r |= 4;
    if (a <= b) r |= 8;
    if (a > b) r |= 16;
    if (a >= b) r |= 32;
    if (sa < sb) r |= 64;
    if (sa <= sb) r |= 128;
    if (sa > sb) r |= 256;
    if (sa >= sb) r |= 512;
    if (a < 1000) r |= 1024;
    if (a >= 1000) r |= 2048;
    if (sa < -300) r |= 4096;
    if (sa > -300) r |= 8192;
    while (a != b && a > 1000) { a >>= 1; r += 16384; }
    return r;
}
static u16 cb32(u32 a, u32 b) {
    s32 sa = (s32)a, sb = (s32)b;
    u16 r = 0;
    if (a == b) r |= 1;
    if (a != b) r |= 2;
    if (a < b) r |= 4;
    if (a <= b) r |= 8;
    if (a > b) r |= 16;
    if (a >= b) r |= 32;
    if (sa < sb) r |= 64;
    if (sa <= sb) r |= 128;
    if (sa > sb) r |= 256;
    if (sa >= sb) r |= 512;
    if (a < 70000UL) r |= 1024;
    if (a >= 70000UL) r |= 2048;
    if (sa < -5) r |= 4096;
    if (sa > -5) r |= 8192;
    while (a != b && a > 70000UL) { a >>= 1; r += 16384; }
    return r;
}

/* __noescape literals (#pragma cc45 noescape_strings): far p-code copies each
 * into its frame from its bank (CSTR), here chosen, used in a loop, passed on */
#pragma cc45 noescape_strings
__attribute__((noinline)) static u16 ssum(const char *s) { u16 h = 0; while (*s) h = h * 31 + (u8)*s++; return h; }
__attribute__((noinline)) static u16 lits(u8 i) {
    const char *a = "alpha", *b = i & 1 ? "beta gamma" : "delta";
    u16 h = ssum(a) ^ ssum(b);
    u8 k;
    for (k = 0; k < (i & 7); k++) h += ssum("loop");
    return h + (a[0] == 'a') + ssum(i > 3 ? "" : "x") + (u8)b[2];
}
#pragma cc45 escape_strings

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
    for (i = 0; i < N8; i++) { fields(bp[i & 1], v8[i]); sum(); }
    group(5);
    for (i = 0; i < N8; i++) { calls(i); sum(); }
    group(6);
    for (i = 0; i < N8; i++) { w8(br(v8[i], v16[i], v32[i % N32])); sum(); }
    group(7);
    for (i = 0; i < N8; i++) for (j = 0; j < N8; j++) { w16(cb8(v8[i], v8[j])); sum(); }
    for (i = 0; i < N16; i++) for (j = 0; j < N16; j++) { w16(cb16(v16[i], v16[j])); sum(); }
    for (i = 0; i < N32; i++) for (j = 0; j < N32; j++) { w16(cb32(v32[i], v32[j])); sum(); }
    group(8);
    for (i = 0; i < N8; i++) { w16(lits(i)); sum(); }
    group(9);
    *(volatile u8 *)0xC0C0 = 0xAA;
    return 0;
}
