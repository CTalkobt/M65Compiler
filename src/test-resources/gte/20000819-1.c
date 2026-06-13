// Adapted from SDCC GCC torture test: 20000819-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

int a[2] = { 2, 0 };

void foo(int *sp, int cnt)
{
  int *p, *top;

  top = sp; sp -= cnt;

  for(p = sp; p <= top; p++)
    if (*p < 2) exit(0);
}

int main()
{
  foo(a + 1, 1);
  abort();
}
