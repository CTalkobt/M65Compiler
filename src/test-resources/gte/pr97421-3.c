// Adapted from SDCC GCC torture test: pr97421-3.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR rtl-optimization/97421 */
/* { dg-additional-options "-fmodulo-sched" } */

int a, b, c;
short d;
void e(void) {
  unsigned f = 0;
  for (; f <= 2; f++) {
    int g[1];
    int h = (long)g;
    c = 0;
    for (; c < 10; c++)
      g[0] = a = 0;
    for (; a <= 2; a++)
      b = d;
  }
}
int main(void) {
  e();
  if (a != 3)
    __builtin_abort();
}
