// Adapted from SDCC GCC torture test: pr55875.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

int a[251];
__attribute__ ((noinline))
t(int i)
{
  if (i==0)
    exit(0);
  if (i>255)
    abort ();
}
main()
{
  unsigned int i;
  for (i=0;;i++)
    {
      a[i]=t((unsigned char)(i+5));
    }
}
