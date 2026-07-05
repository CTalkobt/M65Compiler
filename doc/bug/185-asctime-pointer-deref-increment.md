# Bug #185: asctime produces empty string — *p++ codegen for char pointers

**Status:** Open
**Labels:** bugfix, codegen

## Description

`asctime()` returns a valid pointer to `__asc_buf` but the buffer is empty (all zeros). The function body uses `*p++ = expr` to build the output string character by character, but the generated code does not write through the pointer — it stores the character value to a temporary instead of dereferencing `p`.

## Root Cause

The IRBuilder/IRCodeGen for `*p++ = value` where `p` is `char *` appears to generate incorrect code. Instead of:
1. Loading the value
2. Storing it at the address pointed to by `p` (indirect store)
3. Incrementing `p`

The generated assembly stores the character to a ZP temporary and computes pointer arithmetic, but never performs the indirect `sta (p),y` store through the pointer.

## Reproduction

```c
#include <time.h>
volatile char *r = (char *)0x4000;
void main() {
    struct tm t;
    t.tm_year = 126; t.tm_mon = 0; t.tm_mday = 15;
    t.tm_hour = 9; t.tm_min = 5; t.tm_sec = 30;
    t.tm_wday = 4; t.tm_yday = 14; t.tm_isdst = 0;
    char *s = asctime(&t);
    // s points to valid buffer but contains all zeros
    r[0] = s[0];  // expected 'T' (0x54), got 0x00
}
```

Simpler reproduction (no library needed):
```c
volatile char *r = (char *)0x4000;
void main() {
    char buf[8];
    char *p = buf;
    *p++ = 'A';
    *p++ = 'B';
    *p = 0;
    r[0] = buf[0];  // expected 'A' (0x41)
    r[1] = buf[1];  // expected 'B' (0x42)
    r[2] = 0xAA;
}
```

## Related

- #183 (bsearch — fixed): store-forwarding corruption after frame stores
- #179 (struct tm): struct member access across linked objects — appears to work now; mktime passes
- Pointer array element size and string literal init bugs were fixed alongside this investigation
