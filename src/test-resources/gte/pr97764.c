// Adapted from SDCC GCC torture test: pr97764.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR tree-optimization/97764 */
/* { dg-require-effective-target int32plus } */

struct S { long b : 3; long c : 28; long d : 1; };

int
main ()
{
  struct S e = {};
  e.c = -1;
  if (e.d)
    __builtin_abort ();
  return 0;
}
