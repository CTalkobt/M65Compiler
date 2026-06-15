// Adapted from SDCC GCC torture test: alias-access-path-2.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

int
main (void)
{
  unsigned char out[][1] = { {71}, {71}, {71} };

  for (int i = 0; i < 3; i++)
    if (!out[i][0])
      __builtin_abort ();
  return 0;
}
