// Adapted from SDCC GCC torture test: 20010605-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

int main ()
{
  int v = 42;

  inline int fff (int x)
    {
      return x*10;
    }

  return (fff (v) != 420);
}
