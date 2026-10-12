/* routines.c — C routines for a BASIC 65 program (menu.bas)
 *
 * basic45 --link routines.c builds the C code onto the end of the BASIC
 * program. BASIC calls a function with SYS {name}, a, x, y: A, X and Y are
 * the function's first three parameters (unsigned char or unsigned int),
 * and its result comes back in A/X, read with RREG A, X.
 */
#include <mega65.h>

static unsigned int calls;      /* globals keep their value between SYS calls */

/* 1 + 2 + ... + n */
unsigned int sum_to(unsigned char n) {
    unsigned int s = 0;
    unsigned char i;
    calls++;
    for (i = 1; i <= n; i++) s += i;
    return s;
}

/* how many times sum_to ran */
unsigned int call_count(void) {
    return calls;
}

/* colour the 40-column screen's colour RAM with colour c */
void paint(unsigned char c) {
    volatile unsigned char *colour = (volatile unsigned char *)0xD800;
    unsigned int i;
    for (i = 0; i < 1000; i++) colour[i] = c;
}
