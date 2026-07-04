# Bug #183: bsearch returns NULL for existing elements when linked from library

**Status:** Open
**Labels:** bugfix

## Description

bsearch returns NULL when searching for a known-existing element in a sorted array, but only when bsearch is linked from the library. Compiling bsearch.c directly with user code works correctly.

## Reproduction

```c
#include <stdlib.h>
int sorted[5] = {10, 20, 30, 40, 50};
int cmp(const void *a, const void *b) { return *(int*)a - *(int*)b; }
int key = 30;
int *found = bsearch(&key, sorted, 5, sizeof(int), cmp);
// found is NULL (should point to sorted[2])
```

## Likely Cause

The `const void *key` parameter or the mid-point calculation `lo + (hi - lo) / 2` may produce incorrect results when compiled through the library. Possibly related to `const` qualifier handling across linked objects.

## Investigation Notes

- qsort works correctly from library (same calling pattern but different parameter types)
- qsort uses `void *` parameters, bsearch uses `const void *` — const may cause issues
- The `do_cmp` wrapper pattern works for qsort but fails for bsearch
- May be related to how `const void *key` is passed as first argument to the comparator

## Related

- #179: struct tm across linked objects (similar cross-object issue)
