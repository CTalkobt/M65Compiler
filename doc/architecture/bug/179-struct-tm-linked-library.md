# Bug #179: struct tm functions produce wrong results when linked from library

**Status:** Open
**Labels:** bugfix

## Description

Functions that operate on struct tm (mktime, asctime) produce incorrect results when called from user code linked against the library. The struct member access appears to read wrong offsets.

## Reproduction

```c
#include <time.h>
struct tm t;
t.tm_year = 126; t.tm_mon = 6; t.tm_mday = 3;
t.tm_hour = 12; t.tm_min = 0; t.tm_sec = 0;
time_t result = mktime(&t);   // Expected: non-zero, Got: 0
```

## Possible Cause

Struct layout mismatch between the library compilation and user code compilation, or incorrect pointer-to-struct parameter handling across linked objects.

## Investigation Notes

- Struct tm layout verified correct in caller (members at correct offsets, 2 bytes each)
- Inline mktime (same file) works correctly — reads all struct members properly
- Separately compiled mktime with trace output shows correct struct member values
- The full `lib/stdlib/rtc.c` fails but a minimal version of just mktime works
- The issue appears related to the full rtc.c compilation having many static functions/variables that interfere
- Minimal mktime (without rtc_read, localtime, gmtime, asctime, ctime) returns correct results when linked

## Impact

Blocks calendar time functions (mktime, asctime, ctime, localtime) from working via library linking.
