// Adapted from SDCC GCC torture test: pr93582.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR tree-optimization/93582 */

short a;
int b, c;

__attribute__((noipa)) void
foo (void)
{
  b = c;
  a &= 7;
}

int
main ()
{
  c = 27;
  a = 14;
  foo ();
  if (b != 27 || a != 6)
    __builtin_abort ();
  return 0;
}
