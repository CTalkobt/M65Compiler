#include "InlineSelector.hpp"

InlineSelector::InlineSelector() {}

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

    // Strategy 1: Tiny functions (< 20 code size units)
    // Always inline if they're leaf functions
    if (chars->estimatedCodeSize < 20 && chars->isLeaf) {
        hints.shouldInline = true;
        hints.inlineThreshold = 200;  // Very aggressive
        hints.reason = "Tiny leaf function";
        return hints;
    }

    // Strategy 2: Small functions (20-50 code units)
    // Inline if they have few branches and are called frequently
    if (chars->estimatedCodeSize < 50 && chars->branchCount < 3) {
        hints.shouldInline = true;
        hints.inlineThreshold = 150;  // Aggressive
        hints.reason = "Small simple function";
        return hints;
    }

    // Strategy 3: Medium functions (50-100 code units)
    // Only inline if they're very simple (no branches)
    if (chars->estimatedCodeSize < 100 && chars->branchCount == 0 && chars->isLeaf) {
        hints.shouldInline = true;
        hints.inlineThreshold = 100;  // Conservative
        hints.reason = "Medium simple leaf";
        return hints;
    }

    // Strategy 4: Functions with no loops and no branches
    // Good candidates for inlining if small enough
    if (chars->loopCount == 0 && chars->branchCount == 0 &&
        chars->estimatedCodeSize < 60) {
        hints.shouldInline = true;
        hints.inlineThreshold = 120;
        hints.reason = "No loops/branches";
        return hints;
    }

    // Default: Don't inline large or complex functions
    hints.reason = "Too large or complex";
    return hints;
}

InlineSelector::InlineHints InlineSelector::getInlineHints(
    const std::string& funcName) const {

    auto it = inlineHints_.find(funcName);
    if (it != inlineHints_.end()) {
        return it->second;
    }
    return InlineHints();
}
