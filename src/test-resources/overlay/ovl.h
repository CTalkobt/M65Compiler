/* ovl.h — the functions of the overlay test (test_overlay_xemu.sh) */
#pragma once

/* resident (main.c) */
int res_inc(int x);
int res_via_b(int x);
extern int res_calls;

/* overlay a (ovl_a.c) */
int a_add(int x, int y);
int a_twice(int x);
int a_via_b(int x);
int a_apply(int (*f)(int), int x);
int a_rec(int n);
long a_long(long x);
int a_count(void);
int a_via_res(int x);

/* overlay b (ovl_b.c) */
int b_mul(int x, int y);
int b_neg(int x);
int b_rec(int n);
int b_count(void);
int b_file(void);
