// Adapted from SDCC GCC torture test: pr58387.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

extern void abort(void);

int a = -1; 

int main ()
{
  int b = a == 0 ? 0 : -a;
  if (b < 1)
    abort ();
  return 0;
}
