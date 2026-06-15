// Adapted from SDCC GCC torture test: 970214-2.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

#define m(L) (L'1' + (L))
void main (void)
{
  exit (m (0) != L'1');
}
