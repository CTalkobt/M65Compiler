# Bug #183: bsearch returns NULL for existing elements when linked from library

**Status:** FIXED
**Labels:** bugfix

## Description

bsearch returns NULL when searching for a known-existing element in a sorted array. qsort was also affected but is now fixed.

## Root Causes Found

### 1. Unsigned int default (FIXED)
cc45 defaults `int` to unsigned. This caused:
- Integer literals (e.g., `0`, `2`) to be unsigned, making `bothSigned = false` in comparison/arithmetic ops
- `>`, `<=`, `>=`, `<` comparisons using unsigned CMP.16 instead of signed CMP.S16
- `/ 2` using unsigned LSR instead of signed division
- qsort's `cmp_positive` function using manual sign-bit checks that could fail

**Fix:** All stdlib code now uses explicit `signed int`. Compiler's `getExprTypeInfo()` now propagates signedness through binary sub-expressions. Unsigned literals in signed range (0-32767) are promoted to signed when compared against signed operands.

### 2. CMP.16 comparison order (FIXED in earlier commit)
CMP.16 and CMP.S16 compared low bytes first, then high bytes. This produced wrong flag results when high bytes were equal. Fixed to compare high bytes first.

### 3. Store-forwarding corrupts hi byte after frame store (FIXED)
The store-forwarding optimization in IRCodeGen's STORE handler assumed AX was still valid after the previous instruction's `storeVreg()` call. However, when the dest vreg was frame-allocated, `stax.fp` internally uses TSX which clobbers X (replacing the hi byte with the stack pointer). The next STORE would then write the stack pointer as the hi byte, corrupting the value.

**Root cause:** `stax.fp` performs `PHA; TXA; TAZ; PLA; TSX; STA ...,X; TSX; STZ ...,X`. After this sequence, X = SP (not the original hi byte), and Z = original hi byte. The store-forwarding path blindly set `valueByte_[1] = REG_X`, not accounting for the clobber.

**Fix:** Two changes in `IRCodeGen.cpp`:
1. Store-forwarding path now checks if the source vreg was frame-allocated. If so, sets `valueByte_[1] = REG_Z` (since stax.fp leaves hi byte in Z), causing the second store to use `staz.fp` instead.
2. All `resultInAX_` assignment sites now check the dest vreg allocation — if frame-stored, `resultInAX_` is set to -1 (preventing stale forwarding).

## Reproduction

```c
signed int sorted[5] = {10, 20, 30, 40, 50};
signed int cmp(const void *a, const void *b) { return *(signed int*)a - *(signed int*)b; }
signed int key = 30;
void *found = bsearch(&key, sorted, 5, sizeof(signed int), cmp);
// FIXED: found correctly points to sorted[2]
```
