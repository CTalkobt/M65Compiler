// Adapted from SDCC GCC torture test: pr31072.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

extern volatile int ReadyFlag_NotProperlyInitialized;

volatile int ReadyFlag_NotProperlyInitialized=1;

int main(void)
{
  if (ReadyFlag_NotProperlyInitialized != 1)
    __builtin_abort ();
  return 0;
}
