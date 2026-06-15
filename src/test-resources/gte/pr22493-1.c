// Adapted from SDCC GCC torture test: pr22493-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* { dg-options "-fwrapv" } */


extern void abort ();
extern void exit (int);
void f(int i)
{
  if (i>0)
    abort();
  i = -i;
  if (i<0)
    return;
  abort ();
}

int main(void)
{
  f(INT_MIN);
  exit (0);
}
