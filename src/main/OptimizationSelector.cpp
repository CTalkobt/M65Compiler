#include "OptimizationSelector.hpp"

OptimizationSelector::OptimizationSelector(int globalUnrollFactor)
    : globalUnrollFactor_(globalUnrollFactor) {}

void OptimizationSelector::selectOptimizations(TranslationUnit& unit, FunctionAnalyzer& analyzer) {
    // Apply per-function optimization flags
    for (auto& decl : unit.topLevelDecls) {
        if (auto* func = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            auto chars = analyzer.getCharacteristics(func->name);
            auto flags = selectForFunction(chars);

            func->optimizeLoopUnroll = flags.enableLoopUnrolling;
            func->optimizeLoopInterchange = flags.enableLoopInterchange;
            func->optimize2PassFolding = flags.enable2PassFolding;
            func->unrollFactor = flags.unrollFactor;
            func->optimizeSAC = flags.enableSAC;
        }
    }
}

OptimizationSelector::OptimizationFlags OptimizationSelector::selectForFunction(
    const FunctionAnalyzer::FunctionCharacteristics* chars) {

    OptimizationFlags flags;

    if (!chars) {
        // Default flags for unknown functions
        flags.unrollFactor = globalUnrollFactor_;
        return flags;
    }

    // Loop unrolling decisions
    if (chars->shouldUnrollLoops) {
        flags.enableLoopUnrolling = true;
        flags.unrollFactor = computeUnrollFactor(chars);
    } else {
        flags.enableLoopUnrolling = false;
        flags.unrollFactor = 0;
    }

    // Loop interchange: only for well-nested loops
    flags.enableLoopInterchange = chars->shouldInterchangeLoops;

    // Second-pass constant folding: for code-size-heavy functions
    flags.enable2PassFolding = chars->shouldFoldConstants2x;

    // SAC: for leaf, non-complex functions
    flags.enableSAC = chars->shouldUseSAC;

    return flags;
}

int OptimizationSelector::computeUnrollFactor(
    const FunctionAnalyzer::FunctionCharacteristics* chars) {

    if (!chars) return globalUnrollFactor_;

    // Simple heuristic based on function characteristics
    int factor = globalUnrollFactor_;

    // Small simple functions: aggressive unrolling
    if (chars->estimatedCodeSize < 50 && chars->branchCount == 0) {
        factor = std::min(8, globalUnrollFactor_ * 2);
    }
    // Complex functions with branches: conservative unrolling
    else if (chars->branchCount > 5) {
        factor = std::max(2, globalUnrollFactor_ / 2);
    }
    // Normal functions: use global factor
    else {
        factor = globalUnrollFactor_;
    }

    return factor;
}
