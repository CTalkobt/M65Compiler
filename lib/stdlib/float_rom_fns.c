/* float_rom_fns.c — ROM-backed math functions (sinf, cosf, ...)
 *
 * Each wraps a __float_* routine from float_rom.s45, which operates on the
 * 5-byte buffer __float_a. Written in C so they follow the compiler's calling
 * conventions, including the hidden result pointer for float returns.
 * (C names gain a leading underscore: _float_a is the symbol __float_a.)
 */

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
float logf(float x)  { _float_a = x; _float_log(); return _float_a; }
float expf(float x)  { _float_a = x; _float_exp(); return _float_a; }
float sqrtf(float x) { _float_a = x; _float_sqr(); return _float_a; }
float fabsf(float x) { _float_a = x; _float_abs(); return _float_a; }
