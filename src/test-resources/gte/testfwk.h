// testfwk.h — GCC Torture Test Framework adapter for cc45/mmemu
//
// Maps abort()/exit() to memory-mapped result reporting:
//   $4000 = 0xAA on success (exit(0))
//   $4000 = 0xFF on failure (abort())
//
// Tests are compiled with: cc45 -DTESTFWK test.c -o test.s
// and run in mmemu, checking $4000 for 0xAA.

#ifndef TESTFWK_H
#define TESTFWK_H

// Result reporting address
#define RESULT_ADDR ((volatile unsigned char *)0x4000)

// Replace abort() — test failure
#define abort() do { *RESULT_ADDR = 0xFF; __asm__("brk"); } while(0)

// Replace exit() — test success (only exit(0) means pass)
#define exit(code) do { *RESULT_ADDR = ((code) == 0) ? 0xAA : 0xFF; __asm__("brk"); } while(0)

// Suppress string.h include — provide minimal stubs
#ifndef _STRING_H
#define _STRING_H
// cc45 provides these via <string.h>, but include guard prevents double-include
#endif

#endif // TESTFWK_H
