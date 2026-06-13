// Adapted from SDCC GCC torture test: pr94134.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR target/94134 */

static volatile int a = 0;
static volatile int b = 1;

int
main ()
{
  a++;
  b++;
  if (a != 1 || b != 2)
    __builtin_abort ();
  return 0;
}
