// Adapted from SDCC GCC torture test: pr22061-3.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

void
bar (int N)
{
  int foo (char a[2][++N]) { N += 4; return sizeof (a[0]); }
  if (foo (0) != 2)
    abort ();
  if (foo (0) != 7)
    abort ();
  if (N != 11)
    abort ();
}

int
main()
{
  bar (1);
  exit (0);
}
