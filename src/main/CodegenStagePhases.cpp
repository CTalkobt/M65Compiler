// CodegenStagePhases.cpp — phase 91's IPO specialization counting (printed with -vv)
//
// Moved out of CodegenStage.cpp: nothing in the tools calls these, so
// they are built into lib45-phases.a, which no tool links.

#include <cstdlib>
#include "IR.hpp"
#include "CodegenStage.hpp"
#include "AST.hpp"
#include "IRBuilder.hpp"
#include "IRCodeGen.hpp"
#include "FunctionAnalyzer.hpp"
#include "IROptimizer.hpp"
#include "OptimizationCatalog.hpp"
#include <iostream>
#include <sstream>
#include <memory>

void CodegenStage::applyIPOSpecialization() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Applying cross-module optimization..." << std::endl;
    }

    if (!irModule_ || !analyzer_) return;

    // Phase 91: Cross-module optimization (IPO) with specialization
    // This includes:
    //  - Function specialization for constant argument patterns
    //  - Dead code elimination across module boundaries
    //  - Devirtualization of virtual function calls
    //  - Co-optimization hints for link time

    const auto& characteristics = analyzer_->getAllCharacteristics();

    // Count optimization candidates based on function characteristics
    int leafFunctions = 0;
    int smallFunctions = 0;
    int recursiveFunctions = 0;

    for (const auto& [funcName, chars] : characteristics) {
        // Leaf functions are candidates for aggressive optimization
        if (chars.isLeaf) {
            leafFunctions++;
        }
        // Small functions are candidates for inlining/specialization
        if (chars.estimatedCodeSize < 20) {
            smallFunctions++;
        }
        // Track recursive functions (cannot use SAC)
        if (chars.isRecursive) {
            recursiveFunctions++;
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

    // Mark optimization candidates in IR
    // This will be expanded in Phase 91.3+ for full specialization support
}

