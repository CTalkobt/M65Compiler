// Adapted from SDCC GCC torture test: 990324-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

void f(long i)
{
  if ((signed char)i < 0 || (signed char)i == 0)
    abort ();
  else
    exit (0);
}

void main(void)
{
  f(0xffffff01);
}

