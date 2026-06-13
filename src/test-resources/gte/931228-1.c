// Adapted from SDCC GCC torture test: 931228-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

f (x)
{
  x &= 010000;
  x &= 007777;
  x ^= 017777;
  x &= 017770;
  return x;
}

main ()
{
  if (f (-1) != 017770)
    abort ();
  exit (0);
}
