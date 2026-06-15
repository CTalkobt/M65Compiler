// Adapted from SDCC GCC torture test: pr33631.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

typedef union
{
  int __lock;
} pthread_mutex_t;

extern void abort (void);

int main()
{
    struct { int c; pthread_mutex_t m; } r = { .m = 0 };
    if (r.c != 0)
      abort ();
    return 0;
}
