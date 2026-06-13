// Adapted from SDCC GCC torture test: pr85169.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR target/85169 */

typedef char V __attribute__((vector_size (64)));

static void __attribute__ ((noipa))
foo (V *p)
{
  V v = *p;
  v[63] = 1;
  *p = v;
}

int
main ()
{
  V v = (V) { };
  foo (&v);
  for (unsigned i = 0; i < 64; i++)
    if (v[i] != (i == 63))
      __builtin_abort ();
  return 0;
}
