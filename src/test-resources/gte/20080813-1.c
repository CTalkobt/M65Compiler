// Adapted from SDCC GCC torture test: 20080813-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR middle-end/37103 */

extern void abort (void);

void
foo (unsigned short x)
{
  signed char y = -1;
  if (x == y)
    abort ();
}

void
bar (unsigned short x)
{
  unsigned char y = -1;
  if (x == y)
    abort ();
}

int
main (void)
{
  if (sizeof (int) == sizeof (short))
    return 0;
  foo (-1);
  if (sizeof (short) > 1)
    bar (-1);
  return 0;
}
