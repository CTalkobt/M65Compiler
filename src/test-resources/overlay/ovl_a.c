/* ovl_a.c — overlay a of the overlay test */
#pragma cc45 overlay(a)
#include "ovl.h"

static int a_calls;             /* (BSS: resident, kept across swaps) */
static int a_seen = 100;        /* (data: resident too) */

int a_add(int x, int y) { a_calls++; return x + y; }

/* a call inside the overlay: direct */
int a_twice(int x) { return a_add(x, x); }

/* into overlay b and back: a is copied in again for the return */
int a_via_b(int x) { return b_mul(x, 3) + a_add(1, 1); }

/* a function through a pointer: may be in either overlay or resident */
int a_apply(int (*f)(int), int x) { a_calls++; return f(x) + 1; }

/* recursion bouncing between the overlays */
#pragma cc45 recurse
int a_rec(int n) { a_seen++; return n <= 0 ? 0 : 1 + b_rec(n - 1); }

long a_long(long x) { return x * 3L + 70000L; }

int a_count(void) { return a_calls * 1000 + a_seen; }
