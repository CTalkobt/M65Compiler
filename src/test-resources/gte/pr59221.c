// Adapted from SDCC GCC torture test: pr59221.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"



int a = 1, b, d;
short e;

int
main ()
{
  for (; b; b++)
    ;
  short f = a;
  int g = 15;
  e = f ? f : 1 << g;
  int h = e;
  d = h == 83647 ? 0 : h;
  if (d != 1)
    __builtin_abort ();
  return 0;
}
