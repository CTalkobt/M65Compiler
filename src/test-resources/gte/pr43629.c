// Adapted from SDCC GCC torture test: pr43629.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* suppress warning W_LOCAL_NOINIT, it is valid and tested */
#pragma disable_warning 84

int flag;
extern void abort (void);
int main(void)
{
  int x;
  if (flag)
    x = -1;
  else
    x &= 0xff;
  if (x & ~0xff)
    abort ();
  return 0;
}
