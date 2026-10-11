/* __builtin_overflow.c — run-time part of __builtin_{add,sub,mul}_overflow
 *
 * int __overflow(unsigned long a, unsigned long b, unsigned char *res,
 *                unsigned int flags);
 *
 * cc45 expands __builtin_add_overflow(a, b, &r) (and the sub/mul, _p and
 * typed s/u variants) to a call of this helper. a and b are the operands
 * widened to 32 bits (sign-extended when signed); flags holds
 *   bits 0-1  operation: 0 add, 1 sub, 2 mul
 *   bit  2    a is signed
 *   bit  3    b is signed
 *   bit  4    the result type is signed
 *   bits 8-15 the result type's size in bytes (1, 2 or 4)
 * res is 0 for the _p forms (no store).
 * The exact result is computed in 64 bits, its low bytes are stored at res,
 * and the return value is 1 when it does not fit the result type.
 */

int __overflow(unsigned long a, unsigned long b, unsigned char *res, unsigned int flags) {
    unsigned int op = flags & 3;
    unsigned long ahi = 0, bhi = 0, lo, hi;
    int neg;
    if ((flags & 4) && (a & 0x80000000UL)) ahi = 0xFFFFFFFFUL;
    if ((flags & 8) && (b & 0x80000000UL)) bhi = 0xFFFFFFFFUL;

    if (op == 0) {
        lo = a + b;
        hi = ahi + bhi + (lo < a ? 1 : 0);
        neg = (hi & 0x80000000UL) != 0;
    } else if (op == 1) {
        lo = a - b;
        hi = ahi - bhi - (a < b ? 1 : 0);
        neg = (hi & 0x80000000UL) != 0;
    } else {
        /* Sign and magnitude: |a| * |b| < 2^64 */
        neg = 0;
        if (ahi) { a = 0 - a; neg = 1; }
        if (bhi) { b = 0 - b; neg = !neg; }
        unsigned long a0 = a & 0xFFFF, a1 = a >> 16, b0 = b & 0xFFFF, b1 = b >> 16;
        unsigned long p00 = a0 * b0, p01 = a0 * b1, p10 = a1 * b0, p11 = a1 * b1;
        unsigned long mid = (p00 >> 16) + (p01 & 0xFFFF) + (p10 & 0xFFFF);
        lo = (p00 & 0xFFFF) | (mid << 16);
        hi = p11 + (p01 >> 16) + (p10 >> 16) + (mid >> 16);
        if (!lo && !hi) neg = 0;
        if (neg) {
            /* Two's complement of the 64-bit magnitude */
            lo = ~lo + 1;
            hi = ~hi + (lo == 0 ? 1 : 0);
        }
    }

    unsigned int size = flags >> 8;
    unsigned int bits = size * 8;
    int ovf;
    if (bits == 0) bits = 32;
    if (!(flags & 16)) {
        ovf = neg || hi != 0 || (bits < 32 && (lo >> bits) != 0);
    } else if (!neg) {
        ovf = hi != 0 || (lo >> (bits - 1)) != 0;
    } else {
        ovf = hi != 0xFFFFFFFFUL || (lo >> (bits - 1)) != (0xFFFFFFFFUL >> (bits - 1));
    }

    unsigned int i;
    for (i = 0; res && i < size; i++) {
        res[i] = (unsigned char)lo;
        lo = lo >> 8;
    }
    return ovf;
}
