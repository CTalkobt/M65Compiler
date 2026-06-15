// Adapted from SDCC GCC torture test: pr57568.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR target/57568 */

extern void abort (void);
int a[6][9] = { }, b = 1, *c = &a[3][5];

int
main ()
{
  if (b && (*c = *c + *c))
    abort ();
  return 0;
}
