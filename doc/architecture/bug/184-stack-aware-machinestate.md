# Enhancement #184: Stack-relative MachineState tracking

**Status:** Open
**Labels:** enhancement, optimizer
**Depends on:** OpEffect table (#183 fix)

## Description

Enhance MachineState to track stack-relative register/memory relationships that shift correctly through push/pull operations, instead of treating the stack as a flat block that must be fully invalidated on any SP change.

## Current Behavior

MachineState has `stackMem[256]` indexed by absolute stack offset. Any push/pull operation calls `invalidateAllStack()` or `spModified()`, which throws away all knowledge about stack contents. This forces redundant reloads of frame variables after every push/pull.

## Proposed Behavior

OpEffect should include structured stack effects:
- `pushes: [REG_X]` — declares what register is pushed and that SP decrements
- `pulls: [REG_A]` — declares what register receives the pulled value and that SP increments
- `frame_read: offset` — declares which frame slot is read
- `frame_write: offset` — declares which frame slot is written

When SP changes by ±1 (push/pull), MachineState should:
1. Shift all `SAME_AS_MEM(stack_offset)` references by the delta
2. Shift the `stackMem[]` array contents accordingly
3. Record the new value at the pushed/pulled location
4. Preserve knowledge about unchanged stack slots

### Example

```
Before PHX:    SP = N
  stack[N+1] = known_param_value
  A = SAME_AS_MEM(stack[N+1])

After PHX:     SP = N-1
  stack[N]   = CONSTANT(X_value)     ← newly pushed
  stack[N+1] = known_param_value     ← unchanged, offset shifted
  A = SAME_AS_MEM(stack[N+2])        ← reference adjusted +1
```

## Benefits

- Fewer redundant stack loads/stores in functions with nested calls
- Better optimization across push/pull boundaries (common in argument setup)
- More accurate MachineState for the assembler optimizer's redundant-load elimination

## Implementation Notes

- Builds on the OpEffect table infrastructure
- Stack shift operation should be O(1) if using a circular buffer with a base offset
- Only needs to handle small shifts (±1 for pha/pla/phx/plx/phy/ply/phz/plz, ±2 for phw)
- Frame-pointer mode (`($FP),Y`) is unaffected — this only matters for TSX-based stack access
