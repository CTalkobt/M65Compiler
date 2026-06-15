// Adapted from SDCC GCC torture test: 20040709-3.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR rtl-optimization/68205 */
/* { dg-require-effective-target int32plus } */
/* { dg-additional-options "-fno-common" } */

#include "20040709-2.c"
