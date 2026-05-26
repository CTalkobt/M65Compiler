/* math.h — Integer math functions for cc45 / MEGA65
 *
 * No floating-point support (no FPU on 45GS02).
 * Provides integer abs, labs, div, ldiv.
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

/* TODO: future software-float functions (sqrt, sin, cos, etc.)
 * would go here with optional EDOM validation via errno */
