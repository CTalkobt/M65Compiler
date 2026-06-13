// Adapted from SDCC GCC torture test: pr34130.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

extern void abort (void);
int foo (int i)
{
  return -2 * __builtin_abs(i - 2);
}
int main()
{
  if (foo(1) != -2
      || foo(3) != -2)
    abort ();
  return 0;
}
