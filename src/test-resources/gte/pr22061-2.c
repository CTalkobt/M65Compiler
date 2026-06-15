// Adapted from SDCC GCC torture test: pr22061-2.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

int *x;
static void bar (char a[2][(*x)++]) {}
int
main (void)
{
  exit (0);
}
