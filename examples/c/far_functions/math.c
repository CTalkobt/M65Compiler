/* math.c — far functions: their code is kept in far memory (bank 5) and
 * mapped into the CPU's window when they are called
 */
#include "fardemo.h"

/* The sieve's table is data: it stays resident (only code goes far) */
static unsigned char composite[1000];

__far unsigned int primes_below(unsigned int n) {
    unsigned int i, j, count = 0;
    if (n > sizeof composite) n = sizeof composite;
    for (i = 2; i < n; i++) {
        if (composite[i]) continue;
        count++;
        for (j = i + i; j < n; j += i) composite[j] = 1;
    }
    return count;
}

__far unsigned long fib(unsigned char n) {
    unsigned long a = 0, b = 1, t;
    while (n--) { t = a + b; a = b; b = t; }
    return a;
}

/* a static far function, called only from this file's far code: a direct
 * call (the same overlay is already mapped) */
static __far unsigned int rem(unsigned int a, unsigned int b) { return a % b; }

__far unsigned int gcd(unsigned int a, unsigned int b) {
    while (b) { unsigned int t = rem(a, b); a = b; b = t; }
    return a;
}
