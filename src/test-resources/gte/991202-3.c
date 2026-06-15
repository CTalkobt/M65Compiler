// Adapted from SDCC GCC torture test: 991202-3.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"


unsigned int f (unsigned int a)
{
  return a * 65536 / 8;
}

unsigned int g (unsigned int a)
{
  return a * 65536;
}

unsigned int h (unsigned int a)
{
  return a / 8;
}

int main ()
{
  if (f (65536) != h (g (65536)))
    abort ();
  exit (0);
}
