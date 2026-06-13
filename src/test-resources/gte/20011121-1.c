// Adapted from SDCC GCC torture test: 20011121-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

struct s
{
  int i[18];
  char f;
  char b[2];
};

struct s s1;

int
main()
{
  struct s s2;
  s2.b[0] = 100;
  __builtin_memcpy(&s2, &s1, ((unsigned int) &((struct s *)0)->b));
  if (s2.b[0] != 100)
    abort();
  exit(0);
}
