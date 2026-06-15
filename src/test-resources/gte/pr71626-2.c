// Adapted from SDCC GCC torture test: pr71626-2.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR middle-end/71626 */
/* { dg-additional-options "-fpic" { target fpic } } */

#include "pr71626-1.c"
