// Adapted from SDCC GCC torture test: 920410-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* { dg-require-stack-size "40000 * 4 + 256" } */

main(){int d[40000];d[0]=0;exit(0);}

