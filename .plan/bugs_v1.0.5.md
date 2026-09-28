# v1.0.5 Known Bugs

## Initialized global pointers placed in BSS instead of DATA

**Status**: Open
**Severity**: High
**Found**: 2026-09-28

**Issue**: Global pointer variables with constant initializers are placed in BSS (zero-filled) instead of DATA (with initial value).

```c
volatile char *r = (char *)0xC000;  // r ends up as 0x0000, not 0xC000
```

**Impact**: All writes through `r[i]` go to address $0000 instead of $C000. Affects all xemu tests that use `volatile char *r = (char *)0x4000` pattern.

**Workaround**: Use direct volatile casts instead:
```c
#define R(i) (*(volatile unsigned char *)(0xC000 + (i)))
```

**Root cause**: Likely in IRBuilder global variable handling — initializer value not propagated to linker DATA segment for pointer types.

## alloca() incompatible with cc45 calling convention

**Status**: Open
**Severity**: Medium
**Found**: 2026-09-28

**Issue**: `alloca()` in `lib/stdlib/alloca.s45` pops arguments from the hardware stack (`pla`), but cc45 passes function arguments on the software parameter stack (`__sp_base`). This causes stack corruption and BRK.

**Workaround**: Use local arrays (compiler-managed stack frames) instead of `alloca()`.

**Fix needed**: Rewrite `alloca.s45` to read size from the software parameter stack, or implement alloca as a compiler intrinsic.

## Function call with local arrays doesn't return correctly

**Status**: Open
**Severity**: High
**Found**: 2026-09-28

**Issue**: When a function containing local arrays is called from another function, the caller's execution doesn't resume after the call returns. The callee executes correctly (writes to memory are visible) but the return doesn't work.

```c
void do_work() {
    unsigned char a[4];
    a[0] = 0x42;
    // This works: *(volatile unsigned char *)0xC000 = 0x01;
}
void main() {
    do_work();
    // This never executes:
    *(volatile unsigned char *)0xC001 = 0xAA;
}
```

**Workaround**: Put all array operations in `main()` directly instead of separate functions.

**Root cause**: Likely SAC (static allocation) frame management corrupting return address when local arrays are present. Needs investigation of frame setup/teardown in IRCodeGen for functions with local array variables.
