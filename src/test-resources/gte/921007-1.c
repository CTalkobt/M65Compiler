// Adapted from SDCC GCC torture test: 921007-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

static int strcmp(){return-1;}
#define strcmp __builtin_strcmp
main()
{
if(strcmp("X","X\376")>=0)abort();
exit(0);
}
