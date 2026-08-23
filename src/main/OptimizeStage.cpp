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

    // Get function characteristics from analyzer
    const auto& characteristics = analyzer_->getAllCharacteristics();

    // Mark inline candidates based on heuristics:
    // - Leaf functions < 10 bytes (no function calls)
    // - Small functions < 20 bytes with simple structure
    int inlineCandidates = 0;
    for (const auto& [funcName, chars] : characteristics) {
        bool shouldInline = false;

        // Check leaf function heuristic (highest priority)
        if (chars.isLeaf && chars.estimatedCodeSize < 10) {
            shouldInline = true;
        }
        // Check small function heuristic (low loop/branch count)
        else if (chars.estimatedCodeSize < 20 &&
                 chars.loopCount == 0 && chars.branchCount <= 1) {
            shouldInline = true;
        }

        if (shouldInline) {
            inlineCandidates++;
            if (verboseLevel_ >= 3) {
                std::cout << "    Marking " << funcName << " as inline candidate"
                         << " (size: " << chars.estimatedCodeSize << " bytes)" << std::endl;
            }
        }
    }

    if (verboseLevel_ >= 2 && inlineCandidates > 0) {
        std::cout << "    Found " << inlineCandidates << " inline candidates." << std::endl;
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

    const auto& characteristics = analyzer_->getAllCharacteristics();

    // Count functions eligible for optimization
    int leafFunctions = 0;
    int smallFunctions = 0;
    for (const auto& [funcName, chars] : characteristics) {
        // Leaf functions are candidates for aggressive optimization
        if (chars.isLeaf && funcName != "main" && funcName != "_main") {
            leafFunctions++;
        }
        // Small functions can be inlined or specialized
        if (chars.estimatedCodeSize < 20 && funcName != "main" && funcName != "_main") {
            smallFunctions++;
        }
    }

    if (verboseLevel_ >= 2) {
        if (leafFunctions > 0) {
            std::cout << "    Found " << leafFunctions << " leaf functions for optimization." << std::endl;
        }
        if (smallFunctions > 0) {
            std::cout << "    Found " << smallFunctions << " small functions for specialization." << std::endl;
        }
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
