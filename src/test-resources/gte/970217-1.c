// Adapted from SDCC GCC torture test: 970217-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

sub (int i, int array[i++])
{
  return i;
}

main()
{
  int array[10];
  exit (sub (10, array) != 11);
}
