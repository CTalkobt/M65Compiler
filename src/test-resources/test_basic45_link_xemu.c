/* test_basic45_link_xemu.c — C functions called from BASIC
 * (test_basic45_link_xemu.bas, built with basic45 --link this file).
 *
 * SYS {name}, a, x, y passes A, X and Y as the first three parameters
 * (unsigned char or unsigned int). The result comes back in A/X (RREG A, X
 * in BASIC).
 */
#include <string.h>

unsigned int counter;            /* BSS: zero on the first call, kept after */
char message[] = "BASIC45";      /* data */

unsigned int add(unsigned char a, unsigned char x) {
    return a + x;
}

void bump(void) {
    counter++;
}

unsigned int get_counter(void) {
    return counter;
}

/* a library call: strlen of the data string */
unsigned char msglen(void) {
    return (unsigned char)strlen(message);
}

/* writes a byte where BASIC asks (a = low byte, x = high byte of the address) */
void poke_from_c(unsigned char lo, unsigned char hi) {
    *(volatile unsigned char *)(lo | (hi << 8)) = 0x5A;
}
