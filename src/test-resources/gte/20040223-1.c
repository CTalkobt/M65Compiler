// Adapted from SDCC GCC torture test: 20040223-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* { dg-require-effective-target alloca } */
#include <string.h>


void
a(void *x,int y)
{
  if (y != 1234)
    abort ();
}

int
main()
{
  a(strcpy(alloca(100),"abc"),1234);
  return 0;
}
