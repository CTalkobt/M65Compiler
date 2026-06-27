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

/* C89/C99 double aliases (downgraded to float on this target) */
#define sin(x)   sinf(x)
#define cos(x)   cosf(x)
#define tan(x)   tanf(x)
#define atan(x)  atanf(x)
#define log(x)   logf(x)
#define exp(x)   expf(x)
#define sqrt(x)  sqrtf(x)
#define fabs(x)  fabsf(x)
