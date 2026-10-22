/* intwide.c — Width-parameterized integer arithmetic for cc45 / MEGA65
 *
 * These functions operate on byte arrays of arbitrary width.
 * Called by operator methods in struct __intN types.
 */

void __intN_add(char *r, char *a, char *b, int n) {
    int carry = 0;
    int i = 0;
    while (i < n) {
        int sum = (unsigned char)a[i] + (unsigned char)b[i] + carry;
        r[i] = (char)sum;
        carry = sum > 255 ? 1 : 0;
        i = i + 1;
    }
}

void __intN_sub(char *r, char *a, char *b, int n) {
    int borrow = 0;
    int i = 0;
    while (i < n) {
        int diff = (unsigned char)a[i] - (unsigned char)b[i] - borrow;
        if (diff < 0) { diff = diff + 256; borrow = 1; } else { borrow = 0; }
        r[i] = (char)diff;
        i = i + 1;
    }
}

int __intN_cmp_u(char *a, char *b, int n) {
    int i = n - 1;
    while (i >= 0) {
        if ((unsigned char)a[i] > (unsigned char)b[i]) return 1;
        if ((unsigned char)a[i] < (unsigned char)b[i]) return -1;
        i = i - 1;
    }
    return 0;
}

/* Signed compare (two's complement): -1, 0 or 1 */
int __intN_cmp_s(char *a, char *b, int n) {
    int sa = (a[n - 1] & 0x80) != 0, sb = (b[n - 1] & 0x80) != 0;
    if (sa != sb) return sa ? -1 : 1;
    return __intN_cmp_u(a, b, n);
}

void __intN_neg(char *a, int n) {
    int i = 0;
    while (i < n) { a[i] = a[i] ^ 0xFF; i = i + 1; }
    int carry = 1;
    i = 0;
    while (i < n) {
        int sum = (unsigned char)a[i] + carry;
        a[i] = (char)sum;
        carry = sum > 255 ? 1 : 0;
        i = i + 1;
    }
}

void __intN_not(char *a, int n) {
    int i = 0;
    while (i < n) { a[i] = a[i] ^ 0xFF; i = i + 1; }
}

void __intN_and(char *r, char *a, char *b, int n) {
    int i = 0;
    while (i < n) { r[i] = a[i] & b[i]; i = i + 1; }
}

void __intN_or(char *r, char *a, char *b, int n) {
    int i = 0;
    while (i < n) { r[i] = a[i] | b[i]; i = i + 1; }
}

void __intN_xor(char *r, char *a, char *b, int n) {
    int i = 0;
    while (i < n) { r[i] = a[i] ^ b[i]; i = i + 1; }
}

void __intN_shl(char *a, int n, int shift) {
    while (shift > 0) {
        int carry = 0;
        int i = 0;
        while (i < n) {
            int val = ((unsigned char)a[i] << 1) | carry;
            carry = (unsigned char)a[i] >> 7;
            a[i] = (char)val;
            i = i + 1;
        }
        shift = shift - 1;
    }
}

void __intN_shr_u(char *a, int n, int shift) {
    while (shift > 0) {
        int carry = 0;
        int i = n - 1;
        while (i >= 0) {
            int val = ((unsigned char)a[i] >> 1) | (carry << 7);
            carry = a[i] & 1;
            a[i] = (char)val;
            i = i - 1;
        }
        shift = shift - 1;
    }
}

/* Arithmetic shift right (the sign bit copied in: the structs' operators
   are signed, as the comparisons) */
void __intN_shr_s(char *a, int n, int shift) {
    char fill = (a[n - 1] & 0x80) ? 1 : 0;
    while (shift > 0) {
        int carry = fill;
        int i = n - 1;
        while (i >= 0) {
            int val = ((unsigned char)a[i] >> 1) | (carry << 7);
            carry = a[i] & 1;
            a[i] = (char)val;
            i = i - 1;
        }
        shift = shift - 1;
    }
}

/* Unsigned division, shift and subtract: q = a / b, m = a % b (b == 0:
   q all ones, m = a). q and m may not be a or b; n at most 16 */
void __intN_divmod_u(char *q, char *m, char *a, char *b, int n) {
    int i = 0, bits = n * 8;
    while (i < n) { q[i] = a[i]; m[i] = 0; i = i + 1; }
    while (bits > 0) {
        /* (m:q) <<= 1 */
        int carry = 0;
        i = 0;
        while (i < n) {
            int val = ((unsigned char)q[i] << 1) | carry;
            carry = (unsigned char)q[i] >> 7;
            q[i] = (char)val;
            i = i + 1;
        }
        i = 0;
        while (i < n) {
            int val = ((unsigned char)m[i] << 1) | carry;
            carry = (unsigned char)m[i] >> 7;
            m[i] = (char)val;
            i = i + 1;
        }
        if (carry || __intN_cmp_u(m, b, n) >= 0) {
            __intN_sub(m, m, b, n);
            q[0] = q[0] | 1;
        }
        bits = bits - 1;
    }
}

/* Signed division, truncating toward zero (C): the quotient negative when
   the signs differ, the remainder with the dividend's sign */
void __intN_divmod_s(char *q, char *m, char *a, char *b, int n) {
    char ua[16], ub[16];
    int i = 0;
    int na = (a[n - 1] & 0x80) != 0, nb = (b[n - 1] & 0x80) != 0;
    while (i < n) { ua[i] = a[i]; ub[i] = b[i]; i = i + 1; }
    if (na) __intN_neg(ua, n);
    if (nb) __intN_neg(ub, n);
    __intN_divmod_u(q, m, ua, ub, n);
    if (na != nb) __intN_neg(q, n);
    if (na) __intN_neg(m, n);
}

void __intN_mul(char *r, char *a, char *b, int n) {
    /* Clear result */
    char result[16];
    int i = 0;
    while (i < n && i < 16) { result[i] = 0; i = i + 1; }

    /* Shift-and-add: for each bit of b, if set, add a to result */
    char acopy[16];
    char bcopy[16];
    i = 0;
    while (i < n && i < 16) { acopy[i] = a[i]; bcopy[i] = b[i]; i = i + 1; }

    int bits = n * 8;
    i = 0;
    while (i < bits) {
        if (bcopy[0] & 1) {
            __intN_add(result, result, acopy, n);
        }
        __intN_shl(acopy, n, 1);
        __intN_shr_u(bcopy, n, 1);
        i = i + 1;
    }

    i = 0;
    while (i < n) { r[i] = result[i]; i = i + 1; }
}
