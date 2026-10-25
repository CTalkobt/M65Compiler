/* test_bits_xemu.c — __bit(v, n) and __bits(v, lo, hi) (#204)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] reads of a char: a bit, a range, the whole byte; sizeof of the result
 *   [1] writes to a char: set / clear a bit, a constant range, a variable
 *       (masked to the field), the whole width; the other bits kept
 *   [2] an int: a range across the two bytes, bit 15
 *   [3] a long: bits 12-27, bit 31; the result's size follows the width
 *   [4] compound assignment and ++/--: the field wraps, the rest kept;
 *       prefix gives the new value, postfix the old
 *   [5] operands: a struct member, p->m, an array element whose index has a
 *       side effect (evaluated once), *p, a global, a static, a parameter
 *   [6] the value of an assignment: the field as stored (masked)
 *   [7] in conditions and a loop (a write in the loop is seen by the next
 *       pass); a volatile memory byte
 *   [8] a signed operand: the field reads unsigned
 *   [9] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

struct regs { unsigned char ctrl; unsigned int mode; };
struct regs dev = { 0x00, 0x1234 };
unsigned char gflags = 0x0F;
unsigned char arr[4] = { 0, 0, 0, 0 };
volatile unsigned char hw;              /* a "hardware register" */
#define REG (*(volatile unsigned char *)&hw)

static unsigned char param(unsigned char v) {
    __bits(v, 0, 3) = 9;
    return v;
}

int main(void) {
    unsigned char x = 0xB4;           /* 1011 0100 */
    unsigned int i;
    unsigned long l;
    unsigned char ok, y, n, k;
    static unsigned char st = 0xFF;
    struct regs *p = &dev;
    unsigned char *q = &arr[3];

    OUT[0] = __bit(x, 2) == 1 && __bit(x, 3) == 0 && __bits(x, 4, 7) == 0xB && __bits(x, 0, 7) == 0xB4 &&
             __bits(x, 1, 2) == 2 && sizeof(__bit(x, 7)) == 1;

    ok = 1;
    __bit(x, 0) = 1;            if (x != 0xB5) ok = 0;
    __bit(x, 7) = 0;            if (x != 0x35) ok = 0;
    __bits(x, 4, 6) = 6;        if (x != 0x65) ok = 0;
    n = 0x1F;
    __bits(x, 1, 3) = n;        if (x != 0x6F) ok = 0;   /* 0x1F masked to 7 */
    n = 2;
    __bits(x, 1, 3) = n;        if (x != 0x65) ok = 0;
    __bits(x, 0, 7) = 0x5A;     if (x != 0x5A) ok = 0;
    OUT[1] = ok;

    i = 0x1234;
    ok = __bits(i, 4, 11) == 0x23 && __bit(i, 15) == 0 && __bit(i, 12) == 1;
    __bits(i, 4, 11) = 0xCD;    if (i != 0x1CD4) ok = 0;
    __bit(i, 15) = 1;           if (i != 0x9CD4) ok = 0;
    OUT[2] = ok && sizeof(__bits(i, 4, 11)) == 1 && sizeof(__bits(i, 0, 11)) == 2;

    l = 0x12345678UL;
    ok = __bits(l, 12, 27) == 0x2345 && __bit(l, 31) == 0 && __bit(l, 28) == 1;
    __bits(l, 12, 27) = 0xBEEF; if (l != 0x1BEEF678UL) ok = 0;
    __bit(l, 31) = 1;           if (l != 0x9BEEF678UL) ok = 0;
    OUT[3] = ok && sizeof(__bits(l, 0, 31)) == 4 && sizeof(__bits(l, 8, 23)) == 2;

    ok = 1;
    x = 0xF3;
    __bits(x, 4, 7) += 2;       if (x != 0x13) ok = 0;   /* 15 + 2 wraps to 1 */
    __bits(x, 0, 3) -= 4;       if (x != 0x1F) ok = 0;   /* 3 - 4 wraps to 15 */
    __bits(x, 4, 7) |= 8;       if (x != 0x9F) ok = 0;
    __bits(x, 0, 3) &= 5;       if (x != 0x95) ok = 0;
    __bits(x, 0, 3) ^= 0xF;     if (x != 0x9A) ok = 0;
    __bits(x, 4, 7) <<= 1;      if (x != 0x2A) ok = 0;   /* 9 << 1 = 18, 4 bits: 2 */
    __bits(x, 0, 3) >>= 1;      if (x != 0x25) ok = 0;
    __bits(x, 0, 3) *= 3;       if (x != 0x2F) ok = 0;
    y = ++__bits(x, 4, 7);      if (y != 3 || x != 0x3F) ok = 0;
    y = __bits(x, 0, 3)++;      if (y != 15 || x != 0x30) ok = 0;
    y = __bit(x, 4)--;          if (y != 1 || x != 0x20) ok = 0;
    y = --__bit(x, 4);          if (y != 1 || x != 0x30) ok = 0;
    OUT[4] = ok;

    ok = 1;
    __bit(dev.ctrl, 3) = 1;          if (dev.ctrl != 0x08) ok = 0;
    __bits(p->mode, 8, 11) = 0xA;    if (dev.mode != 0x1A34) ok = 0;
    k = 1;
    __bits(arr[k++], 2, 3) = 3;      if (k != 2 || arr[1] != 0x0C || arr[2] != 0) ok = 0;
    __bits(*q, 6, 7) = 2;            if (arr[3] != 0x80) ok = 0;
    __bits(gflags, 2, 5) = 0;        if (gflags != 0x03) ok = 0;
    __bit(st, 0) = 0;                if (st != 0xFE) ok = 0;
    if (param(0xF0) != 0xF9) ok = 0;
    OUT[5] = ok;

    x = 0;
    y = (__bits(x, 0, 2) = 13);
    n = __bits(x, 4, 7) = __bits(x, 0, 2);
    OUT[6] = y == 5 && n == 5 && x == 0x55;

    ok = 1;
    x = 0x80;
    if (!__bit(x, 7)) ok = 0;
    if (__bit(x, 6)) ok = 0;
    n = 0;
    for (k = 0; k < 6; k++) {
        __bit(x, 0) = !__bit(x, 0);   /* toggles each pass */
        n += x;
    }
    if (n != (unsigned char)(0x81 + 0x80 + 0x81 + 0x80 + 0x81 + 0x80) || x != 0x80) ok = 0;
    REG = 0x0F;
    __bits(REG, 2, 5) = 0xA;
    if (REG != 0x2B) ok = 0;
    __bit(REG, 7) = 1;
    if (__bits(REG, 4, 7) != 0xA) ok = 0;
    OUT[7] = ok;

    {
        signed char s = -1;
        signed int si = -2;
        __bits(s, 4, 7) = 5;
        OUT[8] = __bits(s, 0, 3) == 15 && s == 0x5F && __bits(si, 0, 15) == 0xFFFE &&
                 __bit(si, 15) == 1 && __bits(si, 12, 15) > 0;
    }
    OUT[9] = 0xAA;
    return 0;
}
