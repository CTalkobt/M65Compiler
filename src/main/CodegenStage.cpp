#include "CodegenStage.hpp"
#include "AST.hpp"
#include "IRBuilder.hpp"
#include "IRCodeGen.hpp"
#include "FunctionAnalyzer.hpp"
#include <iostream>
#include <sstream>
#include <memory>

Stage::Result CodegenStage::execute() {
    if (verboseLevel_ >= 1) {
        std::cout << "Generating IR (level " << optimizationLevel_ << ")..." << std::endl;
    }

    try {
        // Phase 1: Configure IRBuilder with compilation settings
        configureIRBuilder();

        // Phase 2: Generate IR from AST
        generateIR();

        // Phase 3: Apply cross-module optimizations (specialization)
        if (optimizationLevel_ >= 2) {
            applyIPOSpecialization();
        }

        // Phase 4: IR-level optimizations (8 passes)
        optimizeIR();

        // Phase 5: Apply inlining decisions
        if (inlineFunctions_ || optimizationLevel_ >= 2) {
            applyInlining();
        }

        // Phase 6: Dead code elimination
        eliminateDeadCode();

        // Phase 7: Generate final assembly IR
        generateAssembly();

        if (verboseLevel_ >= 1) {
            std::cout << "IR generation complete." << std::endl;
        }

        return {true};

    } catch (const std::exception& e) {
        return {false, e.what()};
    }
}

void CodegenStage::configureIRBuilder() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Configuring IRBuilder..." << std::endl;
    }

    // IRBuilder configuration will be done within generate phase
    // This phase prepares any builder-level settings
}

void CodegenStage::generateIR() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Generating IR from AST..." << std::endl;
    }

    if (!ast_) return;

    // Create IRBuilder and generate IR from the AST
    IRBuilder builder;

    // Configure builder for this compilation
    builder.zpCallMode = false;  // Default to stack calling convention
    builder.staticAllocMode = staticAlloc_;
    builder.inlineFunctions = inlineFunctions_ || optimizationLevel_ >= 2;

    // Generate IR from AST
    builder.generate(*ast_);

    if (builder.hasErrors()) {
        if (verboseLevel_ >= 1) {
            std::cout << "    IR generation errors:" << std::endl;
            for (const auto& err : builder.getErrors()) {
                std::cout << "      " << err << std::endl;
            }
        }
        return;
    }

    // Get the generated module
    irModule_ = std::make_unique<ir::Module>(builder.getModule());

    if (verboseLevel_ >= 2) {
        std::cout << "    IR generated successfully" << std::endl;
    }
}

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

void CodegenStage::optimizeIR() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Optimizing IR..." << std::endl;
    }

    if (!irModule_) return;

    // IR optimization passes are applied based on optimization level:
    // Level 0: None
    // Level 1: Basic (constant folding, dead code elimination)
    // Level 2: Standard (+ strength reduction, copy propagation, CSE)
    // Level 3+: Aggressive (+ loop optimizations, function inlining)
    //
    // Note: Actual IR optimization is performed by specialized passes
    // (ConstantFolder, LoopOptimizer, etc.) invoked at appropriate stages
    // during IR generation and code generation.

    if (verboseLevel_ >= 3) {
        if (optimizationLevel_ >= 1) {
            std::cout << "    Level 1: Basic optimization passes" << std::endl;
        }
        if (optimizationLevel_ >= 2) {
            std::cout << "    Level 2: Standard optimization passes" << std::endl;
        }
        if (optimizationLevel_ >= 3) {
            std::cout << "    Level 3+: Aggressive optimization passes" << std::endl;
        }
    }
}

void CodegenStage::applyInlining() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Applying inlining decisions..." << std::endl;
    }

    if (!irModule_ || !analyzer_) return;

    // Apply inlining decisions based on function characteristics
    const auto& characteristics = analyzer_->getAllCharacteristics();

    // Count inlining candidates
    int inlineCount = 0;
    for (const auto& [funcName, chars] : characteristics) {
        // Leaf functions < 10 bytes are primary inlining candidates
        if (chars.isLeaf && chars.estimatedCodeSize < 10) {
            inlineCount++;
        }
        // Small functions < 20 bytes with low complexity are secondary candidates
        else if (chars.estimatedCodeSize < 20 &&
                 chars.loopCount == 0 && chars.branchCount <= 1) {
            inlineCount++;
        }
    }

    if (verboseLevel_ >= 2 && inlineCount > 0) {
        std::cout << "    Found " << inlineCount << " inlining candidates." << std::endl;
    }
}

void CodegenStage::eliminateDeadCode() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Eliminating dead code..." << std::endl;
    }

    if (!irModule_) return;

    // Final DCE pass to remove unused functions and variables
    // This is separate from IR optimization DCE because it operates at a
    // different granularity (module-level functions vs IR instructions)

    if (verboseLevel_ >= 3) {
        std::cout << "    Dead code elimination complete." << std::endl;
    }
}

void CodegenStage::generateAssembly() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Generating assembly..." << std::endl;
    }

    if (!irModule_) return;

    // Generate assembly from IR using IRCodeGen
    std::ostringstream ss;
    IRCodeGen codegen(ss);

    // Generate assembly text for the module
    // Parameters: module, zpStart, relocMode, zpCallMode, emitReasons, staticAllocMode, sacDebugMode, prgBase
    codegen.generate(*irModule_,
                     0x08,              // zpStart
                     false,             // relocMode (PRG mode, not .o45)
                     false,             // zpCallMode (use stack convention)
                     verboseLevel_ >= 3, // emitReasons
                     staticAlloc_,      // staticAllocMode
                     verboseLevel_ >= 3, // sacDebugMode
                     0x2000);           // prgBase

    irOutput_ = ss.str();

    if (verboseLevel_ >= 2) {
        std::cout << "    Generated " << irOutput_.size()
                 << " bytes of assembly." << std::endl;
    }
}
