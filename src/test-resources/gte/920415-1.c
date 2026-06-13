// Adapted from SDCC GCC torture test: 920415-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* { dg-require-effective-target label_values } */
main(){__label__ l;void*x(){return&&l;}goto*x();abort();return;l:exit(0);}
