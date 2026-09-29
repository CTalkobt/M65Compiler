#include "OptimizationCatalog.hpp"
#include <sstream>
#include <algorithm>

std::vector<OptimizationInfo> OptimizationCatalog::catalog_;
bool OptimizationCatalog::initialized_ = false;

const std::vector<OptimizationInfo>& OptimizationCatalog::getAllOptimizations() {
    if (!initialized_) {
        initializeCatalog();
        initialized_ = true;
    }
    return catalog_;
}

const OptimizationInfo* OptimizationCatalog::getOptimization(const std::string& name) {
    auto& opts = getAllOptimizations();
    auto it = std::find_if(opts.begin(), opts.end(),
        [&name](const OptimizationInfo& o) { return o.name == name; });
    return it != opts.end() ? &(*it) : nullptr;
}

std::vector<OptimizationInfo> OptimizationCatalog::getOptimizationsAtLevel(int level) {
    std::vector<OptimizationInfo> result;
    auto& opts = getAllOptimizations();
    for (const auto& opt : opts) {
        if (opt.enabledAtLevel <= level &&
            (opt.disabledAtLevel == -1 || opt.disabledAtLevel > level)) {
            result.push_back(opt);
        }
    }
    return result;
}

std::vector<OptimizationInfo> OptimizationCatalog::getOptimizationsByCategory(OptimizationCategory category) {
    std::vector<OptimizationInfo> result;
    auto& opts = getAllOptimizations();
    for (const auto& opt : opts) {
        if (opt.category == category || opt.category == OptimizationCategory::BOTH) {
            result.push_back(opt);
        }
    }
    return result;
}

bool OptimizationCatalog::isEnabledAtLevel(const std::string& name, int level) {
    auto opt = getOptimization(name);
    if (!opt) return false;
    return opt->enabledAtLevel <= level &&
           (opt->disabledAtLevel == -1 || opt->disabledAtLevel > level);
}

void OptimizationCatalog::initializeCatalog() {
    // Level 0: No optimizations (baseline)
    // Level 1: Basic constant folding and dead code elimination
    catalog_.push_back({
        "ConstantFolding", "Constant Folding",
        "Evaluate constant expressions at compile time",
        OptimizationCategory::BOTH, 1, -1, "-fconstant-folding", false, 5
    });

    catalog_.push_back({
        "DeadCodeElimination", "Dead Code Elimination",
        "Remove unreachable code and dead statements",
        OptimizationCategory::SPACE, 1, -1, "-fdead-code-elimination", true, 3
    });

    // Level 2: Function analysis and inlining
    catalog_.push_back({
        "InlineSmallFunctions", "Inline Small Functions",
        "Inline functions < 20 bytes to eliminate call overhead",
        OptimizationCategory::SPEED, 2, -1, "-finline-small-functions", true, 2
    });

    catalog_.push_back({
        "TailCallOptimization", "Tail Call Optimization",
        "Convert tail calls to jumps (JSR + RTS → JMP)",
        OptimizationCategory::BOTH, 2, -1, "-ftail-call-optimization", true, 4
    });

    // Level 3: Strength reduction and algebraic simplification
    catalog_.push_back({
        "StrengthReduction", "Strength Reduction",
        "Replace multiply/divide by powers of 2 with bit shifts",
        OptimizationCategory::BOTH, 3, -1, "-fstrength-reduction", false, 3
    });

    catalog_.push_back({
        "AlgebraicSimplify", "Algebraic Simplification",
        "Eliminate identity/annihilator patterns (a*1=a, a*0=0)",
        OptimizationCategory::BOTH, 3, -1, "-falgebraic-simplify", false, 2
    });

    // Level 4: Loop optimizations
    catalog_.push_back({
        "LoopUnrolling", "Loop Unrolling",
        "Unroll small loops (20-1000 iterations) to reduce branch overhead",
        OptimizationCategory::SPEED, 4, -1, "-floop-unrolling", true, 5
    });

    catalog_.push_back({
        "LoopInterchange", "Loop Interchange",
        "Reorder nested loops for better cache locality",
        OptimizationCategory::SPEED, 4, -1, "-floop-interchange", true, 4
    });

    catalog_.push_back({
        "LICM", "Loop-Invariant Code Motion",
        "Hoist loop-invariant computations outside loops",
        OptimizationCategory::SPEED, 4, -1, "-floop-invariant-code-motion", true, 3
    });

    // Level 5: Advanced inlining and devirtualization
    catalog_.push_back({
        "CrossFunctionInlining", "Cross-Function Inlining",
        "Inline functions with 1-3 call sites (Phase 86-87)",
        OptimizationCategory::SPACE, 5, -1, "-fcross-function-inlining", true, 3
    });

    catalog_.push_back({
        "Devirtualization", "Devirtualization",
        "Replace single-implementation virtual calls with direct calls",
        OptimizationCategory::BOTH, 5, -1, "-fdevirtualization", true, 2
    });

    // Level 6: CSE and copy propagation
    catalog_.push_back({
        "CommonSubexprElim", "Common Subexpression Elimination",
        "Eliminate redundant subexpression computations",
        OptimizationCategory::SPEED, 6, -1, "-fcse", true, 4
    });

    catalog_.push_back({
        "CopyPropagation", "Copy Propagation",
        "Replace copies with original values to reduce moves",
        OptimizationCategory::BOTH, 6, -1, "-fcopy-propagation", true, 2
    });

    // Level 7: Branch and jump optimizations
    catalog_.push_back({
        "BranchInversion", "Branch Inversion",
        "Invert branch conditions to eliminate jumps (BRA + inverted branch)",
        OptimizationCategory::SPACE, 7, -1, "-fbranch-inversion", false, 2
    });

    catalog_.push_back({
        "BranchFolding", "Branch Folding",
        "Eliminate unreachable code via conditional branches",
        OptimizationCategory::SPACE, 7, -1, "-fbranch-folding", true, 3
    });

    catalog_.push_back({
        "JumpOptimization", "Jump Optimization",
        "Convert JMP to BRA for backward branches, eliminate no-op jumps",
        OptimizationCategory::SPACE, 7, -1, "-fjump-optimization", false, 2
    });

    // Level 8: Memory and register optimizations
    catalog_.push_back({
        "RedundantLoadElim", "Redundant Load Elimination",
        "Eliminate redundant memory loads via reverse store-forwarding",
        OptimizationCategory::SPEED, 8, -1, "-fredundant-load-elimination", true, 3
    });

    catalog_.push_back({
        "DeadStoreElim", "Dead Store Elimination",
        "Eliminate stores whose values are never used",
        OptimizationCategory::SPACE, 8, -1, "-fdead-store-elimination", true, 2
    });

    catalog_.push_back({
        "FramePointerOpt", "Frame Pointer Optimization",
        "Lazy frame pointer initialization (SAC mode)",
        OptimizationCategory::SPACE, 8, -1, "-fframe-pointer-optimization", true, 2
    });

    // Level 9: Aggressive interprocedural optimizations
    catalog_.push_back({
        "CoOptimization", "Co-Optimization",
        "Coordinated optimization of related function groups",
        OptimizationCategory::BOTH, 9, -1, "-fco-optimization", true, 3
    });

    catalog_.push_back({
        "ProcedureInlining", "Procedure Inlining",
        "Aggressive inlining of functions (recursive calls, larger sizes)",
        OptimizationCategory::SPEED, 9, -1, "-fprocedure-inlining", true, 5
    });

    catalog_.push_back({
        "InterproceduralOpt", "Interprocedural Optimization",
        "Cross-module optimization hints and analysis",
        OptimizationCategory::BOTH, 9, -1, "-finterprocedural-optimization", true, 4
    });
}

std::string OptimizationCatalog::getDocumentation() {
    std::ostringstream doc;
    doc << "=== MEGA65 C Compiler: Optimization Catalog ===\n\n";
    doc << "Optimization Levels: -O0 (none) through -O9 (aggressive)\n\n";

    auto& opts = getAllOptimizations();

    int currentLevel = -1;
    for (const auto& opt : opts) {
        if (opt.enabledAtLevel != currentLevel) {
            currentLevel = opt.enabledAtLevel;
            doc << "\n--- Level " << currentLevel << " Optimizations ---\n";
        }

        doc << "\n" << opt.displayName << " (" << opt.flag << ")\n";
        doc << "  Category: ";
        switch (opt.category) {
            case OptimizationCategory::SPACE: doc << "Space"; break;
            case OptimizationCategory::SPEED: doc << "Speed"; break;
            case OptimizationCategory::BOTH: doc << "Both"; break;
        }
        doc << "\n  Benefit: ~" << opt.estimatedBenefit << "%\n";
        doc << "  Description: " << opt.description << "\n";
    }

    doc << "\n\n=== Usage ===\n";
    doc << "cc45 input.c -O2                              # Use level 2 optimizations\n";
    doc << "cc45 input.c -O3 -fno-loop-invariant-code-motion  # Level 3 but disable LICM\n";
    doc << "cc45 input.c -O0 -fconstant-folding          # Start at 0, enable only constant folding\n";
    doc << "#pragma cc45 optimize(none)                   # Disable optimization (function level)\n";
    doc << "#pragma cc45 optimize(speed)                  # Speed-focused (function level)\n";
    doc << "#pragma cc45 optimize(space)                  # Space-focused (function level)\n";

    return doc.str();
}
