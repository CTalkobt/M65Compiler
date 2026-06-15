// Adapted from SDCC GCC torture test: medce-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"


extern void abort (void);
extern void link_error (void);

static int ok = 0;

void bar (void)
{
  ok = 1;
}

void foo(int x)
{
  switch (x)
  {
  case 0:
    if (0)
    {
      link_error();
  case 1:
      bar();
    }
  }
}

int main()
{
  foo (1);
  if (!ok)
    abort ();
  return 0;
}

