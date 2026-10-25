#include "OptimizeStage.hpp"
#include "Logger.hpp"
#include "AST.hpp"
#include "ConstantFolder.hpp"
#include "LoopInterchange.hpp"
#include "OptimizationCatalog.hpp"
#include "FunctionAnalyzer.hpp"
#include <iostream>
#include <algorithm>

Stage::Result OptimizeStage::execute() {
    if (verboseLevel_ >= 1) {
        Logger::stream(LogLevel::INFO) << "Optimizing (level " << optimizationLevel_ << ")..." << std::endl;
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

        // Phase 5: Loop optimizations (interchange, unrolling, invariant hoisting)
        if (optimizationLevel_ >= 3) {
            performLoopOptimizations();
        }

        // -floop-interchange (also per function by #pragma cc45 optimize)
        performLoopInterchange();

        if (verboseLevel_ >= 1) {
            Logger::stream(LogLevel::INFO) << "AST optimization complete." << std::endl;
        }

        return {true};

    } catch (const std::exception& e) {
        return {false, e.what(), 85};  // Exit code 85-126: optimization error
    }
}

void OptimizeStage::performConstantFolding() {
    if (verboseLevel_ >= 2) {
        Logger::stream(LogLevel::DEBUG) << "Constant folding..." << std::endl;
    }

    // Apply constant folder visitor to entire AST
    ConstantFolder folder;
    if (ast_) {
        // The ConstantFolder visitor modifies the AST in place
        ast_->accept(folder);
    }
    for (const auto& w : folder.warnings) std::cerr << w << std::endl;
}

void OptimizeStage::performFunctionAnalysis() {
    if (verboseLevel_ >= 2) {
        Logger::stream(LogLevel::DEBUG) << "Function analysis..." << std::endl;
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
        Logger::stream(LogLevel::DEBUG) << "Analyzed " << analyzer_->getFunctionCount()
                 << " functions." << std::endl;
    }
}

void OptimizeStage::performInlineSelection() {
    if (verboseLevel_ >= 2) {
        Logger::stream(LogLevel::DEBUG) << "Inline selection..." << std::endl;
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
                Logger::stream(LogLevel::DEEP) << "Marking " << funcName << " as inline candidate"
                         << " (size: " << chars.estimatedCodeSize << " bytes)" << std::endl;
            }
        }
    }

    if (verboseLevel_ >= 2 && inlineCandidates > 0) {
        Logger::stream(LogLevel::DEBUG) << "Found " << inlineCandidates << " inline candidates." << std::endl;
    }
}

void OptimizeStage::performLoopOptimizations() {
    if (verboseLevel_ >= 2) {
        Logger::stream(LogLevel::DEBUG) << "Loop optimizations..." << std::endl;
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
        Logger::stream(LogLevel::DEEP) << "Loop optimization hints marked for IR phase." << std::endl;
    }
}

void OptimizeStage::performLoopInterchange() {
    if (!ast_) return;
    auto it = optOverrides_.find("LoopInterchange");
    bool on = it != optOverrides_.end() ? it->second
                                        : OptimizationCatalog::isEnabledAtLevel("LoopInterchange", optimizationLevel_);
    LoopInterchange li;
    if (traceIROpt_) {
        li.trace = [](const std::string& fn, int line, const std::string& msg) {
            std::cerr << "[interchange] _" << fn << " (line " << line << "): " << msg << std::endl;
        };
    }
    // Runs even when off: #pragma cc45 optimize(loop-interchange) can turn it on per function
    int n = li.optimizeTranslationUnit(*ast_, on);
    if (verboseLevel_ >= 2 && n > 0) Logger::stream(LogLevel::DEBUG) << "Interchanged " << n << " loop nest(s)." << std::endl;
}
