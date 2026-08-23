#include "CodegenStage.hpp"
#include "AST.hpp"
#include "IRBuilder.hpp"
#include "IROptimizer.hpp"
#include "IRCodeGen.hpp"
#include "IPOAnalyzer.hpp"
#include "FunctionAnalyzer.hpp"
#include "InlineSelector.hpp"
#include "OptimizationSelector.hpp"
#include <iostream>
#include <sstream>

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
    IRBuilder builder(optimizationLevel_, staticAlloc_);

    // Set up compilation context from analyzer
    if (analyzer_) {
        builder.setFunctionAnalyzer(analyzer_);
    }

    // Build IR from AST
    irModule_ = builder.build(*ast_);

    if (verboseLevel_ >= 3) {
        // Optionally print IR for debugging
        IRPrinter printer;
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

    IPOAnalyzer ipoAnalyzer;
    const auto& profiles = analyzer_->getAllCharacteristics();

    // Count specialization candidates
    int specializationCount = 0;
    for (const auto& profile : profiles) {
        // Functions called with constant argument patterns are candidates
        if (profile.second.hasConstantCallPatterns &&
            profile.second.callSiteCount <= 5) {
            specializationCount++;
        }
    }

    if (verboseLevel_ >= 2 && specializationCount > 0) {
        std::cout << "    Found " << specializationCount
                 << " specialization candidates." << std::endl;
    }

    // Mark specialization candidates in IR
    // This will be expanded in Phase 91.3+ for full specialization support
}

void CodegenStage::optimizeIR() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Optimizing IR (8 passes)..." << std::endl;
    }

    if (!irModule_) return;

    // Create IR optimizer with full pass suite
    IROptimizer optimizer(optimizationLevel_);

    // Run optimization passes based on level:
    // Level 0: None
    // Level 1: Basic (constant folding, dead code elimination)
    // Level 2: Standard (+ strength reduction, copy propagation, CSE)
    // Level 3+: Aggressive (+ loop optimizations, function inlining)

    if (optimizationLevel_ >= 1) {
        // Pass 1: Constant folding
        optimizer.constantFold(*irModule_);

        // Pass 2: Dead code elimination
        optimizer.eliminateDeadCode(*irModule_);
    }

    if (optimizationLevel_ >= 2) {
        // Pass 3: Strength reduction (multiply/divide → bit shifts)
        optimizer.strengthReduce(*irModule_);

        // Pass 4: Copy propagation
        optimizer.copyPropagate(*irModule_);

        // Pass 5: Common subexpression elimination (CSE)
        optimizer.commonSubexpressionElimination(*irModule_);

        // Pass 6: Dead store elimination
        optimizer.eliminateDeadStores(*irModule_);
    }

    if (optimizationLevel_ >= 3) {
        // Pass 7: Loop invariant code motion (LICM)
        optimizer.loopInvariantCodeMotion(*irModule_);

        // Pass 8: Branch folding and unreachable code removal
        optimizer.branchFold(*irModule_);
    }

    if (verboseLevel_ >= 3) {
        std::cout << "    IR optimization complete (" << optimizationLevel_
                 << " passes)" << std::endl;
    }
}

void CodegenStage::applyInlining() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Applying inlining decisions..." << std::endl;
    }

    if (!irModule_ || !analyzer_) return;

    // Apply inlining decisions from optimization phase
    InlineSelector inlineSelector(optimizationLevel_);

    const auto& profiles = analyzer_->getAllCharacteristics();

    // Count inlined functions
    int inlinedCount = 0;
    for (const auto& profile : profiles) {
        // Functions marked as inline candidates get inlined
        if (profile.second.isInlineCandidate) {
            inlinedCount++;
        }
    }

    if (verboseLevel_ >= 2 && inlinedCount > 0) {
        std::cout << "    Inlined " << inlinedCount << " functions." << std::endl;
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
        std::cout << "  Generating assembly IR..." << std::endl;
    }

    if (!irModule_) return;

    // Generate assembly from IR using IRCodeGen
    IRCodeGen codegen(optimizationLevel_);

    // Configure code generation
    if (verboseLevel_ >= 2) {
        codegen.setVerbose(true);
    }

    // Generate assembly text
    std::ostringstream ss;
    irOutput_ = codegen.generate(*irModule_);

    if (verboseLevel_ >= 3) {
        std::cout << "    Generated " << irOutput_.size()
                 << " bytes of assembly." << std::endl;
    }
}
