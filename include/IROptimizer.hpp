#pragma once
#include "IR.hpp"
#include <functional>
#include <set>
#include <ostream>

namespace ir {
    // Trace stream for IR optimizer logging (-Roptir)
    // When non-null, optimization passes report actions to this stream
    extern std::ostream* optTrace;

    // Per-function switch consulted by every pass: (function, catalog name of
    // the pass) -> run it on this function? Unset: run on every function.
    extern std::function<bool(const Function&, const char*)> passEnabled;

    // Run Common Subexpression Elimination and Copy Propagation on the IR Module
    void optimizeCSE(Module& mod);

    // Run Loop-Invariant Code Motion: hoist invariant computations out of loops
    void optimizeLICM(Module& mod);

    // Algebraic simplification: eliminate identity/annihilator/idempotent patterns
    void optimizeAlgebraic(Module& mod);

    // ADDR_ELEM fusion: merge single-use ADDR_ELEM into following LOAD/STORE
    void optimizeAddrElemFusion(Module& mod);
    // *p++: the access uses p, the increment moves after it
    void optimizePostIncrementSink(Module& mod);

    // COPY chain elimination: resolve transitive copies, remove dead COPYs
    void optimizeCopyChains(Module& mod);

    // Parameter narrowing: an int/long parameter of a static function whose
    // every call passes a value that fits in fewer bytes gets that type
    void optimizeParamNarrowing(Module& mod);

    // Type narrowing: replace wide ops with I8 when result is truncated
    void optimizeTypeNarrowing(Module& mod);

    // Branch folding and dead block elimination
    void optimizeBranchFold(Module& mod);

    // Loop unrolling: fully unroll counted for loops whose unrolled code is
    // estimated to fit unrollMaxBytes (-fmax-unroll-bytes)
    extern int unrollMaxBytes;
    void optimizeLoopUnroll(Module& mod);

    // Remove pure instructions whose temporary result is never used
    // A leaf function's pointer parameters used twice or more: a register
// variable (ZP) copy, so dereferences use the pointer in place
void leafPointerParamsToZp(Module& mod);
// A ZP-convention leaf function keeps its parameters in the ZP parameter
// block where the caller put them (nothing it runs can overwrite it)
bool paramsStayInZp(const Function& fn);
void removeDeadTemps(Module& mod);
// a[x + c] with a global base: element x of the base + c*stride (after the passes)
void foldElemIndexOffset(Module& mod);
// a load / store through add ptr @g, c: at @g+c itself (a member of a global
// struct, a constant-index element; after the passes)
void foldGlobalOffsetAccess(Module& mod);
// x / c and x % c in one block: one divide, x % c = x - (x / c) * c (#352)
void optimizeShareDivMod(Module& mod);
// p[c] through a far pointer: the access at p with the byte offset c (its
// Z index; c >= 256: p + (c & $FF00) shared by the accesses in a block)
void foldFarOffset(Module& mod);
// a commutative operation whose second operand is the instruction just
// before's result: that operand first (it is still in A/X, #350)
void orderCommutativeOperands(Module& mod);
// const / pure functions of the module, inferred (-fipa-pure-const)
void inferPurity(Module& mod);
// the cold blocks (__unlikely branches) moved to the function's end
std::set<std::string> optimizeLoopRotation(Module& mod, const std::function<bool(const Function&)>& on);
void optimizeBlockReorder(Module& mod);
// The blocks (indices) inside a loop of fn: the natural loops of its back
// edges (a branch to a block at or before the branch's, as cc45 lays out
// structured code)
std::set<size_t> loopBlocks(const Function& fn);

    // Strength reduction: replace expensive ops with cheaper equivalents
    // (MUL by power-of-2 → LSL, DIV_U by power-of-2 → LSR, MOD_U by power-of-2 → AND)
    void optimizeStrengthReduction(Module& mod);
    void optimizeLoopStrengthReduction(Module& mod);
}
