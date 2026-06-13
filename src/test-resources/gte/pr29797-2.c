// Adapted from SDCC GCC torture test: pr29797-2.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

extern void abort(void);

unsigned long bar(void) { return 32768; }

int main()
{
  unsigned long nStyle = bar ();
  if (nStyle & 32768)
    nStyle |= 65536;
  if (nStyle != (32768 | 65536))
    abort ();
  return 0;
}

