// Adapted from SDCC GCC torture test: pr58385.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR tree-optimization/58385 */

extern void abort (void);

int a, b = 1;

int
foo ()
{
  b = 0;
  return 0;
}

int
main ()
{
  ((0 || a) & foo () >= 0) <= 1 && 1;
  if (b)
    abort ();
  return 0;
}
