// Adapted from SDCC GCC torture test: pr97888-2.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR tree-optimization/97888 */

__attribute__((noipa)) void
foo (int i)
{
  if ((i % 7) >= 0)
    {
      if (i >= 0)
        __builtin_abort ();
    }
}

int
main ()
{
  foo (-7);
  foo (-21);
  return 0;
}
