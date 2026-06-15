// Adapted from SDCC GCC torture test: 940115-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

f (cp, end)
     char *cp;
     char *end;
{
  return (cp < end);
}

main ()
{
  if (! f ((char *) 0, (char *) 1))
    abort();
  exit (0);
}
