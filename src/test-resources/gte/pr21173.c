// Adapted from SDCC GCC torture test: pr21173.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

void abort (void);

char q;
void *a[2];

void foo (char *p)
{
  int i;
  for (i = 0; i < 2; i++)
    a[i] += p - &q;
}

int main (void)
{
  int i;
  foo (&q);
  for (i = 0; i < 2; i ++)
    if (a[i])
      abort ();
  return 0;
}
