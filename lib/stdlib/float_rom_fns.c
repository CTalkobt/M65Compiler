/* float_rom_fns.c — ROM-backed math functions (sinf, cosf, ...)
 *
 * Each wraps a __float_* routine from float_rom.s45, which operates on the
 * 5-byte buffer __float_a. Written in C so they follow the compiler's calling
 * conventions, including the hidden result pointer for float returns.
 * (C names gain a leading underscore: _float_a is the symbol __float_a.)
 *
 * The ROM reports a domain or range error as a BASIC error (?ILLEGAL
 * QUANTITY, ?OVERFLOW), which never returns to the program, so those
 * arguments are caught here: the result is the nearest representable value
 * (there is no NaN or infinity in the CBM format) and errno is set.
 */

#include <errno.h>

#define FLOAT_MAX 1.7014118342085515e38

extern float _float_a;
void _float_sin(void);
void _float_cos(void);
void _float_tan(void);
void _float_atn(void);
void _float_log(void);
void _float_exp(void);
void _float_sqr(void);
void _float_abs(void);

float sinf(float x)  { _float_a = x; _float_sin(); return _float_a; }
float cosf(float x)  { _float_a = x; _float_cos(); return _float_a; }
float tanf(float x)  { _float_a = x; _float_tan(); return _float_a; }
float atanf(float x) { _float_a = x; _float_atn(); return _float_a; }

/* log(0) is -inf: -FLOAT_MAX, ERANGE. log(x < 0) has no value: 0, EDOM. */
float logf(float x) {
    if (x == 0.0) { errno = ERANGE; return -FLOAT_MAX; }
    if (x < 0.0) { errno = EDOM; return 0.0; }
    _float_a = x; _float_log(); return _float_a;
}

/* e^x overflows above ln(FLOAT_MAX) = 88.0296919 (the ROM stops with
   ?OVERFLOW ERROR): FLOAT_MAX, ERANGE. Far below -88.7 it underflows to 0
   (and the ROM's own scaling overflows for huge negative x): 0, ERANGE. */
float expf(float x) {
    if (x > 88.0296) { errno = ERANGE; return FLOAT_MAX; }
    if (x < -88.8) { errno = ERANGE; return 0.0; }
    _float_a = x; _float_exp(); return _float_a;
}

/* sqrt(x < 0) has no value: 0, EDOM */
float sqrtf(float x) {
    if (x < 0.0) { errno = EDOM; return 0.0; }
    _float_a = x; _float_sqr(); return _float_a;
}
float fabsf(float x) { _float_a = x; _float_abs(); return _float_a; }
