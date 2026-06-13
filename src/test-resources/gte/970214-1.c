// Adapted from SDCC GCC torture test: 970214-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

#define L 1
void main (void)
{
  exit (L'1' != L'1');
}
