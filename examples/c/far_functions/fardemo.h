/* fardemo.h — the far functions of the __far demo (examples/c/far_functions)
 *
 * A prototype does not need __far: only the definition decides where the
 * code goes, so callers are written as for any function.
 */
#pragma once

/* math.c: __far */
unsigned int primes_below(unsigned int n);
unsigned long fib(unsigned char n);
unsigned int gcd(unsigned int a, unsigned int b);

/* text.c: __far */
unsigned int count_words(const char *s);
void caesar(char *s, unsigned char shift);
void report(const char *s);
