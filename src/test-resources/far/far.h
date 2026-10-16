/* far.h — __far functions (cc45), kept in far memory and run through the
 * mapped window (ln45). A prototype may say __far or not: only the
 * definition decides where the code goes. */
#pragma once

extern int counter;                      /* main.c (resident) */
int resident_inc(int x);                 /* main.c (resident) */

__far int fa_add(int a, int b);          /* far_a.c */
int fa_sq_inc(int x);
int fa_apply(int (*f)(int), int x);
int fa_rec(int n);
unsigned char fa_group(void);
unsigned int fa_where(void);

int fb_mul(int a, int b);                /* far_b.c */
long fb_long(long x);
int fb_rec(int n);
unsigned char fb_group(void);
