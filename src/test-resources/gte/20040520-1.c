// Adapted from SDCC GCC torture test: 20040520-1.c
// Original: gcc/testsuite/gcc.c-torture/execute/
#include "testfwk.h"

/* PR 15454 */

void abort ();
int main () {
        int foo;
        int bar (void)
        {
                int baz = 0;
                if (foo!=45)
                        baz = foo;
                return baz;
        }
        foo = 1;
        if (!bar ())
                abort ();
        return 0;
}
