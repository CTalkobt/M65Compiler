/* test_far_offsets_xemu.c — far accesses at constant offsets (#348): p[c]
 * through a far pointer is the access at p with Z starting at c (c >= 256:
 * p + (c & $FF00), shared), the address p + $xxxx0000 without carries in its
 * low word, x + c stored as it is added, and a game's map accessors (the
 * issue's vm_kind / vm_set) in attic RAM.
 *
 * Results at $C0C0 (01 = pass):
 *   +0 int stores p[0], p[1], p[128], p[129] and their loads
 *   +1 offsets at the Z limit: char p[255], int at bytes 254-255 and 255-256
 *   +2 longs at offsets (loaded and stored), x - c stored
 *   +3 the address base + 16-bit index (base $xxxx0000) in bank 4
 *   +4 vm_set / vm_kind in attic RAM ($8300000)
 *   +5 a char function's result (no flags), a value read back through a
 *      second far pointer
 *   +6 $AA
 */
#define OUT ((volatile unsigned char *)0xC0C0)

#define MAP_LIN 0x8300000UL
#define VM ((unsigned char __far *)MAP_LIN)

unsigned char vm_kind(unsigned char x, unsigned char y) {
    return (unsigned char)(VM[(unsigned int)y << 9 | (unsigned int)x << 2] - 0x10) >> 2;
}

void vm_set(unsigned char x, unsigned char y, unsigned char kind) {
    unsigned int __far *p = (unsigned int __far *)(VM + ((unsigned int)y << 9 | (unsigned int)x << 2));
    unsigned int c = 0x1010 + (unsigned int)kind * 4;
    p[0] = c;
    p[1] = c + 1;
    p[128] = c + 2;
    p[129] = c + 3;
}

static void put4(unsigned int __far *p, unsigned int c) {
    p[0] = c;
    p[1] = c + 1;
    p[128] = c + 2;
    p[129] = c - 3;
}

static unsigned int sum4(const unsigned int __far *p) { return p[0] + p[1] + p[128] + p[129]; }

static void edges(unsigned char __far *b) {
    b[255] = 0x55;
    *(unsigned int __far *)(b + 254) = 0x1122;      /* bytes 254-255 */
    *(unsigned int __far *)(b + 255 + 256) = 0x3344; /* bytes 511-512 (Z would wrap) */
}

static void longs(unsigned long __far *l) {
    l[0] = 0x01020304UL;
    l[3] = 0x0A0B0C0DUL;
    l[70] = l[3] + 1;   /* byte 280 */
}

static unsigned char at16(unsigned int i) { return ((unsigned char __far *)0x40000UL)[i]; }

int main(void) {
    unsigned int __far *p = (unsigned int __far *)0x40100UL;
    unsigned char __far *b = (unsigned char __far *)0x41000UL;
    unsigned long __far *l = (unsigned long __far *)0x42000UL;
    unsigned char __far *raw = (unsigned char __far *)0x40000UL;
    unsigned char k, ok;

    put4(p, 0x2000);
    OUT[0] = p[0] == 0x2000 && p[1] == 0x2001 && p[128] == 0x2002 && p[129] == 0x1FFD &&
             sum4(p) == 0x2000 + 0x2001 + 0x2002 + 0x1FFD && raw[0x100] == 0x00 && raw[0x101] == 0x20 &&
             raw[0x200] == 0x02 && raw[0x203] == 0x1F;

    edges(b);
    OUT[1] = b[255] == 0x11 && b[254] == 0x22 && b[511] == 0x44 && b[512] == 0x33 &&
             *(unsigned int __far *)(b + 254) == 0x1122;

    longs(l);
    OUT[2] = l[0] == 0x01020304UL && l[3] == 0x0A0B0C0DUL && l[70] == 0x0A0B0C0EUL &&
             ((unsigned char __far *)l)[280] == 0x0E;

    raw[0x1234] = 0x77;
    raw[0xFFFF] = 0x66;
    OUT[3] = at16(0x1234) == 0x77 && at16(0xFFFF) == 0x66;

    vm_set(3, 2, 5);   /* MAP_LIN + 2 * 512 + 3 * 4 */
    vm_set(0, 0, 9);
    ok = vm_kind(3, 2) == 5 && vm_kind(0, 0) == 9;
    {
        unsigned int __far *q = (unsigned int __far *)(MAP_LIN + 2 * 512 + 12);
        ok = ok && q[0] == 0x1010 + 20 && q[1] == 0x1010 + 21 && q[128] == 0x1010 + 22 && q[129] == 0x1010 + 23;
    }
    OUT[4] = ok;

    k = vm_kind(0, 0);
    OUT[5] = k == 9 && (unsigned char)(vm_kind(3, 2) + k) == 14;

    OUT[6] = 0xAA;
    return 0;
}
