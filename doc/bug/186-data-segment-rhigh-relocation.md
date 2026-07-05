# Bug #186: R_HIGH relocation off-by-one for data segment references in simulated ops

**Status:** Open
**Labels:** bugfix, linker

## Description

When a simulated op (e.g. `addr_elem.16`) references a symbol in the data segment via `emitLoadAddrConst`, the R_HIGH relocation produces a high byte that is off by 1 ($27 instead of $28), resulting in an address error of exactly $0100 (256 bytes).

This affects `char *` pointer arrays initialized with string literals when compiled with `-c` and linked. The low byte is correct but the high byte underflows.

## Root Cause

The `emitLoadAddrConst` function in `AssemblerSimulatedOps.cpp` records `recordSymbolRelocHi(name, offset & 0xFF)` with `offset=0`. The R_HIGH `extra` field stores the original low byte for carry correction. If the extra value is $00 but the assembly-time low byte is nonzero (e.g. $0C), the linker's R_HIGH fixup computes the wrong carry, producing a high byte off by 1.

## Reproduction

```c
static char *names[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static char *months[12] = {"Jan","Feb","Mar","Apr","May","Jun",
                            "Jul","Aug","Sep","Oct","Nov","Dec"};
struct info { int wd; int mo; };

void fmt(struct info *t) {
    char *dn = names[t->wd % 7];   // works (first array)
    char *mn = months[t->mo % 12];  // FAILS (second array — address off by $0100)
    // dn[0..2] correct, mn[0..2] reads garbage
}
```

The first array reference works because its data offset happens to not cross a page boundary. The second array's offset is large enough that the R_HIGH carry correction fails.

## Impact

Affects `asctime()` in the stdlib — day name strings display correctly but month name strings are garbled.

## Related

- #185: *p++ codegen (FIXED — DEREF opcode)
- Pointer array element size and string literal init (FIXED)
