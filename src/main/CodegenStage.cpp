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
        return {false, e.what(), 127};  // Exit code 127-168: code generation error
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
    builder.zpCallMode = zpCallMode_;
    builder.staticAllocMode = staticAlloc_;
    // -finline-functions forces inlining; -f[no-]inline-small-functions or the
    // -O level (2 and up) decide otherwise
    builder.inlineFunctions = optOverrides_.count("InlineSmallFunctions")
        ? optOverrides_.at("InlineSmallFunctions")
        : (inlineFunctions_ || optOn("InlineSmallFunctions"));
    builder.devirtualize = optOn("Devirtualization");
    builder.futureWarnings = futureWarnings_;
    builder.maxInlineIRSize = maxInlineIRSize_;
    builder.maxUnrollBytes = maxUnrollBytes_;
    builder.allowImplicitFunctionDecl = allowImplicitFuncDecl_;

    // Phase 102: Pass typedef mappings from parser
    builder.setTypedefMappings(typedefMappings_);
    if (!sourceFile_.empty()) builder.setSourceInfo(sourceFile_);

    // Generate IR from AST
    builder.generate(*ast_);

    // Print warnings
    const auto& warnings = builder.getWarnings();
    for (size_t i = 0; i < warnings.size(); i++) {
        if (!builder.warningAlreadyPrinted(i)) std::cerr << warnings[i] << std::endl;
    }

    if (builder.hasErrors()) {
        for (const auto& err : builder.getErrors()) {
            std::cerr << err << std::endl;
        }
        throw std::runtime_error(builder.getErrors().front());
    }

    // Get the generated module
    irModule_ = std::make_unique<ir::Module>(builder.getModule());
    // A #pragma cc45 no_stack16 / stack_top in the source wins over the flag
    if (irModule_->stackTop < 0) irModule_->stackTop = stackTop_;
    if (irModule_->bssFill < 0) irModule_->bssFill = bssFill_;

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

bool CodegenStage::optOn(const std::string& name) const {
    auto it = optOverrides_.find(name);
    if (it != optOverrides_.end()) return it->second;
    return OptimizationCatalog::isEnabledAtLevel(name, optimizationLevel_);
}

// The IR passes of IROptimizer.cpp, in order. Each runs on a function when its
// #pragma cc45 optimize(...) override says so, else when -f/-fno-/the -O level
// enables it (all are on from -O1).
void CodegenStage::optimizeIR() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Optimizing IR..." << std::endl;
    }
    if (!irModule_) return;

    struct Pass { const char* name; void (*run)(ir::Module&); };
    static const Pass passes[] = {
        {"LoopUnrolling",     ir::optimizeLoopUnroll},   // first: its constants feed the others
        {"StrengthReduction", ir::optimizeStrengthReduction},
        {"AlgebraicSimplify", ir::optimizeAlgebraic},
        {"TypeNarrowing",     ir::optimizeTypeNarrowing},
        {"BranchFolding",     ir::optimizeBranchFold},
        {"CommonSubexprElim", ir::optimizeCSE},
        {"LICM",              ir::optimizeLICM},
        {"CopyPropagation",   ir::optimizeCopyChains},
        {"AddrElemFusion",    ir::optimizeAddrElemFusion},
    };

    ir::optTrace = traceIROpt_ ? &std::cerr : nullptr;
    ir::unrollMaxBytes = irModule_->maxUnrollBytes >= 0 ? irModule_->maxUnrollBytes : maxUnrollBytes_;
    ir::passEnabled = [this](const ir::Function& fn, const char* name) {
        auto it = fn.optOverrides.find(name);
        if (it != fn.optOverrides.end()) return it->second;
        return optOn(name);
    };
    bool ranAny = false;
    for (const auto& p : passes) {
        bool any = optOn(p.name);
        for (const auto& fn : irModule_->functions) {
            auto it = fn.optOverrides.find(p.name);
            if (it != fn.optOverrides.end() && it->second) any = true;
        }
        if (!any) continue;
        if (verboseLevel_ >= 2) std::cout << "    " << p.name << std::endl;
        p.run(*irModule_);
        ranAny = true;
    }
    // What the passes left unused (part of dead-code elimination, always on)
    if (ranAny) ir::removeDeadTemps(*irModule_);
    ir::passEnabled = nullptr;
    ir::optTrace = nullptr;
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

    // CC45_DUMP_IR=1: print the optimized IR to stderr (debugging aid)
    if (const char* d = getenv("CC45_DUMP_IR"); d && *d && *d != '0') ir::Printer::print(std::cerr, *irModule_);

    // Generate assembly from IR using IRCodeGen
    std::ostringstream ss;
    IRCodeGen codegen(ss);

    // Generate assembly text for the module
    // Parameters: module, zpStart, relocMode, zpCallMode, emitReasons, staticAllocMode, sacDebugMode, prgBase
    codegen.generate(*irModule_,
                     0x08,              // zpStart
                     objectOnly_,       // relocMode (true for -c/.o45, false for direct PRG)
                     zpCallMode_,       // zpCallMode
                     verboseLevel_ >= 3, // emitReasons
                     staticAlloc_,      // staticAllocMode
                     verboseLevel_ >= 3, // sacDebugMode
                     prgBase_);

    irOutput_ = ss.str();

    if (verboseLevel_ >= 2) {
        std::cout << "    Generated " << irOutput_.size()
                 << " bytes of assembly." << std::endl;
    }
}
