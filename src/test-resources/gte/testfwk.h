// testfwk.h — GCC Torture Test Framework adapter for cc45/mmemu
//
// Provides standard headers needed by GTE tests. abort() and exit()
// are real library functions from <stdlib.h> — no macro overrides.
//
// Tests are compiled with: cc45 -c -Isrc/test-resources/gte test.c
// linked with: ln45 test.o45 lib/build/c45.lib -o test.prg
// and run in mmemu, checking exit behavior (BRK for abort, RTS for exit).

#ifndef TESTFWK_H
#define TESTFWK_H

#include <stdlib.h>
#include <string.h>
#include <limits.h>

// cc45: int is 16-bit; GTE tests written for 32-bit int need long for
// bitfields >16 bits. Define int32_bf as the 32-bit-backed bitfield type.
#ifdef __cc45__
typedef long int32_bf;
#else
typedef int int32_bf;
#endif

#endif // TESTFWK_H
