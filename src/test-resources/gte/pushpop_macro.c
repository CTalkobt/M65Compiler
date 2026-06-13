// Adapted from SDCC GCC torture test: pushpop_macro.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

extern void abort ();

#define _ 2
#pragma push_macro("_")
#undef _
#define _ 1
#pragma pop_macro("_")

int main ()
{
  if (_ != 2)
    abort ();
  return 0;
}

