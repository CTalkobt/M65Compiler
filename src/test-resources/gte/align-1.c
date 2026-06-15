// Adapted from SDCC GCC torture test: align-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

typedef int new_int __attribute__ ((aligned(16)));
struct S { int x; };
 
int main()
{
  if (sizeof(struct S) != sizeof(int))
    abort ();
  return 0;
}
