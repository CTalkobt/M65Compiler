// Adapted from SDCC GCC torture test: pr97421-2.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR rtl-optimization/97421 */
/* { dg-additional-options "-fmodulo-sched -fno-dce -fno-strict-aliasing" } */

static int a, b, c;
int *d = &c;
int **e = &d;
int ***f = &e;
int main()
{
  int h;
  for (a = 2; a; a--)
    for (h = 0; h <= 2; h++)
      for (b = 0; b <= 2; b++)
        ***f = 6;

  if (b != 3)
    __builtin_abort();
}
