// Adapted from SDCC GCC torture test: pr97695.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

int *a, b, **c = &a, d, e;

int f(int g, int h) { return !h || (g && h == 1) ? 0 : g / h; }

static void *i(int g) {
  while (e < 2)
    if (!f(g, 9)) {
      while (b)
        ;
      return 0;
    }
  return 0;
}

void j() {
  i(1);
  *c = i(d);
}

int main() { j(); return 0; }
