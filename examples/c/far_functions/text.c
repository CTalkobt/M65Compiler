/* text.c — far functions that call resident code: the library (printf,
 * isalpha) stays resident, and the call returns to the far code still
 * mapped (the ROM calls printf makes are handled too)
 */
#include <stdio.h>
#include <ctype.h>
#include "fardemo.h"

__far unsigned int count_words(const char *s) {
    unsigned int words = 0;
    char in = 0;
    for (; *s; s++) {
        if (isalpha(*s)) { if (!in) words++; in = 1; }
        else in = 0;
    }
    return words;
}

/* shifts the letters of s (char literals are PETSCII: 'a'..'z' a range) */
__far void caesar(char *s, unsigned char shift) {
    for (; *s; s++)
        if (*s >= 'a' && *s <= 'z') *s = 'a' + (*s - 'a' + shift) % 26;
}

/* printing from far code */
__far void report(const char *s) {
    printf("  %-22s%s\n", "caesar +3", s);
}
