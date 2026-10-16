/* primes.c — overlay "primes": a sieve of Eratosthenes
 *
 * The code of this file is in the code overlay "primes": ln45 links it to
 * run at the overlay region and keeps it in far memory until a call needs
 * it. Its variables (the sieve below) stay in bank 0 like any other.
 */
#pragma cc45 overlay(primes)
#include "demo.h"

#define LIMIT 10000
static unsigned char composite[LIMIT / 8 + 1];   /* one bit per number */

static unsigned char bit(unsigned int n) { return composite[n >> 3] & (1 << (n & 7)); }

unsigned int primes_count(unsigned int limit) {
    unsigned int i, j, count = 0;
    if (limit > LIMIT) limit = LIMIT;
    for (i = 0; i < sizeof composite; i++) composite[i] = 0;
    for (i = 2; i <= limit; i++) {
        if (bit(i)) continue;
        count++;
        for (j = i + i; j <= limit && j > i; j += i) composite[j >> 3] |= (unsigned char)(1 << (j & 7));
    }
    return count;
}

/* also called from the overlay "words": a call between overlays */
unsigned char is_prime(unsigned int n) {
    unsigned int d;
    if (n < 2) return 0;
    for (d = 2; d * d <= n; d++)
        if (n % d == 0) return 0;
    return 1;
}
