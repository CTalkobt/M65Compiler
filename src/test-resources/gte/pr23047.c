// Adapted from SDCC GCC torture test: pr23047.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* { dg-options "-fwrapv" } */

extern void abort (void);
extern void exit (int);
void f(int i)
{
  i = i > 0 ? i : -i;
  if (i<0)
    return;
  abort ();
}

int main(void)
{
  f(INT_MIN);
  exit (0);
}
