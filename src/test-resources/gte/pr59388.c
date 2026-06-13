// Adapted from SDCC GCC torture test: pr59388.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR tree-optimization/59388 */

int a;
struct S { unsigned int f:1; } b;

int
main ()
{
  a = (0 < b.f) | b.f;
  return a;
}
