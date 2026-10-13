/* __builtin_bits.c — GCC bit builtins for cc45 / MEGA65
 *
 * int is 16 bits, long (and long long) 32 bits. ffs is 1-based (0 for 0);
 * clz/ctz of 0 return the width.
 */

int __builtin_ffs(int x) {
    unsigned int u = (unsigned int)x;
    int n = 1;
    if (u == 0) return 0;
    while (!(u & 1)) { u >>= 1; n++; }
    return n;
}

int __builtin_ffsl(long x) {
    unsigned long u = (unsigned long)x;
    int n = 1;
    if (u == 0) return 0;
    while (!(u & 1)) { u >>= 1; n++; }
    return n;
}

int __builtin_ffsll(long x) { return __builtin_ffsl(x); }

int __builtin_clz(unsigned int x) {
    int n = 0;
    if (x == 0) return 16;
    while (!(x & 0x8000u)) { x <<= 1; n++; }
    return n;
}

int __builtin_clzl(unsigned long x) {
    int n = 0;
    if (x == 0) return 32;
    while (!(x & 0x80000000ul)) { x <<= 1; n++; }
    return n;
}

int __builtin_clzll(unsigned long x) { return __builtin_clzl(x); }

int __builtin_ctz(unsigned int x) {
    int n = 0;
    if (x == 0) return 16;
    while (!(x & 1)) { x >>= 1; n++; }
    return n;
}

int __builtin_ctzl(unsigned long x) {
    int n = 0;
    if (x == 0) return 32;
    while (!(x & 1)) { x >>= 1; n++; }
    return n;
}

int __builtin_ctzll(unsigned long x) { return __builtin_ctzl(x); }

int __builtin_popcount(unsigned int x) {
    int n = 0;
    while (x) { n += x & 1; x >>= 1; }
    return n;
}

int __builtin_popcountl(unsigned long x) {
    int n = 0;
    while (x) { n += (int)(x & 1); x >>= 1; }
    return n;
}

int __builtin_popcountll(unsigned long x) { return __builtin_popcountl(x); }

int __builtin_parity(unsigned int x) { return __builtin_popcount(x) & 1; }
int __builtin_parityl(unsigned long x) { return __builtin_popcountl(x) & 1; }
int __builtin_parityll(unsigned long x) { return __builtin_popcountl(x) & 1; }
