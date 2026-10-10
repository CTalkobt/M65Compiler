/* ldiv.c — long quotient and remainder (truncates toward zero, C99) */
#include <math.h>

ldiv_t ldiv(signed long numer, signed long denom) {
    ldiv_t r;
    r.quot = numer / denom;
    r.rem = numer % denom;
    return r;
}
