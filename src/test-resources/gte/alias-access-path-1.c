// Adapted from SDCC GCC torture test: alias-access-path-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* Test that variable
     int val;
   may hold value of tyope "struct c" which has same size.
   This is valid in GIMPLE memory model.  */

struct a {int val;} a={1},a2;
struct b {struct a a;};
int val;
struct c {struct b b;} *cptr=(void *)&val;

int
main(void)
{
  cptr->b.a=a;
  val = 2;
  a2=cptr->b.a;
  if (a2.val == a.val)
    __builtin_abort ();
}
