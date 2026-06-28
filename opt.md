# IR Optimization Roadmap

**Status**: Initial IR optimizer foundation in place (constant folding, strength reduction, control flow simplification, dead code elimination).

## High-Priority Optimizations

### 1. **Unreachable Block Elimination** ✓ IMPLEMENTING
After folding constant BR_COND, unreachable blocks persist. Remove blocks that no predecessors can reach.
- Compounds with control flow simplification for significant cleanup
- Unlocks dead code elimination at the block level

### 2. **Constant Propagation with Dataflow** ✓ IMPLEMENTING
Current folding only works within blocks. Add:
- Inter-block constant propagation (track constants flowing through φ nodes)
- Branch-aware analysis: mark branches as always-true/false, eliminates unreachable paths
- Simple worklist-based dataflow pass

### 3. **Algebraic Simplification** (strength reduction extensions)
Current: only MUL by power-of-2 and identity ops. Future additions:
- `SUB x, x → 0`, `AND x, 0 → 0`, `OR x, x → x`, `XOR x, x → 0`
- `LSL/LSR by 0 → identity`
- `ADD x, -y → SUB x, y` (canonicalize negation)
- DIV/MOD by 1 → 0 (for MOD)

### 4. **Dead Block Removal**
Paired with constant folding, eliminate blocks with:
- No incoming edges (after BR_COND folding)
- Only `BR` to themselves
- All φ nodes have undefined predecessors

### 5. **Copy Propagation** ✓ IMPLEMENTING
Track vregs assigned only once from COPY ops, substitute them away.
- Effective for removing scratch vregs created during code generation
- Pairs well with copy elimination

### 6. **Phi Node Simplification**
- Remove φ nodes where all incoming values are identical → replace with that value
- Remove φ nodes where predecessor count = 1 (merge point from single source)

## Medium-Priority

7. **Loop Optimization** — hoist loop-invariant code, eliminate loop-result assignments
8. **Global Value Numbering (GVN)** — eliminate redundant computations (high density, 45 min)
9. **Address Computation Folding** — constant-fold address operations

## Implementation Order

**Phase 1** (this session):
1. Unreachable block elimination (5 min)
2. Dataflow constant propagation (30 min)
3. Copy propagation (15 min)

Expected impact: ~10–15% size reduction on typical code via cascading effects.

**Phase 2** (future):
- Phi simplification
- Global Value Numbering
- Loop optimization

## Integration

Optimizations run iteratively within `IROptimizer::optimize()` until convergence (max 10 iterations). Passes execute in order:
1. foldConstants → constant propagation + folding
2. reduceStrength → algebraic simplification
3. simplifyControlFlow → branch folding
4. eliminateDeadCode → remove unused vregs
5. **eliminateUnreachableBlocks** (new)
6. **copyPropagation** (new)
7. **phiSimplification** (future)

Each pass may enable the next, hence the iteration loop.
