// Adapted from SDCC GCC torture test: pr97836.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

int a;

int b(int c) { (void)c; return 0; }

static int *d(int *e) {
  if (a) {
    a = a && b(*e);
  }
  return e;
}

int main() {
  int f;
  if (d(&f) != &f)
    __builtin_abort();
  return 0;
}
