// Adapted from SDCC GCC torture test: 20080222-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* suppress warning W_IDX_OUT_OF_BOUNDS as this is tested here */
#pragma disable_warning 24

extern void abort (void);

struct container
{
  unsigned char data[1];
};

unsigned char space[6] = {1, 2, 3, 4, 5, 6};

int
foo (struct container *p)
{
  return p->data[4];
}

int
main ()
{
  if (foo ((struct container *) space) != 5)
    abort ();
  return 0;
}
