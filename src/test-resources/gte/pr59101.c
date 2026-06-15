// Adapted from SDCC GCC torture test: pr59101.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR target/59101 */

__attribute__((noinline, noclone)) int
foo (int a)
{
  return (~a & 4102790424LL) > 0 | 6;
}

int
main ()
{
  if (foo (0) != 7)
    __builtin_abort ();
  return 0;
}
