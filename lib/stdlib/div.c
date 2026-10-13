/* div.c — integer quotient and remainder (truncates toward zero, C99) */
#include <math.h>

div_t div(signed int numer, signed int denom) {
    div_t r;
    r.quot = numer / denom;
    r.rem = numer % denom;
    return r;
}
