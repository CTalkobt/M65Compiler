// Adapted from SDCC GCC torture test: 20050203-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* Reduced testcase extracted from Samba source code.  */



static void __attribute__((__noinline__))
     foo (unsigned char *p) {
  *p = 0x81;
}

static void __attribute__((__noinline__))
     bar (int x) {
  asm ("");
}

int main() {
  unsigned char b;

  foo(&b);
  if (b & 0x80)
    {
      bar (b & 0x7f);
      exit (0);
    }
  else
    {
      bar (b & 1);
      abort ();
    }
}
