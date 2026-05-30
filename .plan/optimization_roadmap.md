# Optimization Roadmap — Future Work

## IR-Level Passes

### Common Subexpression Elimination (CSE)
- Detect identical `ADDR_ELEM` instructions with same base + same index
- Replace later uses with the first result, dead-code eliminate duplicates
- Primary target: `grid[row][col+N]` patterns where `row * 40` is repeated
- Requires: dominance analysis or simple linear scan within basic blocks

### Function Inlining
- Inline `static` functions called with constant arguments
- Constant-fold parameters at inline site (e.g., `grid[10][19]` → direct address)
- Criteria: static, ≤N statements, called ≤M times
- Eliminates: parameter passing, frame setup, multiply for constant indices

### Interprocedural Parameter Narrowing
- Detect static functions where all call sites pass values fitting in `char`
- Generate I8 code path internally while maintaining `int` ABI externally
- Complex: requires whole-program analysis
- Alternative: emit `-Wnarrow-param` warning (implemented) to guide user

## Assembler-Level

### Binary-Mode Dead Store Elimination
- After RELOC_CONST forwarding, `sta $ZP` stores to scratch that are never
  read can be eliminated (the forwarding made the reads use immediate mode)
- Requires tracking ZP reads within simulated op expansions

### Array Indexing Pseudo-Op
- Consolidated `addr_elem.16 dest, base, index, stride` pseudo-op
- Computes `base + index * stride` in one expansion without A:X round-trip
- Would replace mul.16 + add.16 two-step pattern
