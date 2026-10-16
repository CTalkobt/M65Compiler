/* words.c — overlay "words": statistics of a text
 *
 * It reports through say() in the resident part and asks the overlay
 * "primes" whether each word's length is prime: when a call leaves this
 * overlay, another can take its place in the region; the return copies
 * this one back.
 */
#pragma cc45 overlay(words)
#include "demo.h"

static unsigned char is_letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

void words_stats(const char *text) {
    unsigned int words = 0, longest = 0, primelen = 0, len = 0;
    const char *p = text;
    for (;;) {
        char c = *p++;
        if (is_letter(c)) { len++; continue; }
        if (len) {
            words++;
            if (len > longest) longest = len;
            if (is_prime(len)) primelen++;   /* overlay "primes" */
            len = 0;
        }
        if (!c) break;
    }
    say("words", words);
    say("longest word", longest);
    say("prime-length words", primelen);
}
