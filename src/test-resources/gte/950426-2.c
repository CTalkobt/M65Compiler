// Adapted from SDCC GCC torture test: 950426-2.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

void main(void)
{
  long int i = -2147483647L - 1L; /* 0x80000000 */
  char ca = 1;

  if (i >> ca != -1073741824L)
    abort ();

  if (i >> i / -2000000000L != -1073741824L)
    abort ();

  exit (0);
}
