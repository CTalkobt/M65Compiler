/* math.h — Math functions for cc45 / MEGA65
 *
 * Integer: abs, labs, div, ldiv, min, max, gcd, lcm
 * Float: sin, cos, tan, atan, log, exp, sqrt, fabs (CBM 40-bit via ROM)
 */

#pragma once

#include <stdlib.h>

typedef struct {
    int quot;
    int rem;
} div_t;

typedef struct {
    long quot;
    long rem;
} ldiv_t;

/* abs/labs are declared in stdlib.h per C standard; re-declared here */
#ifdef __NOMACRO_ABS
int abs(int value);
#endif
#ifdef __NOMACRO_LABS
long labs(long value);
#endif

div_t div(int numer, int denom);
ldiv_t ldiv(long numer, long denom);

int min(int a, int b);
int max(int a, int b);
int gcd(int a, int b);
int lcm(int a, int b);

/* Floating-point math functions (CBM 40-bit float via BASIC 65 ROM) */
float sinf(float x);
float cosf(float x);
float tanf(float x);
float atanf(float x);
float logf(float x);
float expf(float x);
float sqrtf(float x);
float fabsf(float x);

/* Additional float math functions (implemented in C using ROM primitives) */
float powf(float x, float y);
float fmodf(float x, float y);
float ceilf(float x);
float floorf(float x);
float roundf(float x);
float truncf(float x);
float atan2f(float y, float x);
float log10f(float x);
float log2f(float x);
float ldexpf(float x, int n);
float frexpf(float x, int *exp);
float modff(float x, float *iptr);
float copysignf(float x, float y);
float fmaxf(float x, float y);
float fminf(float x, float y);
float fdimf(float x, float y);

/* Classification macros */
int __fpclassifyf(float x);
int __isnanf(float x);
int __isinff(float x);
#define isnan(x)    __isnanf(x)
#define isinf(x)    __isinff(x)
#define isfinite(x) (!__isnanf(x) && !__isinff(x))

/* Constants */
#define M_PI        3.14159265
#define M_PI_2      1.57079632
#define M_PI_4      0.78539816
#define M_E         2.71828182
#define M_LOG2E     1.44269504
#define M_LOG10E    0.43429448
#define M_LN2       0.69314718
#define M_LN10      2.30258509
#define M_SQRT2     1.41421356
#define M_SQRT1_2   0.70710678
#define HUGE_VALF   1.7014118e38
#define INFINITY    HUGE_VALF

/* C89/C99 double aliases (downgraded to float on this target) */
#define sin(x)      sinf(x)
#define cos(x)      cosf(x)
#define tan(x)      tanf(x)
#define atan(x)     atanf(x)
#define atan2(y,x)  atan2f(y,x)
#define log(x)      logf(x)
#define log10(x)    log10f(x)
#define log2(x)     log2f(x)
#define exp(x)      expf(x)
#define pow(x,y)    powf(x,y)
#define sqrt(x)     sqrtf(x)
#define fabs(x)     fabsf(x)
#define fmod(x,y)   fmodf(x,y)
#define ceil(x)     ceilf(x)
#define floor(x)    floorf(x)
#define round(x)    roundf(x)
#define trunc(x)    truncf(x)
#define ldexp(x,n)  ldexpf(x,n)
#define frexp(x,e)  frexpf(x,e)
#define modf(x,i)   modff(x,i)
#define copysign(x,y) copysignf(x,y)
#define fmax(x,y)   fmaxf(x,y)
#define fmin(x,y)   fminf(x,y)
#define fdim(x,y)   fdimf(x,y)
