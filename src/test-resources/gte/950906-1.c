// Adapted from SDCC GCC torture test: 950906-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

g (int i)
{
}

f (int i)
{
  g (0);
  while ( ({ i--; }) )
    g (0);
}

main ()
{
  f (10);
  exit (0);
}
