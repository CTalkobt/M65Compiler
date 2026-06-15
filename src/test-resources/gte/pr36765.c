// Adapted from SDCC GCC torture test: pr36765.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

int __attribute__((noinline))
foo(int i)
{
  int *p = __builtin_malloc (4 * sizeof(int));
  *p = 0;
  p[i] = 1;
  return *p;
}
extern void abort (void);
int main()
{
  if (foo(0) != 1)
    abort ();
  return 0;
}
