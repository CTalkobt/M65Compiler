/* ovl_b.c — overlay b of the overlay test */
#pragma cc45 overlay(b)
#include <stdio.h>
#include <string.h>
#include "ovl.h"

static int b_calls;

int b_mul(int x, int y) { b_calls++; return res_inc(x * y) - 1; }

int b_neg(int x) { b_calls++; return -x; }

#pragma cc45 recurse
int b_rec(int n) { return n <= 0 ? 0 : 10 + a_rec(n - 1); }

int b_count(void) { return b_calls; }

/* disk I/O from inside an overlay (the KERNAL's disk code changes the
   memory map itself): write a file, read it back */
int b_file(void) {
    char buf[16];
    FILE *f = fopen("ovlfile,s,w", "w");
    if (!f) return 0;
    fputs("overlay io", f);
    fclose(f);
    f = fopen("ovlfile,s,r", "r");
    if (!f) return 0;
    buf[0] = 0;
    fgets(buf, sizeof buf, f);
    fclose(f);
    return strcmp(buf, "overlay io") == 0;
}
