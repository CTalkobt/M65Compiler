#pragma once
#include "IR.hpp"

namespace ir {
    // Run Common Subexpression Elimination and Copy Propagation on the IR Module
    void optimizeCSE(Module& mod);

    // Run Loop-Invariant Code Motion: hoist invariant computations out of loops
    void optimizeLICM(Module& mod);

    // Algebraic simplification: eliminate identity/annihilator/idempotent patterns
    void optimizeAlgebraic(Module& mod);

    // COPY chain elimination: resolve transitive copies, remove dead COPYs
    void optimizeCopyChains(Module& mod);

    // Type narrowing: replace wide ops with I8 when result is truncated
    void optimizeTypeNarrowing(Module& mod);

    // Branch folding and dead block elimination
    void optimizeBranchFold(Module& mod);

    // Strength reduction: replace expensive ops with cheaper equivalents
    // (MUL by power-of-2 → LSL, DIV_U by power-of-2 → LSR, MOD_U by power-of-2 → AND)
    void optimizeStrengthReduction(Module& mod);
}
