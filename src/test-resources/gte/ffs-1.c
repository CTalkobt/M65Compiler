// Adapted from SDCC GCC torture test: ffs-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

__volatile int a = 0;

extern void abort (void);
extern void exit (int);

int
main (void)
{
  if (__builtin_ffs (a) != 0)
    abort ();
  exit (0);
}
