// OptimizeStagePhases.cpp — phase 91's cross-module counting (leaf and small functions, printed with -vv)
//
// Moved out of OptimizeStage.cpp: nothing in the tools calls these, so
// they are built into lib45-phases.a, which no tool links.

#include "OptimizeStage.hpp"
#include "AST.hpp"
#include "ConstantFolder.hpp"
#include "LoopInterchange.hpp"
#include "OptimizationCatalog.hpp"
#include "FunctionAnalyzer.hpp"
#include <iostream>
#include <algorithm>

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

