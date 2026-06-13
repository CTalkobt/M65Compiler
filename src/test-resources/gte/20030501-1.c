// Adapted from SDCC GCC torture test: 20030501-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

int
main (int argc, char **argv)
{
  int size = 10;

  {
    int retframe_block()
      {
        return size + 5;
      }

  if (retframe_block() != 15)
      abort ();
  exit (0);

  }
}
