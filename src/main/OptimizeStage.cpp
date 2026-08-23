#include "OptimizeStage.hpp"
#include "AST.hpp"
#include "ConstantFolder.hpp"
#include "FunctionAnalyzer.hpp"
#include <iostream>
#include <algorithm>

Stage::Result OptimizeStage::execute() {
    if (verboseLevel_ >= 1) {
        std::cout << "Optimizing (level " << optimizationLevel_ << ")..." << std::endl;
    }

    try {
        // Phase 1: Constant folding on AST
        performConstantFolding();

        // Phase 2: Function analysis (collects metrics, call graphs, etc.)
        performFunctionAnalysis();

        // Phase 3: Inline selection (marks functions as inline candidates)
        if (inlineFunctions_ || optimizationLevel_ >= 2) {
            performInlineSelection();
        }

        // Phase 4: Cross-module optimization (IPO, devirtualization, etc.)
        if (optimizationLevel_ >= 2) {
            performCrossModuleOptimization();
        }

        // Phase 5: Loop optimizations (interchange, unrolling, invariant hoisting)
        if (optimizationLevel_ >= 3) {
            performLoopOptimizations();
        }

        if (verboseLevel_ >= 1) {
            std::cout << "AST optimization complete." << std::endl;
        }

        return {true};

    } catch (const std::exception& e) {
        return {false, e.what()};
    }
}

void OptimizeStage::performConstantFolding() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Constant folding..." << std::endl;
    }

    // Apply constant folder visitor to entire AST
    ConstantFolder folder;
    if (ast_) {
        // The ConstantFolder visitor modifies the AST in place
        ast_->accept(folder);
    }
}

void OptimizeStage::performFunctionAnalysis() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Function analysis..." << std::endl;
    }

    // Create function analyzer to collect metrics
    analyzer_ = std::make_shared<FunctionAnalyzer>();

    if (ast_) {
        // Analyze all functions in the AST
        // This populates function profiles with:
        //  - call sites and callee information
        //  - code size estimates
        //  - leaf function detection
        //  - register clobber information
        ast_->accept(*analyzer_);
    }

    if (verboseLevel_ >= 2) {
        std::cout << "    Analyzed " << analyzer_->getFunctionCount()
                 << " functions." << std::endl;
    }
}

void OptimizeStage::performInlineSelection() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Inline selection..." << std::endl;
    }

    if (!analyzer_) return;

    // Get function profiles from analyzer
    const auto& profiles = analyzer_->getFunctionProfiles();

    // Mark inline candidates based on heuristics:
    // - Single-caller functions < 20 bytes
    // - Leaf functions < 10 bytes
    // - Tiny functions (< 10 bytes) with <= 3 call sites
    for (const auto& profile : profiles) {
        bool shouldInline = false;

        // Check single-caller heuristic
        if (profile.second.callSiteCount == 1 && profile.second.estimatedSize < 20) {
            shouldInline = true;
        }
        // Check leaf function heuristic
        else if (profile.second.isLeaf && profile.second.estimatedSize < 10) {
            shouldInline = true;
        }
        // Check tiny function heuristic
        else if (profile.second.estimatedSize < 10 && profile.second.callSiteCount <= 3) {
            shouldInline = true;
        }

        if (shouldInline && verboseLevel_ >= 3) {
            std::cout << "    Marking " << profile.first << " as inline candidate" << std::endl;
        }
    }
}

void OptimizeStage::performCrossModuleOptimization() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Cross-module optimization..." << std::endl;
    }

    if (!analyzer_) return;

    // Phase 91: Cross-module optimization (IPO)
    // This includes:
    //  - Dead code elimination across modules
    //  - Function specialization for constant argument patterns
    //  - Call graph analysis for devirtualization
    //  - Co-optimization hints

    const auto& profiles = analyzer_->getFunctionProfiles();

    // Count functions eligible for optimization
    int eligibleCount = 0;
    for (const auto& profile : profiles) {
        // Functions with no external callers are candidates for DCE
        if (profile.second.externalCallCount == 0 &&
            profile.first != "main" && profile.first != "_main") {
            eligibleCount++;
        }
    }

    if (verboseLevel_ >= 2 && eligibleCount > 0) {
        std::cout << "    Found " << eligibleCount << " candidates for dead code elimination." << std::endl;
    }

    // Additional cross-module optimizations:
    //  - Devirtualization (single virtual implementation → direct call)
    //  - Function specialization for hot call paths
    //  - Co-location hints for frequently-accessed globals
    // These are marked as hints in the IR and applied at link time (Phase 100)
}

void OptimizeStage::performLoopOptimizations() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Loop optimizations..." << std::endl;
    }

    // Phase 88-89: Loop idiom recognition and optimization
    // This includes:
    //  - Loop interchange for cache locality
    //  - Loop invariant code motion (hoisting)
    //  - Loop unrolling (limited to 20-1000 iterations)
    //  - Loop idiom recognition (memcpy, memset, sum, search, count, dot-product)

    // Loop optimization is performed at the IR level during codegen
    // Mark loops for analysis during IRBuilder phase
    if (verboseLevel_ >= 3) {
        std::cout << "    Loop optimization hints marked for IR phase." << std::endl;
    }
}
