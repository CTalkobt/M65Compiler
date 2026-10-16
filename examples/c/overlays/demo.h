/* demo.h — the parts of the overlay demo (examples/c/overlays) */
#pragma once

/* resident (main.c): output for the overlays */
void say(const char *label, unsigned long value);

/* overlay "primes" (primes.c) */
unsigned int primes_count(unsigned int limit);
unsigned char is_prime(unsigned int n);

/* overlay "hanoi" (hanoi.c) */
unsigned long hanoi_moves(unsigned char discs);

/* overlay "words" (words.c) */
void words_stats(const char *text);
