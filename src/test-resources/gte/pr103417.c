// Adapted from SDCC GCC torture test: pr103417.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR tree-optimization/103417 */

struct { int a : 8; int b : 24; } c = { 0, 1 };

int
main ()
{
  if (c.b && !c.b)
    __builtin_abort ();
  return 0;
}
