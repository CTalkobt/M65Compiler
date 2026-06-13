// Adapted from SDCC GCC torture test: pr96549.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR c/96549 */

long c = -1L;
long b = 0L;

int
main ()
{
  if (3L > (short) ((c ^= (b = 1L)) * 3L))
    return 0;
  __builtin_abort ();
}
