#include "InlineSelector.hpp"
#include "CallGraphAnalyzer.hpp"
#include "CoOptimizationSelector.hpp"

InlineSelector::InlineSelector(int optimizationLevel)
    : optimizationLevel_(optimizationLevel) {}

void InlineSelector::selectInlineCandidates(TranslationUnit& unit,
                                           FunctionAnalyzer& analyzer) {
    for (auto& decl : unit.topLevelDecls) {
        if (auto* func = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            auto chars = analyzer.getCharacteristics(func->name);
            auto hints = computeInlineDecision(chars);
            inlineHints_[func->name] = hints;

            // Mark function with inline hint if beneficial
            if (hints.shouldInline) {
                func->isInline = true;
            }
        }
    }
}

InlineSelector::InlineHints InlineSelector::computeInlineDecision(
    const FunctionAnalyzer::FunctionCharacteristics* chars) {

    InlineHints hints;
    hints.reason = "Default (no inline)";

    if (!chars) {
        return hints;
    }

    // Don't inline recursive functions (risk of infinite expansion)
    if (chars->isRecursive) {
        hints.reason = "Recursive (cannot inline)";
        return hints;
    }

    // Don't inline functions with inline asm that reference parameters
    // These require special handling that inlining doesn't support
    if (chars->hasAsmParamRefs) {
        hints.reason = "Has inline asm with parameter refs (cannot inline)";
        return hints;
    }

    // Adjust thresholds based on optimization level
    // -O0-1: conservative, -O2-3: moderate, -O4+: aggressive
    int tinyThreshold = 20;
    int smallThreshold = 50;
    int mediumThreshold = 100;
    int noLoopThreshold = 60;

    if (optimizationLevel_ >= 4) {
        // -O4+: aggressive inlining for functions with low complexity
        tinyThreshold = 60;
        smallThreshold = 150;
        mediumThreshold = 300;
        noLoopThreshold = 180;
    } else if (optimizationLevel_ >= 2) {
        // -O2-3: moderate-aggressive inlining
        tinyThreshold = 35;
        smallThreshold = 80;
        mediumThreshold = 150;
        noLoopThreshold = 100;
    }

    // Strategy 1: Tiny functions
    if (chars->estimatedCodeSize < tinyThreshold && chars->isLeaf) {
        hints.shouldInline = true;
        hints.inlineThreshold = 200;
        hints.reason = "Tiny leaf function";
        return hints;
    }

    // Debug output for optimization level investigation
    if (optimizationLevel_ >= 4 && chars->estimatedCodeSize < 200) {
        std::cerr << "[InlineSelector] Func: leaf=" << chars->isLeaf
                  << " size=" << chars->estimatedCodeSize
                  << " branches=" << chars->branchCount
                  << " loops=" << chars->loopCount
                  << " tinyThresh=" << tinyThreshold << "\n";
    }

    // Strategy 2: Small functions (relax branch requirement at higher levels)
    int branchLimit = (optimizationLevel_ >= 4) ? 10 : 3;
    if (chars->estimatedCodeSize < smallThreshold && chars->branchCount < branchLimit) {
        hints.shouldInline = true;
        hints.inlineThreshold = 150;
        hints.reason = "Small simple function";
        return hints;
    }

    // Strategy 3: Medium functions (relax at higher levels)
    int mediumBranchLimit = (optimizationLevel_ >= 4) ? 5 : 0;
    if (chars->estimatedCodeSize < mediumThreshold && chars->branchCount <= mediumBranchLimit && chars->isLeaf) {
        hints.shouldInline = true;
        hints.inlineThreshold = 100;
        hints.reason = "Medium simple leaf";
        return hints;
    }

    // Strategy 4: Leaf functions (aggressive mode for -O4+)
    if (optimizationLevel_ >= 4 && chars->isLeaf && chars->estimatedCodeSize < noLoopThreshold) {
        hints.shouldInline = true;
        hints.inlineThreshold = 150;
        hints.reason = "Leaf function (aggressive mode)";
        return hints;
    }

    // Default: Don't inline large or complex functions
    hints.reason = "Too large or complex";
    return hints;
}

void InlineSelector::applyRecommendations(const CallGraphAnalyzer* callGraph,
                                          const CoOptimizationSelector* selector) {
    // Phase 87: Apply cross-function optimization recommendations
    if (!selector) return;

    // Get recommended inline pairs from Phase 86 analysis
    auto inlinePairs = selector->getRecommendedInlines();

    // Apply top recommendations with safety checks
    int appliedCount = 0;
    const int maxInlines = 5; // Conservative limit to prevent code bloat

    for (const auto& pair : inlinePairs) {
        if (appliedCount >= maxInlines) break;

        // Check if callee is safe to inline
        auto calleeHints = getInlineHints(pair.callee);
        if (calleeHints.shouldInline || !calleeHints.reason.empty()) {
            // Override with cross-function recommendation
            InlineHints newHints = calleeHints;
            newHints.shouldInline = true;
            newHints.fromCrossFunc = true;
            newHints.benefitScore = pair.benefitScore;
            newHints.reason = "Phase 87 cross-function recommendation";
            newHints.inlineThreshold = 200; // Aggressive for recommended functions

            inlineHints_[pair.callee] = newHints;
            appliedCount++;
        }
    }
}

void InlineSelector::applyRecommendationsToAST(TranslationUnit& unit) {
    // Phase 88: Apply inline recommendations to AST nodes
    // Mark functions that should be inlined based on recommendations

    for (auto& decl : unit.topLevelDecls) {
        if (auto* func = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            auto hints = getInlineHints(func->name);

            // Mark for inlining if recommended and safe
            if (hints.shouldInline && hints.fromCrossFunc) {
                func->isInline = true;
            }
        }
    }
}

InlineSelector::InlineHints InlineSelector::getInlineHints(
    const std::string& funcName) const {

    auto it = inlineHints_.find(funcName);
    if (it != inlineHints_.end()) {
        return it->second;
    }
    return InlineHints();
}
