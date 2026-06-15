// Adapted from SDCC GCC torture test: pr52209.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR middle-end/52209 */

extern void abort (void);
struct S0 { int f2 : 1; } c;
int b;

int
main ()
{
  b = -1 ^ c.f2;
  if (b != -1)
    abort ();
  return 0;
}
