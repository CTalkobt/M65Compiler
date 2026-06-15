// Adapted from SDCC GCC torture test: builtin-constant.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR optimization/8423.  */

#define btest(x) __builtin_constant_p(x) ? "1" : "0"

#ifdef __OPTIMIZE__
void
foo (char *i)
{
  if (*i == '0')
    abort ();
}
#else
void
foo (char *i)
{
}
#endif

int
main (void)
{
  int size = sizeof (int);
  foo (btest (size));
  foo (btest (size));
  exit (0);
}
